/**
 * @file lwcli_config.h
 * @author GYM (48060945@qq.com)
 * @brief 用户配置参数文件
 * @version V0.0.3
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __LWCLI_CONFIG_H__
#define __LWCLI_CONFIG_H__

/**
 * @brief LWCLI 配置
 * @note 这里定义了LWCLI 的配置参数
 */

/**
 * @brief 命令字符串最大长度
 * @note 命令字符值命令匹配字符，不包含参数的长度
 *       例如 "system -t", "system reboot", 均属于"system" 的命令，只不过参数不同，他们的命令长度为strelen("system")
 */
#define LWCLI_COMMAND_STR_MAX_LENGTH 10


/**
 * @brief 帮助字符串最大长度
 */
#define LWCLI_HELP_STR_MAX_LENGTH 100

/**
 * @brief 接收缓冲区大小
 */
#define LWCLI_RECEIVE_BUFFER_SIZE 50


/**
 * @brief 历史命令最大数量
 * @note 支持查询的历史命令最大数量 
 * @note 0 表示不支持查询历史命令
 * @note 开启查找历史命令需要消耗大约(LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE) + LWCLI_HISTORY_COMMAND_NUM 字节的内存
 */
#define LWCLI_HISTORY_COMMAND_NUM 10

/**
 * @brief 终端输出缓冲区大小
 */
#define LWCLI_SHELL_OUTPUT_BUFFER_SIZE 128



/**
 * @brief 是否支持文件系统
 * @note true 表示支持文件系统，false 表示不支持文件系统
 * @note 开启后会输出一个用户名和当前路径的提示信息，文件路径获取由用户实现 @ref lwcli_get_file_path in lwcli_port.c
 */
#define LWCLI_WITH_FILE_SYSTEM false

#if (LWCLI_WITH_FILE_SYSTEM == true)
/**
 * @brief 用户名
 */
#define LWCLI_USER_NAME "lwcli@STM32"   
#endif // (LWCLI_WITH_FILE_SYSTEM == true)



#endif
