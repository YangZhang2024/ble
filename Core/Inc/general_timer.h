#ifndef GENERAL_TIMER_H
#define GENERAL_TIMER_H
#include "main.h"

/* TIM2 为被学习的定时器；TIM3 只在自测案例中从 PA6 输出已知信号。
 * 本芯片 STM32F103RCT6 的 TIM2/TIM3 都是 16 位，不要套用其他系列的 32 位 TIM2。
 */
extern TIM_HandleTypeDef htim2, htim3;
void Timer_StopAll(void);
void Timer_Base(TIM_HandleTypeDef *timer, TIM_TypeDef *instance,
                uint32_t prescaler, uint32_t counts);
void Timer_Pin(uint16_t pins, uint32_t mode, uint32_t pull);
void Timer_Output(TIM_HandleTypeDef *timer, uint32_t channel,
                  uint32_t mode, uint32_t compare);
void Timer_Generator(uint32_t counts, uint32_t high_counts);
void Timer_EnableIRQ(void);
void Timer_Check(HAL_StatusTypeDef result);
#endif
