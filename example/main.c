#include "lwcli.h"


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
 * @brief lwcli 初始化
 * @note  调用lwcli_task_start()启动lwcli任务
 * @note  调用lwcli_register_command()注册命令
 * @param  
 */
void lwcli_init(void)
{
    lwcli_task_start(512, 4); // 启动lwcli 任务 传入任务栈大小和优先级

    /* 注册命令 */
    lwcli_register_command("test", "help string of test command", test_command_callback);
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
