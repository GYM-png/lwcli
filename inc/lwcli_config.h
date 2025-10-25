/**
 * @file lwcli_config.h
 * @author your name (you@domain.com)
 * @brief 用户配置参数文件
 * @version 0.1
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
 * @defgroup ANSI 控制序列
 * { 
 */
/**
 * @brief 终端删除字符
 */
#define LWCLI_SHELL_DELETE_CHAR 0x7F


/**
 * @brief 终端退格字符
 */
#define LWCLI_SHELL_BACK_CHAR '\177'

/**
 * @brief 控制终端光标右移
 */
#define LWCLI_SHELL_CURSOR_RIGHT_STRING "\033[C"

/**
 * @brief 控制终端光标左移
 */
#define LWCLI_SHELL_CURSOR_LEFT_STRING "\033[D"

/**
 * @brief 控制终端清屏字符
 */
#define LWCLI_SHELL_CLEAR_STRING "\x1B[2J"


/**
 * @brief 终端光标移动到0,0位置
 */
#define LWCLI_SHELL_CURSOR_TO_ZERO_STRING "\x1B[0;0H"
/**
 * @defgroup
 * } 
 */

/**
 * @brief 命令未找到错误信息
 */
#define LWCLI_COMMAND_FIND_FAIL_MESSAGE "Command not recognised.  Enter 'help' to view a list of available commands.\r\n"

#endif
