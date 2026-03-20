#include "lwcli.h"
#include "lwcli_task.h"
#include "lwcli_config.h"
#include "FreeRTOS.h"
#include <stdio.h>

/* FreeRTOS 平台接口实现（用户可替换为 UART 等） */
static void *opt_malloc(size_t size) { return pvPortMalloc(size); }
static void opt_free(void *ptr) { vPortFree(ptr); }
static void opt_output(const char *s, uint16_t len) {
    /* TODO: 替换为 UART 发送 */
    if (len == 1) putchar(*s);
    else printf("%.*s", len, s);
}
#if (LWCLI_WITH_FILE_SYSTEM == true)
static char *opt_get_file_path(void) { return "/"; }
#endif

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
    static const lwcli_opt_t opt = {
        .malloc = opt_malloc,
        .free = opt_free,
        .output = opt_output,
        .hardware_init = NULL,
#if (LWCLI_WITH_FILE_SYSTEM == true)
        .get_file_path = opt_get_file_path,
#endif
    };
    lwcli_task_start(&opt, 512, 4);  /* 启动 lwcli 任务 */

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
