#ifndef VSCODE_MODEL_CONFIG_H
#define VSCODE_MODEL_CONFIG_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VSCODE_MODEL_CONFIG_ERROR = -1,
    VSCODE_MODEL_CONFIG_ALREADY_PRESENT = 0,
    VSCODE_MODEL_CONFIG_ADDED = 1
} VscodeModelConfigResult;

VscodeModelConfigResult vscode_model_config_add(
    const wchar_t *config_path,
    const char *model_id,
    char *out_error,
    size_t error_capacity
);

#ifdef __cplusplus
}
#endif

#endif
