/**
 * @file lwcli_config.h
 * @author GYM (48060945@qq.com)
 * @brief 用户配置参数文件
 * @version V0.0.4
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __LWCLI_CONFIG_H__
#define __LWCLI_CONFIG_H__

/* 供预处理器使用的布尔值（开关配置使用 true/false，避免与 stdbool.h 冲突） */
#ifndef LWCLI_TRUE
#define LWCLI_TRUE 1
#endif  // LWCLI_TRUE
#ifndef LWCLI_FALSE
#define LWCLI_FALSE 0
#endif  // LWCLI_FALSE
#if !defined(__bool_true_false_are_defined)
#define true LWCLI_TRUE
#define false LWCLI_FALSE
#endif  // !defined(__bool_true_false_are_defined)

/**
 * @brief LWCLI 配置
 * @note 本文件定义 lwcli 所有可配置参数
 */

/**
 * @brief 命令字符串最大长度
 * @note 用于命令匹配的长度，不含参数。例如 "system -t" 和 "system reboot" 均匹配为 "system"，
 *       命令长度为 strlen("system")。
 */
#define LWCLI_COMMAND_STR_MAX_LENGTH 10

/**
 * @brief 帮助/简介字符串最大长度
 */
#define LWCLI_BRIEF_MAX_LENGTH 100

/**
 * @brief 接收/输入缓冲区大小
 */
#define LWCLI_RECEIVE_BUFFER_SIZE 50

/**
 * @brief 历史命令最大数量
 * @note 可使用上下箭头键回看的历史命令条数
 * @note 设为 0 可禁用命令历史
 * @note 启用历史约占用
 *       (LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE) + LWCLI_HISTORY_COMMAND_NUM 字节内存
 */
#define LWCLI_HISTORY_COMMAND_NUM 10

/**
 * @brief 格式化终端输出的缓冲区大小
 */
#define LWCLI_SHELL_OUTPUT_BUFFER_SIZE 512

/**
 * @brief 是否启用参数分割/提取
 * @note true:  回调 (int argc, char *argv[])，自动提取参数
 *       false: 回调 (char *argvs)，传入原始参数字符串
 */
#define LWCLI_PARAMETER_SPLIT true

/**
 * @brief 是否启用参数补全
 * @note 当为 false 或 LWCLI_PARAMETER_SPLIT 为 false 时，参数补全自动关闭。
 */
#define LWCLI_PARAMETER_COMPLETION true

#if (LWCLI_PARAMETER_SPLIT == false)
#undef LWCLI_PARAMETER_COMPLETION
#define LWCLI_PARAMETER_COMPLETION false
#endif  // LWCLI_PARAMETER_SPLIT == false

#if (LWCLI_PARAMETER_COMPLETION == true)
/**
 * @brief 存储已注册参数字符串的内存池大小
 * @note 使用固定大小池以避免频繁小块动态分配
 */
#define LWCLI_PARAMETER_BUFFER_POOL_SIZE 512
#endif  // LWCLI_PARAMETER_COMPLETION == true

/**
 * @brief 是否启用文件系统风格提示符
 * @note 为 true 时，提示符显示为 用户名:当前路径 $
 * @note 当前路径由 opt->get_file_path 返回（可为 NULL，默认 "/"）
 */
#define LWCLI_WITH_FILE_SYSTEM true

#if (LWCLI_WITH_FILE_SYSTEM == true)
/**
 * @brief 提示符中显示的用户名
 */
#define LWCLI_USER_NAME "lwcli@STM32"
#endif  // LWCLI_WITH_FILE_SYSTEM == true


#endif  // __LWCLI_CONFIG_H__
