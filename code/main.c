#include "stm8s.h"
#include <string.h>
#include "stm8s_it.h"
#include "keyLine.h"
#include "driver_triac.h"
#include "stm8s_tim1.h"
#include "main.h"
#include "pwm_comm.h"
#include "stm8s_uart1.h"
#include "protocol.h"
#include "stm8s_iwdg.h"

static void CLK_Init(void); //系统时钟初始化
static void KK_GPIO_INIT(void);  // DIO不需要开启外部时钟
static void KK_Interrupt_INIT(void);  //中断优先级配置:  0,1,2,3  其中0为最低优先级
static void KK_UART1_INIT(void); //串口UART1初始化
static void TaskProcess(void);
static void TaskRemarking(void);


void keyLineUpdataCallback(void);
uint8_t tr_delay;				 //K线信号转化输出延时
void Task_IOtest(void);
void Task_TRIO_handle(void);

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
/************************************** 初始化函数 **************************************/

/*  系统时钟初始化
	CPU = 主时钟 = 外部晶振 = 8MHZ
	外设时钟 = 主时钟 = 8MHZ
*/
static void CLK_Init(void)
{
    CLK_HSECmd(ENABLE);									//外部时钟开
    CLK_LSICmd(ENABLE);									//内部低频RC开
    CLK_HSICmd(ENABLE);									//内部高频RC开
    //while(SET != CLK_GetFlagStatus(CLK_FLAG_HSERDY));    //等待外部晶振起振
    CLK_ClockSwitchCmd(ENABLE);							//切换使能
    CLK_ClockSwitchConfig(CLK_SWITCHMODE_MANUAL,CLK_SOURCE_HSI,DISABLE,CLK_CURRENTCLOCKSTATE_DISABLE);//切换到外部晶振
    CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);				//1分频
}


static void KK_GPIO_INIT(void)  // DIO不需要开启外部时钟
{
	GPIO_Init(GPIOD,GPIO_PIN_3|GPIO_PIN_4,GPIO_MODE_OUT_PP_LOW_FAST);

	GPIO_Init(GPIOC,GPIO_PIN_7,GPIO_MODE_OUT_PP_LOW_FAST);  // PC7: 控制光耦，高电平有效，初始化低电平
	GPIO_WriteLow(GPIOC, GPIO_PIN_7); //关闭光耦
	disableInterrupts();
	GPIO_Init(GPIOC, GPIO_PIN_5, GPIO_MODE_IN_PU_IT); // Pc5: 上拉输入，使能中断
	//GPIO_ExternalPullUpConfig(GPIOC, GPIO_PIN_5, ENABLE); // 这句可以不使用，上面  上拉输入，已经 使能 上拉电阻
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_FALL_ONLY);  //下降沿触发
	EXTI_SetTLISensitivity(EXTI_TLISENSITIVITY_FALL_ONLY);
	enableInterrupts();  // 总中断要开启，不然所有中断都没法使用
}

/*  中断优先级配置:  0,1,2,3  其中0为最低优先级
	定时器最高
	DIO中断第二
	UART中断第三
*/
static void KK_Interrupt_INIT(void)
{
    disableInterrupts();
	ITC_SetSoftwarePriority(ITC_IRQ_TIM4_OVF, ITC_PRIORITYLEVEL_1);
	ITC_SetSoftwarePriority(ITC_IRQ_TIM2_OVF, ITC_PRIORITYLEVEL_2);
	ITC_SetSoftwarePriority(ITC_IRQ_PORTC, ITC_PRIORITYLEVEL_2);
	ITC_SetSoftwarePriority(ITC_IRQ_UART1_RX, ITC_PRIORITYLEVEL_1);
    enableInterrupts();
}
static void KK_UART1_INIT(void)
{
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE); // 打开外设时钟
	UART1_Init((u32)9600, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO, UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
	UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);  //接收中断  使能
	UART1_ITConfig(UART1_IT_TXE, DISABLE);     //发送寄存器空中断  禁能
	UART1_ITConfig(UART1_IT_TC, DISABLE);      //发送完成中断  禁能
	UART1_Cmd(ENABLE);                         //启动UART1
}

