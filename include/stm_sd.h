#ifndef STM_SD_H
#define STM_SD_H
#include "stm_sd_host.h"
#include "stm_sd_device.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STM_SD_VERSION "1.0.0"
typedef struct sd_context *sd_handle_t;
typedef struct { const sd_device_t *device; sd_host_t host; uint32_t timeout_ms; } sd_config_t;
typedef struct { sd_geometry_t geometry; stm_err_t last_error; uint8_t ready; } sd_info_t;
/* Serialized thread context. Device copied; host borrowed for entire lifetime.
 * No automatic format/retry, no atomic multi-sector writes. Success waits for
 * card-ready. A fatal IO/removal disables the object; explicitly recreate it.
 * delete never deinitializes the application-owned controller. */
stm_err_t sd_create(const sd_config_t *config, sd_handle_t *out);
stm_err_t sd_delete(sd_handle_t *handle);
stm_err_t sd_get_info(sd_handle_t handle, sd_info_t *info);
stm_err_t sd_status(sd_handle_t handle);
stm_err_t sd_read_blocks(sd_handle_t handle, void *data, uint64_t sector, uint32_t count);
stm_err_t sd_write_blocks(sd_handle_t handle, const void *data, uint64_t sector, uint32_t count);
stm_err_t sd_sync(sd_handle_t handle);
#ifdef __cplusplus
}
#endif
#endif
