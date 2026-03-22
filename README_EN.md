
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
- **Command & parameter registration**: Dynamically register commands and parameters with brief help, and **detailed description**
- **Tab completion**: Supports command prefix and parameter completion; press Tab to show matching suggestions
- **Parameter parsing**: Automatically splits parameters, supports quoted arguments (up to `LWCLI_RECEIVE_BUFFER_SIZE` length)
- **Command history**: Supports up to `LWCLI_HISTORY_COMMAND_NUM` entries; navigate with up/down arrow keys
- **Cursor editing**: Supports left/right arrow keys for cursor movement, Backspace/Delete for character removal
- **File-system-style prompt**: When `LWCLI_WITH_FILE_SYSTEM` is enabled, displays `username:path $` (similar to Linux shell)
- **Cross-platform**: Function pointer injection via `lwcli_opt_t` adapts to different MCUs, serial, or USB without port files
- **Zero malloc at runtime**: Tab completion, parameter splitting, etc. allocate from a dynamic memory pool; no heap fragmentation
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
   - Implement the function pointers in `lwcli_opt_t` (`malloc`, `free`, `output`, etc.) and pass them to `lwcli_hardware_init(&opt)`.

3. Build the project:
   - Include `lwcli.c`, `lwcli_list.c`, `lwcli.h`, `lwcli_config.h` in your embedded project.


### Usage Examples

`lwcli/example/linux/main.c` provides a bare-metal/Linux example showing initialization, command registration, and processing.

**Interface injection example**:
```c
static const lwcli_opt_t opt = {
    .malloc = my_malloc,
    .free = my_free,
    .output = my_uart_output,
    .hardware_init = my_uart_init,   /* optional, NULL to skip */
    .get_file_path = my_get_path,   /* optional, when LWCLI_WITH_FILE_SYSTEM */
};
lwcli_hardware_init(&opt);
lwcli_software_init();
```

`lwcli/example/FreeRTOS/main.c` provides a FreeRTOS example with task-based integration.

In `lwcli/example/linux/`, the script `build_run.sh` allows you to compile and run the example directly on Linux.


#### Command History
- Supports up to 10 command history entries.
- Use `↑` (up arrow) to view the previous command and `↓` (down arrow) to view the next command.
- History consumes approximately 510 bytes of memory (`LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE + LWCLI_HISTORY_COMMAND_NUM`).

## Configuration

The `lwcli_config.h` file defines the following configuration parameters (switch options use `true`/`false`):

| Parameter Name                    | Default Value | Description                              |
|-----------------------------------|---------------|------------------------------------------|
| `LWCLI_COMMAND_STR_MAX_LENGTH`    | 10            | Maximum command string length (excluding parameters) |
| `LWCLI_BRIEF_MAX_LENGTH`       | 100           | Maximum help string length                |
| `LWCLI_RECEIVE_BUFFER_SIZE`       | 50            | Receive buffer size                       |
| `LWCLI_HISTORY_COMMAND_NUM`       | 10            | Maximum number of history commands (0 to disable) |
| `LWCLI_DYNAMIC_POOL_SIZE`         | 256           | Runtime dynamic pool size (Tab completion, parameter splitting, etc.) |
| `LWCLI_PARAMETER_SPLIT`           | true          | Split parameters: true = `(int argc, char *argv[])`, false = `(char *argvs)` |
| `LWCLI_PARAMETER_COMPLETION`      | true          | Enable parameter completion (requires `LWCLI_PARAMETER_SPLIT=true`) |
| `LWCLI_STATIC_POOL_SIZE`          | 512           | Parameter registration pool size (only when parameter completion enabled) |
| `LWCLI_WITH_FILE_SYSTEM`              | true                  | Enable file system prompt                |
| `LWCLI_USER_NAME`                     | "lwcli@STM32"         | Username (only valid when file system is enabled) |

> **File System Support**:  
> - When `LWCLI_WITH_FILE_SYSTEM = true`, the prompt will display:  
>   `LWCLI_USER_NAME` + `:` + `current path` + `$ `  
> - The current path is returned by `opt->get_file_path`.  
> - If the function is `NULL` or not implemented, the default path `/` will be shown.

> **Parameter Mode**:  
> - `LWCLI_PARAMETER_SPLIT = true`: Callback signature is `(int argc, char *argv[])`, parameters are auto-split.  
> - `LWCLI_PARAMETER_SPLIT = false`: Callback signature is `(char *argvs)`, raw argument string is passed, parameter completion is disabled.

> **Memory Pools**:  
> - `LWCLI_DYNAMIC_POOL_SIZE`: Runtime allocations (Tab completion, parameter splitting) come from this pool; no malloc at runtime, no heap fragmentation.  
> - `LWCLI_STATIC_POOL_SIZE`: Used for parameter registration; only when `LWCLI_PARAMETER_COMPLETION=true`.

Modify these parameters to suit your needs, keeping memory constraints in mind.

## Project Structure

```plain
lwcli/
├── src/                # Source files required for porting
│   ├── lwcli.c         # Core command parsing logic
│   └── lwcli_list.c    # Singly linked list implementation
├── inc/                # Header files required for porting
│   ├── lwcli.h         # User interface header (includes lwcli_opt_t)
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
