#include "protocol.h"
#include "main.h"
uint16_t checksum_calculate(uint8_t* buff,uint8_t length);
void code_buffer_to_send(uint8_t* payload,uint8_t len,uint8_t seq);
uint8_t framebuff[32];

Breath_st breath;
void freameAnalysis()
{
    static uint8_t buff[20];
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
                if(seq == pFreame->seq) return;
                seq = pFreame->seq;
                uint8_t ret;
                uint8_t length=0;
                DataField_st* pDF;
                pDF = (DataField_st*)&pFreame->gd; 
                switch(pDF->cmd)
                {
                    case CMD_SET_BRIGHT:
                        if((pDF->payload[0] <= 100)&&((pDF->payload[2] <= 90) || (pDF->payload[2] == 0xff)))
                        {
                            setTriacLeve(pDF->payload[0],pDF->payload[2],pDF->payload[1]);
                            ret = 0;
                            breath.times = 0;           //clear breath struct
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
                        if((pFreame->contral & ACK_FLAG_NEED) == 0) break;
                        code_buffer_to_send(buff,length,seq);
                        break;
                    case CMD_LOAD_STOP:
                        uint16_t c = getTriacCurrentContinue();
                        buff[length++] = 0;             //payload length
                        buff[length++] = pDF->channel;  //channel 
                        buff[length++] = (uint8_t)(CMD_LOAD_STOP>>8);
                        buff[length++] = (uint8_t)CMD_LOAD_STOP;
                        buff[length++] = 0;             //ret
                        buff[length++] = getLevelFromeContinue(c);
                        buff[0] = length;
                        setTriacLeve(buff[5],0xff,0);
                        if((pFreame->contral & ACK_FLAG_NEED) == 0) break;
                        code_buffer_to_send(buff,length,seq);
                        break;
                    case CMD_LOAD_BREATH:
                        if((pDF->payload[0] <= 100)                                  //恢复后的有效level
                         && (pDF->payload[1] <= 70)                                  //呼吸的最小亮度
                         && (pDF->payload[2] <= 100) && (pDF->payload[2] -pDF->payload[1] >= 30)
                         && (pDF->payload[3] <= 1)                                   //mode 0:闪烁 1：呼吸
                         && (pDF->payload[4] <= 250)                                 //速率
                         && ((pDF->payload[5] <= 250) || (pDF->payload[5] == 0xff))) //次数
                        {
                            ret = 0;
                            memcpy(&breath,&pDF->payload[0],sizeof(breath));
                        }
                        else
                            ret = 1;
                        buff[length++] = 5;             //payload length
                        buff[length++] = pDF->channel;  //channel 
                        buff[length++] = (uint8_t)(CMD_LOAD_BREATH>>8);
                        buff[length++] = (uint8_t)CMD_LOAD_BREATH;
                        buff[length++] = ret;           //ret
                        if((pFreame->contral & ACK_FLAG_NEED) == 0) break;
                        code_buffer_to_send(buff,length,seq);
                        break;
                    default:
                        break;
                }  
                setCommMode(comm_uart);             
            }
            else
            {   //校验错误
                buff[0] = 0x45;buff[1] = 0x52;buff[2] = 0x52;buff[3] = 0x21;
                uart1_send_buff(buff,4);
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
    checksum = checksum_calculate(sendbuff,length+2);
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
    static uint8_t time = 0;
    static uint8_t dir = 0;
    static uint8_t cnt = 0;
    uint8_t targ_level = 0;
    if(breath.times)
    {
        if((dir & 0x80) == 0)                               //初始化blink参数（方向、速率）
        {
            dir |= 0x80;
            time = breath.time;
            if(breath.mode == 0)                            //闪烁
            {
                if(triac.onoff == 1)
                {
                     dir &= 0xfe;
                     targ_level = 0;
                }
                else
                {
                     dir |= 0x01;
                     targ_level = 80;
                }
                setTriacLeve(targ_level,0xff,0);
            }
            else                                            //呼吸
            {
                if((triac.onoff == 1)&&(triac.level > ((breath.max - breath.min)>>1)+breath.min))
                {
                    dir &= 0xfe;
                    targ_level = breath.min;
                }     
                else
                {
                    dir |= 0x01;
                    targ_level = breath.max;
                } 
                setTriacLeve(targ_level,0xff,time>>1);
            }
        }
        if(time == 0)                                      
        {
            time = breath.time;
            if(cnt++ == 1)                                  //一个周期计数
            {
                if(breath.times <= 250)breath.times--;
                cnt = 0;
            }
            if(breath.mode == 0)                            //闪烁模式
            {
                if(dir & 0x01) targ_level = 0;
                else targ_level = 80;
                if(breath.times == 0)  setTriacLeve(breath.recover,0xff,0);  //闪烁结束恢复recover
                else setTriacLeve(targ_level,0xff,0);
            }
            else                                            //呼吸模式
            {
                if(dir & 0x01) targ_level = breath.min;
                else targ_level = breath.max;
                if(breath.times == 0) setTriacLeve(breath.recover,0xff,time>>1);//呼吸结束恢复recover
                else setTriacLeve(targ_level,0xff,time>>1);
            }
            dir ^= 0x01;
        }
        else time--;
    }
    else
    {
        dir = 0;
        time = 0;
        cnt = 0;
    }
}