static void KK_Timer4_Init()
{
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4,ENABLE); // 打开外设时钟

	
	TIM4_TimeBaseInit(TIM4_PRESCALER_128,124);
	TIM4_ClearFlag(TIM4_FLAG_UPDATE);
	TIM4_SelectOnePulseMode(TIM4_OPMODE_REPETITIVE);
	TIM4_ITConfig(TIM4_IT_UPDATE,ENABLE);        			 //计数溢出中断
	TIM4_Cmd(ENABLE);                            			 //启动定时器
}

void main(void)
{
	CLK_Init();                                   //系统时钟初始化
	KK_GPIO_INIT();
	keyLine_init(keyLineUpdataCallback);
	KK_UART1_INIT();
	KK_TIME2_INIT();
	KK_Timer4_Init();							  //10ms时基
	KK_Interrupt_INIT();                          //优先级
	KK_Timer1_Init();
	/* IWDG Configuration */
	IWDG->KR = IWDG_KEY_ENABLE;	
	IWDG->KR = (uint8_t)IWDG_WriteAccess_Enable;	//unlock
	IWDG->PR = (uint8_t)IWDG_Prescaler_256;			//
	IWDG->RLR = 0xcc;								//800ms timeout
	IWDG->KR = IWDG_KEY_REFRESH;					//lock
	while(1)
	{
		IWDG->KR = IWDG_KEY_REFRESH;				//喂狗
		TaskProcess();
	}
}

typedef struct 
{
	uint8_t Run;						//运行标志位：0-不运行 1-运行
	uint8_t Timer;						//计数器
	uint8_t ItvTimes;					//任务间隔时间
	void (*TaskHook)(void);				//任务
}TASK_COMPONENTS;

typedef enum TaskTotalList_EN
{
	TASK_CAPTURE_TRIGGER,
	TASK_SCAN_KEYLINE,
	TASK_TRIO_HANDLE,
	TASK_IO_TEST,
	TASK_ANALYSISFRAME,
	TASK_LOAD_BREATH,
	TASK_MAX,
}TaskList;
TASK_COMPONENTS TaskComps[TASK_MAX] =
{
	{0,7,7,Task_CaptureSampleTrigger},
	{0,1,1,Task_scan_keyLine},
	{0,20,20,Task_TRIO_handle},
	{0,200,200,Task_IOtest},
	{0,5,5,Task_AnalysisFrame},
	{0,100,100,Task_Load_Breath},
};
/********************************************
 * 函 数 名：TaskRemarking()
 * 描    述：任务运行标志位set
 * 输    入：none 
 * 输    出：none
 * ******************************************/
void TaskRemarking(void)
{
	uint8_t i = 0;
	for(i=0;i< TASK_MAX;i++)
	{
		if(TaskComps[i].Timer)
		{
			TaskComps[i].Timer--;
			if(TaskComps[i].Timer == 0)
			{
				TaskComps[i].Run = 1;
				TaskComps[i].Timer = TaskComps[i].ItvTimes;
			}
		}
	}
}

void TaskProcess(void)
{
	uint8_t i = 0;
	for(i=0;i<TASK_MAX;i++)
	{
		if(TaskComps[i].Run == 1)
		{
			TaskComps[i].Run = 0;
			TaskComps[i].TaskHook();		
		}
	}
}

extern TRIAC_ARG_ST triac;
void Task_IOtest(void)
{
	GPIO_WriteReverse(GPIOD,GPIO_PIN_4);
}

void keyLineUpdataCallback(void)
{
	tr_delay = 10;
	GPIO_WriteLow(GPIO_TR_OP_PORT,GPIO_TR_OP_PIN);
}
void Task_TRIO_handle(void)
{
	if(tr_delay) tr_delay--;
	else 
	{
		tr_delay = 0;
		GPIO_WriteHigh(GPIO_TR_OP_PORT,GPIO_TR_OP_PIN);
	}
}
/**
  * @brief Timer4 Update/Overflow Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
 {
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
    TIM4_ClearFlag(TIM4_FLAG_UPDATE);
	TaskRemarking();
    IncMsCntForTriac();
 }
 
 uint8_t getSoftVersion()
 { 
	 return FIRMWARE_VERSION;
 }

