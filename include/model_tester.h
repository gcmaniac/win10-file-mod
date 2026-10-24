#ifndef MODEL_TESTER_H
#define MODEL_TESTER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODEL_TEST_NAME_CAPACITY 256
#define MODEL_TEST_PROVIDER_CAPACITY 128
#define MODEL_TEST_RESULT_CAPACITY 96

typedef enum {
    MODEL_SORT_UPTIME = 0,
    MODEL_SORT_OUTPUT_PRICE,
    MODEL_SORT_INPUT_PRICE
} ModelSortColumn;

typedef struct {
    char model_name[MODEL_TEST_NAME_CAPACITY];
    char provider[MODEL_TEST_PROVIDER_CAPACITY];
    char currency[8];
    bool has_input_price;
    bool has_output_price;
    double input_price_per_1m;
    double output_price_per_1m;
    bool has_uptime;
    double uptime_percent;
    int test_count;
    int success_count;
    bool has_last_latency;
    double last_latency_ms;
    char last_result[MODEL_TEST_RESULT_CAPACITY];
} ModelTestRow;

bool model_tester_row_matches_price_filter(
    const ModelTestRow *row,
    bool has_max_input_price,
    double max_input_price,
    bool has_max_output_price,
    double max_output_price
);

typedef void (*ModelTestProgressCallback)(
    int completed,
    int total,
    const char *model_name,
    bool succeeded,
    const char *result,
    void *user_data
);

bool model_tester_load_rows(
    const wchar_t *database_path,
    ModelSortColumn sort_column,
    bool descending,
    ModelTestRow **out_rows,
    size_t *out_count,
    char *out_error,
    size_t error_capacity
);

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
);

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
);

#ifdef __cplusplus
}
#endif

#endif
