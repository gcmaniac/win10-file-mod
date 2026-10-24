#include "model_tester.h"
#include <sqlite3.h>

bool model_tester_row_matches_price_filter(
    const ModelTestRow *row,
    bool has_max_input_price,
    double max_input_price,
    bool has_max_output_price,
    double max_output_price
) {
    if (!row) return false;
    if (has_max_input_price &&
        (!row->has_input_price || row->input_price_per_1m > max_input_price)) {
        return false;
    }
    if (has_max_output_price &&
        (!row->has_output_price || row->output_price_per_1m > max_output_price)) {
        return false;
    }
    return true;
}
#include <winhttp.h>

#include <stdio.h>

#define MODEL_TEST_TIMEOUT_MS 15000
#define MODEL_TEST_RESPONSE_CAPACITY 16384

typedef struct {
    char *name;
    bool image_generation;
} ModelName;

static void copy_error(char *out_error, size_t capacity, const char *message) {
    if (!out_error || capacity == 0) return;
    strncpy(out_error, message, capacity - 1);
    out_error[capacity - 1] = '\0';
}

static sqlite3 *open_database(const wchar_t *path, char *out_error, size_t error_capacity) {
    if (!path || !path[0]) {
        copy_error(out_error, error_capacity, "Database path is empty.");
        return NULL;
    }

    sqlite3 *database = NULL;
    int result = sqlite3_open16(path, &database);
    if (result != SQLITE_OK) {
        copy_error(out_error, error_capacity, database ? sqlite3_errmsg(database) : "Could not open model database.");
        if (database) sqlite3_close(database);
        return NULL;
    }
    sqlite3_busy_timeout(database, 5000);
    return database;
}

static bool ensure_results_table(sqlite3 *database, char *out_error, size_t error_capacity) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS model_test_results ("
        "test_id INTEGER PRIMARY KEY,"
        "model_name TEXT NOT NULL REFERENCES models(model_name),"
        "tested_at_utc TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
        "succeeded INTEGER NOT NULL CHECK (succeeded IN (0,1)),"
        "latency_ms REAL,"
        "http_status INTEGER,"
        "result TEXT NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_model_test_results_model_time "
        "ON model_test_results(model_name, test_id);";
    char *sqlite_error = NULL;
    int result = sqlite3_exec(database, sql, NULL, NULL, &sqlite_error);
    if (result != SQLITE_OK) {
        copy_error(out_error, error_capacity, sqlite_error ? sqlite_error : sqlite3_errmsg(database));
        sqlite3_free(sqlite_error);
        return false;
    }
    return true;
}

static const char *sort_clause(ModelSortColumn column, bool descending) {
    switch (column) {
        case MODEL_SORT_OUTPUT_PRICE:
            return descending
                ? "ORDER BY (m.ui_output_price_per_1m IS NULL), m.ui_output_price_per_1m DESC, (COUNT(r.test_id)=0), 100.0*SUM(r.succeeded)/COUNT(r.test_id) DESC, m.model_name COLLATE NOCASE"
                : "ORDER BY (m.ui_output_price_per_1m IS NULL), m.ui_output_price_per_1m ASC, (COUNT(r.test_id)=0), 100.0*SUM(r.succeeded)/COUNT(r.test_id) DESC, m.model_name COLLATE NOCASE";
        case MODEL_SORT_INPUT_PRICE:
            return descending
                ? "ORDER BY (m.ui_input_price_per_1m IS NULL), m.ui_input_price_per_1m DESC, (COUNT(r.test_id)=0), 100.0*SUM(r.succeeded)/COUNT(r.test_id) DESC, m.model_name COLLATE NOCASE"
                : "ORDER BY (m.ui_input_price_per_1m IS NULL), m.ui_input_price_per_1m ASC, (COUNT(r.test_id)=0), 100.0*SUM(r.succeeded)/COUNT(r.test_id) DESC, m.model_name COLLATE NOCASE";
        case MODEL_SORT_UPTIME:
        default:
            return descending
                ? "ORDER BY (COUNT(r.test_id)=0), 100.0*SUM(r.succeeded)/COUNT(r.test_id) DESC, (m.ui_output_price_per_1m IS NULL), m.ui_output_price_per_1m ASC, (m.ui_input_price_per_1m IS NULL), m.ui_input_price_per_1m ASC, m.model_name COLLATE NOCASE"
                : "ORDER BY (COUNT(r.test_id)=0), 100.0*SUM(r.succeeded)/COUNT(r.test_id) ASC, m.model_name COLLATE NOCASE";
    }
}

