#include "sdmmc.h"
#include <stdint.h>

static stm_err_t status(sdmmc_context_t *ctx, HAL_StatusTypeDef value)
{
    ctx->last_hal_status = value;
    if (value == HAL_OK) return STM_OK;
    if (value == HAL_TIMEOUT) return STM_ERR_TIMEOUT;
    if (value == HAL_BUSY) return STM_ERR_INVALID_CONTEXT;
    return STM_ERR_IO;
}

static stm_err_t check_context(void *opaque)
{
    sdmmc_context_t *ctx = opaque;
    return ctx && ctx->hal ? STM_OK : STM_ERR_INVALID_CONTEXT;
}

static stm_err_t initialize(void *opaque, uint32_t max_clock_hz, sd_geometry_t *geometry)
{
    sdmmc_context_t *ctx = opaque;
    if (!ctx || !ctx->hal || !geometry || !ctx->clock_hz || ctx->clock_hz > max_clock_hz) {
        return STM_ERR_INVALID_CONFIG;
    }
    stm_err_t e = status(ctx, HAL_SD_Init(ctx->hal));
    if (e) return e;
#ifdef SDMMC_BUS_WIDE_4B
    if (ctx->wide_bus) {
        e = status(ctx, HAL_SD_ConfigWideBusOperation(ctx->hal, SDMMC_BUS_WIDE_4B));
        if (e) return e;
    }
#endif
    HAL_SD_CardInfoTypeDef card;
    HAL_SD_GetCardInfo(ctx->hal, &card);
    if (!card.BlockSize || !card.BlockNbr) return STM_ERR_INVALID_CONFIG;
    if (max_clock_hz == 0U) return STM_ERR_INVALID_CONFIG;
    geometry->sector_size = 512U;
    geometry->sector_count = card.BlockNbr * ((uint64_t)card.BlockSize / 512U);
    geometry->clock_hz = ctx->clock_hz;
    geometry->write_protected = 0U;
    return STM_OK;
}

static stm_err_t card_status(void *opaque)
{
    sdmmc_context_t *ctx = opaque;
    HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(ctx->hal);
    if (state == HAL_SD_CARD_TRANSFER) return STM_OK;
    if (state == HAL_SD_CARD_ERROR) return STM_ERR_IO;
    return STM_ERR_TIMEOUT;
}

static stm_err_t read_blocks(void *opaque, void *data, uint64_t sector, uint32_t count, uint32_t timeout)
{
    sdmmc_context_t *ctx = opaque;
    if (sector > UINT32_MAX || count == 0U || count > UINT32_MAX - (uint32_t)sector) {
        return STM_ERR_OUT_OF_RANGE;
    }
    return status(ctx, HAL_SD_ReadBlocks(ctx->hal, data, (uint32_t)sector, count, timeout));
}

static stm_err_t write_blocks(void *opaque, const void *data, uint64_t sector, uint32_t count, uint32_t timeout)
{
    sdmmc_context_t *ctx = opaque;
    if (sector > UINT32_MAX || count == 0U || count > UINT32_MAX - (uint32_t)sector) {
        return STM_ERR_OUT_OF_RANGE;
    }
    return status(ctx, HAL_SD_WriteBlocks(ctx->hal, (uint8_t *)data, (uint32_t)sector, count, timeout));
}

static stm_err_t sync(void *opaque, uint32_t timeout)
{
    sdmmc_context_t *ctx = opaque;
    uint32_t start = HAL_GetTick();
    do {
        stm_err_t e = card_status(ctx);
        if (!e) return STM_OK;
        if ((uint32_t)(HAL_GetTick() - start) >= timeout) return STM_ERR_TIMEOUT;
    } while (1);
}

const sd_host_ops_t sdmmc_ops = {
    .check_context = check_context, .initialize = initialize, .status = card_status,
    .read = read_blocks, .write = write_blocks, .sync = sync,
};

sd_host_t sdmmc_bind(sdmmc_context_t *ctx, SD_HandleTypeDef *hal,
                     uint32_t clock_hz, uint8_t wide_bus)
{
    if (ctx) {
        ctx->hal = hal;
        ctx->last_hal_status = HAL_OK;
        ctx->clock_hz = clock_hz;
        ctx->wide_bus = wide_bus;
    }
    return (sd_host_t){.ops = &sdmmc_ops, .ctx = ctx};
}
