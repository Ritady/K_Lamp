#include "driver_triac.h"
#include <stdlib.h>

#define HARDWARTE_ZERO_CHECK_DELAY  8000      //4000*1.5(ms)/100
#define TRIAC_DRIVER_HOLDE_TIME     4000      //100*10US

TRIAC_ARG_ST triac;
/* 
*	可控硅延时用
*/
void KK_TIME2_INIT(void)
{
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2,ENABLE); // 打开外设时钟

	TIM2_ClearFlag(TIM2_FLAG_UPDATE);            //清除更新标志位
	TIM2_TimeBaseInit(TIM2_PRESCALER_4,60000);   // 1分频，
	TIM2_ARRPreloadConfig(ENABLE);               // 预装载值更新时，等待更新才生效
	TIM2_SelectOnePulseMode(TIM2_OPMODE_REPETITIVE); // 连续计数不停止
	TIM2_UpdateDisableConfig(DISABLE);           //允许事件更新，中断才能触发
	TIM2_ITConfig(TIM2_IT_UPDATE,ENABLE);        //计数溢出中断

	//TIM2_Cmd(ENABLE);                          //启动定时器
    triac.line_check_status = 50;                // powerLine.status = 50;  
}

void KK_Timer2_Change(u16 Time_us)            	 // 0--1000
{
	u16 TimeNum_Tmp = 0;
	TimeNum_Tmp = Time_us;

	TIM2_ITConfig(TIM2_IT_UPDATE,DISABLE);       // 更新中断: 禁能
	TIM2_SetAutoreload(TimeNum_Tmp);             //设置预装载寄存器的值
	TIM2_SetCounter(0x0000);                     //设置定时器计数器值
  	TIM2_PrescalerConfig(TIM2_PRESCALER_4, TIM2_PSCRELOADMODE_IMMEDIATE); // 无动作
	TIM2_GenerateEvent(TIM2_EVENTSOURCE_UPDATE); //软件产生更新事件
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);

	TIM2_ITConfig(TIM2_IT_UPDATE,ENABLE);         // 更新中断: 使能
	TIM2_Cmd(ENABLE);                             //启动定时器
}

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
    if ((GPIO_ReadInputData(GPIOC) & GPIO_PIN_5) != 0x00) return;
    if(triac.line_check_status)                  //if(powerLine.status)
    {
        triac.line_check_status--;               //powerLine.status--;
        if(triac.line_check_status == 2)
        {
            triac.cntMs = 0;                     //powerLine.cntMs = 0;
        }
        else if(triac.line_check_status == 0)
        {
            //获取电网周期    
            if(triac.cntMs > 36)
            {
                triac.zero_cycle = 40000;
            }    
            else
                triac.zero_cycle = 33333;
        }
    }
    else
    {
        if(triac.cntMs < 15) return;            //防止干扰
        if(triac.cntMs > 22) 
        {
            triac.cntMs = 0;
            return;
        }
        triac.cntMs = 0;
        
        GPIO_WriteLow(GPIOC,GPIO_PIN_7);        //关闭光耦
        triac.lum_st = LUM_CALULATE_ST_HALF_1;
        if(triac.onoff == SWITCH_ST_ON) 
        {   
            if(triac.level <= 100)
            {      
                if(abs(triac.current_continue - triac.continue_time) > triac.continue_step)
                {
                    if(triac.current_continue > triac.continue_time) triac.current_continue -= triac.continue_step;
                    else triac.current_continue += triac.continue_step;
                }
                else
                {
                    triac.current_continue = triac.continue_time;
                    if(triac.level == 0) 
                    {
                        triac.onoff = SWITCH_ST_OFF;
                        //triac.current_continue = triac.zero_cycle - HARDWARTE_ZERO_CHECK_DELAY;
                    }
                }
                if(triac.current_continue < 10) triac.current_continue = 10;
    	        KK_Timer2_Change(triac.current_continue);
            }
        }
        else 
        {
            TIM2_Cmd(DISABLE);
        }   
    }   
 
}

