/**
 * @file lwcli_task.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 任务
 * @note 创建lwcli 任务，实现对接收字符串的解析
 * @version 0.1
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "lwcli.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwcli_config.h"
#include "string.h"


/* lwcli_port 接口 */
extern uint16_t lwcli_receive(char *buffer, uint16_t buffer_size, uint32_t time_out);
extern void *lwcli_malloc(size_t size);
extern void lwcli_output_char(char output_char);
extern void lwcli_hardware_init(void);

void lwcli_task(void *pvparameters)
{
    uint8_t *receive_buffer  = NULL;
    uint16_t receive_length = 0;
    lwcli_hardware_init();
    receive_buffer = (uint8_t *)lwcli_malloc(LWCLI_RECEIVE_BUFFER_SIZE);
    char *wellcome_message = "lwcli start, nType Help to view a list of registered commands\r\n";
    if (receive_buffer == NULL)
    {
        char *error_message = "lwcli malloc error before start\r\n";
        for (size_t i = 0; i < strlen(error_message); i++)
        {
            lwcli_output_char(error_message[i]);
        }
        while (1);
    }
    for (size_t i = 0; i < strlen(wellcome_message); i++)
    {
        lwcli_output_char(wellcome_message[i]);
    }
    while(1)
    {
        receive_length = lwcli_receive(receive_buffer, LWCLI_RECEIVE_BUFFER_SIZE, portMAX_DELAY);
        for (int i = 0; i < receive_length; i++)
        {
            lwcli_process_receive_char(receive_buffer[i]);
        }
    }
}


void lwcli_task_start(const uint16_t StackDepth, const uint8_t uxPriority)
{
    taskENTER_CRITICAL();
    xTaskCreate((TaskFunction_t )lwcli_task,    //任务函数
                (const char *   )"lwcli",       //任务名称
                (uint16_t       )StackDepth,    //任务堆栈大小
                (void *         )NULL,          //任务参数
                (UBaseType_t    )uxPriority,    //任务优先级
                (TaskHandle_t * )NULL);         //任务句句柄
    taskEXIT_CRITICAL();  
}

