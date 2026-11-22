
#include <stdio.h>
#include "stdlib.h"
#include "lcd.h"
#include "lcdfont.h"



/* ILI9341，16 位 GPIO 并口驱动。 */


/* LCD的画笔颜色和背景色 */
uint32_t g_point_color = WHITE;    /* 画笔颜色 */
uint32_t g_back_color = BLACK;    /* 背景色 */

/* 管理LCD重要参数 */
_lcd_dev lcddev;


/**
 * @brief       LCD写数据
 *   @note      该函数的功能和 lcd_wr_data 完全一模一样
 *              lcd_wr_data 是用空间换时间, 该函数则是时间换空间
 *              该函数用于 ILI9341 寄存器初始化，以节
 *              省代码空间.
 * @param       data : 要写入的数据
 * @retval      无
 */
void lcd_wr_xdata(uint16_t data) {
    LCD_RS(1);
    LCD_CS(0);
    LCD_DATA_OUT(data);
    LCD_WR(0);
    LCD_WR(1);
    LCD_CS(1);
}

/**
 * @brief       LCD写寄存器编号/地址函数
 * @param       regno: 寄存器编号/地址
 * @retval      无
 */
FORCE_INLINE
void lcd_wr_regno(volatile uint16_t regno) {
    LCD_RS(0);              /* RS=0,表示写寄存器 */
    LCD_CS(0);
    LCD_DATA_OUT(regno);    /* 写入要写的寄存器序号 */
    LCD_WR(0);
    LCD_WR(1);
    LCD_CS(1);
}

/**
 * @brief       LCD写寄存器
 * @param       regno:寄存器编号/地址
 * @param       data:要写入的数据
 * @retval      无
 */
void lcd_write_reg(uint16_t regno, uint16_t data) {
    lcd_wr_regno(regno);    /* 写入要写的寄存器序号 */
    lcd_wr_data(data);      /* 写入数据 */
}


/**
 * @brief       LCD读数据
 * @param       无
 * @retval      读取到的数据
 */
static uint16_t lcd_rd_data(void) {
    volatile uint16_t ram;  /* 防止被优化 */

    GPIO_InitTypeDef gpio_init_struct;
    /* LCD_DATA 引脚模式设置, 上拉输入, 准备接收数据 */
    gpio_init_struct.Pin = LCD_DATA_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DATA_GPIO_PORT, &gpio_init_struct);

    LCD_RS(1);              /* RS=1,表示操作数据 */
    LCD_CS(0);
    LCD_RD(0);
    /* Wait for the LCD to drive the bus, then sample while RD is LOW.
     * Explicit NOPs remain present in optimized builds as well. */
    for (uint32_t i = 0; i < 72U; ++i) { __NOP(); }
    ram = LCD_DATA_IN;      /* 读取数据 */
    LCD_RD(1);
    /* Allow a recovery interval before the next read cycle. */
    for (uint32_t i = 0; i < 72U; ++i) { __NOP(); }
    LCD_CS(1);

    /* LCD_DATA 引脚模式设置, 推挽输出, 恢复输出状态 */
    gpio_init_struct.Pin = LCD_DATA_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DATA_GPIO_PORT, &gpio_init_struct);

    return ram;
}

/**
 * @brief       准备写GRAM
 * @param       无
 * @retval      无
 */
FORCE_INLINE
void lcd_write_ram_prepare(void) {
    lcd_wr_regno(lcddev.wramcmd);
}


