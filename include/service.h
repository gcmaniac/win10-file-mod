#ifndef SERVICE_H
#define SERVICE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERVICE_MUTEX_NAME          L"Local\\Win10FileEnc_ServiceMutex_v1"
#define SERVICE_WINDOW_CLASS        L"Win10FileEnc_ServiceWindowClass"

#define WM_APP_TRAYMSG              (WM_APP + 20)
#define WM_APP_SERVICE_STOP         (WM_APP + 21)

/* Check whether the service autostart and explorer shell association are installed */
bool service_is_installed(void);

/* Check whether the background watcher service is currently running */
bool service_is_running(void);

/* Install Windows Explorer shell associations (.enc) & autostart entry */
bool service_install(void);

/* Remove Windows Explorer shell associations & autostart entry */
bool service_uninstall(void);

/* Start the background daemon process */
bool service_start_process(void);

/* Signal the running background daemon process to terminate */
bool service_stop_process(void);

/* Main entry point for the background daemon process */
int service_run_daemon(HINSTANCE hInstance);

/* Decrypt a single .enc file and optionally open the decrypted file in default program */
bool service_auto_decrypt_single_file(const wchar_t *enc_path, bool launch_after);

/* Decrypt all .enc files inside a target folder */
int service_auto_decrypt_folder(const wchar_t *folder_path);

/* Encrypt a single plaintext file and wipe original */
bool service_auto_encrypt_single_file(const wchar_t *plain_path);

/* Tracking and 30-minute auto re-encryption subsystem */
void tracker_init(void);
void tracker_register_decrypted_file(const wchar_t *plain_path);
void tracker_touch_file(const wchar_t *plain_path);
void tracker_remove_file(const wchar_t *plain_path);
bool tracker_is_file_decrypted(const wchar_t *plain_path);
int tracker_check_inactivity(void);
int tracker_encrypt_all_decrypted_files(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_H */
