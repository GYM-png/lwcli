#ifndef __LWCLI_TASK_H
#define __LWCLI_TASK_H

#include "stdint.h"

/**
 * @brief 是否支持远程命令
 * @note true 允许远程命令，false 禁止远程命令
 * @note 允许远程命令后，用户可以通过 lwcli_write_remote_command 写入远程命令
 */
#define LWCLI_ENABLE_REMOTE_COMMAND true

/**
 * @brief 启动lwcli任务
 * @param StackDepth 栈大小
 * @param uxPriority 优先级
 */
void lwcli_task_start(const uint16_t StackDepth, const uint8_t uxPriority);



#if (LWCLI_ENABLE_REMOTE_COMMAND == true)
/**
 * @brief 写入远程命令
 * @param command 命令字符串
 * @param command_len 命令长度
 */
void lwcli_write_remote_command(char *command, uint16_t command_len);
#endif  // LWCLI_ENABLE_REMOTE_COMMAND

#endif // !__LWCLI_TASK_H