bool model_tester_load_rows(
    const wchar_t *database_path,
    ModelSortColumn sort_column,
    bool descending,
    ModelTestRow **out_rows,
    size_t *out_count,
    char *out_error,
    size_t error_capacity
) {
    if (out_rows) *out_rows = NULL;
    if (out_count) *out_count = 0;
    if (out_error && error_capacity > 0) out_error[0] = '\0';
    if (!out_rows || !out_count) {
        copy_error(out_error, error_capacity, "Invalid model-list request.");
        return false;
    }

    sqlite3 *database = open_database(database_path, out_error, error_capacity);
    if (!database) return false;
    bool success = ensure_results_table(database, out_error, error_capacity);
    if (!success) {
        sqlite3_close(database);
        return false;
    }

    char sql[2048];
    int sql_len = snprintf(
        sql, sizeof(sql),
        "SELECT m.model_name, COALESCE(m.vendor_name,''), m.ui_input_price_per_1m, "
        "m.ui_output_price_per_1m, COALESCE(m.ui_currency,'$'), "
        "COUNT(r.test_id), COALESCE(SUM(r.succeeded),0), "
        "CASE WHEN COUNT(r.test_id)>0 THEN 100.0*SUM(r.succeeded)/COUNT(r.test_id) END, "
        "(SELECT latency_ms FROM model_test_results lr WHERE lr.model_name=m.model_name ORDER BY lr.test_id DESC LIMIT 1), "
        "COALESCE((SELECT result FROM model_test_results lr WHERE lr.model_name=m.model_name ORDER BY lr.test_id DESC LIMIT 1),'') "
        "FROM models m LEFT JOIN model_test_results r ON r.model_name=m.model_name "
        "GROUP BY m.model_name %s",
        sort_clause(sort_column, descending)
    );
    if (sql_len < 0 || (size_t)sql_len >= sizeof(sql)) {
        copy_error(out_error, error_capacity, "Could not construct model query.");
        sqlite3_close(database);
        return false;
    }

    sqlite3_stmt *statement = NULL;
    int result = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (result != SQLITE_OK) {
        copy_error(out_error, error_capacity, sqlite3_errmsg(database));
        sqlite3_close(database);
        return false;
    }

    size_t capacity = 0;
    ModelTestRow *rows = NULL;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        if (*out_count == capacity) {
            size_t new_capacity = capacity == 0 ? 32 : capacity * 2;
            ModelTestRow *new_rows = (ModelTestRow *)realloc(rows, new_capacity * sizeof(*rows));
            if (!new_rows) {
                free(rows);
                rows = NULL;
                *out_count = 0;
                copy_error(out_error, error_capacity, "Out of memory while loading model list.");
                sqlite3_finalize(statement);
                sqlite3_close(database);
                return false;
            }
            rows = new_rows;
            capacity = new_capacity;
        }

        ModelTestRow *row = &rows[*out_count];
        SecureZeroMemory(row, sizeof(*row));
        const unsigned char *text = sqlite3_column_text(statement, 0);
        if (text) strncpy(row->model_name, (const char *)text, sizeof(row->model_name) - 1);
        text = sqlite3_column_text(statement, 1);
        if (text) strncpy(row->provider, (const char *)text, sizeof(row->provider) - 1);

        row->has_input_price = sqlite3_column_type(statement, 2) != SQLITE_NULL;
        row->has_output_price = sqlite3_column_type(statement, 3) != SQLITE_NULL;
        if (row->has_input_price) row->input_price_per_1m = sqlite3_column_double(statement, 2);
        if (row->has_output_price) row->output_price_per_1m = sqlite3_column_double(statement, 3);
        text = sqlite3_column_text(statement, 4);
        if (text) strncpy(row->currency, (const char *)text, sizeof(row->currency) - 1);

        row->test_count = sqlite3_column_int(statement, 5);
        row->success_count = sqlite3_column_int(statement, 6);
        row->has_uptime = sqlite3_column_type(statement, 7) != SQLITE_NULL;
        if (row->has_uptime) row->uptime_percent = sqlite3_column_double(statement, 7);
        row->has_last_latency = sqlite3_column_type(statement, 8) != SQLITE_NULL;
        if (row->has_last_latency) row->last_latency_ms = sqlite3_column_double(statement, 8);
        text = sqlite3_column_text(statement, 9);
        if (text) strncpy(row->last_result, (const char *)text, sizeof(row->last_result) - 1);
        (*out_count)++;
    }

    if (result != SQLITE_DONE) {
        copy_error(out_error, error_capacity, sqlite3_errmsg(database));
        free(rows);
        *out_count = 0;
        rows = NULL;
        success = false;
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
    if (!success) return false;

    *out_rows = rows;
    return true;
}

