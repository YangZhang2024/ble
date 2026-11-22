# 软件 I2C 模块

将 Core/Inc/soft_i2c.h 和 Core/Src/soft_i2c.c 加入 STM32F1 HAL 工程。
本模块使用 Cortex-M3 DWT 周期计数器延时，初始化不会清零共享计数器。
初始化前先配置系统时钟、调用 HAL_Init，并启用相应 GPIO 端口时钟。

```c
#include "soft_i2c.h"

static const SoftI2C bus = {GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7};

__HAL_RCC_GPIOB_CLK_ENABLE();
if (!SoftI2C_Init(&bus)) {
    /* 总线被拉低，检查接线、供电和上拉电阻。 */
}

/* 地址不需要左移。以下示例向 SSD1306 写控制字节和开启显示命令。 */
const uint8_t data[] = {0x00, 0xAF};
uint8_t ok = SoftI2C_Write(&bus, 0x3C, data, sizeof(data));

/* 多个缓冲区会在同一次 START/STOP 事务内顺序发送，无需复制拼接。 */
const uint8_t control = 0x40;
const uint8_t pixels[] = {0xFF, 0x81, 0xFF};
const SoftI2C_Buffer parts[] = {{&control, 1}, {pixels, sizeof(pixels)}};
ok = SoftI2C_WriteV(&bus, 0x3C, parts, 2);
```

接口返回 1 表示成功，0 表示参数错误、未应答或总线超时。
SoftI2C_WriteV(&bus, address, NULL, 0) 可探测地址是否应答。
同一总线可连接不同地址的设备，其他设备驱动按各自协议组织数据。
OLED 的控制字节、初始化命令、页地址、字体仍由 oled.c 负责。

边界：当前仅支持同步写入、单主机，不支持读操作、重复 START、多主机仲裁，
也不提供并发锁。不要从多个任务或中断同时使用；SCL/SDA 都需外部上拉。
默认半周期至少 5 us，实际频率因 HAL 调用开销低于 100 kHz。
