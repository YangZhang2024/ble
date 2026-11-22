#ifndef DEMO_H
#define DEMO_H
#include "main.h"
void Demo_Init(void);
void Demo_Poll(void);
void Demo_Status(unsigned line, const char *format, ...);
/* 由 HAL 中断回调调用，只做短处理，不在中断里显示或打印。 */
void Demo_OnUpdate(void);
void Demo_OnCapture(void);
void Case1_Init(void);
void Case1_Poll(void);
void Case1_Update(void);
void Case2_Init(void);
void Case2_Poll(void);
void Case3_Init(void);
void Case3_Poll(void);
void Case4_Init(void);
void Case4_Poll(void);
void Case4_Capture(void);
void Case5_Init(void);
void Case5_Poll(void);
void Case6_Init(void);
void Case6_Poll(void);
#endif
