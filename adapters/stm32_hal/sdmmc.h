#ifndef STM_SD_SDMMC_H
#define STM_SD_SDMMC_H
#include "stm_sd_host.h"
#ifndef STM_SD_HAL_HEADER
#define STM_SD_HAL_HEADER "stm32h7xx_hal.h"
#endif
#include STM_SD_HAL_HEADER
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    SD_HandleTypeDef *hal;
    HAL_StatusTypeDef last_hal_status;
    uint32_t clock_hz; /* Actual SD clock after the CubeMX divider. */
    uint8_t wide_bus;
} sdmmc_context_t;
sd_host_t sdmmc_bind(sdmmc_context_t *ctx, SD_HandleTypeDef *hal,
                     uint32_t clock_hz, uint8_t wide_bus);
extern const sd_host_ops_t sdmmc_ops;
#ifdef __cplusplus
}
#endif
#endif
