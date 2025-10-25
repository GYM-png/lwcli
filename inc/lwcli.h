/**
 * @file lwcli.h
 * @author GYM (48060945@qq.com)
 * @brief 用户包含头文件
 * @version 0.1
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef LWCLI_LWCLI_H
#define LWCLI_LWCLI_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "stdint.h"

#define LWCLI_VERSION "V0.0.1"


/**
 * @brief 用户自定义命令处理函数
 * @param cmdParameterArry 参数数组
 * @param parameterNum 参数数量
 */
typedef void (*cliCmdFunc)(char **command_arry, uint8_t parameter_num);

void lwcli_regist_command(char *cmdStr, char *helpStr, cliCmdFunc);
void lwcli_task_start(const uint16_t StackDepth, const uint8_t uxPriority);

#ifdef __cplusplus
    }
#endif


#endif //LWCLI_LWCLI_H
