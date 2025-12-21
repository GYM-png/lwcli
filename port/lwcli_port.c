/**
 * @file lwcli_port.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 对外接口实现
 * @version V0.0.3
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */
/* library includes. */
#include "stdint.h"
#include "lwcli_config.h"

/* Your includes. */
#include "stdlib.h"
#include "stdio.h"

/**
 * @brief 内存申请函数
 * @param size 申请的内存大小
 * @return 申请的内存指针
 */
void *lwcli_malloc(size_t size)
{
    /* add your malloc funcion here */
    return malloc( size );
}

/**
 * @brief 内存释放函数
 * @param[in] ptr 释放的内存指针
 */
void lwcli_free(void *ptr)
{
    /* add your free funcion here */
    free(ptr);
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
    printf("%c", output_char);
}

/**
 * @brief 字符串输出函数
 * @param output_string 输出的字符串
 * @param string_len 字符串长度
 */
void lwcli_output_string(const char *output_string, uint16_t string_len)
{
    /* add your output function here */
    printf("%s", output_string);
}


#if (LWCLI_WITH_FILE_SYSTEM == true)
/**
 * @brief 获取当前文件系统路径
 * @return 当前文件路径
 */
char *lwcli_get_file_path(void)
{
    /* add your get file path function here */
    return "/";
}
#endif