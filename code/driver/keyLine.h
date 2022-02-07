#ifndef _KEYLINE_H
#define _KEYLINE_H
#include "stm8s.h"
#include "stm8s_gpio.h"

#define KEY_LINE_PORT   GPIOC
#define KEY_LINE_PIN    GPIO_PIN_6

typedef void (*pKeyLineUpdataCallback)(void);
void keyLine_init(pKeyLineUpdataCallback pFunc);
void Task_scan_keyLine(void);

#endif