static void ili9341_reginit(void)
{
    lcd_wr_regno(0xCF);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0xC1);
    lcd_wr_xdata(0X30);
    lcd_wr_regno(0xED);
    lcd_wr_xdata(0x64);
    lcd_wr_xdata(0x03);
    lcd_wr_xdata(0X12);
    lcd_wr_xdata(0X81);
    lcd_wr_regno(0xE8);
    lcd_wr_xdata(0x85);
    lcd_wr_xdata(0x10);
    lcd_wr_xdata(0x7A);
    lcd_wr_regno(0xCB);
    lcd_wr_xdata(0x39);
    lcd_wr_xdata(0x2C);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x34);
    lcd_wr_xdata(0x02);
    lcd_wr_regno(0xF7);
    lcd_wr_xdata(0x20);
    lcd_wr_regno(0xEA);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x00);
    lcd_wr_regno(0xC0); /* Power control */
    lcd_wr_xdata(0x1B); /* VRH[5:0] */
    lcd_wr_regno(0xC1); /* Power control */
    lcd_wr_xdata(0x01); /* SAP[2:0];BT[3:0] */
    lcd_wr_regno(0xC5); /* VCM control */
    lcd_wr_xdata(0x30); /* 3F */
    lcd_wr_xdata(0x30); /* 3C */
    lcd_wr_regno(0xC7); /* VCM control2 */
    lcd_wr_xdata(0XB7);
    lcd_wr_regno(0x36); /*  Memory Access Control */
    lcd_wr_xdata(0x48);
    lcd_wr_regno(0x3A);
    lcd_wr_xdata(0x55);
    lcd_wr_regno(0xB1);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x1A);
    lcd_wr_regno(0xB6); /*  Display Function Control */
    lcd_wr_xdata(0x0A);
    lcd_wr_xdata(0xA2);
    lcd_wr_regno(0xF2); /*  3Gamma Function Disable */
    lcd_wr_xdata(0x00);
    lcd_wr_regno(0x26); /* Gamma curve selected */
    lcd_wr_xdata(0x01);
    lcd_wr_regno(0xE0); /* Set Gamma */
    lcd_wr_xdata(0x0F);
    lcd_wr_xdata(0x2A);
    lcd_wr_xdata(0x28);
    lcd_wr_xdata(0x08);
    lcd_wr_xdata(0x0E);
    lcd_wr_xdata(0x08);
    lcd_wr_xdata(0x54);
    lcd_wr_xdata(0XA9);
    lcd_wr_xdata(0x43);
    lcd_wr_xdata(0x0A);
    lcd_wr_xdata(0x0F);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x00);
    lcd_wr_regno(0XE1); /* Set Gamma */
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x15);
    lcd_wr_xdata(0x17);
    lcd_wr_xdata(0x07);
    lcd_wr_xdata(0x11);
    lcd_wr_xdata(0x06);
    lcd_wr_xdata(0x2B);
    lcd_wr_xdata(0x56);
    lcd_wr_xdata(0x3C);
    lcd_wr_xdata(0x05);
    lcd_wr_xdata(0x10);
    lcd_wr_xdata(0x0F);
    lcd_wr_xdata(0x3F);
    lcd_wr_xdata(0x3F);
    lcd_wr_xdata(0x0F);
    lcd_wr_regno(0x2B);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x01);
    lcd_wr_xdata(0x3f);
    lcd_wr_regno(0x2A);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0x00);
    lcd_wr_xdata(0xef);
    lcd_wr_regno(0x11); /* Exit Sleep */
    HAL_Delay(120);
    lcd_wr_regno(0x29); /* display on */
}
/* 读像素：丢弃 dummy read，将 RGB888 转换成 RGB565。 */
uint32_t lcd_read_point(uint16_t x, uint16_t y) {
    if (x >= lcddev.width || y >= lcddev.height) return 0;
    lcd_set_cursor(x, y);
    lcd_wr_regno(0x2E);
    (void)lcd_rd_data();
    uint16_t rg = lcd_rd_data();
    uint16_t b = lcd_rd_data();
    return (rg & 0xF800U) | ((rg & 0x00FCU) << 3) | (b >> 11);
}
void lcd_display_on(void) { lcd_wr_regno(0x29); }
void lcd_display_off(void) { lcd_wr_regno(0x28); }

/* 窗口终点包含在显示范围内，宽高必须大于零。 */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height) {
    if (!width || !height) return;
    uint16_t ex = sx + width - 1;
    uint16_t ey = sy + height - 1;
    lcd_wr_regno(0x2A);
    lcd_wr_data(sx >> 8);
    lcd_wr_data(sx & 0xFF);
    lcd_wr_data(ex >> 8);
    lcd_wr_data(ex & 0xFF);
    lcd_wr_regno(0x2B);
    lcd_wr_data(sy >> 8);
    lcd_wr_data(sy & 0xFF);
    lcd_wr_data(ey >> 8);
    lcd_wr_data(ey & 0xFF);
}
void lcd_set_cursor(uint16_t x, uint16_t y) {
    if (x >= lcddev.width || y >= lcddev.height) return;
    lcd_set_window(x, y, lcddev.width - x, lcddev.height - y);
}

/* MY/MX/MV 决定扫描方向；BGR 位沿用原有 ILI9341 配置。 */
void lcd_scan_dir(uint8_t dir) {
    static const uint8_t landscape_map[8] = {6, 7, 4, 5, 1, 0, 3, 2};
    static const uint8_t scan_bits[8] = {0x00, 0x80, 0x40, 0xC0, 0x20, 0x60, 0xA0, 0xE0};
    if (dir > 7) return;
    if (lcddev.dir) dir = landscape_map[dir];
    uint8_t value = scan_bits[dir] | 0x08;
    lcd_write_reg(0x36, value);
    lcddev.width = (value & 0x20) ? 320 : 240;
    lcddev.height = (value & 0x20) ? 240 : 320;
    lcd_set_window(0, 0, lcddev.width, lcddev.height);
}

