#include "general_timer.h"
#include "demo.h"
#include <string.h>

TIM_HandleTypeDef htim2, htim3;

void Timer_Check(HAL_StatusTypeDef result)
{
    if (result != HAL_OK) Error_Handler();
}

void Timer_Pin(uint16_t pins, uint32_t mode, uint32_t pull)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = pins;
    gpio.Mode = mode;
    gpio.Pull = pull;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void Timer_StopAll(void)
{
    /* 切换前先屏蔽 CPU 中断，再复位整个外设。仅 Stop_IT 不会清除所有通道、
     * 从模式和编码器设置；换模式时用硬件复位避免旧配置影响新案例。
     */
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();
    __HAL_RCC_TIM3_FORCE_RESET();
    __HAL_RCC_TIM3_RELEASE_RESET();
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    HAL_NVIC_ClearPendingIRQ(TIM3_IRQn);
    memset(&htim2, 0, sizeof(htim2));
    memset(&htim3, 0, sizeof(htim3));
    /* PA0/PA1 为 TIM2 默认通道，PA6 为 TIM3_CH1。不使用重映射，避开全部 PB。
     * 暂时将这些引脚改为下拉输入，使上一个案例的输出不再驱动外部连线。
     */
    __HAL_RCC_AFIO_CLK_ENABLE();
    /* MAPR 的 SWJ_CFG 读值不可靠，修改重映射时同时显式写入“关JTAG、留SWD”，
     * 不依赖读回的调试位，保证PA15仍可作按键，PA13/PA14仍可下载。
     */
    MODIFY_REG(AFIO->MAPR,
               AFIO_MAPR_TIM2_REMAP | AFIO_MAPR_TIM3_REMAP | AFIO_MAPR_SWJ_CFG,
               AFIO_MAPR_SWJ_CFG_JTAGDISABLE);
    Timer_Pin(GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_6, GPIO_MODE_INPUT, GPIO_PULLDOWN);
}

void Timer_Base(TIM_HandleTypeDef *timer, TIM_TypeDef *instance,
                uint32_t prescaler, uint32_t counts)
{
    /* 72MHz / (PSC+1) 为 CNT 计数频率；从 0 到 ARR，共 ARR+1 次。
     * 时钟已在 Timer_StopAll 打开；此处只配置，不启动。
     */
    if (counts == 0U || counts > 65536U || prescaler > 65535U) Error_Handler();
    timer->Instance = instance;
    timer->Init.Prescaler = prescaler;
    timer->Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->Init.Period = counts - 1U;
    timer->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    Timer_Check(HAL_TIM_Base_Init(timer));
    __HAL_TIM_SET_COUNTER(timer, 0);
    /* 初始化 UG 可能产生 UIF；清干净后才允许中断，避免误报首次到时。 */
    __HAL_TIM_CLEAR_FLAG(timer, TIM_FLAG_UPDATE);
}

void Timer_Output(TIM_HandleTypeDef *timer, uint32_t channel,
                  uint32_t mode, uint32_t compare)
{
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = mode;
    oc.Pulse = compare; /* Pulse 对应 CCR 比较寄存器，不是毫秒数。 */
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (mode == TIM_OCMODE_PWM1) {
        Timer_Check(HAL_TIM_PWM_Init(timer));
        Timer_Check(HAL_TIM_PWM_ConfigChannel(timer, &oc, channel));
        Timer_Check(HAL_TIM_PWM_Start(timer, channel));
    } else {
        Timer_Check(HAL_TIM_OC_Init(timer));
        Timer_Check(HAL_TIM_OC_ConfigChannel(timer, &oc, channel));
        Timer_Check(HAL_TIM_OC_Start(timer, channel));
    }
}

void Timer_Generator(uint32_t counts, uint32_t high_counts)
{
    /* 自测信号：TIM3_CH1 → PA6，必须用实体跳线接到 PA0 才能被 TIM2 测到。
     * 这里使用 10kHz 计数时钟，counts=10 为 1kHz，counts=1000 为 10Hz。
     */
    Timer_Base(&htim3, TIM3, 7199, counts);
    Timer_Pin(GPIO_PIN_6, GPIO_MODE_AF_PP, GPIO_NOPULL);
    Timer_Output(&htim3, TIM_CHANNEL_1, TIM_OCMODE_PWM1, high_counts);
}

void Timer_EnableIRQ(void)
{
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{
    if (timer->Instance == TIM2) Demo_OnUpdate();
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *timer)
{
    if (timer->Instance == TIM2 && timer->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        Demo_OnCapture();
}
