#ifndef _FILTER_H
#define _FILTER_H


#define SIZE_2                              2
#define SIZE_4                              4
#define SIZE_8                              8
#define SIZE_16                             16
#define SIZE_32                             32
#define SIZE_64                             64

//***************滤波器配置******************
#define config_FILTER_BUFFER_SIZE           SIZE_64                     //配置滤波器size  2/4/8/16/32/64
#define config_DELTA_DEBOUNCE_SWITCH        1                           //跳变消抖使能/禁止 打开后变化量大于设定值是视作干扰处理
#if config_DELTA_DEBOUNCE_SWITCH 
#define config_MAX_DELTA                    20                          //设置有效的最大变化量
#define config_MIN_DELAT                    -20                         //设置有效的最小变化量
#endif
//*******************************************

#define FILTER_BUFFER_SIZE_2                1
#define FILTER_BUFFER_SIZE_4                2
#define FILTER_BUFFER_SIZE_8                3
#define FILTER_BUFFER_SIZE_16               4
#define FILTER_BUFFER_SIZE_32               5
#define FILTER_BUFFER_SIZE_64               6

#define _FILTER_BUFFER_Set(n)               FILTER_BUFFER_SIZE_##n
#define FILTER_BUFFER_Set(n)                _FILTER_BUFFER_Set(n)
#define FILTER_SIZE                         FILTER_BUFFER_Set(config_FILTER_BUFFER_SIZE)
#define FILTER_BUFFER_SIZE                  (1<<FILTER_SIZE)     


unsigned char linear_smooth(unsigned char entrant);
#endif