/* dir=0 竖屏 240×320；dir=1 横屏 320×240。 */
void lcd_display_dir(uint8_t dir) {
    lcddev.dir = dir ? 1 : 0;
    lcddev.wramcmd = 0x2C;
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;
    lcd_scan_dir(DFT_SCAN_DIR);
}
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color) {
    if (x >= lcddev.width || y >= lcddev.height) return;
    lcd_set_cursor(x, y);
    lcd_write_ram_prepare();
    lcd_wr_data(color);
}

void lcd_init(void) {
    GPIO_InitTypeDef gpio_init_struct;

    LCD_BL_GPIO_CLK_ENABLE();   /* LCD_BL脚时钟使能 */
    LCD_CS_GPIO_CLK_ENABLE();   /* LCD_CS脚时钟使能 */
    LCD_WR_GPIO_CLK_ENABLE();   /* LCD_WR脚时钟使能 */
    LCD_RD_GPIO_CLK_ENABLE();   /* LCD_RD脚时钟使能 */
    LCD_RS_GPIO_CLK_ENABLE();   /* LCD_RS脚时钟使能 */
    LCD_DATA_GPIO_CLK_ENABLE(); /* LCD_DATA脚时钟使能 */
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG(); /* 禁止JTAG, 使能SWD, 释放PB3,PB4两个引脚做普通IO用 */

    gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);     /* LCD_BL引脚模式设置(推挽输出) */
    /* 初始化期间先关闭背光，避免 LCD 显存尚未清空时短暂显示白屏。 */
    LCD_BL(0);

    gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_CS引脚 */

    gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
    HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_WR引脚 */

    gpio_init_struct.Pin = LCD_RD_GPIO_PIN;
    HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_RD引脚 */

    gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
    HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_RS引脚 */

    gpio_init_struct.Pin = LCD_DATA_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* 推挽输出 */
    HAL_GPIO_Init(LCD_DATA_GPIO_PORT, &gpio_init_struct);   /* LCD_DATA引脚模式设置 */
    LCD_WR(1);                  /* WR 默认高电平 */
    LCD_RD(1);                  /* RD 默认高电平 */
    LCD_CS(1);                  /* CS 默认高电平 */
    LCD_RS(1);                  /* RS 默认高电平 */
    LCD_DATA_OUT(0XFFFF);       /* DATA 默认高电平 */

    /* 只读 ILI9341 的 D3 寄存器，保留实测 ID，不用固定值掩盖通信故障。 */
    HAL_Delay(50);
    lcd_wr_regno(0xD3);
    (void)lcd_rd_data();
    (void)lcd_rd_data();
    uint16_t id_high = lcd_rd_data() & 0xFF;
    uint16_t id_low = lcd_rd_data() & 0xFF;
    lcddev.id = (id_high << 8) | id_low;

    ili9341_reginit();
    lcd_display_dir(0);
    lcd_clear(BLACK);
    LCD_BL(1);
    
}

/* 清屏、绘图和字体显示接口。 */
void lcd_clear(uint16_t color) {
    uint32_t index = 0;
    uint32_t totalpoint = lcddev.width;
    totalpoint *= lcddev.height;    /* 得到总点数 */
    lcd_set_cursor(0x00, 0x0000);   /* 设置光标位置 */
    lcd_write_ram_prepare();        /* 开始写入GRAM */

    /* 为了提高写入速度, 将lcd_wr_data函数进行拆分, 避免重复设置
     * RS, CS的操作, 从而提升速度, 从51帧提高到78帧左右, 提高50%
     * 测试条件: -O2优化, 纯刷屏
     * 在有速度要求的时候, 可以继续优化: lcd_fill, lcd_color_fill,
     * lcd_set_cursor 和 lcd_draw_point 等函数, 大家可以自行优化
     */
    LCD_RS(1);                      /* RS=1,表示写数据 */
    LCD_CS(0);

    for (index = 0; index < totalpoint; index++) {
        LCD_DATA_OUT(color);        /* 写入要写的数据 */
        LCD_WR(0);
        LCD_WR(1);
    }

    LCD_CS(1);
}

