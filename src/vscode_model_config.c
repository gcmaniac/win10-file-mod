#include "vscode_model_config.h"
#include <sqlite3.h>

#define VSCODE_CONFIG_MAX_BYTES (4u * 1024u * 1024u)

static void set_error(char *out_error, size_t capacity, const char *message) {
    if (!out_error || capacity == 0) return;
    snprintf(out_error, capacity, "%s", message ? message : "Unknown configuration error.");
    out_error[capacity - 1] = '\0';
}

static bool read_config_file(const wchar_t *path, char **out_text, size_t *out_length, bool *out_has_bom) {
    *out_text = NULL;
    *out_length = 0;
    *out_has_bom = false;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size)) {
        DWORD error = GetLastError();
        CloseHandle(file);
        SetLastError(error);
        return false;
    }
    if (size.QuadPart <= 0 || size.QuadPart > VSCODE_CONFIG_MAX_BYTES) {
        CloseHandle(file);
        SetLastError(ERROR_FILE_TOO_LARGE);
        return false;
    }

    char *data = (char *)malloc((size_t)size.QuadPart + 1);
    if (!data) {
        CloseHandle(file);
        SetLastError(ERROR_OUTOFMEMORY);
        return false;
    }
    DWORD bytes_read = 0;
    BOOL read_ok = ReadFile(file, data, (DWORD)size.QuadPart, &bytes_read, NULL);
    DWORD read_error = read_ok ? ERROR_HANDLE_EOF : GetLastError();
    bool ok = read_ok && bytes_read == (DWORD)size.QuadPart;
    if (!ok) SetLastError(read_error);
    CloseHandle(file);
    if (!ok) {
        free(data);
        return false;
    }

    size_t offset = 0;
    if (bytes_read >= 3 && (unsigned char)data[0] == 0xef &&
        (unsigned char)data[1] == 0xbb && (unsigned char)data[2] == 0xbf) {
        offset = 3;
        *out_has_bom = true;
    }
    memmove(data, data + offset, (size_t)bytes_read - offset);
    *out_length = (size_t)bytes_read - offset;
    data[*out_length] = '\0';
    *out_text = data;
    return true;
}

