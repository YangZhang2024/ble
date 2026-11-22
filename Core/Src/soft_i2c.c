#include "soft_i2c.h"

/* I2C 基本规则：SCL 高电平期间 SDA 保持稳定，接收方此时读取数据。
 * 普通数据在 SCL 低时改变；START/STOP 是 SDA 在 SCL 高时变化的特例。
 * static 函数仅供本文件内部使用；SoftI2C_ 开头的函数供其他模块调用。
 */

static uint8_t valid(const SoftI2C *bus)
{
    /* && 从左向右短路求值，bus 为空时不会继续访问 bus->成员。
     * 非零掩码 x 满足 x & (x-1) == 0，说明只有一位为 1，即只选一个引脚。
     * 这里检查配置形式，不能替代调用者确认端口地址和引脚确实可用。
     */
    return bus && bus->scl_port && bus->sda_port &&
        bus->scl_pin && !(bus->scl_pin & (bus->scl_pin - 1U)) &&
        bus->sda_pin && !(bus->sda_pin & (bus->sda_pin - 1U)) &&
        (bus->scl_port != bus->sda_port || bus->scl_pin != bus->sda_pin);
}
static void wait_half(void)
{
    /* DWT->CYCCNT 每个 CPU 周期加 1，SystemCoreClock 的单位是 Hz。
     * 除以 200000 得到约 5 us 对应的周期数：8 MHz 时是 40 个周期。
     * 无符号减法允许计数器回绕；这是忙等，等待期间 CPU 不做其他主循环工作。
     * GPIO/HAL 调用也耗时，因此实际 I2C 频率低于理想的 100 kHz。
     */
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = SystemCoreClock / 200000U;
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) { }
}
static void line(GPIO_TypeDef *port, uint16_t pin, uint8_t high)
{
    /* 开漏模式：写 0 主动拉低，写 1 是释放，由外部上拉电阻拉高。
     * 改完电平后留出时间，让信号稳定，而不是立即产生下一次变化。
     */
    HAL_GPIO_WritePin(port, pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    wait_half();
}
static void scl(const SoftI2C *bus, uint8_t high)
{
    line(bus->scl_port, bus->scl_pin, high);
}
static void sda(const SoftI2C *bus, uint8_t high)
{
    line(bus->sda_port, bus->sda_pin, high);
}
static uint8_t clock_high(const SoftI2C *bus)
{
    /* 释放 SCL 不保证它马上变高：设备可能拉住它（时钟延展），也可能短路。
     * 读取实际引脚确认；最多约 200 次基础等待，避免死等，实际超时略长于 1 ms。
     */
    scl(bus, 1);
    for (uint32_t i = 0; i < 200; ++i) {
        if (HAL_GPIO_ReadPin(bus->scl_port, bus->scl_pin) == GPIO_PIN_SET) return 1;
        wait_half();
    }
    return 0;
}
static uint8_t stop(const SoftI2C *bus)
{
    /* STOP：先在 SCL 低时拉低 SDA，再升高 SCL，最后释放 SDA。
     * SCL 高时 SDA 从低变高代表停止。返回时检查两条线是否都已释放。
     */
    scl(bus, 0); sda(bus, 0);
    uint8_t ok = clock_high(bus);
    sda(bus, 1);
    return ok && HAL_GPIO_ReadPin(bus->sda_port, bus->sda_pin) == GPIO_PIN_SET;
}
static uint8_t start(const SoftI2C *bus)
{
    /* START：先确认 SCL/SDA 都高（空闲），再将 SDA 拉低，最后拉低 SCL。
     * SDA 被其他电路拉低时拒绝开始；本实现仅适用于单主机总线。
     */
    sda(bus, 1);
    if (!clock_high(bus) || HAL_GPIO_ReadPin(bus->sda_port, bus->sda_pin) == GPIO_PIN_RESET) return 0;
    sda(bus, 0); scl(bus, 0);
    return 1;
}
static uint8_t write_byte(const SoftI2C *bus, uint8_t value)
{
    /* 按 bit7 -> bit0 顺序发送。0x80=二进制10000000，用于取出最高位。
     * 每轮：SCL 低时放数据 -> SCL 高让设备读取 -> SCL 低 -> 左移准备下一位。
     */
    for (uint8_t i = 0; i < 8; ++i) {
        sda(bus, (value & 0x80) != 0);
        if (!clock_high(bus)) return 0;
        scl(bus, 0); value <<= 1;
    }
    /* 第 9 个时钟是 ACK：主机释放 SDA，让设备控制数据线。
     * 设备拉低表示 ACK（收到），保持高表示 NACK（未应答）。
     */
    sda(bus, 1);
    if (!clock_high(bus)) return 0;
    uint8_t ack = HAL_GPIO_ReadPin(bus->sda_port, bus->sda_pin) == GPIO_PIN_RESET;
    scl(bus, 0);
    return ack;
}
uint8_t SoftI2C_Init(const SoftI2C *bus)
{
    /* 1. 检查参数；GPIO 端口时钟由调用者提前启用。 */
    if (!valid(bus)) return 0;
    /* 2. 开启 Cortex-M3 的跟踪功能和周期计数器，为微秒延时提供计时源。
     * |= 只设置目标位，不清除寄存器的其他位。
     */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Do not reset the shared cycle counter, which other modules may use. */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* 3. 先设置输出锁存值为 1，再切为输出，避免切换时意外拉低总线。 */
    HAL_GPIO_WritePin(bus->scl_port, bus->scl_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(bus->sda_port, bus->sda_pin, GPIO_PIN_SET);
    /* {0} 将结构体清零，然后填写需要的字段。
     * OUTPUT_OD=开漏，NOPULL=不用内部上拉，需要模块或外部电阻提供上拉。
     * SPEED_FREQ_LOW 是 GPIO 输出速度设置，不是 I2C 时钟频率设置。
     */
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Speed = GPIO_SPEED_FREQ_LOW; gpio.Pull = GPIO_NOPULL;
    gpio.Pin = bus->scl_pin; HAL_GPIO_Init(bus->scl_port, &gpio);
    gpio.Pin = bus->sda_pin; HAL_GPIO_Init(bus->sda_port, &gpio);
    /* 4. 释放 SDA，产生 9 个时钟，尝试让复位前未结束的传输走完。
     * 一次字节传输是 8 数据位 + 1 应答位；恢复不保证解决所有设备故障。
     */
    sda(bus, 1);
    for (uint8_t i = 0; i < 9; ++i) {
        scl(bus, 0);
        if (!clock_high(bus)) { sda(bus, 1); return 0; }
    }
    /* 5. 发送 STOP，确认总线空闲。设备地址检测由后续写入完成。 */
    return stop(bus);
}
uint8_t SoftI2C_WriteV(const SoftI2C *bus, uint8_t address,
                     const SoftI2C_Buffer *buffers, size_t count)
{
    /* 完整事务：START -> 地址+写入位 -> 第1段数据 -> 第2段数据... -> STOP。
     * 每个字节都检查 ACK，一旦失败就停止发送后续数据。
     * size=0 允许空指针；size>0 时必须有可读的数据。
     */
    if (!valid(bus) || address > 0x7f || (count && !buffers)) return 0;
    for (size_t i = 0; i < count; ++i)
        if (buffers[i].size && !buffers[i].data) return 0;
    if (!start(bus)) { (void)stop(bus); return 0; }
    /* I2C 首字节 = 7位地址左移1位 | 读写位。这里写入位为0。
     * 因而传入0x3C，线上实际发0x78；调用者不要提前左移地址。
     */
    uint8_t ok = write_byte(bus, (uint8_t)(address << 1));
    for (size_t i = 0; ok && i < count; ++i)
        for (size_t j = 0; ok && j < buffers[i].size; ++j)
            ok = write_byte(bus, buffers[i].data[j]);
    /* 即使某字节未应答，也尝试 STOP 释放总线；两步都成功才返回1。 */
    uint8_t stopped = stop(bus);
    return ok && stopped;
}
uint8_t SoftI2C_Write(const SoftI2C *bus, uint8_t address,
                    const uint8_t *data, size_t size)
{
    /* 普通写入只是“只有一段数据”的 WriteV，复用同一套通信逻辑。 */
    const SoftI2C_Buffer buffer = {data, size};
    return SoftI2C_WriteV(bus, address, &buffer, 1);
}

/* 从设备读取一个字节。ack=1表示后面还要继续读，ack=0表示这是最后一个字节。 */
static uint8_t read_byte(const SoftI2C *bus, uint8_t *value, uint8_t ack)
{
    uint8_t result = 0;
    sda(bus, 1); /* 主机释放SDA，接下来由从机驱动数据线。 */
    for (uint8_t bit = 0; bit < 8; ++bit) {
        if (!clock_high(bus)) return 0;
        result = (uint8_t)((result << 1) |
            (HAL_GPIO_ReadPin(bus->sda_port, bus->sda_pin) == GPIO_PIN_SET));
        scl(bus, 0);
    }
    /* 读取阶段的第9拍由主机发送ACK/NACK。 */
    sda(bus, ack ? 0U : 1U);
    if (!clock_high(bus)) return 0;
    scl(bus, 0);
    sda(bus, 1);
    *value = result;
    return 1;
}

uint8_t SoftI2C_ReadRegisters(const SoftI2C *bus, uint8_t address,
                              uint8_t reg, uint8_t *data, size_t size)
{
    if (!valid(bus) || address > 0x7fU || !data || size == 0U) return 0;
    if (!start(bus)) { (void)stop(bus); return 0; }

    /* 先写寄存器地址；MPU6050等设备用这一步指定后续要读的寄存器。 */
    uint8_t ok = write_byte(bus, (uint8_t)(address << 1));
    if (ok) ok = write_byte(bus, reg);
    /* 不发STOP，直接重新START切换到读方向。 */
    if (ok) ok = start(bus);
    if (ok) ok = write_byte(bus, (uint8_t)((address << 1) | 1U));

    for (size_t i = 0; ok && i < size; ++i)
        ok = read_byte(bus, &data[i], i + 1U < size);
    uint8_t stopped = stop(bus);
    return ok && stopped;
}