/**
 * @brief       在指定区域内填充单个颜色
 * @param       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * @param       color: 要填充的颜色(32位颜色,方便兼容LTDC)
 * @retval      无
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color) {
    uint16_t i, j;
    uint16_t xlen = 0;
    xlen = ex - sx + 1;

    for (i = sy; i <= ey; i++) {
        lcd_set_cursor(sx, i);      /* 设置光标位置 */
        lcd_write_ram_prepare();    /* 开始写入GRAM */

        for (j = 0; j < xlen; j++) {
            lcd_wr_data(color);     /* 写入数据 */
        }
    }
}

/**
 * @brief       在指定区域内填充指定颜色块
 * @param       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * @param       color: 要填充的颜色数组首地址
 * @retval      无
 */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color) {
    uint16_t height, width;
    uint16_t i, j;
    width = ex - sx + 1;            /* 得到填充的宽度 */
    height = ey - sy + 1;           /* 高度 */

    for (i = 0; i < height; i++) {
        lcd_set_cursor(sx, sy + i); /* 设置光标位置 */
        lcd_write_ram_prepare();    /* 开始写入GRAM */

        for (j = 0; j < width; j++) {
            lcd_wr_data(color[i * width + j]);  /* 写入数据 */
        }
    }
}

/**
 * @brief       画线
 * @param       x1,y1: 起点坐标
 * @param       x2,y2: 终点坐标
 * @param       color: 线的颜色
 * @retval      无
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;
    delta_x = x2 - x1;          /* 计算坐标增量 */
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)incx = 1;   /* 设置单步方向 */
    else if (delta_x == 0)incx = 0; /* 垂直线 */
    else {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)incy = 1;
    else if (delta_y == 0)incy = 0; /* 水平线 */
    else {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)distance = delta_x;  /* 选取基本增量坐标轴 */
    else distance = delta_y;

    for (t = 0; t <= distance + 1; t++)   /* 画线输出 */
    {
        lcd_draw_point(row, col, color); /* 画点 */
        xerr += delta_x;
        yerr += delta_y;

        if (xerr > distance) {
            xerr -= distance;
            row += incx;
        }

        if (yerr > distance) {
            yerr -= distance;
            col += incy;
        }
    }
}

/**
 * @brief       画水平线
 * @param       x,y: 起点坐标
 * @param       len  : 线长度
 * @param       color: 矩形的颜色
 * @retval      无
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color) {
    if ((len == 0) || (x > lcddev.width) || (y > lcddev.height))return;

    lcd_fill(x, y, x + len - 1, y, color);
}

/**
 * @brief       画矩形
 * @param       x1,y1: 起点坐标
 * @param       x2,y2: 终点坐标
 * @param       color: 矩形的颜色
 * @retval      无
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * @brief       画圆
 * @param       x,y  : 圆中心坐标
 * @param       r    : 半径
 * @param       color: 圆的颜色
 * @retval      无
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color) {
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1);       /* 判断下个点位置的标志 */

    while (a <= b) {
        lcd_draw_point(x0 + a, y0 - b, color);  /* 5 */
        lcd_draw_point(x0 + b, y0 - a, color);  /* 0 */
        lcd_draw_point(x0 + b, y0 + a, color);  /* 4 */
        lcd_draw_point(x0 + a, y0 + b, color);  /* 6 */
        lcd_draw_point(x0 - a, y0 + b, color);  /* 1 */
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - a, y0 - b, color);  /* 2 */
        lcd_draw_point(x0 - b, y0 - a, color);  /* 7 */
        a++;

        /* 使用Bresenham算法画圆 */
        if (di < 0) {
            di += 4 * a + 6;
        } else {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 * @brief       填充实心圆
 * @param       x,y: 圆中心坐标
 * @param       r    : 半径
 * @param       color: 圆的颜色
 * @retval      无
 */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
    uint32_t i;
    uint32_t imax = ((uint32_t) r * 707) / 1000 + 1;
    uint32_t sqmax = (uint32_t) r * (uint32_t) r + (uint32_t) r / 2;
    uint32_t xr = r;

    lcd_draw_hline(x - r, y, 2 * r, color);

    for (i = 1; i <= imax; i++) {
        if ((i * i + xr * xr) > sqmax) {
            /* draw lines from outside */
            if (xr > imax) {
                lcd_draw_hline(x - i + 1, y + xr, 2 * (i - 1), color);
                lcd_draw_hline(x - i + 1, y - xr, 2 * (i - 1), color);
            }

            xr--;
        }

        /* draw lines from inside (center) */
        lcd_draw_hline(x - xr, y + i, 2 * xr, color);
        lcd_draw_hline(x - xr, y - i, 2 * xr, color);
    }
}

