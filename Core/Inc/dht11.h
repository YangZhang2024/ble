#ifndef DHT11_H
#define DHT11_H
#include "stm32f1xx_hal.h"

/* 三针模块：VCC=3.3V，GND共地，DATA=PA0。
 * DATA需要上拉到3.3V，通常模块已自带；没有时外接约4.7kΩ。
 * 先调用HAL_Init和系统时钟配置，再初始化。上电后至少等2秒才读取，
 * 两次读取至少间隔2秒。它是DHT专用单线协议，不是I2C或Dallas 1-Wire。
 */
typedef struct {
    uint8_t temperature_c; /* 整数摄氏度：本示例忽略DHT11的小数数据字节。 */
    uint8_t humidity_rh;   /* 整数相对湿度，单位%RH。 */
} DHT11_Data;

typedef enum {
    DHT11_OK = 0,
    DHT11_NO_RESPONSE,
    DHT11_TIMEOUT,
    DHT11_CHECKSUM,
    DHT11_RANGE,
    DHT11_INVALID_ARGUMENT
} DHT11_Status;

void DHT11_Init(void);
/* 同步读取：主机启动约20ms，接收约5ms；不关闭中断。
 * 长中断可能扰乱微秒时序，此时返回错误，下个采样周期重试。
 * 成功才更新*data，失败不修改。采样间隔由调用方管理。
 */
DHT11_Status DHT11_Read(DHT11_Data *data);
#endif