static bool append_model_escape(char *buffer, size_t capacity, size_t *used, const char *value) {
    for (const unsigned char *p = (const unsigned char *)value; *p; p++) {
        char escaped[7];
        size_t escaped_len = 1;
        escaped[0] = (char)*p;
        if (*p == '"' || *p == '\\') {
            escaped[1] = (char)*p;
            escaped_len = 2;
        } else if (*p < 0x20) {
            snprintf(escaped, sizeof(escaped), "\\u%04x", *p);
            escaped_len = 6;
        }
        if (*used + escaped_len >= capacity) return false;
        memcpy(buffer + *used, escaped, escaped_len);
        *used += escaped_len;
    }
    return true;
}

static bool probe_model(
    HINTERNET connection,
    const char *api_key,
    const char *model_name,
    bool image_generation,
    double *out_latency_ms,
    int *out_http_status,
    char *out_result,
    size_t result_capacity
) {
    if (out_latency_ms) *out_latency_ms = 0;
    if (out_http_status) *out_http_status = 0;
    copy_error(out_result, result_capacity, "Connection failed");

    char body[1024] = "{\"model\":\"";
    size_t used = strlen(body);
    if (!append_model_escape(body, sizeof(body), &used, model_name)) {
        copy_error(out_result, result_capacity, "Invalid model identifier");
        return false;
    }
    const char *suffix = image_generation
        ? "\",\"prompt\":\"A simple blue circle on a white background.\",\"n\":1,\"size\":\"256x256\"}"
        : "\",\"messages\":[{\"role\":\"user\",\"content\":\"Reply with OK\"}],\"max_tokens\":8,\"stream\":false}";
    size_t suffix_len = strlen(suffix);
    if (used + suffix_len >= sizeof(body)) {
        copy_error(out_result, result_capacity, "Invalid model request");
        return false;
    }
    memcpy(body + used, suffix, suffix_len + 1);
    used += suffix_len;

    wchar_t headers[8192];
    int header_len = swprintf(
        headers,
        sizeof(headers) / sizeof(headers[0]),
        L"Content-Type: application/json\r\nAuthorization: Bearer %hs\r\n",
        api_key
    );
    if (header_len < 0 || (size_t)header_len >= sizeof(headers) / sizeof(headers[0])) {
        SecureZeroMemory(headers, sizeof(headers));
        copy_error(out_result, result_capacity, "API key is too large");
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"POST",
        image_generation ? L"/v1/images/generations" : L"/v1/chat/completions",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    if (!request) {
        SecureZeroMemory(headers, sizeof(headers));
        return false;
    }
    WinHttpSetTimeouts(request, MODEL_TEST_TIMEOUT_MS, MODEL_TEST_TIMEOUT_MS, MODEL_TEST_TIMEOUT_MS, MODEL_TEST_TIMEOUT_MS);

    LARGE_INTEGER start;
    LARGE_INTEGER finish;
    QueryPerformanceCounter(&start);
    BOOL sent = WinHttpSendRequest(
        request,
        headers,
        (DWORD)-1L,
        body,
        (DWORD)used,
        (DWORD)used,
        0
    );
    bool success = false;
    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (sent && WinHttpReceiveResponse(request, NULL) &&
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX)) {
        char response[MODEL_TEST_RESPONSE_CAPACITY + 1];
        size_t response_len = 0;
        bool read_ok = true;
        while (response_len < MODEL_TEST_RESPONSE_CAPACITY) {
            DWORD bytes_read = 0;
            DWORD remaining = (DWORD)(MODEL_TEST_RESPONSE_CAPACITY - response_len);
            if (!WinHttpReadData(request, response + response_len, remaining, &bytes_read)) {
                read_ok = false;
                break;
            }
            if (bytes_read == 0) break;
            response_len += bytes_read;
        }
        response[response_len] = '\0';

        const char *expected_response_field = image_generation ? "\"data\"" : "\"choices\"";
        if (status == 200 && read_ok && response_len > 0 && strstr(response, expected_response_field) != NULL) {
            success = true;
            copy_error(out_result, result_capacity, "HTTP 200");
        } else if (status != 200) {
            char status_message[MODEL_TEST_RESULT_CAPACITY];
            snprintf(status_message, sizeof(status_message), "HTTP %lu", (unsigned long)status);
            copy_error(out_result, result_capacity, status_message);
        } else if (!read_ok) {
            copy_error(out_result, result_capacity, "Response read failed");
        } else {
            copy_error(out_result, result_capacity, "Invalid model response");
        }
    }
    QueryPerformanceCounter(&finish);
    if (out_latency_ms) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        if (frequency.QuadPart > 0) {
            *out_latency_ms = (double)(finish.QuadPart - start.QuadPart) * 1000.0 / (double)frequency.QuadPart;
        }
    }
    if (out_http_status) *out_http_status = (int)status;
    WinHttpCloseHandle(request);
    SecureZeroMemory(headers, sizeof(headers));
    return success;
}

