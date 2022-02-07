#ifndef _DRIVER_TRIAC_H
#define _DRIVER_TRIAC_H
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_tim2.h"
#include "main.h"

#define  SWITCH_ST_ON               1
#define  SWITCH_ST_OFF              0
typedef enum
{
    comm_pwm=0,
    comm_uart,
}COMM_MODE_ENUM;

typedef enum {
    LUM_CALULATE_ST_HALF_1 = 0,
    LUM_CALULATE_ST_HALF_2,
    LUM_CALULATE_ST_HALF_3,
    LUM_CALULATE_ST_HALF_4,
}LUM_CALULATE_ST_T;
typedef struct 
{
    uint8_t  onoff;
    uint8_t  level;
    uint8_t  line_freq;
    uint8_t  cntMs;
    
    uint16_t zero_cycle;
    uint16_t continue_time;
    uint16_t current_continue;
    int16_t  continue_step;
    uint8_t  line_check_status;
    LUM_CALULATE_ST_T lum_st;
    COMM_MODE_ENUM    comm;  
}TRIAC_ARG_ST;

extern TRIAC_ARG_ST triac;
void KK_TIME2_INIT(void);
void IncMsCntForTriac(void);
uint8_t getLineFreq(void);
void setCommMode(COMM_MODE_ENUM mode);
COMM_MODE_ENUM getCommMode(void);
void setTriacLeve(uint8_t level,uint8_t trainsition);
#endif
