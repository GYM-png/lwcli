/**
 * @file lwcli.h
 * @author GYM (48060945@qq.com)
 * @brief 用户包含头文件
 * @version V0.0.3
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

#define LWCLI_VERSION "V0.0.2"


/**
 * @brief 用户自定义命令处理函数
 * @param cmdParameterArry 参数数组
 * @param parameterNum 参数数量
 */
typedef void (*cliCmdFunc)(int argc, char* argv[]);

/**
 * @brief 注册命令
 * @param cmdStr 命令字符串 
 * @param helpStr 帮助字符串 
 * @param userCallback 用户自定义命令处理函数
 */
void lwcli_regist_command(char *cmdStr, char *helpStr, cliCmdFunc userCallback);


/**
 * @brief 处理接收字符
 * @param revChar 输入的字符
 */
void lwcli_process_receive_char(char revChar);


#if (LWCLI_ENABLE_REMOTE_COMMAND == true)
/**
 * @brief 写入远程命令
 * @param command 命令字符串
 * @param command_len 命令长度
 */
void lwcli_write_remote_command(char *command, uint16_t command_len);
#endif  // LWCLI_ENABLE_REMOTE_COMMAND

#ifdef __cplusplus
    }
#endif


#endif //LWCLI_LWCLI_H