static u16 g_triacHold = 0;
/**
  * @brief Timer2 Update/Overflow/Break Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
{
    ITStatus its;
    
    its = TIM2_GetITStatus(TIM2_IT_UPDATE);//its=RESET or SET
    TIM2_ClearITPendingBit(TIM2_IT_UPDATE); //清除中断标志位
    if(its == SET) // 中断发生
    {    
        if((triac.level >= 70 || triac.level <= 30)) {
            g_triacHold = TRIAC_DRIVER_HOLDE_TIME << 1;
        } else {
            g_triacHold = TRIAC_DRIVER_HOLDE_TIME;
        }	
		if(triac.lum_st == LUM_CALULATE_ST_HALF_1)
		{
            GPIO_WriteHigh(GPIOC, GPIO_PIN_7); //打开光耦
		    KK_Timer2_Change(g_triacHold);
            triac.lum_st = LUM_CALULATE_ST_HALF_2;
		}
		else if(triac.lum_st == LUM_CALULATE_ST_HALF_2)
		{
            GPIO_WriteLow(GPIOC,GPIO_PIN_7); //关闭光耦
            KK_Timer2_Change(triac.zero_cycle - g_triacHold);
            triac.lum_st = LUM_CALULATE_ST_HALF_3;
		}
        else if(triac.lum_st == LUM_CALULATE_ST_HALF_3)
        {
            GPIO_WriteHigh(GPIOC, GPIO_PIN_7); //打开光耦
		    KK_Timer2_Change(g_triacHold);
            triac.lum_st = LUM_CALULATE_ST_HALF_4;
        }
        else 
        {
            GPIO_WriteLow(GPIOC,GPIO_PIN_7); //关闭光耦
            TIM2_Cmd(DISABLE);
        }
    }    
}


void IncMsCntForTriac(void)
{
    triac.cntMs++;
}
uint16_t getTriacCurrentContinue(void)
{
    return triac.current_continue;
}
/**
 * @brief  通过导通角计算百分比
 * @param  导通角
 * @retval 百分比
 */
uint8_t getLevelFromeContinue(uint16_t triac_continue)
{
    int8_t ret_level =0;
    if(triac.zero_cycle <= HARDWARTE_ZERO_CHECK_DELAY) return 0;
    if(triac.onoff == 0) return 0;
    ret_level = 100 - ((uint32_t)triac_continue*100/(triac.zero_cycle - HARDWARTE_ZERO_CHECK_DELAY));
    if(ret_level <= 0) ret_level = 1;
    else if(ret_level > 100) ret_level = 100;
    return ret_level;
}
void setTriacLeve(uint8_t level,uint8_t min,uint8_t trainsition)
{
    uint16_t dealt;
    if(level <= 100)
    { 
        triac.level = level;
        if(triac.onoff == 0) triac.current_continue = triac.zero_cycle - HARDWARTE_ZERO_CHECK_DELAY;
        if(level)
        {
            triac.onoff = SWITCH_ST_ON;
            if(min < 100)
            {
                uint16_t temp = (100 -  min)*((triac.zero_cycle - HARDWARTE_ZERO_CHECK_DELAY)/100);
                if(triac.current_continue > temp) triac.current_continue = temp;
            } 
            triac.continue_time = (100 -  triac.level)*((triac.zero_cycle - HARDWARTE_ZERO_CHECK_DELAY)/100);  
        }
        else
        {
            if(min < 100) triac.continue_time = (100 -  min)*((triac.zero_cycle - HARDWARTE_ZERO_CHECK_DELAY)/100); 
            else triac.continue_time = triac.zero_cycle - HARDWARTE_ZERO_CHECK_DELAY;
        }
        
        if(trainsition > 0)
        {
            if(triac.continue_time != triac.current_continue)
            {
                if(triac.continue_time > triac.current_continue)
                    dealt = triac.continue_time - triac.current_continue;
                else
                    dealt = triac.current_continue - triac.continue_time;
                triac.continue_step = 1 + (dealt/trainsition)/5;             //+1 防止小数计算偏小  /10*2 = /5  20hz过零信号
            }
            // if(triac.continue_time > triac.current_continue)
            // {
            //     dealt = triac.continue_time - triac.current_continue;
            //     triac.continue_step = dealt/trainsition/10;
            // }
            // else if(triac.continue_time < triac.current_continue)
            // {
            //     dealt = triac.current_continue -triac.continue_time;
            //     triac.continue_step = dealt/trainsition/10;
            // }
            else 
                triac.continue_step = 0;
        }
        else
            triac.current_continue = triac.continue_time;
    }
    else
    {
        triac.onoff = SWITCH_ST_OFF;
        triac.level = 0;
    }
}
uint8_t getLineFreq(void)
{
    if(triac.line_check_status)
        return 0xff;
    return triac.line_freq;
}

void setCommMode(COMM_MODE_ENUM mode)
{
    triac.comm = mode;
}
COMM_MODE_ENUM getCommMode(void)
{
    return triac.comm;
}