static bool record_result(
    sqlite3 *database,
    const char *model_name,
    bool succeeded,
    double latency_ms,
    int http_status,
    const char *result_text,
    char *out_error,
    size_t error_capacity
) {
    sqlite3_stmt *statement = NULL;
    int result = sqlite3_prepare_v2(
        database,
        "INSERT INTO model_test_results(model_name,succeeded,latency_ms,http_status,result) VALUES(?,?,?,?,?)",
        -1,
        &statement,
        NULL
    );
    if (result == SQLITE_OK) {
        sqlite3_bind_text(statement, 1, model_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement, 2, succeeded ? 1 : 0);
        sqlite3_bind_double(statement, 3, latency_ms);
        sqlite3_bind_int(statement, 4, http_status);
        sqlite3_bind_text(statement, 5, result_text, -1, SQLITE_TRANSIENT);
        result = sqlite3_step(statement);
    }
    if (result != SQLITE_DONE) copy_error(out_error, error_capacity, sqlite3_errmsg(database));
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}

static bool model_tester_run_models(
    const wchar_t *database_path,
    const char *api_key,
    const char *const *selected_model_names,
    size_t selected_model_count,
    volatile bool *cancel_flag,
    ModelTestProgressCallback progress,
    void *user_data,
    int *out_completed,
    int *out_succeeded,
    int *out_failed,
    bool *out_cancelled,
    char *out_error,
    size_t error_capacity
) {
    if (out_completed) *out_completed = 0;
    if (out_succeeded) *out_succeeded = 0;
    if (out_failed) *out_failed = 0;
    if (out_cancelled) *out_cancelled = false;
    if (out_error && error_capacity > 0) out_error[0] = '\0';
    if (!api_key || !api_key[0] || !out_completed || !out_succeeded || !out_failed || !out_cancelled ||
        (selected_model_count > 0 && !selected_model_names)) {
        copy_error(out_error, error_capacity, "An unlocked API key is required.");
        return false;
    }

    sqlite3 *database = open_database(database_path, out_error, error_capacity);
    if (!database) return false;
    if (!ensure_results_table(database, out_error, error_capacity)) {
        sqlite3_close(database);
        return false;
    }

    sqlite3_stmt *statement = NULL;
    int result = sqlite3_prepare_v2(
        database,
        "SELECT model_name, supported_endpoints_json FROM models ORDER BY model_name COLLATE NOCASE",
        -1,
        &statement,
        NULL
    );
    if (result != SQLITE_OK) {
        copy_error(out_error, error_capacity, sqlite3_errmsg(database));
        sqlite3_close(database);
        return false;
    }

    size_t count = 0;
    size_t capacity = 0;
    ModelName *models = NULL;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(statement, 0);
        if (!name) continue;
        if (selected_model_count > 0) {
            bool selected = false;
            for (size_t i = 0; i < selected_model_count; i++) {
                if (selected_model_names[i] && strcmp((const char *)name, selected_model_names[i]) == 0) {
                    selected = true;
                    break;
                }
            }
            if (!selected) continue;
        }
        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 32 : capacity * 2;
            ModelName *new_models = (ModelName *)realloc(models, new_capacity * sizeof(*models));
            if (!new_models) {
                copy_error(out_error, error_capacity, "Out of memory while preparing model tests.");
                result = SQLITE_NOMEM;
                break;
            }
            models = new_models;
            capacity = new_capacity;
        }
        size_t name_len = strlen((const char *)name);
        models[count].name = (char *)malloc(name_len + 1);
        if (!models[count].name) {
            copy_error(out_error, error_capacity, "Out of memory while preparing model tests.");
            result = SQLITE_NOMEM;
            break;
        }
        memcpy(models[count].name, name, name_len + 1);
        const unsigned char *endpoints = sqlite3_column_text(statement, 1);
        models[count].image_generation = endpoints &&
            strstr((const char *)endpoints, "\"image-generation\"") != NULL;
        count++;
    }
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE || count == 0) {
        if (result == SQLITE_DONE && count == 0) copy_error(out_error, error_capacity, "No models were found in the database.");
        for (size_t i = 0; i < count; i++) free(models[i].name);
        free(models);
        sqlite3_close(database);
        return false;
    }

    HINTERNET session = WinHttpOpen(L"Win10FileEncryptor/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        copy_error(out_error, error_capacity, "Could not initialize WinHTTP.");
        for (size_t i = 0; i < count; i++) free(models[i].name);
        free(models);
        sqlite3_close(database);
        return false;
    }
    WinHttpSetTimeouts(session, MODEL_TEST_TIMEOUT_MS, MODEL_TEST_TIMEOUT_MS, MODEL_TEST_TIMEOUT_MS, MODEL_TEST_TIMEOUT_MS);
    HINTERNET connection = WinHttpConnect(session, L"kiosapi.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection) {
        copy_error(out_error, error_capacity, "Could not connect to kiosapi.com.");
        WinHttpCloseHandle(session);
        for (size_t i = 0; i < count; i++) free(models[i].name);
        free(models);
        sqlite3_close(database);
        return false;
    }

    bool success = true;
    for (size_t i = 0; i < count; i++) {
        if (cancel_flag && *cancel_flag) {
            *out_cancelled = true;
            break;
        }

        double latency_ms = 0;
        int http_status = 0;
        char result_text[MODEL_TEST_RESULT_CAPACITY] = {0};
        bool model_success = probe_model(
            connection,
            api_key,
            models[i].name,
            models[i].image_generation,
            &latency_ms,
            &http_status,
            result_text,
            sizeof(result_text)
        );

        if (!record_result(database, models[i].name, model_success, latency_ms, http_status, result_text, out_error, error_capacity)) {
            success = false;
            break;
        }

        (*out_completed)++;
        if (model_success) (*out_succeeded)++;
        else (*out_failed)++;
        if (progress) {
            progress(
                *out_completed,
                (int)count,
                models[i].name,
                model_success,
                result_text,
                user_data
            );
        }
    }

    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    for (size_t i = 0; i < count; i++) free(models[i].name);
    free(models);
    sqlite3_close(database);
    return success;
}

