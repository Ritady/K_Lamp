#ifndef _PWM_COMM_H
#define _PWM_COMM_H
#include "stm8s.h"
#include "stm8s_tim1.h"
#include "main.h"



typedef enum
{
	normol = 0,
	sampleStart,
	sampleDutyEnd,
	sampleCycleEnd,
	dutyCalc,
}CaptureStatue;
typedef struct 
{	
	uint16_t start;
	uint16_t stop;
	uint16_t duty;
	uint16_t cycle;
	uint8_t  step;
	uint8_t  outTime;
}capture_st;

extern void Task_CaptureSampleTrigger(void);
void KK_Timer1_Init(void);
#endif

