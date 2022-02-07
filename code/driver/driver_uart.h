#ifndef _DRIVER_UART_H
#define _DRIVER_UART_H
#include "stm8s.h"
#include "stm8s_uart1.h"

#define BUFFER_SIZE         32
#define BUFFER_INDEX_MASK   (BUFFER_SIZE-1)    
typedef struct 
{
    uint8_t rxbuff[BUFFER_SIZE];
    uint8_t length;
    uint8_t frame;
}RING_BUFF_ST;

uint8_t getFrameFlag(void);
void setFrameFlag(uint8_t flag);
uint8_t rxdata_copy_extract(uint8_t* desBuff);
void uart1_send_buff(uint8_t* buff,uint8_t length);
#endif

