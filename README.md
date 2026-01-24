
# lwcli

[![GitHub stars](https://img.shields.io/github/stars/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/GYM-png/lwcli?style=social)](https://github.com/GYM-png/lwcli/network/members)
[![GitHub issues](https://img.shields.io/github/issues/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/issues)
[![License](https://img.shields.io/github/license/GYM-png/lwcli)](https://github.com/GYM-png/lwcli/blob/master/LICENSE)

[中文](https://github.com/GYM-png/lwcli/blob/master/README.md)|[English](https://github.com/GYM-png/lwcli/blob/master/README_EN.md)

## 概述

**lwcli** 是一个轻量级命令行界面 (CLI) 组件，专为嵌入式系统设计，用于实现高效的命令解析和处理。它支持动态命令注册、参数解析、命令历史记录等功能，适用于串口、USB 等多种硬件接口。lwcli 基于 C 语言开发，结合 FreeRTOS 实现任务管理，适合资源受限的嵌入式环境。

当前版本：`V0.0.4`

![效果演示](/example.gif)

## 特性

- **轻量级设计**：占用资源少，适合嵌入式系统
- **动态命令注册**：支持注册命令、简要帮助、**用法（usage）** 和 **详细说明（description）**
- **Tab 自动补全**：支持命令名前缀补全，Tab 显示匹配列表
- **参数解析**：自动分割参数，支持引号包裹参数（支持最多 `LWCLI_RECEIVE_BUFFER_SIZE` 长度）
- **命令历史记录**：支持最多 `LWCLI_HISTORY_COMMAND_NUM` 条记录，使用上下箭头键浏览
- **光标编辑**：支持左右方向键移动光标、Backspace/Delete 删除字符
- **文件系统风格提示符**：启用 `LWCLI_WITH_FILE_SYSTEM` 后显示用户名:路径 $ （类似 Linux shell）
- **跨平台**：通过 `lwcli_port.c` 硬件抽象层适配不同 MCU/串口/USB
- **FreeRTOS 集成**：提供独立任务处理输入输出
- **增强的帮助系统**：支持 `help` 列出所有命令、`help <cmd>` 查看详细用法和说明

## 快速开始

### 依赖

- **编译环境**：C 编译器 (如 GCC)
- **操作系统**：支持操作系统（例如FReeRTOS）或裸机
- **硬件**：支持串口或 USB 的硬件平台
- **库**：标准 C 库 (`stdlib.h`, `stdio.h`, `string.h`)

### 安装

1. 克隆仓库：
   ```bash
   git clone https://github.com/GYM-png/lwcli.git
   cd lwcli
   ```

2. 配置硬件接口：
    - 编辑 `lwcli_port.c` 中的接口函数 (`lwcli_malloc`, `lwcli_free`, `lwcli_hardware_init`, `lwcli_output`)，适配目标硬件平台。

3. 编译项目：
    - 将 `lwcli.c`, `lwcli_port.c`, `lwcli.h`, `lwcli_config.h` 加入您的嵌入式项目。


### 使用示例

`lwcli/example/linux/main.c` 提供了一个裸机示例，展示如何初始化 lwcli、注册命令和调用处理接口

`lwcli/example/FReeRTOS/main.c` 提供了一个FreeRTOS示例，展示如何初始化 lwcli、注册命令和调用处理接口

`lwcli/example/linux/` 中提供了编译并运行的脚本 `build_run.sh` 可以在Linux环境下中直接运行示例


#### 命令历史记录
- 支持最多 10 条命令历史记录。
- 使用 `↑` (上箭头) 查看上一条命令，`↓` (下箭头) 查看下一条命令。
- 历史记录占用约 510 字节内存 (`LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE + LWCLI_HISTORY_COMMAND_NUM`).

## 配置说明

`lwcli_config.h` 中定义了以下配置参数：

| 参数名                        | 默认值 | 描述                              |
|-------------------------------|--------|-----------------------------------|
| `LWCLI_COMMAND_STR_MAX_LENGTH` | 10                   | 命令字符串最大长度（不含参数）    |
| `LWCLI_BRIEF_MAX_LENGTH`        | 100              | 帮助字符串最大长度                |
| `LWCLI_RECEIVE_BUFFER_SIZE`        | 50               | 接收缓冲区大小                    |
| `LWCLI_HISTORY_COMMAND_NUM`        | 10               | 历史命令最大数量（0 禁用历史记录）|
| `LWCLI_WITH_FILE_SYSTEM`          | true              | 是否启用文件系统提示符     |
| `LWCLI_USER_NAME`                 | "lwcli@STM32"     | 用户名（仅在文件系统启用时有效）|

> **文件系统支持**：  
> - 启用 `LWCLI_WITH_FILE_SYSTEM = true` 后，提示符将显示为：  
>   `LWCLI_USER_NAME` + `:` + `当前路径` + `$ `  
> - 当前路径由用户在 `lwcli_port.c` 中实现 `lwcli_get_file_path()` 函数返回。  
> - 若未实现该函数或返回 `NULL`，将显示默认路径 `/`。
修改这些参数以适配您的需求，但需注意内存占用。

## 项目结构

```plain
lwcli/
├── src/                # 移植所需源文件
│   ├── lwcli.c         # 核心命令解析逻辑
│   └── lwcli_port.c    # 硬件抽象层（需用户实现）
├── inc/                # 移植所需头文件
│   ├── lwcli.h         # 用户接口头文件
│   └── lwcli_config.h  # 配置参数头文件
├── example/            # 使用示例
|   ├──linux            # Linux示例代码
│   └──FReeRTOS         # FreeRTOS示例代码
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

