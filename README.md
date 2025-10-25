
# lwcli

[![GitHub stars](https://img.shields.io/github/stars/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/network/members)
[![GitHub issues](https://img.shields.io/github/issues/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/issues)
[![License](https://img.shields.io/github/license/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/blob/master/LICENSE)

[中文](https://github.com/GYM-png/lwcli/blob/master/README.md)|[English](https://github.com/GYM-png/lwcli/blob/master/README_EN.md)

## 概述

**lwcli** 是一个轻量级命令行界面 (CLI) 组件，专为嵌入式系统设计，用于实现高效的命令解析和处理。它支持动态命令注册、参数解析、命令历史记录等功能，适用于串口、USB 等多种硬件接口。lwcli 基于 C 语言开发，结合 FreeRTOS 实现任务管理，适合资源受限的嵌入式环境。

当前版本：`V0.0.1`

## 特性

- **轻量级设计**：占用资源少，适合嵌入式系统。
- **命令注册**：支持动态注册用户自定义命令及帮助信息。
- **参数解析**：自动解析命令参数，最大支持 `LWCLI_COMMAND_STR_MAX_LENGTH` 字节的命令长度。
- **历史记录**：支持最多 `LWCLI_HISTORY_COMMAND_NUM` 条命令历史记录，占用约 `LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE` 字节内存。
- **光标编辑**: 支持左右方向键移动光标，实现命令行编辑（插入、删除字符）。
- **跨平台**：通过硬件抽象层 (`lwcli_port.c`) 支持不同硬件平台的串口/USB 接口。
- **FreeRTOS 集成**：提供任务创建和处理功能，易于嵌入式多任务环境。
- **错误提示**：内置帮助命令和未识别命令提示。

## 快速开始

### 依赖

- **编译环境**：C 编译器 (如 GCC)
- **操作系统**：FreeRTOS (用于任务管理)
- **硬件**：支持串口或 USB 的硬件平台
- **库**：标准 C 库 (`stdlib.h`, `stdio.h`, `string.h`)

### 安装

1. 克隆仓库：
   ```bash
   git clone https://github.com/GYM-png/lwcli.git
   cd lwcli
   ```

2. 配置硬件接口：
    - 编辑 `lwcli_port.c` 中的接口函数 (`lwcli_malloc`, `lwcli_free`, `lwcli_hardware_init`, `lwcli_output_char`, `lwcli_output_string`, `lwcli_receive`)，适配目标硬件平台。
    - 确保依赖的硬件驱动 (`usart_drv.h`, `mydebug.h`, `malloc.h`) 已正确配置。

3. 编译项目：
    - 将 `lwcli.c`, `lwcli_task.c`, `lwcli_port.c`, `lwcli.h`, `lwcli_config.h`, `main.c` 加入您的嵌入式项目。
    - 配置 FreeRTOS 环境，设置任务堆栈大小和优先级。

4. 启动任务：
    - 调用 `lwcli_task_start(StackDepth, uxPriority)` 创建 lwcli 任务。
    - 示例：
      ```c
      lwcli_task_start(512, 4); // 堆栈大小 512 字节，优先级 4
      ```

### 使用示例

`main.c` 提供了一个示例，展示如何初始化 lwcli 并注册命令。**注意**：该文件仅用于演示接口使用，无法直接运行，需结合具体硬件平台实现。

#### 示例代码
```c
#include "lwcli.h"

void test_command_callback(char **args, uint8_t arge_num) {
    for (int i = 0; i < arge_num; i++) {
        printf("%s ", args[i]); // 打印所有参数
    }
    printf("\r\n");
}

void lwcli_init(void) {
    lwcli_task_start(512, 4); // 启动 lwcli 任务
    lwcli_regist_command("test", "help string of test command", test_command_callback); // 注册 test 命令
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

#### 运行效果
- 启动后，lwcli 输出欢迎信息：
  ```
  lwcli start, nType Help to view a list of registered commands
  ```
- 输入 `help` 查看已注册命令列表：
  ```
  test:    help string of test command
  ```
- 输入 `test 123 456 -789` 执行自定义命令，输出：
  ```
  123 456 -789
  ```
- 使用上下箭头键 (若启用历史记录) 浏览历史命令。

#### 命令历史记录
- 支持最多 10 条命令历史记录。
- 使用 `↑` (上箭头) 查看上一条命令，`↓` (下箭头) 查看下一条命令。
- 历史记录占用约 510 字节内存 (`LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE + LWCLI_HISTORY_COMMAND_NUM`).

## 配置说明

`lwcli_config.h` 中定义了以下配置参数：

| 参数名                        | 默认值 | 描述                              |
|-------------------------------|--------|-----------------------------------|
| `LWCLI_COMMAND_STR_MAX_LENGTH` | 10           | 命令字符串最大长度（不含参数）    |
| `LWCLI_HELP_STR_MAX_LENGTH`        | 100      | 帮助字符串最大长度                |
| `LWCLI_RECEIVE_BUFFER_SIZE`        | 50       | 接收缓冲区大小                    |
| `LWCLI_HISTORY_COMMAND_NUM`        | 10       | 历史命令最大数量（0 禁用历史记录）|
| `LWCLI_SHELL_DELETE_CHAR`          | 0x7F     | 删除字符                          |
| `LWCLI_SHELL_BACK_CHAR`            | '\177'   | 退格字符                          |
| `LWCLI_SHELL_CURSOR_RIGHT_STRING`  | "\033[C"   | 光标右移 ANSI 序列            |
| `LWCLI_SHELL_CURSOR_LEFT_STRING`   | "\033[D"   | 光标左移 ANSI 序列           |
| `LWCLI_SHELL_CLEAR_STRING`            | "\x1B[2J"   | 清屏 ANSI 序列      |
| `LWCLI_SHELL_CURSOR_TO_ZERO_STRING`   | "\x1B[0;0H"   | 光标移至 (0,0) 位置      |
| `LWCLI_COMMAND_FIND_FAIL_MESSAGE` | "Command not recognised. Enter 'help' to view a list of available commands.\r\n" | 命令未找到提示 |

修改这些参数以适配您的需求，但需注意内存占用。

## 项目结构

```plain
lwcli/
├── src/                # 移植所需源文件
│   ├── lwcli.c         # 核心命令解析逻辑
│   ├── lwcli_task.c    # FreeRTOS 任务实现
│   └── lwcli_port.c    # 硬件抽象层（需用户实现）
├── inc/                # 移植所需头文件
│   ├── lwcli.h         # 用户接口头文件
│   └── lwcli_config.h  # 配置参数头文件
├── example/            # 使用示例
│   └── main.c          # 示例代码（仅演示接口，无法直接运行）
├── LICENSE             # MIT 许可证
├── README.md           # 中文文档
└── README_EN.md        # 英文文档
```

## 贡献指南

1. Fork 仓库。
2. 创建新分支：`git checkout -b feature/your-feature`。
3. 提交更改：`git commit -m 'Add your feature'`。
4. 推送分支：`git push origin feature/your-feature`。
5. 提交 Pull Request。

请确保代码符合以下要求：
- 遵循现有代码风格。
- 添加必要的注释和文档。
- 测试硬件接口的兼容性。

## 许可证

此项目采用 [MIT License](LICENSE)。详见 [LICENSE](https://github.com/GYM-png/lwcli/blob/master/LICENSE.txt) 文件。

## 联系方式

- 作者：GYM (48060945@qq.com)
- 问题反馈：[Issues](https://github.com/GYM-png/lwcli/issues)
- 讨论：[Discussions](https://github.com/GYM-png/lwcli/discussions)

感谢您的关注与支持！🚀