/**
 * @brief       在指定位置显示一个字符
 * @param       x,y  : 坐标
 * @param       chr  : 要显示的字符:" "--->"~"
 * @param       size : 字体大小 12/16/24/32
 * @param       mode : 叠加方式(1); 非叠加方式(0);
 * @param       color : 字符的颜色;
 * @retval      无
 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color) {
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = 0;
    uint8_t *pfont = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    chr = chr - ' ';    /* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */

    switch (size) {
        case 12:
            pfont = (uint8_t *) asc2_1206[(int) chr];  /* 调用1206字体 */
            break;

        case 16:
            pfont = (uint8_t *) asc2_1608[(int) chr];  /* 调用1608字体 */
            break;

        case 24:
            pfont = (uint8_t *) asc2_2412[(int) chr];  /* 调用2412字体 */
            break;

        case 32:
            pfont = (uint8_t *) asc2_3216[(int) chr];  /* 调用3216字体 */
            break;

        default:
            return;
    }

    for (t = 0; t < csize; t++) {
        temp = pfont[t];    /* 获取字符的点阵数据 */

        for (t1 = 0; t1 < 8; t1++)   /* 一个字节8个点 */
        {
            if (temp & 0x80)        /* 有效点,需要显示 */
            {
                lcd_draw_point(x, y, color);        /* 画点出来,要显示这个点 */
            } else if (mode == 0)     /* 无效点,不显示 */
            {
                lcd_draw_point(x, y, g_back_color); /* 画背景色,相当于这个点不显示(注意背景色由全局变量控制) */
            }

            temp <<= 1; /* 移位, 以便获取下一个位的状态 */
            y++;

            if (y >= lcddev.height)return;  /* 超区域了 */

            if ((y - y0) == size)   /* 显示完一列了? */
            {
                y = y0; /* y坐标复位 */
                x++;    /* x坐标递增 */

                if (x >= lcddev.width)return;   /* x坐标超区域了 */

                break;
            }
        }
    }
}

/**
 * @brief       平方函数, m^n
 * @param       m: 底数
 * @param       n: 指数
 * @retval      m的n次方
 */
static uint32_t lcd_pow(uint8_t m, uint8_t n) {
    uint32_t result = 1;

    while (n--)result *= m;

    return result;
}

/**
 * @brief       显示len个数字
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @param       color : 数字的颜色;
 * @retval      无
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color) {
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)   /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;   /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0) {
                lcd_show_char(x + (size / 2) * t, y, ' ', size, 0, color);/* 显示空格,占位 */
                continue;   /* 继续下个一位 */
            } else {
                enshow = 1; /* 使能显示 */
            }

        }

        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, 0, color); /* 显示字符 */
    }
}

/**
 * @brief       扩展显示len个数字(高位是0也显示)
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @param       mode: 显示模式
 *              [7]:0,不填充;1,填充0.
 *              [6:1]:保留
 *              [0]:0,非叠加显示;1,叠加显示.
 * @param       color : 数字的颜色;
 * @retval      无
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color) {
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)   /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;    /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0) {
                if (mode & 0X80)   /* 高位需要填充0 */
                {
                    lcd_show_char(x + (size / 2) * t, y, '0', size, mode & 0X01, color);  /* 用0占位 */
                } else {
                    lcd_show_char(x + (size / 2) * t, y, ' ', size, mode & 0X01, color);  /* 用空格占位 */
                }

                continue;
            } else {
                enshow = 1; /* 使能显示 */
            }

        }

        lcd_show_char(x + (size / 2) * t, y, temp + '0', size, mode & 0X01, color);
    }
}

/**
 * @brief       显示字符串
 * @param       x,y         : 起始坐标
 * @param       width,height: 区域大小
 * @param       size        : 选择字体 12/16/24/32
 * @param       p           : 字符串首地址
 * @param       color       : 字符串的颜色;
 * @retval      无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color) {
    uint16_t x0 = x;
    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))   /* 判断是不是非法字符! */
    {
        if (x >= width) {
            x = x0;
            y += size;
        }

        if (y >= height)break;  /* 退出 */

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}


/**
 * 支持 printf 输出模式, buffer 长度为 64 注意字符串长度, 默认size 16
 */
void lcd_printf(uint16_t x, uint16_t y, const char *format, ...) {
    char buffer[64] = {0};
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    lcd_show_string(x, y, (lcddev.width - x - 10), lcddev.height - y, 16, buffer, g_point_color);
}

void lcd_printf_err(uint16_t x, uint16_t y, const char *format, ...) {
    char buffer[64] = {0};
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    lcd_show_string(x, y, (lcddev.width - x - 10), lcddev.height - y, 16, buffer, RED);
}
















