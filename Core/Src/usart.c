#include "usart.h"
#include <errno.h>
#include <stddef.h>

UART_HandleTypeDef huart2;
static uint8_t rx_byte;
static volatile uint8_t rx_buf[256];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
#define BLE_UART_BAUDRATE 9600U

void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = BLE_UART_BAUDRATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

void HAL_UART_MspInit(UART_HandleTypeDef *uart)
{
    if (uart->Instance != USART2) return;
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    /* USART2: PA2 是 TX，PA3 是 RX。 */
    gpio.Pin = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uart)
{
    if (uart->Instance != USART2) return;
    __HAL_RCC_USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
    HAL_NVIC_DisableIRQ(USART2_IRQn);
}

void BLE_UART_StartReceive(void)
{
    rx_head = 0;
    rx_tail = 0;
    if (HAL_UART_Receive_IT(&huart2, &rx_byte, 1) != HAL_OK) Error_Handler();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance != USART2) return;
    uint16_t next = (uint16_t)((rx_head + 1U) & 0xFFU);
    if (next != rx_tail) {
        rx_buf[rx_head] = rx_byte;
        rx_head = next;
    }
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART2) HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

void BLE_UART_Poll(void)
{
    while (rx_tail != rx_head) {
        uint8_t c = rx_buf[rx_tail];
        rx_tail = (uint16_t)((rx_tail + 1U) & 0xFFU);
        HAL_UART_Transmit(&huart2, &c, 1, 100);
    }
}

int _write(int file, char *ptr, int len)
{
    if (file != 1 && file != 2) { errno = EBADF; return -1; }
    if (len <= 0) return 0;
    if (ptr == NULL) { errno = EIO; return -1; }
    int sent = 0;
    while (sent < len) {
        uint16_t count = (len - sent > 64) ? 64 : (uint16_t)(len - sent);
        if (HAL_UART_Transmit(&huart2, (uint8_t *)(ptr + sent), count, 100) != HAL_OK) {
            errno = EIO;
            return sent ? sent : -1;
        }
        sent += count;
    }
    return sent;
}


