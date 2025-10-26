
# lwcli

[![GitHub stars](https://img.shields.io/github/stars/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/network/members)
[![GitHub issues](https://img.shields.io/github/issues/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/issues)
[![License](https://img.shields.io/github/license/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/blob/master/LICENSE)

[中文](https://github.com/GYM-png/lwcli/blob/master/README.md)|[English](https://github.com/GYM-png/lwcli/blob/master/README_EN.md)

## Overview

**lwcli** is a lightweight command-line interface (CLI) component designed for embedded systems, enabling efficient command parsing and processing. It supports dynamic command registration, parameter parsing, and command history, making it suitable for various hardware interfaces such as serial ports or USB. Built in C and integrated with FreeRTOS for task management, lwcli is ideal for resource-constrained embedded environments.

Current version: `V0.0.1`

## Features

- **Lightweight Design**: Minimal resource usage, tailored for embedded systems.
- **Command Registration**: Supports dynamic registration of user-defined commands with help messages.
- **Parameter Parsing**: Automatically parses command parameters, supporting up to `LWCLI_COMMAND_STR_MAX_LENGTH` bytes for command names.
- **Command History**: Stores up to `LWCLI_HISTORY_COMMAND_NUM` command history entries, using approximately `LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE` bytes of memory.
- **Cross-Platform**: Hardware abstraction layer (`lwcli_port.c`) supports various hardware platforms with serial/USB interfaces.
- **FreeRTOS Integration**: Provides task creation and processing for seamless integration in multi-tasking embedded environments.
- **Error Handling**: Includes built-in help command and unrecognized command prompts.

## Getting Started

### Dependencies

- **Build Environment**: C compiler (e.g., GCC)
- **Operating System**: FreeRTOS (for task management)
- **Hardware**: Platform with serial or USB interface support
- **Libraries**: Standard C libraries (`stdlib.h`, `stdio.h`, `string.h`)

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/GYM-png/lwcli.git
   cd lwcli
   ```

2. Configure hardware interfaces:
   - Modify `lwcli_port.c` to implement interface functions (`lwcli_malloc`, `lwcli_free`, `lwcli_hardware_init`, `lwcli_output_char`, `lwcli_output_string`, `lwcli_receive`) for your target hardware platform.
   - Ensure hardware driver dependencies (`usart_drv.h`, `mydebug.h`, `malloc.h`) are properly configured.

3. Build the project:
   - Include `lwcli.c`, `lwcli_task.c`, `lwcli_port.c`, `lwcli.h`, `lwcli_config.h`, and `main.c` in your embedded project.
   - Configure the FreeRTOS environment, setting task stack size and priority.

4. Start the task:
   - Call `lwcli_task_start(StackDepth, uxPriority)` to create the lwcli task.
   - Example:
     ```c
     lwcli_task_start(512, 4); // Stack size: 512 bytes, Priority: 4
     ```

### Usage Example

The `main.c` file provides a sample implementation demonstrating how to initialize lwcli and register commands. **Note**: This file is for interface demonstration only and cannot be run directly without hardware-specific implementation.

#### Sample Code
```c
#include "lwcli.h"

void test_command_callback(char **args, uint8_t arge_num) {
    for (int i = 0; i < arge_num; i++) {
        printf("%s ", args[i]); // Print all parameters
    }
    printf("\r\n");
}

void lwcli_init(void) {
    lwcli_task_start(512, 4); // Start lwcli task
    lwcli_regist_command("test", "help string of test command", test_command_callback); // Register test command
}

int main(void) {
    lwcli_init();
    vTaskStartScheduler();
    while (1) {
        /* never run to here */
    }
    return 0;
}
```

#### Runtime Behavior
- On startup, lwcli outputs a welcome message:
  ```
  lwcli start, nType Help to view a list of registered commands
  ```
- Enter `help` to list registered commands:
  ```
  test:    help string of test command
  ```
- Enter `test 123 456 -789` to execute the custom command, outputting:
  ```
  123 456 -789
  ```
- Use the up (`↑`) and down (`↓`) arrow keys (if history is enabled) to navigate command history.

#### Command History
- Supports up to 10 command history entries.
- Use `↑` (up arrow) to view the previous command and `↓` (down arrow) to view the next command.
- History consumes approximately 510 bytes of memory (`LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE + LWCLI_HISTORY_COMMAND_NUM`).

## Configuration

The `lwcli_config.h` file defines the following configuration parameters:

| Parameter Name                    | Default Value | Description                              |
|-----------------------------------|---------------|------------------------------------------|
| `LWCLI_COMMAND_STR_MAX_LENGTH`    | 10            | Maximum command string length (excluding parameters) |
| `LWCLI_HELP_STR_MAX_LENGTH`       | 100           | Maximum help string length                |
| `LWCLI_RECEIVE_BUFFER_SIZE`       | 50            | Receive buffer size                       |
| `LWCLI_HISTORY_COMMAND_NUM`       | 10            | Maximum number of history commands (0 to disable) |
| `LWCLI_SHELL_DELETE_CHAR`         | 0x7F          | Delete character                          |
| `LWCLI_SHELL_BACK_CHAR`           | '\177'        | Backspace character                       |
| `LWCLI_SHELL_CURSOR_RIGHT_STRING` | "\033[C"      | ANSI sequence for cursor right            |
| `LWCLI_SHELL_CURSOR_LEFT_STRING`  | "\033[D"      | ANSI sequence for cursor left             |
| `LWCLI_SHELL_CLEAR_STRING`        | "\x1B[2J"     | ANSI sequence for screen clear            |
| `LWCLI_SHELL_CURSOR_TO_ZERO_STRING`| "\x1B[0;0H"  | ANSI sequence to move cursor to (0,0)     |
| `LWCLI_WITH_FILE_SYSTEM`              | true                  | Enable file system prompt                |
| `LWCLI_USER_NAME`                     | "lwcli@STM32"         | Username (only valid when file system is enabled) |
| `LWCLI_COMMAND_FIND_FAIL_MESSAGE` | "Command not recognised. Enter 'help' to view a list of available commands.\r\n" | Unrecognized command message |

> **File System Support**:  
> - When `LWCLI_WITH_FILE_SYSTEM = true`, the prompt will display:  
>   `LWCLI_USER_NAME` + `:` + `current path` + `$ `  
> - The current path is returned by the user-implemented `lwcli_get_file_path()` function in `lwcli_port.c`.  
> - If the function is not implemented or returns `NULL`, the default path `/` will be shown.

Modify these parameters to suit your needs, keeping memory constraints in mind.

## Project Structure

```plain
lwcli/
├── src/                # Source files required for porting
│   ├── lwcli.c         # Core command parsing logic
│   ├── lwcli_task.c    # FreeRTOS task implementation
│   └── lwcli_port.c    # Hardware abstraction layer (user-implemented)
├── inc/                # Header files required for porting
│   ├── lwcli.h         # User interface header
│   └── lwcli_config.h  # Configuration header
├── example/            # Usage examples
│   └── main.c          # Example code (interface demo only, not directly runnable)
├── LICENSE             # MIT License
├── README.md           # Chinese documentation
└── README_EN.md        # English documentation
```

## Contributing

1. Fork the repository.
2. Create a new branch: `git checkout -b feature/your-feature`.
3. Commit changes: `git commit -m 'Add your feature'`.
4. Push the branch: `git push origin feature/your-feature`.
5. Submit a Pull Request.

Please ensure your code meets the following requirements:
- Follows the existing code style.
- Includes necessary comments and documentation.
- Tests hardware interface compatibility.

## License

This project is licensed under the [MIT License](LICENSE). See the [LICENSE](https://github.com/GYM-png/lwcli/blob/master/LICENSE.txt) file for details.

## Contact

- Author: GYM (48060945@qq.com)
- Issues: [Issues](https://github.com/GYM-png/lwcli/issues)
- Discussions: [Discussions](https://github.com/GYM-png/lwcli/discussions)

Thank you for your interest and support! 🚀
