# stm_sd

`stm_sd` 是同步 SD/SD NAND 块设备核心，采用“器件描述符 + host 操作表 + 实例上下文”分层。核心只依赖 C11 和 `stm_common`，不依赖 STM32 HAL、RTOS、RTT 或日志；SDMMC 适配器单独编译。

当前源码内置 `sd_generic` 和 `sd_xczsdnand4gas` 两个默认速率描述符，默认速率上限为 25 MHz；后者只表达 XCZSDNAND4GAS 的已确认默认模式限制，不伪造卡的容量或身份。卡容量、512 字节逻辑扇区和写保护状态由 host 初始化返回。

当前发布 **v1.0.2** 为文档补丁，补充支持的 Driver 清单；驱动源码/API 与 v1.0.1 相同，不新增实板验证结论。

## 支持的 Driver

| 类型 | Driver / 型号 | 源码或接入入口 | 支持范围与限制 |
| --- | --- | --- | --- |
| SD 器件描述符 | 通用 SD / TF 卡默认速率模式 | [`sd_generic`](src/devices/sd_generic.c) | 默认速率上限 25 MHz；容量由 host 初始化查询，不按卡品牌预置容量 |
| SD NAND 器件描述符 | XCZSDNAND4GAS | [`sd_xczsdnand4gas`](src/devices/xczsdnand4gas.c) | SD 协议、默认速率上限 25 MHz；描述符只约束速率，不自动识别该型号或伪造容量 |
| 控制器后端 | STM32 HAL SDMMC | [`sdmmc_bind()`](adapters/stm32_hal/sdmmc.h)；target `stm_sd_sdmmc` | `STM_SD_WITH_SDMMC=ON`；同步块读写；1-bit / 可选 4-bit 总线；512 B 逻辑扇区 |

当前没有 SPI 模式 SD、eMMC 或裸 NAND Driver。`sd_generic` 不是所有卡的兼容性/板测保证；自定义 host 是扩展契约。实际 SD 时钟由板级提供，不能把 HAL 编译或主机测试通过等同于卡、布线和供电验证通过。

## 接入

核心可以接入任意提供同步块传输的 host：

```cmake
add_subdirectory(stm_common)
add_subdirectory(stm_sd)
target_link_libraries(app PRIVATE stm_sd)
```

STM32 HAL SDMMC 适配器需要显式加入：

```cmake
set(STM_SD_WITH_SDMMC ON)
add_subdirectory(stm_sd/adapters/stm32_hal)
target_compile_definitions(stm_sd_sdmmc PUBLIC
    STM_SD_HAL_HEADER="stm32h7xx_hal.h")
target_link_libraries(app PRIVATE stm_sd_sdmmc)
```

```c
#include "stm_sd.h"
#include "sdmmc.h"

static sdmmc_context_t host_context;
static sd_handle_t card;
const sd_config_t config = {
    .device = &sd_xczsdnand4gas,
    .host = sdmmc_bind(&host_context, &hsd2, 24000000U, 1U),
    .timeout_ms = 1000U,
};
stm_err_t error = sd_create(&config, &card);
```

`sd_create` 会调用 host 初始化并读取几何信息，不格式化、不写入。`sd_read_blocks`、`sd_write_blocks` 使用逻辑扇区号和数量，当前要求 512 字节扇区；成功写入返回前会等待卡回到 transfer 状态。通信故障、超时或无卡会使实例失效，应用应卸载上层文件系统、删除实例并在硬件恢复后重新创建。删除实例不会释放或重新配置 HAL 句柄。

`sdmmc_bind` 的第三个参数是 CubeMX 分频之后的实际 SD 时钟，第四个参数 `wide_bus` 控制是否请求 4-bit 总线。SDMMC 外设、卡检测、GPIO、时钟、DMA 和 Cache 策略由 CubeMX/BSP 负责。XCZSDNAND4GAS 默认模式的 SD 时钟必须保持不高于 25 MHz；如果板级确认高速度模式和时序，再由新的器件描述符明确放宽。

## 设备映射和验证

同一个核心可以创建多个独立实例，例如把 SD NAND 绑定到 `SDMMC2`、TF 卡绑定到 `SDMMC1`。FatFs 接入由 `stm_fatfs_sd` 完成，不在本组件内注册盘号。

```sh
cmake -S tests -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

主机测试覆盖多块读写、回读和边界检查。`example/` 是板级参考入口，需要使用工程提供 HAL、`main.h` 和 `sdmmc.h`，不会自动加入库目标。裸扇区写测试只能在已备份并明确预留的区域执行。
