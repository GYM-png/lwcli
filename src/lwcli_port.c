/**
 * @file lwcli_port.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 对外接口实现
 * @version 0.1
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */
/* library includes. */
#include "stdlib.h"
#include "stdio.h"

/* Your includes. */
#include "malloc.h"
#include "mydebug.h"
#include "usart_drv.h"


/**
 * @brief 内存申请函数
 * @param size 申请的内存大小
 * @return 申请的内存指针
 */
void *lwcli_malloc(size_t size)
{
    /* add your malloc funcion here */
    return mymalloc(MEM_SRM3, size );
}

/**
 * @brief 内存释放函数
 * @param[in] ptr 释放的内存指针
 */
void lwcli_free(void *ptr)
{
    /* add your free funcion here */
    myfree(ptr);
}

/**
 * @brief 硬件初始化函数
 * @note 串口/USB 初始化 
 */
void lwcli_hardware_init(void)
{
    /* add your hardware init function here */
}


/**
 * @brief 字符输出函数
 * @param output_char 输出的字符
 */
void lwcli_output_char(char output_char)
{
    /* add your output function here */
    debug_log_print("%c", output_char);
}

/**
 * @brief 字符串输出函数
 * @param output_string 输出的字符串
 * @param string_len 字符串长度
 */
void lwcli_output_string(char *output_string, uint16_t string_len)
{
    /* add your output function here */
    debug_log_print("%s", output_string);
}

/**
 * @brief 接收函数
 * @param[out] buffer 缓冲区指针
 * @param[in] buffer_size 缓冲区大小
 * @param[in] time_out 接收超时时间
 * @return 接收到的字节数
 */
uint16_t lwcli_receive(char *buffer, uint16_t buffer_size, uint32_t time_out)
{
    /* add your receive function here */
    return uart_receive(DEBUG_UART_INDEX, buffer, buffer_size, time_out);
}
