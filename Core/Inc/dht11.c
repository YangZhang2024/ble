#include "dht11.h"

#define DHT_PORT GPIOA
#define DHT_PIN GPIO_PIN_0

static uint32_t microseconds_to_cycles(uint32_t us)
{
    return (SystemCoreClock / 1000000U) * us;
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - start) < microseconds_to_cycles(us)) { }
}

static GPIO_PinState level(void)
{
    return HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN);
}

/* 等待当前电平结束，并记录这段电平的时长；用DWT计时，不依赖毫秒Tick。
 * 每个等待都有上限，即使拔掉传感器或DATA短路，也不会卡死主循环。
 */
static uint8_t wait_end(GPIO_PinState current, uint32_t timeout_us, uint32_t *cycles)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t limit = microseconds_to_cycles(timeout_us);
    while (level() == current) {
        if ((uint32_t)(DWT->CYCCNT - start) >= limit) return 0;
    }
    if (cycles) *cycles = (uint32_t)(DWT->CYCCNT - start);
    return 1;
}

void DHT11_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; /* 不清零共享周期计数器。 */
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = DHT_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD; /* 写0拉低；写1释放，让传感器控制DATA。 */
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT_PORT, &gpio);
}

DHT11_Status DHT11_Read(DHT11_Data *data)
{
    if (!data) return DHT11_INVALID_ARGUMENT;
    uint8_t bytes[5] = {0};

    /* 主机将DATA拉低至少18ms，再释放。DHT11随后拉低约80us、拉高约80us应答。 */
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
    delay_us(5);
    if (!wait_end(GPIO_PIN_SET, 120, NULL)) return DHT11_NO_RESPONSE;
    if (!wait_end(GPIO_PIN_RESET, 120, NULL)) return DHT11_TIMEOUT;
    if (!wait_end(GPIO_PIN_SET, 120, NULL)) return DHT11_TIMEOUT;

    /* 接收40位，高位先到。每位先低约50us，再用高电平长度表示数值：
     * 约26~28us=0，约70us=1，用50us为分界。不要在这里打断点或单步。
     */
    for (uint8_t bit = 0; bit < 40; ++bit) {
        uint32_t high_cycles;
        if (!wait_end(GPIO_PIN_RESET, 100, NULL)) return DHT11_TIMEOUT;
        if (!wait_end(GPIO_PIN_SET, 120, &high_cycles)) return DHT11_TIMEOUT;
        bytes[bit / 8] <<= 1;
        if (high_cycles > microseconds_to_cycles(50)) bytes[bit / 8] |= 1;
    }
    /* 最后一位之后，传感器还会拉低约50us，随后释放总线。 */
    if (!wait_end(GPIO_PIN_RESET, 120, NULL)) return DHT11_TIMEOUT;

    /* 数据依次为：湿度整数、湿度小数、温度整数、温度小数、校验和。
     * 校验和等于前4字节之和的低8位，用于发现传输错误。
     */
    if ((uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]) != bytes[4])
        return DHT11_CHECKSUM;
    if (bytes[0] > 100 || bytes[2] > 50) return DHT11_RANGE;
    data->humidity_rh = bytes[0];
    data->temperature_c = bytes[2];
    return DHT11_OK;
}
