#include "stm32f1xx_hal.h"

/* HAL_Delay() 依赖 HAL_GetTick() 返回的 uwTick。
 * SysTick 每 1 ms 进入一次，必须在这里调用 HAL_IncTick()。 */
void SysTick_Handler(void) { HAL_IncTick(); }
void HardFault_Handler(void) { while (1) { } }
void MemManage_Handler(void) { while (1) { } }
void BusFault_Handler(void) { while (1) { } }
void UsageFault_Handler(void) { while (1) { } }
