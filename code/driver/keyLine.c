#include "keyLine.h"
#include <stddef.h>

pKeyLineUpdataCallback g_pKeyLineUpdataCallback = NULL;

static uint8_t keyLineHoldCntMs = 0; 
void keyLine_init(pKeyLineUpdataCallback pFunc)
{
    GPIO_Init(KEY_LINE_PORT,KEY_LINE_PIN,GPIO_MODE_IN_FL_NO_IT);        //配置IO口为输入模式，开启内部上拉
    if(pFunc != NULL) g_pKeyLineUpdataCallback = pFunc;
}

/*****************************************
 * K线键值采样处理
 * 1ms调用一次
 * 
 * ***************************************/
void Task_scan_keyLine(void)
{
    static uint8_t keylineIO_last,keyLineStatue_last;
    static uint8_t scanCycleCnt_50ms = 0;
    BitStatus keyLineStatue;
    uint8_t keyLine =  GPIO_ReadInputPin(KEY_LINE_PORT,KEY_LINE_PIN);
    if(keylineIO_last != keyLine)
    {
        keyLineHoldCntMs = 0;
        keylineIO_last   = keyLine;
    }
    else 
    {
        if(++keyLineHoldCntMs > 0x80)keyLineHoldCntMs = 0X80;               //防止溢出
    }

    if(++scanCycleCnt_50ms > 50)
    {
        scanCycleCnt_50ms = 0;
        if(keyLineHoldCntMs > 20) keyLineStatue = RESET;
        else keyLineStatue = SET;

        if(keyLineStatue_last != keyLineStatue)
        {
            keyLineStatue_last = keyLineStatue;
            //状态改变，上报
            if(g_pKeyLineUpdataCallback != NULL)
                g_pKeyLineUpdataCallback();
        }
    }
}
