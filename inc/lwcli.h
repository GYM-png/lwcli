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

#define LWCLI_VERSION "V0.0.4"


/**
 * @brief Hardware initialization function
 * @note Initializes the underlying hardware interface (e.g. UART/serial port, USB CDC, etc.)
 *       that lwcli will use for input and output.
 * 
 * This function is called by the user during system startup, typically before
 * calling lwcli_software_init(). It should configure the hardware peripheral,
 * set baud rate, enable interrupts (if used), and prepare the communication
 * channel for character-based I/O.
 */
void lwcli_hardware_init(void);

/**
 * @brief Software initialization for lwcli
 * 
 * Initializes the internal command list, default commands (like "help"),
 * history buffer (if enabled), and other core structures.
 * This function should be called once during system startup.
 */
void lwcli_software_init(void);

/**
 * @brief User-defined command callback function type
 * 
 * This is the function type that user-registered commands must implement.
 * 
 * @param argc  Number of arguments passed to the command (excluding the command name itself)
 * @param argv  Array of argument strings
 *              - argv[0] is the first argument (if any)
 *              - argv[argc-1] is the last argument
 *              - The command name itself is NOT included in argv
 * 
 * @note If no arguments are provided, argc == 0 and argv may be NULL or point to an empty array.
 *       Users should always check argc before accessing argv elements.
 */
typedef void (*user_callback_f)(int argc, char* argv[]);

/**
 * @brief Register a new command
 * @param command       The command string (e.g. "led", "system reboot")
 * @param brief         Short help description shown in "help" list (one line)
 * @param user_callback Callback function to execute when the command is invoked
 * @return              Command descriptor (handle) on success, or negative value on failure
 * 
 * @note The returned descriptor can be used with lwcli_regist_command_help()
 *       to attach detailed usage and description.
 */
int lwcli_regist_command(const char *command, const char *brief, user_callback_f user_callback);

/**
 * @brief Register detailed help information for a command
 * @param command_fd    Command descriptor returned by lwcli_regist_command()
 * @param usage         Usage syntax string (e.g. "led <on|off> [index]"), can be NULL
 * @param description   Detailed explanation, examples, notes (multi-line supported), can be NULL
 * 
 * @note Call this function after lwcli_regist_command() if you want to provide
 *       richer help output when user types "help <command>".
 */
void lwcli_regist_command_help(int command_fd, const char *usage, const char *description);

/**
 * @brief Process one received character
 * @param recv_char  The input character from UART/USB/terminal
 * 
 * @note This is the main entry point for feeding input to lwcli.
 *       Typically called from your UART receive interrupt or polling loop.
 *       It handles line editing, history navigation, tab completion, etc.
 */
void lwcli_process_receive_char(char recv_char);


#ifdef __cplusplus
    }
#endif
#endif //LWCLI_LWCLI_H
