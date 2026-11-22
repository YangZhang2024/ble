#ifndef __USART_H__
#define __USART_H__
#include "main.h"
extern UART_HandleTypeDef huart2;
void MX_USART2_UART_Init(void);
void BLE_UART_StartReceive(void);
void BLE_UART_Poll(void);
#endif
