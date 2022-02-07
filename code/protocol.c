#include "protocol.h"


static uint8_t seq = 0;
uint16_t checksum_calculate(uint8_t* buff,uint8_t length);
void code_buffer_to_send(uint8_t* payload,uint8_t len,uint8_t seq);
uint8_t framebuff[32];

Breath_st breath;
void freameAnalysis()
{
    static uint8_t buff[8];
    static uint8_t seq;
    freame_st* pFreame;
    uint8_t index=0;
    rxdata_copy_extract(framebuff);
    do{
        pFreame = (freame_st*)&framebuff[index];
        if((FREAME_HEAD == pFreame->Head) && (pFreame->contral & DIR_FRAME_Z3_TO_DEVICE)
            && (pFreame->length >= 7) && (pFreame->length <= SINGLE_FRAME_LENGTH_MAX))
        {
            uint16_t checksum = checksum_calculate(&framebuff[index],pFreame->length);
            if((framebuff[index+pFreame->length -2] == (checksum >> 8)) && (framebuff[index+pFreame->length - 1] == (checksum & 0x00ff)))
            {
                //if(seq == pFreame->seq) return;
                seq = pFreame->seq;
                uint8_t ret;
                uint8_t length=0;
                DataField_st* pDF;
                pDF = (DataField_st*)&pFreame->gd; 
                switch(pDF->cmd)
                {
                    case CMD_SET_BRIGHT:
                        if((pDF->payload[0] <= 100)&&(pDF->payload[1] <= 50))
                        {
                            setTriacLeve(pDF->payload[0],pDF->payload[1]);
                            ret = 0;
                        }
                        else    
                            ret =0xe3;
                        if((pFreame->contral & ACK_FLAG_NEED) == 0) break;
                        buff[length++] = 0;             //payload length
                        buff[length++] = pDF->channel;  //channel 
                        buff[length++] = (uint8_t)(CMD_SET_BRIGHT>>8);
                        buff[length++] = (uint8_t)CMD_SET_BRIGHT;
                        buff[length++] = ret;
                        buff[length++] = triac.level;
                        buff[0] = length;
                        code_buffer_to_send(buff,length,seq);
                        break;
                    case CMD_GET_VERSION:
                        buff[length++] = 0;             //payload length
                        buff[length++] = 0;             //channel 
                        buff[length++] = (uint8_t)(CMD_GET_VERSION>>8);
                        buff[length++] = (uint8_t)CMD_GET_VERSION;
                        buff[length++] = 0;             //ret
                        buff[length++] = FIRMWARE_VERSION;
                        buff[0] = length;
                        code_buffer_to_send(buff,length,seq);
                        break;
                    case CMD_LOAD_BLINK:
                        
                        break;
                    case CMD_LOAD_BREATH:
                        if((pDF->payload[0] <= 100) && (pDF->payload[1] >= 1) && (pDF->payload[1] <= 250)) 
                        {
                            ret = 0;
                            breath.end = pDF->payload[0];
                            breath.total = (pDF->payload[1]<<1) + 1 ; 
                        }
                        else
                            ret = 1;
                        buff[length++] = 5;             //payload length
                        buff[length++] = pDF->channel;  //channel 
                        buff[length++] = (uint8_t)(CMD_LOAD_BREATH>>8);
                        buff[length++] = (uint8_t)CMD_LOAD_BREATH;
                        buff[length++] = ret;           //ret
                        code_buffer_to_send(buff,length,seq);
                        break;
                    default:
                        break;
                }  
                setCommMode(comm_uart);             
            }
            else
            {   //校验错误
                
            }
            break;
        }
        else{
            index ++;
        }
    }while(index < (SINGLE_FRAME_LENGTH_MAX - 7));   
} 

uint16_t checksum_calculate(uint8_t* buff,uint8_t length)
{
    uint16_t check = 0;
    uint8_t i = 0;
    for(;i<length-2;i++)
        check += buff[i];
    return check;
}

void code_buffer_to_send(uint8_t* payload,uint8_t len,uint8_t seq)
{
    static uint8_t sendbuff[15];
    uint8_t length = 0;
    uint16_t checksum = 0;
    sendbuff[length++] = (uint8_t)(FREAME_HEAD>>8);
    sendbuff[length++] = (uint8_t)FREAME_HEAD;
    sendbuff[length++] =  0;
    sendbuff[length++] = DIR_FRAME_DEVICE_TO_Z3|ACK_FLAG_NEEDLESS;
    sendbuff[length++] = seq;
    for(uint8_t i=0;i<len;i++)
    {
        sendbuff[length++] = payload[i];
    }
    sendbuff[2] = length + 2;
    checksum = checksum_calculate(sendbuff,length);
    sendbuff[length++] = (uint8_t)(checksum>>8);
    sendbuff[length++] = (uint8_t)checksum;
    uart1_send_buff(sendbuff,length);
}


void Task_AnalysisFrame(void)
{
    if(getFrameFlag() >= 1)
    {
        freameAnalysis();
        setFrameFlag(0);
    }       
}


void Task_Load_Breath()
{
    static uint8_t tick_800ms;
    if(++tick_800ms >= 4)
    {
        tick_800ms = 0;
        if(breath.total)
        {
            breath.total--;
            if((triac.onoff == 1)&&(triac.level > 50))  breath.dir = 0;   
            else breath.dir = 1;  
            uint8_t level=0;
            if(breath.dir)
            {
                level = 100;
                breath.dir = 0;
            }
            else
            {
                level = 10;
                breath.dir = 1;
            }
            if(breath.total == 0) level = breath.end;
            setTriacLeve(level,4);
        }
    }
}