bool model_tester_run_all(
    const wchar_t *database_path,
    const char *api_key,
    volatile bool *cancel_flag,
    ModelTestProgressCallback progress,
    void *user_data,
    int *out_completed,
    int *out_succeeded,
    int *out_failed,
    bool *out_cancelled,
    char *out_error,
    size_t error_capacity
) {
    return model_tester_run_models(
        database_path, api_key, NULL, 0, cancel_flag, progress, user_data,
        out_completed, out_succeeded, out_failed, out_cancelled, out_error, error_capacity
    );
}

bool model_tester_run_selected(
    const wchar_t *database_path,
    const char *api_key,
    const char *const *selected_model_names,
    size_t selected_model_count,
    volatile bool *cancel_flag,
    ModelTestProgressCallback progress,
    void *user_data,
    int *out_completed,
    int *out_succeeded,
    int *out_failed,
    bool *out_cancelled,
    char *out_error,
    size_t error_capacity
) {
    if (!selected_model_names || selected_model_count == 0) {
        copy_error(out_error, error_capacity, "Select at least one model to scan.");
        return false;
    }
    return model_tester_run_models(
        database_path, api_key, selected_model_names, selected_model_count, cancel_flag, progress, user_data,
        out_completed, out_succeeded, out_failed, out_cancelled, out_error, error_capacity
    );
}
