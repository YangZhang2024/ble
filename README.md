# STM32F1xx 基础脚手架

这是一个适合 STM32F1xx 学习的基础 HAL 工程，当前主程序做一件事：PA8 LED 每 500 ms 翻转一次。工程还带有可复用的软件 I2C 和 ILI9341 并口 LCD 驱动，默认不在 LED 示例中初始化 LCD 或发起 I2C 事务。

## 芯片选择：RCT6 和 C8T6

两个芯片都能使用这个脚手架，但必须选择对应的构建预设：

- `Debug`：STM32F103RCT6，`STM32F103xE`，256 KB Flash、48 KB RAM
- `C8T6-Debug`：STM32F103C8T6，`STM32F103xB`，64 KB Flash、20 KB RAM

C8T6 使用自己的 `startup_stm32f103xb.s`、`stm32f103xb.h` 和 `STM32F103C8Tx_FLASH.ld`，不会把 RCT6 的 256 KB 存储器配置误用于 C8T6。VS Code 中选择对应预设后再构建；命令行分别是：

```bash
cmake --preset Debug                 # RCT6
cmake --build build/Debug

cmake --preset C8T6-Debug            # C8T6
cmake --build build/C8T6-Debug
```

## 工程结构

- `Core/`: 用户程序、启动后的系统时钟和 C 运行库支持
- `Drivers/CMSIS/`: 芯片寄存器定义和 Cortex-M3 支持
- `Drivers/STM32F1xx_HAL_Driver/`: 完整 STM32F1 HAL 源码目录
- `Drivers/LCD/`: ILI9341 16 位并口 LCD 驱动和字库
- `cmake/`: ARM GCC 工具链文件
- `.vscode/`: IntelliSense 和 WSL 构建任务

目录里保留了完整 Driver 源码，但 `CMakeLists.txt` 只编译当前 LED 和软件 I2C 所需的 HAL 文件。源码放在目录里不会自动进入固件，只有写入 `add_executable()` 的 `.c` 文件才会参与编译和链接。

## 当前编译的 HAL 文件

`hal`、`cortex`、`rcc`、`gpio`、`flash`、`pwr`。软件 I2C 不需要 `stm32f1xx_hal_i2c.c`，因为它只用 GPIO 手动产生 I2C 波形。以后增加串口时，再加入 `stm32f1xx_hal_uart.c`；增加定时器时，再加入 `stm32f1xx_hal_tim.c`；使用硬件 I2C/SPI 时，再加入对应 HAL 源文件。

## LCD 使用方式

LCD 驱动位于 [Drivers/LCD/](Drivers/LCD/)，已将原来的 `delay_ms()` 改成 HAL 自带的 `HAL_Delay()`，不依赖 `SYSTEM/delay`。当前引脚沿用 RCT6 开发板：PB0~PB15 为 16 位数据总线，PC6/PC7/PC8/PC9/PC10 分别为 RD/WR/RS/CS/背光。

在 `main.c` 中加入：

```c
#include "lcd.h"

lcd_init();
lcd_show_string(10, 10, 220, 30, 16, "STM32F1xx ready", BLUE);
```

LCD 初始化前必须确认这些 GPIO 已连接并供电。RCT6 的 64 引脚封装可以使用上述整套并口；C8T6 是 48 引脚封装，PC6~PC10 没有全部引出，不能直接照搬这组接线，需要先修改 [lcd.h](Drivers/LCD/lcd.h) 的引脚和 `lcd.c` 的数据总线实现，或使用 SPI LCD 驱动。

## 软件 I2C 使用方式

源码位于 [soft_i2c.h](Core/Inc/soft_i2c.h)、[soft_i2c.c](Core/Src/soft_i2c.c)，完整说明位于 [soft_i2c_usage.md](Core/Inc/soft_i2c_usage.md)。它是阻塞式、单主机软件 I2C，SCL/SDA 必须有外部上拉电阻，不能在中断中调用。

例如使用 PB6 作为 SCL、PB7 作为 SDA：

```c
#include "soft_i2c.h"

static const SoftI2C bus = {GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7};

__HAL_RCC_GPIOB_CLK_ENABLE();
if (!SoftI2C_Init(&bus)) {
    /* 检查供电、共地、SCL/SDA 接线和上拉电阻。 */
}

const uint8_t bytes[] = {0x00, 0xAF};
uint8_t ok = SoftI2C_Write(&bus, 0x3C, bytes, sizeof(bytes));
```

地址传入未左移的 7 位地址，例如 OLED 常见 `0x3C`、24C02 常见 `0x50`。`SoftI2C_Init` 返回 1 只表示总线空闲，不表示目标设备一定存在；真正访问设备时还要检查 `SoftI2C_Write` 或 `SoftI2C_ReadRegisters` 的返回值。软件 I2C 的详细注意事项、恢复时钟和多段写入示例见 `soft_i2c_usage.md`。

## 如何自己找驱动

1. 先在代码中确定 HAL 类型，例如 `UART_HandleTypeDef`、`TIM_HandleTypeDef`、`I2C_HandleTypeDef`。
2. 在 `Drivers/STM32F1xx_HAL_Driver/Inc/` 找同名头文件：`stm32f1xx_hal_uart.h`、`stm32f1xx_hal_tim.h`、`stm32f1xx_hal_i2c.h`。
3. 在 `Drivers/STM32F1xx_HAL_Driver/Src/` 找同名实现文件，并把它加入 `CMakeLists.txt` 的 `add_executable()`。
4. 在 `Core/Inc/stm32f1xx_hal_conf.h` 打开对应模块宏，例如 `HAL_UART_MODULE_ENABLED`。
5. 在 `main.c` 或独立的 `uart.c` 中完成 GPIO、时钟和 HAL 初始化。

例如增加 USART1，通常需要：

```cmake
${HAL_DIR}/Src/stm32f1xx_hal_uart.c
```

并在 `stm32f1xx_hal_conf.h` 中启用：

```c
#define HAL_UART_MODULE_ENABLED
```

## 构建

使用 VS Code 的 `Ctrl+Shift+B`，或在 WSL 中执行：

```bash
cmake --preset Debug
cmake --build build/Debug
```

输出为 `build/Debug/stm32f1xx_basic_template.elf` 和 `.hex`。

编译成功只证明软件构建正确；LED 是否闪烁还取决于芯片型号、晶振、烧录和实际接线。PA8 是当前模板假定的板载 LED 引脚，若开发板 LED 在其他引脚，只需修改 `Core/Inc/main.h`。
