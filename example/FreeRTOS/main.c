#include "lwcli.h"
#include "lwcli_task.h"

/**
 * @brief 测试命令回调函数
 * @note 当输入"test 123 456 -789"时，会打印"123 456 -789"
 * @param argc 参数数量
 * @param argv 参数数组 
 */
void test_command_callback(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        printf("%s ", argv[i]);// 打印所有参数
    }
    printf("\r\n");
}

/**
 * @brief FreeRTOS任务监控
 * @param argc 
 * @param argv 
 */
void system_command_callback(int argc, char *argv[])
{
    char *task_info_buffer = NULL;
    uint16_t task_info_buffer_pos = 0;
    if (argc == 1)
    {
        if (strstr(argv[0], "-t"))
        {
            task_info_buffer = (char *)pvPortMalloc(1024);
            if (task_info_buffer == NULL)
            {
                return;
            }
            const char *const pcHeader = "    状态   优先级   堆栈     #\r\n************************************************\r\n";
            BaseType_t xSpacePadding;
            strcpy( task_info_buffer, "任务" );
            task_info_buffer_pos += strlen( task_info_buffer );
            configASSERT( configMAX_TASK_NAME_LEN > 3 );
            for( xSpacePadding = task_info_buffer_pos; xSpacePadding < ( configMAX_TASK_NAME_LEN - 3 ); xSpacePadding++ )
            {
                task_info_buffer[xSpacePadding] = ' ';
            }
            task_info_buffer_pos = ( configMAX_TASK_NAME_LEN - 3 );
            strcpy( task_info_buffer + task_info_buffer_pos, pcHeader );
            task_info_buffer_pos += strlen( pcHeader );
            vTaskList( task_info_buffer + task_info_buffer_pos );
            printf("%s", task_info_buffer); 
            vPortFree( task_info_buffer );
        }
    }
}

/**
 * @brief lwcli 初始化
 * @note  调用lwcli_task_start()启动lwcli任务
 * @note  调用lwcli_register_command()注册命令
 * @param  
 */
void lwcli_init(void)
{
    lwcli_task_start(512, 4); // 启动lwcli 任务 传入任务栈大小和优先级

    /* 注册命令 */
    int command_fd = 0;
    command_fd = lwcli_regist_command("system", "系统相关 ", system_command_callback);
    lwcli_regist_command_parameter(command_fd, "-t", "查看任务信息 ");
    lwcli_regist_command("test", "test command", test_command_callback);
}


int main(void)
{
    /* your code here */
    lwcli_init();

    /* Start scheduler */
    vTaskStartScheduler();
    while (1)
    {
        /* never run to here */
    }
    
    return 0;
}
