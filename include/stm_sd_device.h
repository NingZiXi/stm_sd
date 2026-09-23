#ifndef STM_SD_DEVICE_H
#define STM_SD_DEVICE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { const char *name; uint32_t max_clock_hz; } sd_device_t;
extern const sd_device_t sd_generic;
extern const sd_device_t sd_xczsdnand4gas;
#ifdef __cplusplus
}
#endif
#endif
