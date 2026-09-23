#ifndef STM_SD_HOST_H
#define STM_SD_HOST_H
#include <stddef.h>
#include <stdint.h>
#include "stm_err.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SD_ERR_NO_MEDIA ((stm_err_t)0x4001)
#define SD_ERR_PROTECTED ((stm_err_t)0x4002)
typedef struct {
    uint64_t sector_count;
    uint32_t sector_size, clock_hz, erase_sectors, cid[4], csd[4];
    uint8_t write_protected;
} sd_geometry_t;
/* Protocol-capable host: initialize negotiates card type/addressing. No HAL types.
 * read/write accept arbitrary byte alignment; adapter handles staging if needed.
 * status returns NO_MEDIA only when detection is reliable; sync is bounded. */
typedef struct {
    stm_err_t (*check_context)(void *ctx);
    stm_err_t (*initialize)(void *ctx, uint32_t max_clock_hz, sd_geometry_t *geometry);
    stm_err_t (*status)(void *ctx);
    stm_err_t (*read)(void *ctx, void *data, uint64_t sector, uint32_t count, uint32_t timeout_ms);
    stm_err_t (*write)(void *ctx, const void *data, uint64_t sector, uint32_t count, uint32_t timeout_ms);
    stm_err_t (*sync)(void *ctx, uint32_t timeout_ms);
} sd_host_ops_t;
typedef struct { const sd_host_ops_t *ops; void *ctx; } sd_host_t;
#ifdef __cplusplus
}
#endif
#endif
