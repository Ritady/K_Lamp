#ifndef _PROTOCOL_H
#define _PROTOCOL_H
#include "stm8s.h"
#include "driver_uart.h"
#include "driver_triac.h"

#define FREAME_HEAD                 0XAA55
#define SINGLE_FRAME_LENGTH_MAX     32

#define CMD_SET_BRIGHT              0X1010
#define CMD_GET_VERSION             0X1022
#define CMD_LOAD_BLINK              0X1035
#define CMD_LOAD_BREATH             0X1036
#define CMD_ACK_LEVEL               0X10

#define DIR_FRAME_Z3_TO_DEVICE      0X01
#define DIR_FRAME_DEVICE_TO_Z3      0X00

#define ACK_FLAG_NEED               0X10
#define ACK_FLAG_NEEDLESS           0X00

typedef struct 
{
    uint8_t length;
    uint8_t channel;
    uint16_t cmd;
    uint8_t payload[10];
}DataField_st;

typedef struct 
{
    uint16_t Head;
    uint8_t  length;
    uint8_t  contral;
    uint8_t  seq;
    DataField_st gd;
}freame_st;

typedef struct 
{
    uint8_t recover;        //结束的level
    uint8_t min;            //呼吸时最小亮度
    uint8_t max;            //呼吸时最大亮度
    uint8_t mode;           //模式 0-闪烁 1-呼吸
    uint8_t time;           //速率
    uint8_t times;          //次数          
}Breath_st;

void Task_AnalysisFrame(void);
void Task_Load_Breath(void);
#endif
