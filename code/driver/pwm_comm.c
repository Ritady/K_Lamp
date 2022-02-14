#include "pwm_comm.h"
#include <string.h>
#include "driver_triac.h"
#include "filter.h"

#define CAPTURE_STEP_START				0
#define CAPTURE_STEP_DUTY_START			1
#define CAPTURE_STEP_DUTY_END			2	
#define CAPTURE_STEP_END				3

capture_st capture;
void KK_Timer1_Init(void)
{
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1,ENABLE); // 打开外设时钟
	TIM1_Cmd(DISABLE);

	#define TIM1_ICFilter 0
	TIM1->CCER1 = 0;		//DISABLE CC3 CC4
	TIM1->CCMR3 = (uint8_t)((uint8_t)(TIM1->CCMR3 & (uint8_t)(~(uint8_t)( TIM1_CCMR_CCxS | TIM1_CCMR_ICxF))) 
						  | (uint8_t)(( (TIM1_ICSELECTION_DIRECTTI)) | ((uint8_t)( TIM1_ICFilter << 4))));
					  
	TIM1->CCMR4 = (uint8_t)((uint8_t)(TIM1->CCMR4 & (uint8_t)(~(uint8_t)( TIM1_CCMR_CCxS | TIM1_CCMR_ICxF))) 
                          | (uint8_t)(( (TIM1_ICSELECTION_INDIRECTTI)) | ((uint8_t)( TIM1_ICFilter << 4))));					  
	TIM1->CCER2 = 0x31;
                    
	/* Enable TIM1 */
  	TIM1_Cmd(ENABLE);
	TIM1_ClearFlag(TIM1_FLAG_CC3);            	  //清除更新标志位	
	TIM1_ClearFlag(TIM1_FLAG_CC4);
	TIM1->IER |= 0X18;//TIM1->IER |= 0X06;
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
	if(TIM1_GetITStatus(TIM1_IT_CC3))
	{
		if(capture.step == CAPTURE_STEP_START)
		{
            capture.start = TIM1_GetCounter();
			capture.step = CAPTURE_STEP_DUTY_START;          
		}
		else 
		{
			capture.cycle = TIM1_GetCounter() - capture.start;
			if(capture.step != CAPTURE_STEP_DUTY_END) capture.duty = capture.cycle;
			TIM1->CCER2 = 0;
            capture.step = CAPTURE_STEP_END;	                   
		}
		TIM1_ClearFlag(TIM1_FLAG_CC3);
	}
	if(TIM1_GetITStatus(TIM1_IT_CC4))
	{	
		if((capture.step==1)&&((GPIO_ReadInputData(GPIOC) & GPIO_PIN_3) == 0x00))	
		{       
		    capture.duty = TIM1_GetCounter() - capture.start;            //读取脉宽
            capture.step = CAPTURE_STEP_DUTY_END;
		}	
		TIM1_ClearFlag(TIM1_FLAG_CC4);
	} 
	return;  
}

void Task_CaptureSampleTrigger()
{
	uint8_t percent = 0;
	if(comm_uart == getCommMode())  return;

	if((capture.step == CAPTURE_STEP_END) && capture.cycle) 
	{
		percent = ((uint32_t)capture.duty * 100)/capture.cycle;
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
		capture.duty = 0;
		capture.cycle = 0;
	}	
	setTriacLeve(percent,0);
	capture.step = 0;
	TIM1_ClearFlag(TIM1_FLAG_CC3);
	TIM1_ClearFlag(TIM1_FLAG_CC4);
	TIM1->CCER2 = 0x31;				
}


