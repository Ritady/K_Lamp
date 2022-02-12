#include "pwm_comm.h"
#include <string.h>
#include "driver_triac.h"
#include "filter.h"

capture_st capture;
void KK_Timer1_Init(void)
{
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1,ENABLE); // 打开外设时钟
	TIM1_Cmd(DISABLE);
	// TIM1_ICInit( TIM1_CHANNEL_3, TIM1_ICPOLARITY_RISING, TIM1_ICSELECTION_DIRECTTI,
    //            TIM1_ICPSC_DIV8, 0x0);
	#define TIM1_ICFilter 0
	TIM1->CCER1 = 0;		//DISABLE CC3 CC4
	// TIM1->CCMR1 = (uint8_t)((uint8_t)(TIM1->CCMR1 & (uint8_t)(~(uint8_t)( TIM1_CCMR_CCxS | TIM1_CCMR_ICxF))) 
    //                       | (uint8_t)(( (TIM1_ICSELECTION_DIRECTTI)) | ((uint8_t)( TIM1_ICFilter << 4))));

	// TIM1->CCMR2 = (uint8_t)((uint8_t)(TIM1->CCMR2 & (uint8_t)(~(uint8_t)( TIM1_CCMR_CCxS | TIM1_CCMR_ICxF))) 
    //                       | (uint8_t)(( (TIM1_ICSELECTION_INDIRECTTI)) | ((uint8_t)( TIM1_ICFilter << 4))));

	TIM1->CCMR3 = (uint8_t)((uint8_t)(TIM1->CCMR3 & (uint8_t)(~(uint8_t)( TIM1_CCMR_CCxS | TIM1_CCMR_ICxF))) 
						  | (uint8_t)(( (TIM1_ICSELECTION_DIRECTTI)) | ((uint8_t)( TIM1_ICFilter << 4))));
					  
	// TIM1->CCMR4 = (uint8_t)((uint8_t)(TIM1->CCMR4 & (uint8_t)(~(uint8_t)( TIM1_CCMR_CCxS | TIM1_CCMR_ICxF))) 
    //                       | (uint8_t)(( (TIM1_ICSELECTION_INDIRECTTI)) | ((uint8_t)( TIM1_ICFilter << 4))));
					  
	//TIM1->CCER1 |= 0x4c; 
	TIM1->CCER2 |= 0x01;
	//TIM1->SMCR |= (uint8_t) (5<<4)|(4<<0);

	TIM1->IER |= 0X08;//TIM1->IER |= 0X06;
                          
	/* Enable TIM1 */
  	TIM1_Cmd(ENABLE);
	//TIM1_ClearFlag(TIM1_FLAG_CC3);            	  //清除更新标志位	
	//TIM1_ITConfig(TIM1_IT_CC3,ENABLE);
}

#define COMMUNICATION_PWM_CYCLE				16000
/**
  * @brief Timer1 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */
 uint8_t reg=0;
INTERRUPT_HANDLER(TIM1_CAP_COM_IRQHandler, 12)
{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
 	static uint16_t ICValue1,currentValue;
	static uint8_t dir=0;
	/* Clear CC1 Flag*/
    //TIM1_ClearFlag(TIM1_FLAG_CC3);
    // currentValue = TIM1_GetCapture3();
	// switch(capture.statue)
	// {
	// 	case sampleStart:		//上升沿捕获开始
	// 		ICValue1 = currentValue;
	// 		TIM1->CCER2 &=  (uint8_t)(~TIM1_CCER2_CC3E);
	// 		TIM1->CCER2 |= TIM1_CCER2_CC3P;              	//TIM1_ICPOLARITY_FALLING
	// 		TIM1->CCER2 |=  TIM1_CCER2_CC3E;
	// 		capture.statue = sampleDutyEnd;
	// 		break;
	// 	case sampleDutyEnd:
	// 		capture.duty = currentValue - ICValue1;
	// 		TIM1->CCER2 &=  (uint8_t)(~TIM1_CCER2_CC3E);
	// 		TIM1->CCER2 &= (uint8_t)(~TIM1_CCER2_CC3P);  	//TIM1_ICPOLARITY_RISING
	// 		TIM1->CCER2 |=  TIM1_CCER2_CC3E;	
	// 		capture.statue = sampleCycleEnd;
	// 		break;
	// 	case sampleCycleEnd:
	// 		capture.cycle = currentValue - ICValue1;
	// 		capture.statue = dutyCalc;
	// 		if(capture.cycle > COMMUNICATION_PWM_CYCLE + (COMMUNICATION_PWM_CYCLE>>1))
	// 		{
	// 			capture.cycle -= COMMUNICATION_PWM_CYCLE;
	// 			if(capture.duty >= COMMUNICATION_PWM_CYCLE + 0)
	// 				capture.duty  -= COMMUNICATION_PWM_CYCLE;
	// 		}
	// 		if(capture.duty <= 160) capture.duty = 160;
    //                     UART1_SendData8(capture.duty);
	// 		TIM1_Cmd(DISABLE);
	// 		break;
	// 	default:
	// 		break;
	// }   
	reg = TIM1->SR1 ;
	if(TIM1->SR1 & 0x08)
	{	
		if(++dir & 0x01)
		{
			ICValue1=TIM1_GetCapture3();
			TIM1->CCER2 |= 0x02;
		}
		else
		{
			currentValue = TIM1_GetCapture3() - ICValue1;
			TIM1->CCER2 &= 0xfd;
		}	
		
		TIM1_ClearITPendingBit(TIM1_IT_CC3);
	}



	// if(TIM1_GetITStatus(TIM1_IT_CC3))
	// {
    //     ICValue1=TIM1_GetCapture3();          		//读取高电平时间
	// 	//TIM1_ClearITPendingBit(TIM1_IT_CC3);
	// 	if(TIM1->SR1 & 0x08)
	// 	{

	// 		TIM1_ClearITPendingBit(TIM1_IT_CC3);
	// 	}
 
	// }
	// if(TIM1_GetITStatus(TIM1_IT_CC4))
	// {
		     
	// 	currentValue=TIM1_GetCapture4();            //读取周期
	// 	if(TIM1->SR2 & 0X10)
	// 	{
	// 		TIM1_ClearITPendingBit(TIM1_IT_CC4); 
	// 	}
	// } 
	return;  
}

void Task_CaptureSampleTrigger()
{
	uint8_t percent = 0;
	if(comm_uart == getCommMode())  return;
	if(capture.statue == dutyCalc)
	{
		percent = (capture.duty / 10)>>4;
		UART1_SendData8(percent);
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


