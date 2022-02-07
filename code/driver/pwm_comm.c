#include "pwm_comm.h"
#include <string.h>
#include "driver_triac.h"
#include "filter.h"

capture_st capture;
void KK_Timer1_Init(void)
{
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1,ENABLE); // 打开外设时钟
	TIM1_Cmd(DISABLE);
	TIM1_ICInit( TIM1_CHANNEL_3, TIM1_ICPOLARITY_RISING, TIM1_ICSELECTION_DIRECTTI,
               TIM1_ICPSC_DIV8, 0x0);

	/* Enable TIM1 */
  	TIM1_Cmd(ENABLE);
	TIM1_ClearFlag(TIM1_FLAG_CC3);            	  //清除更新标志位	
	TIM1_ITConfig(TIM1_IT_CC3,ENABLE);
}

#define COMMUNICATION_PWM_CYCLE				16000
/**
  * @brief Timer1 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM1_CAP_COM_IRQHandler, 12)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
 	static uint16_t ICValue1,currentValue;
	/* Clear CC1 Flag*/
    TIM1_ClearFlag(TIM1_FLAG_CC3);
    currentValue = TIM1_GetCapture3();
	switch(capture.statue)
	{
		case sampleStart:		//上升沿捕获开始
			ICValue1 = currentValue;
			TIM1->CCER2 &=  (uint8_t)(~TIM1_CCER2_CC3E);
			TIM1->CCER2 |= TIM1_CCER2_CC3P;              	//TIM1_ICPOLARITY_FALLING
			TIM1->CCER2 |=  TIM1_CCER2_CC3E;
			capture.statue = sampleDutyEnd;
			break;
		case sampleDutyEnd:
			capture.duty = currentValue - ICValue1;
			TIM1->CCER2 &=  (uint8_t)(~TIM1_CCER2_CC3E);
			TIM1->CCER2 &= (uint8_t)(~TIM1_CCER2_CC3P);  	//TIM1_ICPOLARITY_RISING
			TIM1->CCER2 |=  TIM1_CCER2_CC3E;	
			capture.statue = sampleCycleEnd;
			break;
		case sampleCycleEnd:
			capture.cycle = currentValue - ICValue1;
			capture.statue = dutyCalc;
			if(capture.cycle > COMMUNICATION_PWM_CYCLE + (COMMUNICATION_PWM_CYCLE>>1))
			{
				capture.cycle -= COMMUNICATION_PWM_CYCLE;
				if(capture.duty > COMMUNICATION_PWM_CYCLE + 100)
					capture.duty  -= COMMUNICATION_PWM_CYCLE;
			}
			if(capture.duty <= 160) capture.duty = 160;
			TIM1_Cmd(DISABLE);
			break;
		default:
			break;
	}    
    
}

void Task_CaptureSampleTrigger()
{
	uint8_t percent = 0;
	if(comm_uart == getCommMode())  return;
	if(capture.statue == dutyCalc)
	{
		percent = (capture.duty / 10)>>4;
		if((percent > 0)&&(percent <= 100))
		{
			percent = linear_smooth(percent);
		}
		else
		{
			percent = 0;
		}
	}
	else
	{
		percent = 0;
	}
	setTriacLeve(percent,0);
	capture.statue = sampleStart;
	KK_Timer1_Init();
}


