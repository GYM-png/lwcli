
# lwcli

[![GitHub stars](https://img.shields.io/github/stars/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/network/members)
[![GitHub issues](https://img.shields.io/github/issues/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/issues)
[![License](https://img.shields.io/github/license/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/blob/master/LICENSE)

[中文](https://github.com/GYM-png/lwcli/blob/master/README.md)|[English](https://github.com/GYM-png/lwcli/blob/master/README_EN.md)

## Overview

**lwcli** is a lightweight command-line interface (CLI) component designed for embedded systems, enabling efficient command parsing and processing. It supports dynamic command registration, parameter parsing, and command history, making it suitable for various hardware interfaces such as serial ports or USB. Built in C and integrated with FreeRTOS for task management, lwcli is ideal for resource-constrained embedded environments.

Current version: `V0.0.4`

![example](/example.gif)

## Features

- **Lightweight design**: Minimal resource usage, ideal for embedded systems
- **Dynamic command registration**: Register commands along with brief help, **usage**, and **detailed description**
- **Tab auto-completion**: Supports command prefix completion; press Tab to show matching suggestions
- **Parameter parsing**: Automatically splits parameters, supports quoted arguments (up to `LWCLI_RECEIVE_BUFFER_SIZE` length)
- **Command history**: Supports up to `LWCLI_HISTORY_COMMAND_NUM` entries; navigate with up/down arrow keys
- **Cursor editing**: Supports left/right arrow keys for cursor movement, Backspace/Delete for character removal
- **File-system-style prompt**: When `LWCLI_WITH_FILE_SYSTEM` is enabled, displays `username:path $` (similar to Linux shell)
- **Cross-platform**: Hardware abstraction layer in `lwcli_port.c` adapts to different MCUs, serial, or USB
- **FreeRTOS integration**: Provides a dedicated task for input/output handling
- **Enhanced help system**: `help` lists all commands; `help <cmd>` shows detailed usage and description

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
   - Modify `lwcli_port.c` to implement interface functions (`lwcli_malloc`, `lwcli_free`, `lwcli_hardware_init`, `lwcli_output`) for your target hardware platform.

3. Build the project:
   - Include `lwcli.c`, `lwcli_port.c`, `lwcli.h`, `lwcli_config.h` in your embedded project.


### Usage Examples

`lwcli/example/linux/main.c` provides a bare-metal/Linux example showing initialization, command registration, and processing.

`lwcli/example/FreeRTOS/main.c` provides a FreeRTOS example with task-based integration.

In `lwcli/example/linux/`, the script `build_run.sh` allows you to compile and run the example directly on Linux.


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
| `LWCLI_WITH_FILE_SYSTEM`              | true                  | Enable file system prompt                |
| `LWCLI_USER_NAME`                     | "lwcli@STM32"         | Username (only valid when file system is enabled) |

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
│   └── lwcli_port.c    # Hardware abstraction layer (user-implemented)
├── inc/                # Header files required for porting
│   ├── lwcli.h         # User interface header
│   └── lwcli_config.h  # Configuration header
├── example/            # Usage examples
|   ├──linux            # Example code for Linux
│   └──FReeRTOS         # Example code for FreeRTOS
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
