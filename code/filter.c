#include "Filter.h"

typedef struct
{
    unsigned short sum;
    unsigned char  last;
    unsigned char  AVG;
}FilterStruct;



static FilterStruct filter={0,0,0};
unsigned char aFilterBuffer[FILTER_BUFFER_SIZE]={0};


//*********************************************************
//函  数 ：unsigned char linear_smooth(unsigned char entrant)
//功  能 ：线性滤波
//输  入 ：entrant - 进入滤波器的最新数据
//输  出 ：滤波处理后的数据
//*********************************************************
unsigned char linear_smooth(unsigned char entrant)
{
    static unsigned char cnt=0;
    static unsigned char index=0; 
    char tmp;  
    unsigned char avg = 0;
    if(cnt >= FILTER_BUFFER_SIZE)
    {
        #if config_DELTA_DEBOUNCE_SWITCH
        tmp = entrant - filter.last;
        filter.last = entrant;
        if((tmp < config_MIN_DELAT)||(tmp > config_MAX_DELTA))
        {
            avg = filter.AVG;
            //filter.last = entrant;
        }
        else
        #endif
        {
            filter.last = entrant;
            filter.sum -= aFilterBuffer[index];
            filter.sum += entrant;
            aFilterBuffer[index] = entrant;
            filter.AVG = filter.sum >> FILTER_SIZE;
            index++;
            index = index % FILTER_BUFFER_SIZE;
            avg =filter.AVG;
        }
        return avg;
    }
    else
    {
        aFilterBuffer[cnt++] = entrant;
        filter.sum += entrant;
        filter.AVG = entrant;
        filter.last = entrant;
        return entrant;
    }
}



