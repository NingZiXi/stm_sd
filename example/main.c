#include "stm_sd.h"
#include "sdmmc.h"
#include "main.h"
#include <string.h>

volatile stm_err_t example_result = STM_OK;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_SDMMC2_SD_Init();

    static sdmmc_context_t host_context;
    sd_handle_t device = NULL;
    const sd_config_t config = {
        .device = &sd_xczsdnand4gas,
        .host = sdmmc_bind(&host_context, &hsd2, 24000000U, 1U),
        .timeout_ms = 1000U,
    };
    uint8_t tx[512] = {0};
    uint8_t rx[512] = {0};
    for (unsigned i = 0U; i < sizeof(tx); ++i) { tx[i] = (uint8_t)(i ^ 0x5AU); }

    example_result = sd_create(&config, &device);
    if (example_result == STM_OK) { example_result = sd_write_blocks(device, tx, 8U, 1U); }
    if (example_result == STM_OK) { example_result = sd_read_blocks(device, rx, 8U, 1U); }
    if (example_result == STM_OK && memcmp(tx, rx, sizeof(tx)) != 0) { example_result = STM_ERR_VERIFY; }
    (void)sd_delete(&device);
    for (;;) { __WFI(); }
}
