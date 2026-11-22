#ifndef SOFT_I2C_H
#define SOFT_I2C_H
#include "stm32f1xx_hal.h"
#include <stddef.h>

/* 阅读入口：先看 SoftI2C_Init，再看 SoftI2C_Write，最后看 WriteV。
 * 软件 I2C：CPU 手动改变 GPIO 电平产生通信波形，不使用硬件 I2C 外设。
 * 本模块负责“把字节发给设备”；字节代表命令还是数据，由设备驱动决定。
 */

typedef struct {
    GPIO_TypeDef *scl_port; /* 时钟线端口，如 GPIOB；指针指向 GPIO 寄存器。 */
    uint16_t scl_pin;      /* 时钟线引脚掩码，如 GPIO_PIN_6，不是数字 6。 */
    GPIO_TypeDef *sda_port; /* 数据线端口，可与 SCL 使用不同端口。 */
    uint16_t sda_pin;      /* 数据线引脚掩码，如 GPIO_PIN_7。 */
} SoftI2C;

typedef struct {
    const uint8_t *data; /* 一段待发送数据的起始地址；const 表示不会修改内容。 */
    size_t size;        /* 数据字节数，不是指针大小。数组可用 sizeof(array)。 */
} SoftI2C_Buffer;

/* STM32F1/Cortex-M3, blocking, single master, external pull-ups required.
 * Enable both GPIO clocks first. Do not call concurrently or from interrupts.
 * Nominal half-period >= 5 us; software overhead lowers the bus frequency.
 * All functions return 1 on success, 0 on NACK/stuck bus/invalid arguments.
 * Reinitialize after changing SystemCoreClock. Init attempts bus recovery.
 */
uint8_t SoftI2C_Init(const SoftI2C *bus);
/* Init 前：HAL_Init、配置系统时钟、打开相关 GPIO 时钟。
 * Init 内：开启 DWT 延时、配置开漏 GPIO、发送恢复时钟和 STOP。
 * 返回 1 只表示总线空闲，不代表某个设备存在；没有设备也可能成功。
 * 所有公开接口返回 1=成功，0=失败。数据数组只需在调用期间保持有效。
 */
/* 地址使用未左移的7位值。模块同时支持写入和“寄存器地址+连续读取”。
 * Buffers are concatenated in one transaction; no device-specific control byte.
 * Zero buffers performs an address-only probe. Never use on a multi-master bus.
 */
uint8_t SoftI2C_WriteV(const SoftI2C *bus, uint8_t address,
                     const SoftI2C_Buffer *buffers, size_t count);
/* WriteV 的 V 表示多段缓冲区：一次 START + 地址 + 各段数据 + STOP。
 * count 是缓冲区段数，各段 size 是该段字节数。段间不重复发地址。
 * 例如 OLED 使用两段：{控制字节}、{像素数组}，可避免复制拼接。
 */
uint8_t SoftI2C_Write(const SoftI2C *bus, uint8_t address,
                    const uint8_t *data, size_t size);
/* 普通写入示例（bus 已初始化，0x3C 是未左移的 7 位地址）：
 * const uint8_t bytes[] = {0x00, 0xAF};
 * uint8_t ok = SoftI2C_Write(&bus, 0x3C, bytes, sizeof(bytes));
 * 当前只支持同步单主机通信，不支持多主机仲裁。
 */
/* 从设备的reg开始连续读取size个字节。
 * 总线序列：START -> 地址W -> reg -> 重复START -> 地址R
 * -> 数据(主机ACK...最后主机NACK) -> STOP。
 * data必须指向可写数组；返回0时数组可能只写入了部分数据。
 */
uint8_t SoftI2C_ReadRegisters(const SoftI2C *bus, uint8_t address,
                              uint8_t reg, uint8_t *data, size_t size);
#endif
