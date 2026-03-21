/**
 * @file lwcli.h
 * @author GYM (48060945@qq.com)
 * @brief 用户包含头文件
 * @version V0.0.4
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
#include "stddef.h"
#include "lwcli_config.h"

#define LWCLI_VERSION "V0.0.4"

/**
 * @brief lwcli 可选接口结构体（通过函数指针注入）
 * @note 用户实现这些接口并传入 lwcli_hardware_init()，无需再提供 lwcli_port.c
 */
typedef struct lwcli_opt {
    void *(*malloc)(size_t size);                                    /**< 内存分配 */
    void (*free)(void *ptr);                                         /**< 内存释放 */
    void (*output)(const char *output_string, uint16_t string_len);  /**< 输出字符串到终端 */
    void (*hardware_init)(void);                                     /**< 硬件初始化（可为 NULL）*/
#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
    char *(*get_file_path)(void);                                    /**< 获取当前路径（可为 NULL，默认 "/"）*/
#endif
} lwcli_opt_t;

/**
 * @brief 硬件初始化，注册用户接口
 * @param opt 包含 malloc、free、output 等函数指针的结构体，不可为 NULL
 *
 * @note 必须在 lwcli_software_init() 之前调用。
 *       若 opt->hardware_init 非空，会在此函数内调用以完成硬件初始化。
 */
void lwcli_hardware_init(const lwcli_opt_t *opt);

/**
 * @brief lwcli 软件初始化
 * 
 * 初始化内部命令列表、默认命令（如 "help"）、历史缓冲区（若启用）及其他核心结构。
 * 应在系统启动时调用一次。
 */
void lwcli_software_init(void);

#if (LWCLI_PARAMETER_SPLIT == LWCLI_TRUE)
/**
 * @brief 用户命令回调函数类型（参数分割模式）
 * @param argc  传入命令的参数个数（不含命令名本身）
 * @param argv  参数字符串数组（argv[0] 为第一个参数）
 */
typedef void (*user_callback_f)(int argc, char *argv[]);
#else
/**
 * @brief 用户命令回调函数类型（原始字符串模式）
 * @param argvs  原始参数字符串（命令名之后的部分，前导空格已去除）
 */
typedef void (*user_callback_f)(char *argvs);
#endif

/**
 * @brief 注册新命令
 * @param command       命令字符串（如 "led"、"system reboot"）
 * @param brief         在 "help" 列表中显示的简短帮助（一行）
 * @param user_callback 命令被调用时执行的回调函数
 * @return              成功返回命令描述符（句柄），失败返回负值
 * 
 * @note 返回的描述符可用于 lwcli_regist_command_help() 附加详细用法和说明。
 */
int lwcli_regist_command(const char *command, const char *brief, user_callback_f user_callback);

#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
/**
 * @brief 为命令注册参数（用于帮助显示与 Tab 补全）
 * @param command_fd  lwcli_regist_command() 返回的命令描述符
 * @param parameter   参数字符串（如 "<on|off>"、"[filename]"、"-h"、"--help"）
 * @param description 该参数的详细说明（可为 NULL）
 * 
 * @note 用于为命令附加参数信息，可在详细帮助（"help <cmd>"）中显示，
 *       并可后续用于智能 Tab 补全。
 */
void lwcli_regist_command_parameter(int command_fd, const char *parameter, const char *description);
#endif

/**
 * @brief 处理一个接收到的字符
 * @param recv_char  来自 UART/USB/终端的输入字符
 * 
 * @note 这是向 lwcli 输入数据的主入口。通常从 UART 接收中断或轮询循环中调用。
 *       负责行编辑、历史导航、Tab 补全等。
 */
void lwcli_process_receive_char(char recv_char);


#ifdef __cplusplus
    }
#endif
#endif  // LWCLI_LWCLI_H
