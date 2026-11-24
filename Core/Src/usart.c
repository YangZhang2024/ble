#include "usart.h"

UART_HandleTypeDef huart2;
static uint8_t rx_byte;
static volatile uint8_t rx_buf[256];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
#define BLE_UART_BAUDRATE 9600U

void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    // huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
    }
}
void BLE_UART_StartReceive(void)
{
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}
void BLE_UART_Poll(void)
{
    while (rx_tail != rx_head)
    {
        uint8_t c = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % 0xFFU;
        HAL_UART_Transmit(&huart2, &c, 1, 100);
        
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    /* USART2: PA2 是 TX，PA3 是 RX。 */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_AF_INPUT;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

    if (huart->Instance != USART2)
    {
        return;
    }

    uint8_t next = (rx_head + 1) % 0xFFU;
    if (next != rx_tail)
    {
        rx_buf[rx_head] = rx_byte;
        rx_head = next;
    }

    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}