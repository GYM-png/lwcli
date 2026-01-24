/**
 * @file lwcli_config.h
 * @author GYM (48060945@qq.com)
 * @brief User Configuration Parameters File
 * @version V0.0.4
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __LWCLI_CONFIG_H__
#define __LWCLI_CONFIG_H__

/**
 * @brief LWCLI Configuration
 * @note This file defines all configurable parameters for lwcli
 */

/**
 * @brief Maximum length of the command string
 * @note This is the length used for command matching, excluding parameters.
 *       For example, both "system -t" and "system reboot" are matched as "system",
 *       so the command length is strlen("system").
 */
#define LWCLI_COMMAND_STR_MAX_LENGTH 10

/**
 * @brief Maximum length of the command brief/help string
 */
#define LWCLI_BRIEF_MAX_LENGTH 100

/**
 * @brief Size of the receive/input buffer
 */
#define LWCLI_RECEIVE_BUFFER_SIZE 50

/**
 * @brief Maximum number of history commands
 * @note Number of commands that can be recalled using up/down arrows
 * @note Set to 0 to disable command history
 * @note Enabling history consumes approximately
 *       (LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE) + LWCLI_HISTORY_COMMAND_NUM bytes of memory
 */
#define LWCLI_HISTORY_COMMAND_NUM 10

/**
 * @brief Size of the output buffer for formatted terminal output
 */
#define LWCLI_SHELL_OUTPUT_BUFFER_SIZE 512

/**
 * @brief Enable parameter completion
 * @note When enabled, the lwcli_regist_command_parameter() function becomes available
 *       and can be used to provide parameter hints for help and (future) tab completion
 */
#define LWCLI_PARAMETER_COMPLETION true

#if (LWCLI_PARAMETER_COMPLETION == true)
/**
 * @brief Size of the memory pool used to store registered parameter strings
 * @note A fixed-size pool is used to avoid frequent small dynamic allocations
 */
#define LWCLI_PARAMETER_BUFFER_POOL_SIZE 512
#endif  // LWCLI_PARAMETER_COMPLETION == true

/**
 * @brief Enable file system style prompt
 * @note When true, the prompt shows username:current_path $
 * @note The current path is obtained by calling user-implemented lwcli_get_file_path()
 *       in lwcli_port.c
 */
#define LWCLI_WITH_FILE_SYSTEM true

#if (LWCLI_WITH_FILE_SYSTEM == true)
/**
 * @brief Username shown in the prompt
 */
#define LWCLI_USER_NAME "lwcli@STM32"
#endif // (LWCLI_WITH_FILE_SYSTEM == true)


#endif
