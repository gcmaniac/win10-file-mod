#ifndef VAULT_H
#define VAULT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_EMAIL_LEN           256
#define MAX_OTP_LEN             8
#define MAX_FAILED_ATTEMPTS     5
#define OTP_EXPIRY_SECONDS      900     /* 15 minutes */

typedef enum {
    AUTH_SUCCESS = 1,
    AUTH_FAILED = 0,
    AUTH_LOCKED = -1
} AuthResult;

/* Initialize vault subsystem & check if vault exists */
bool vault_init(void);

/* Check if master password & recovery email have been registered */
bool vault_is_configured(void);

/* Setup initial master password & mandatory recovery email */
bool vault_setup(const wchar_t *master_password, const wchar_t *recovery_email);

/* Authenticate master password in login screen */
AuthResult vault_authenticate(const wchar_t *candidate_password, int *out_remaining_attempts);

/* Check if account is locked after 5 failed attempts */
bool vault_is_locked(void);

/* Get number of failed attempts */
int vault_get_failed_attempts(void);

/* Reset failed attempts and lock state (used after OTP verification) */
void vault_unlock(void);

/* Retrieve masked recovery email for UI (e.g. "s***t@gmail.com") */
bool vault_get_masked_email(wchar_t *out_masked, size_t max_len);

/* Retrieve raw registered recovery email (used as pure salt for encryption) */
bool vault_get_recovery_email(wchar_t *out_email, size_t max_len);

/* Generate a cryptographically secure 6-digit OTP code */
bool vault_generate_otp(char *out_otp, size_t max_len);

/* Send reset OTP code to the registered recovery email */
bool vault_send_reset_email(const char *otp_code, wchar_t *out_status, size_t max_status);

/* Verify OTP code and set new master password */
bool vault_verify_otp_and_reset(const char *input_otp, const wchar_t *new_password);

/* Get verified session password (valid only while logged in) */
const wchar_t *vault_get_session_password(void);

/* Derive ginger (pepper) from password to be used in file encryption */
bool vault_derive_ginger(const wchar_t *password, uint8_t *out_ginger, size_t ginger_len);

/* Logout and securely zero session password in memory */
void vault_logout(void);

#ifdef __cplusplus
}
#endif

#endif /* VAULT_H */
