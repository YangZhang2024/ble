#include "main.h"
#include "lcd.h"
#include "dht11.h"
#include "usart.h"

static void SystemClock_Config(void);

uint32_t dht11_last = 0, led_last = 0;

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_USART2_UART_Init();
    BLE_UART_StartReceive();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef led = {0};
    led.Pin = LED0_Pin;
    led.Mode = GPIO_MODE_OUTPUT_OD;
    led.Pull = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED0_GPIO_Port, &led);
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);

    dht11_last = led_last = HAL_GetTick();

    while (1)
    {

        uint32_t now = HAL_GetTick();
        uint32_t led_elapsed = now - led_last;
        if (led_elapsed >= 500)
        {
            HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
            led_last = now;
        }

        BLE_UART_Poll();
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.HSIState = RCC_HSI_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
        Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