static bool write_config_file(
    const wchar_t *path,
    const char *text,
    size_t length,
    bool has_bom,
    char *out_error,
    size_t error_capacity
) {
    wchar_t temporary_path[MAX_PATH * 4];
    wchar_t backup_path[MAX_PATH * 4];
    SYSTEMTIME now;
    GetLocalTime(&now);
    int temp_chars = swprintf(
        temporary_path, sizeof(temporary_path) / sizeof(temporary_path[0]),
        L"%ls.tmp.%lu.%u", path, (unsigned long)GetCurrentProcessId(),
        (unsigned int)GetTickCount()
    );
    int backup_chars = swprintf(
        backup_path, sizeof(backup_path) / sizeof(backup_path[0]),
        L"%ls.bak.%04u%02u%02u-%02u%02u%02u-%03u", path,
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds
    );
    if (temp_chars <= 0 || (size_t)temp_chars >= sizeof(temporary_path) / sizeof(temporary_path[0]) ||
        backup_chars <= 0 || (size_t)backup_chars >= sizeof(backup_path) / sizeof(backup_path[0])) {
        set_error(out_error, error_capacity, "The VS Code configuration path is too long.");
        return false;
    }

    if (!CopyFileW(path, backup_path, TRUE)) {
        char message[128];
        snprintf(message, sizeof(message), "Could not create a configuration backup (Windows error %lu).",
            (unsigned long)GetLastError());
        set_error(out_error, error_capacity, message);
        return false;
    }

    HANDLE file = CreateFileW(temporary_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        char message[128];
        snprintf(message, sizeof(message), "Could not create the temporary configuration file (Windows error %lu).",
            (unsigned long)GetLastError());
        set_error(out_error, error_capacity, message);
        return false;
    }

    bool ok = true;
    DWORD written = 0;
    if (has_bom) {
        static const unsigned char bom[] = {0xef, 0xbb, 0xbf};
        ok = WriteFile(file, bom, sizeof(bom), &written, NULL) && written == sizeof(bom);
    }
    size_t offset = 0;
    while (ok && offset < length) {
        DWORD chunk = (DWORD)((length - offset) > 1024u * 1024u ? 1024u * 1024u : length - offset);
        ok = WriteFile(file, text + offset, chunk, &written, NULL) && written == chunk;
        offset += written;
    }
    if (ok && !FlushFileBuffers(file)) ok = false;
    DWORD write_error = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (!ok) {
        DeleteFileW(temporary_path);
        char message[128];
        snprintf(message, sizeof(message), "Could not write the VS Code configuration (Windows error %lu).",
            (unsigned long)write_error);
        set_error(out_error, error_capacity, message);
        return false;
    }
    if (!MoveFileExW(temporary_path, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DWORD error = GetLastError();
        DeleteFileW(temporary_path);
        char message[128];
        snprintf(message, sizeof(message), "Could not replace the VS Code configuration (Windows error %lu).",
            (unsigned long)error);
        set_error(out_error, error_capacity, message);
        return false;
    }
    return true;
}

static bool sqlite_exec_bound_json(
    sqlite3 *database,
    const char *sql,
    const char *json,
    const char *model_id,
    char **out_value,
    char *out_error,
    size_t error_capacity
) {
    sqlite3_stmt *statement = NULL;
    int result = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (result == SQLITE_OK) result = sqlite3_bind_text(statement, 1, json, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK && model_id) result = sqlite3_bind_text(statement, 2, model_id, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK) result = sqlite3_step(statement);
    if (result == SQLITE_ROW) {
        if (out_value) {
            const unsigned char *value = sqlite3_column_text(statement, 0);
            if (value) {
                *out_value = _strdup((const char *)value);
                if (!*out_value) result = SQLITE_NOMEM;
                else result = SQLITE_OK;
            } else {
                result = SQLITE_OK;
            }
        } else {
            result = SQLITE_OK;
        }
    } else if (result == SQLITE_DONE && out_value) {
        result = SQLITE_OK;
    }
    if (result != SQLITE_OK) {
        set_error(out_error, error_capacity, sqlite3_errmsg(database));
        sqlite3_finalize(statement);
        return false;
    }
    sqlite3_finalize(statement);
    return true;
}

VscodeModelConfigResult vscode_model_config_add(
    const wchar_t *config_path,
    const char *model_id,
    char *out_error,
    size_t error_capacity
) {
    if (out_error && error_capacity > 0) out_error[0] = '\0';
    if (!config_path || !model_id || !model_id[0]) {
        set_error(out_error, error_capacity, "A configuration path and model ID are required.");
        return VSCODE_MODEL_CONFIG_ERROR;
    }

    char *json = NULL;
    size_t json_length = 0;
    bool has_bom = false;
    if (!read_config_file(config_path, &json, &json_length, &has_bom)) {
        char message[128];
        snprintf(message, sizeof(message), "Could not read the VS Code configuration (Windows error %lu).",
            (unsigned long)GetLastError());
        set_error(out_error, error_capacity, message);
        return VSCODE_MODEL_CONFIG_ERROR;
    }
    if (memchr(json, '\0', json_length) != NULL) {
        set_error(out_error, error_capacity, "The VS Code configuration contains an embedded NUL byte.");
        free(json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }

    sqlite3 *database = NULL;
    int result = sqlite3_open(":memory:", &database);
    if (result != SQLITE_OK) {
        set_error(out_error, error_capacity, database ? sqlite3_errmsg(database) : "Could not initialize JSON validation.");
        if (database) sqlite3_close(database);
        free(json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }

    char *root_type = NULL;
    if (!sqlite_exec_bound_json(database, "SELECT json_type(?1,'$')", json, NULL,
            &root_type, out_error, error_capacity)) {
        sqlite3_close(database);
        free(json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }
    if (!root_type || strcmp(root_type, "array") != 0) {
        free(root_type);
        set_error(out_error, error_capacity, "The VS Code configuration must contain a JSON array of providers.");
        sqlite3_close(database);
        free(json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }
    free(root_type);

    char *provider_index_text = NULL;
    if (!sqlite_exec_bound_json(
            database,
            "SELECT key FROM json_each(?1) "
            "WHERE json_extract(value,'$.name')='KiosAPI All' "
            "AND json_type(value,'$.models')='array' LIMIT 1",
            json, NULL, &provider_index_text, out_error, error_capacity)) {
        sqlite3_close(database);
        free(json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }
    if (!provider_index_text) {
        set_error(out_error, error_capacity,
            "The KiosAPI All provider with a models array was not found in the VS Code configuration.");
        sqlite3_close(database);
        free(json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }

    sqlite3_stmt *check = NULL;
    result = sqlite3_prepare_v2(
        database,
        "SELECT EXISTS("
        "SELECT 1 FROM json_each(?1) AS provider, "
        "json_each(json_extract(provider.value,'$.models')) AS model "
        "WHERE json_extract(provider.value,'$.name')='KiosAPI All' "
        "AND json_extract(model.value,'$.id')=?2)",
        -1, &check, NULL
    );
    if (result == SQLITE_OK) result = sqlite3_bind_text(check, 1, json, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK) result = sqlite3_bind_text(check, 2, model_id, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK) result = sqlite3_step(check);
    bool already_present = result == SQLITE_ROW && sqlite3_column_int(check, 0) != 0;
    if (result != SQLITE_ROW) {
        set_error(out_error, error_capacity, sqlite3_errmsg(database));
        sqlite3_finalize(check);
        free(provider_index_text);
        sqlite3_close(database);
        free(json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }
    sqlite3_finalize(check);
    if (already_present) {
        free(provider_index_text);
        sqlite3_close(database);
        free(json);
        return VSCODE_MODEL_CONFIG_ALREADY_PRESENT;
    }

    const char *update_sql =
        "SELECT json_pretty(json_set(?1, "
        "'$[' || ?2 || '].models[#]', "
        "json_object('id',?3,'name',?3,'url','https://kiosapi.com/v1',"
        "'toolCalling',json('true'),'vision',json('true'),"
        "'maxInputTokens',128000,'maxOutputTokens',16000)))";
    sqlite3_stmt *update = NULL;
    result = sqlite3_prepare_v2(database, update_sql, -1, &update, NULL);
    if (result == SQLITE_OK) result = sqlite3_bind_text(update, 1, json, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK) result = sqlite3_bind_text(update, 2, provider_index_text, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK) result = sqlite3_bind_text(update, 3, model_id, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK) result = sqlite3_step(update);
    char *updated_json = NULL;
    if (result == SQLITE_ROW) {
        const unsigned char *value = sqlite3_column_text(update, 0);
        if (value) updated_json = _strdup((const char *)value);
        if (!updated_json) result = SQLITE_NOMEM;
    }
    if (result != SQLITE_ROW) {
        set_error(out_error, error_capacity, sqlite3_errmsg(database));
        sqlite3_finalize(update);
        free(provider_index_text);
        sqlite3_close(database);
        free(json);
        free(updated_json);
        return VSCODE_MODEL_CONFIG_ERROR;
    }
    sqlite3_finalize(update);
    free(provider_index_text);

    size_t updated_length = strlen(updated_json);
    bool written = write_config_file(config_path, updated_json, updated_length, has_bom, out_error, error_capacity);
    free(updated_json);
    sqlite3_close(database);
    free(json);
    return written ? VSCODE_MODEL_CONFIG_ADDED : VSCODE_MODEL_CONFIG_ERROR;
}
