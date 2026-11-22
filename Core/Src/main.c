#include "main.h"
#include "lcd.h"
#include "dht11.h"

static void SystemClock_Config(void);

uint32_t dht11_last = 0, led_last = 0;

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    lcd_init();
    DHT11_Init();
    lcd_show_string(10, 5, 216, 24, 24, "RCT6 STARTED", YELLOW);
    lcd_printf(10, 60, "TEMP:");
    lcd_printf(10, 80, "HUM:");

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

        now = HAL_GetTick();
        uint32_t dht11_elapsed = now - dht11_last;
        if (dht11_elapsed >= 2000)
        {

            DHT11_Data sample = {0};
            DHT11_Status status = DHT11_Read(&sample);
            if (status == DHT11_OK)
            {
                lcd_printf(10, 60, "TEMP: %2d", sample.temperature_c);
                lcd_printf(10, 80, "HUM: %2d%%", sample.humidity_rh);
                lcd_printf(10, 100, "");
            }
            else
            {
                lcd_printf_err(10, 100, "DHT11 error, %d", (uint8_t)status);
            }

            dht11_last = now;
        }
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
