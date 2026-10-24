#include "vscode_model_config.h"
#include <sqlite3.h>
#include <assert.h>

static void write_text_file(const wchar_t *path, const char *text) {
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    DWORD length = (DWORD)strlen(text);
    DWORD written = 0;
    assert(WriteFile(file, text, length, &written, NULL));
    assert(written == length);
    CloseHandle(file);
}

static char *read_text_file(const wchar_t *path) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    LARGE_INTEGER size;
    assert(GetFileSizeEx(file, &size));
    assert(size.QuadPart > 0 && size.QuadPart < 1024 * 1024);
    char *text = (char *)malloc((size_t)size.QuadPart + 1);
    assert(text != NULL);
    DWORD read = 0;
    assert(ReadFile(file, text, (DWORD)size.QuadPart, &read, NULL));
    assert(read == (DWORD)size.QuadPart);
    text[read] = '\0';
    CloseHandle(file);
    return text;
}

static int json_integer(const char *json, const char *sql) {
    sqlite3 *database = NULL;
    assert(sqlite3_open(":memory:", &database) == SQLITE_OK);
    sqlite3_stmt *statement = NULL;
    assert(sqlite3_prepare_v2(database, sql, -1, &statement, NULL) == SQLITE_OK);
    assert(sqlite3_bind_text(statement, 1, json, -1, SQLITE_TRANSIENT) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    int value = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    sqlite3_close(database);
    return value;
}

int wmain(void) {
    wchar_t temp_path[MAX_PATH];
    wchar_t config_path[MAX_PATH];
    assert(GetTempPathW(MAX_PATH, temp_path) > 0);
    assert(GetTempFileNameW(temp_path, L"wfm", 0, config_path) != 0);
    const char *initial_json =
        "[{\"name\":\"KiosAPI Free\",\"models\":[{\"id\":\"keep-free\"}]},"
        "{\"name\":\"KiosAPI All\",\"vendor\":\"customendpoint\","
        "\"apiKey\":\"${input:key}\",\"apiType\":\"responses\","
        "\"models\":[{\"id\":\"existing\"}]}]";
    write_text_file(config_path, initial_json);

    char error[256] = {0};
    assert(vscode_model_config_add(config_path, "new-model", error, sizeof(error)) ==
        VSCODE_MODEL_CONFIG_ADDED);
    char *updated_json = read_text_file(config_path);
    assert(json_integer(updated_json,
        "SELECT json_array_length(json_extract(?1,'$[0].models'))") == 1);
    assert(json_integer(updated_json,
        "SELECT json_array_length(json_extract(?1,'$[1].models'))") == 2);
    assert(json_integer(updated_json,
        "SELECT json_extract(?1,'$[1].models[1].toolCalling')") == 1);
    assert(json_integer(updated_json,
        "SELECT json_extract(?1,'$[1].models[1].maxInputTokens')") == 128000);
    assert(strstr(updated_json, "${input:key}") != NULL);
    free(updated_json);

    assert(vscode_model_config_add(config_path, "new-model", error, sizeof(error)) ==
        VSCODE_MODEL_CONFIG_ALREADY_PRESENT);
    updated_json = read_text_file(config_path);
    assert(json_integer(updated_json,
        "SELECT json_array_length(json_extract(?1,'$[1].models'))") == 2);
    free(updated_json);

    wchar_t pattern[MAX_PATH + 32];
    swprintf(pattern, MAX_PATH + 32, L"%ls.bak.*", config_path);
    WIN32_FIND_DATAW find_data;
    HANDLE find = FindFirstFileW(pattern, &find_data);
    assert(find != INVALID_HANDLE_VALUE);
    do {
        wchar_t backup_path[MAX_PATH + 64];
        swprintf(backup_path, MAX_PATH + 64, L"%ls\\%ls", temp_path, find_data.cFileName);
        assert(DeleteFileW(backup_path));
    } while (FindNextFileW(find, &find_data));
    FindClose(find);
    assert(DeleteFileW(config_path));
    wprintf(L"VS Code model config add, backup, preservation, and duplicate tests passed.\n");
    return 0;
}
