#include "driver_uart.h"
#include <string.h>
RING_BUFF_ST rxdata;

void rxbuffer_init()
{
    rxdata.length = 0;  
    rxdata.frame = 0;
}

void uart1_send_buff(uint8_t* buff,uint8_t length)
{
    uint8_t i = 0;
    do
    {
        while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE);
        UART1_SendData8(buff[i]);
        i++;
    }
    while(i<length);
}
 
/**
  * @brief UART1 RX Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18)
 {
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
   static uint8_t index = 0;
   if(SET == UART1_GetFlagStatus(UART1_FLAG_RXNE))
   {
	   rxdata.rxbuff[index++] = UART1->DR;
       index &= BUFFER_INDEX_MASK;
	   UART1_ITConfig(UART1_IT_IDLE,ENABLE);
   }	
   if(SET == UART1_GetFlagStatus(UART1_FLAG_IDLE))
   {
	   UART1_ITConfig(UART1_IT_IDLE,DISABLE);
       rxdata.length = index;
       index = 0;
       rxdata.frame = 1;
   }  
}

void rxdata_copy_extract(uint8_t* desBuff)
{
     memcpy(desBuff,rxdata.rxbuff,rxdata.length);
}
uint8_t getFrameFlag(void)
{
    return rxdata.frame;
}
void setFrameFlag(uint8_t flag)
{
    rxdata.frame = flag;
}
