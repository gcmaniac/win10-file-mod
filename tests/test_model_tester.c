#include "model_tester.h"
#include <sqlite3.h>
#include <assert.h>

static void execute_sql(sqlite3 *database, const char *sql) {
    char *error = NULL;
    int result = sqlite3_exec(database, sql, NULL, NULL, &error);
    if (result != SQLITE_OK) {
        fprintf(stderr, "SQLite test setup failed: %s\n", error ? error : sqlite3_errmsg(database));
        sqlite3_free(error);
        assert(result == SQLITE_OK);
    }
}

static ModelTestRow *load_rows(const wchar_t *database_path, ModelSortColumn column, bool descending, size_t *count) {
    ModelTestRow *rows = NULL;
    char error[256] = {0};
    bool loaded = model_tester_load_rows(database_path, column, descending, &rows, count, error, sizeof(error));
    if (!loaded) fprintf(stderr, "Could not load model rows: %s\n", error);
    assert(loaded);
    return rows;
}

int wmain(void) {
    ModelTestRow price_row = {0};
    price_row.has_input_price = true;
    price_row.has_output_price = true;
    price_row.input_price_per_1m = 0.5;
    price_row.output_price_per_1m = 0.8;
    assert(model_tester_row_matches_price_filter(&price_row, false, 0.0, false, 0.0));
    assert(model_tester_row_matches_price_filter(&price_row, true, 0.5, true, 0.8));
    assert(!model_tester_row_matches_price_filter(&price_row, true, 0.49, false, 0.0));
    assert(!model_tester_row_matches_price_filter(&price_row, false, 0.0, true, 0.79));
    price_row.has_input_price = false;
    assert(!model_tester_row_matches_price_filter(&price_row, true, 1.0, false, 0.0));

    wchar_t temp_path[MAX_PATH];
    wchar_t database_path[MAX_PATH];
    assert(GetTempPathW(MAX_PATH, temp_path) > 0);
    assert(GetTempFileNameW(temp_path, L"wfm", 0, database_path) != 0);
    assert(DeleteFileW(database_path));

    sqlite3 *database = NULL;
    assert(sqlite3_open16(database_path, &database) == SQLITE_OK);
    execute_sql(database,
        "CREATE TABLE models ("
        "model_name TEXT PRIMARY KEY, vendor_name TEXT, "
        "ui_input_price_per_1m REAL, ui_output_price_per_1m REAL, "
        "ui_currency TEXT, supported_endpoints_json TEXT NOT NULL);"
        "INSERT INTO models VALUES"
        "('alpha-cheap','Provider A',0.1,0.2,'$','[\"openai\"]'),"
        "('beta-mid','Provider B',0.5,0.8,'$','[\"openai\"]'),"
        "('gamma-unknown','Provider C',NULL,NULL,'$','[\"openai\"]');"
    );
    sqlite3_close(database);

    size_t count = 0;
    ModelTestRow *rows = load_rows(database_path, MODEL_SORT_OUTPUT_PRICE, false, &count);
    assert(count == 3);
    assert(strcmp(rows[0].model_name, "alpha-cheap") == 0);
    assert(strcmp(rows[1].model_name, "beta-mid") == 0);
    assert(strcmp(rows[2].model_name, "gamma-unknown") == 0);
    free(rows);

    assert(sqlite3_open16(database_path, &database) == SQLITE_OK);
    execute_sql(database,
        "INSERT INTO model_test_results(model_name,succeeded,latency_ms,http_status,result) VALUES"
        "('beta-mid',1,100,200,'HTTP 200'),"
        "('beta-mid',0,222,500,'HTTP 500'),"
        "('alpha-cheap',1,50,200,'HTTP 200');"
    );
    sqlite3_close(database);

    rows = load_rows(database_path, MODEL_SORT_UPTIME, true, &count);
    assert(count == 3);
    assert(strcmp(rows[0].model_name, "alpha-cheap") == 0);
    assert(rows[0].has_uptime && rows[0].uptime_percent == 100.0);
    assert(rows[0].test_count == 1 && rows[0].success_count == 1);
    assert(rows[0].has_last_latency && rows[0].last_latency_ms == 50.0);
    assert(strcmp(rows[1].model_name, "beta-mid") == 0);
    assert(rows[1].has_uptime && rows[1].uptime_percent == 50.0);
    assert(rows[1].test_count == 2 && rows[1].success_count == 1);
    assert(rows[1].has_last_latency && rows[1].last_latency_ms == 222.0);
    assert(!rows[2].has_uptime && !rows[2].has_last_latency);
    free(rows);

    rows = load_rows(database_path, MODEL_SORT_INPUT_PRICE, false, &count);
    assert(strcmp(rows[0].model_name, "alpha-cheap") == 0);
    assert(strcmp(rows[1].model_name, "beta-mid") == 0);
    assert(strcmp(rows[2].model_name, "gamma-unknown") == 0);
    free(rows);

    rows = load_rows(database_path, MODEL_SORT_OUTPUT_PRICE, true, &count);
    assert(strcmp(rows[0].model_name, "beta-mid") == 0);
    assert(strcmp(rows[1].model_name, "alpha-cheap") == 0);
    assert(strcmp(rows[2].model_name, "gamma-unknown") == 0);
    free(rows);

    assert(DeleteFileW(database_path));
    wprintf(L"Model catalog loading, observed availability, and sorting tests passed.\n");
    return 0;
}
