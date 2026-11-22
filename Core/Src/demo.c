#include "demo.h"
#include "general_timer.h"
#include "lcd.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    void (*init)(void);
    void (*poll)(void);
    void (*isr)(void);
} DemoCase;

static const DemoCase cases[] = {
    {"1. Update IRQ", Case1_Init, Case1_Poll, Case1_Update},
    {"2. PWM duty sweep", Case2_Init, Case2_Poll, NULL},
    {"3. Output compare", Case3_Init, Case3_Poll, NULL},
    {"4. PWM input capture", Case4_Init, Case4_Poll, NULL},
    {"5. Encoder counter", Case5_Init, Case5_Poll, NULL},
    {"6. External counter", Case6_Init, Case6_Poll, NULL}
};
#define CASE_COUNT (sizeof(cases) / sizeof(cases[0]))
static volatile unsigned selected; /* 主循环写、中断读；切换时先屏蔽 TIM2 IRQ。 */

/* 两个独立按键均为上拉输入，按下与 GND 接通。 */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState raw;
    GPIO_PinState stable;
    uint32_t changed_at;
} Key;
static Key previous_key = {.port = GPIOC, .pin = GPIO_PIN_5};
static Key next_key = {.port = GPIOA, .pin = GPIO_PIN_15};

static void key_init(Key *key)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = key->pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(key->port, &gpio);
    key->raw = key->stable = HAL_GPIO_ReadPin(key->port, key->pin);
    key->changed_at = HAL_GetTick();
}

/* 稳定 20ms 才接受变化，只在新按下时返回1，长按不自动连发。 */
static uint8_t pressed(Key *key, uint32_t now)
{
    GPIO_PinState raw = HAL_GPIO_ReadPin(key->port, key->pin);
    if (raw != key->raw) {
        key->raw = raw;
        key->changed_at = now;
    }
    if (raw != key->stable && now - key->changed_at >= 20U) {
        key->stable = raw;
        return raw == GPIO_PIN_RESET;
    }
    return 0;
}

void Demo_Status(unsigned line, const char *format, ...)
{
    if (line > 1) return;
    char text[28]; /* 27 个 ASCII 字符，216 像素，不超出240像素屏宽。 */
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    size_t length = strlen(text);
    memset(text + length, ' ', sizeof(text) - 1U - length);
    text[sizeof(text) - 1U] = '\0';
    lcd_show_string(10, (uint16_t)(120U + line * 20U), 216, 16, 16, text, WHITE);
}

static void select_case(unsigned index)
{
    /* 此函数只在主循环调用。先停中断，再改分发目标和清零案例状态。
     * selected 在 TIM2 IRQ 屏蔽期间修改；只有需要中断的案例会在状态准备完后启用 IRQ。
     */
    Timer_StopAll();
    selected = index;
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
    lcd_clear(BLACK);
    lcd_show_string(10, 10, 216, 16, 16, "GENERAL TIMER TIM2/3", YELLOW);
    lcd_show_string(10, 40, 216, 16, 16, (char *)cases[selected].name, CYAN);
    lcd_show_string(10, 70, 216, 16, 16, "PC5: PREV  PA15: NEXT", WHITE);
    if (selected == 3 || selected == 5) {
        lcd_show_string(10, 180, 216, 16, 16, "Self-test: PA6 -> PA0", GREEN);
    }
    cases[selected].init();
}

void Demo_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /* PA15 默认是 JTDI。main 已 NOJTAG，保留 PA13/PA14 的 SWD。 */
    key_init(&previous_key);
    key_init(&next_key);
    select_case(0);
}

void Demo_Poll(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t prev = pressed(&previous_key, now);
    uint8_t next = pressed(&next_key, now);
    /* 同时按下不切换。1~6 循环切换，每次进入都从头开始实验。 */
    if (prev && !next) select_case((selected + CASE_COUNT - 1U) % CASE_COUNT);
    if (next && !prev) select_case((selected + 1U) % CASE_COUNT);
    cases[selected].poll();
}

void Demo_OnUpdate(void)
{
    if (cases[selected].isr) cases[selected].isr();
}

void Demo_OnCapture(void)
{
    if (selected == 3U) Case4_Capture();
}