/**
 * @file lwcli_task.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 任务
 * @note 创建lwcli 任务，实现对接收字符串的解析
 * @version V0.0.4
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "lwcli.h"
#include "lwcli_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwcli_config.h"
#include "string.h"
#include "stdbool.h"


#if (LWCLI_ENABLE_REMOTE_COMMAND == true)
/**
 * @brief 存储远程命令
 */
typedef struct
{
    char *command;
    uint16_t command_len;
}lwcli_remote_cmd_t;
static lwcli_remote_cmd_t remote_cmd_info = {NULL, 0};

/**
 * @brief 写入远程命令
 * @param command 命令
 * @param command_len 命令长度
 */
void lwcli_write_remote_command(char *command, uint16_t command_len)
{
    if (command == NULL || remote_cmd_info.command == NULL || command_len > LWCLI_RECEIVE_BUFFER_SIZE)
    {
        return;
    }
    memcpy(remote_cmd_info.command, command, command_len);
    remote_cmd_info.command_len = command_len;
}

/**
 * @brief 接收远程命令
 * @param buffer 接收缓冲区
 * @return 接收到的字符串长度 
 */
static uint16_t lwcli_receive_remote_command(char *buffer)
{
    if (remote_cmd_info.command == NULL)
    {
        return 0;
    }
    uint16_t recv_len = 0;
    if (remote_cmd_info.command_len)
    {
        memcpy(buffer, remote_cmd_info.command, remote_cmd_info.command_len);
        recv_len = remote_cmd_info.command_len;
        remote_cmd_info.command_len = 0;
    }
    return recv_len;
}
#endif // (LWCLI_ENABLE_REMOTE_COMMAND == true)

/**
 * @brief 接收函数
 * @param[out] buffer 缓冲区指针
 * @param[in] buffer_size 缓冲区大小
 * @param[in] time_out 接收超时时间
 * @return 接收到的字节数
 */
uint16_t lwcli_receive(char *buffer, uint16_t buffer_size, uint32_t time_out)
{
    /* TODO add your receive function here */
    return 0;
}


void lwcli_task(void *pvparameters)
{
    const lwcli_opt_t *opt = (const lwcli_opt_t *)pvparameters;
    char *receive_buffer  = NULL;
    uint16_t receive_length = 0;
    receive_buffer = (char *)pvPortMalloc(LWCLI_RECEIVE_BUFFER_SIZE);
    #if (LWCLI_ENABLE_REMOTE_COMMAND == true)
    remote_cmd_info.command = (char *)pvPortMalloc(LWCLI_RECEIVE_BUFFER_SIZE);
    #endif
    if (receive_buffer == NULL && opt != NULL && opt->output != NULL)
    {
        const char *error_message = "lwcli malloc error before start\r\n";
        opt->output(error_message, strlen(error_message));
        while (1);
    }
    while(1)
    {
        #if (LWCLI_ENABLE_REMOTE_COMMAND == true)
        receive_length = lwcli_receive_remote_command(receive_buffer);
        if (receive_length == 0)
        {
            receive_length = lwcli_receive(receive_buffer, LWCLI_RECEIVE_BUFFER_SIZE, 100 / portTICK_PERIOD_MS);
        }
        if (receive_length == 0)
        {
            continue;
        }
        #else
        receive_length = lwcli_receive(receive_buffer, LWCLI_RECEIVE_BUFFER_SIZE, portMAX_DELAY);
        #endif
        for (int i = 0; i < receive_length; i++)
        {
            lwcli_process_receive_char(receive_buffer[i]);
        }
    }
}

/**
 * @brief 创建并启动lwcli任务
 * @param opt 接口结构体，不可为 NULL
 * @param StackDepth 堆栈大小
 * @param uxPriority 优先级
 */
void lwcli_task_start(const lwcli_opt_t *opt, const uint16_t StackDepth, const uint8_t uxPriority)
{
    if (opt == NULL) return;
    lwcli_hardware_init(opt);
    lwcli_software_init();
    taskENTER_CRITICAL();
    xTaskCreate((TaskFunction_t )lwcli_task,    //任务函数
                (const char *   )"lwcli",       //任务名称
                (uint16_t       )StackDepth,    //任务堆栈大小
                (void *         )(void *)opt,   //任务参数（传递 opt 供错误输出使用）
                (UBaseType_t    )uxPriority,   //任务优先级
                (TaskHandle_t *  )NULL);        //任务句柄
    taskEXIT_CRITICAL();
}

