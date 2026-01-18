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

#define LWCLI_VERSION "V0.0.3"

/**
 * @brief lwcli 软件初始化
 */
void lwcli_software_init(void);

/**
 * @brief 用户自定义命令处理函数
 * @param cmdParameterArry 参数数组
 * @param parameterNum 参数数量
 */
typedef void (*user_callback_f)(int argc, char* argv[]);

/**
 * @brief 注册命令
 * @param cmdStr 命令字符串 
 * @param helpStr 帮助字符串 
 * @param userCallback 用户自定义命令处理函数
 * @return 命令描述符
 */
int lwcli_regist_command(const char *cmdStr, const char *helpStr, user_callback_f userCallback);


/**
 * @brief 注册命令详细说明
 * @param command_fd 命令描述符 @brief lwcli_regist_command
 * @param usage 命令的用法 可以为NULL
 * @param description 详细的说明 可以为NULL
 */
void lwcli_regist_command_help(int command_fd, const char *usage, const char *description);

/**
 * @brief 处理接收字符
 * @param revChar 输入的字符
 */
void lwcli_process_receive_char(char revChar);


#ifdef __cplusplus
    }
#endif
#endif //LWCLI_LWCLI_H
