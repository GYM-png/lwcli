#include "lwcli.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#define LWCLI_STRSTR(n, str) strstr(argv[n], str)

void test_func(int argc, char *argv[])
{
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        printf("%s, ", argv[i]);
    }
    printf("\n");
}

void echo_func(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

void date_func(int argc, char *argv[])
{
    struct tm *timeinfo = NULL;
    time_t rawtime = 0;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (argc == 0)
    {
        // 格式化时间字符串
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        printf("格式化时间: %s\n", buffer);
        
        // 更多格式选项
        strftime(buffer, sizeof(buffer), "%A, %B %d, %Y %I:%M:%S %p", timeinfo);
        printf("详细格式: %s\n", buffer);
    }
    if (argc == 1)
    {
        if (LWCLI_STRSTR(0, "get") != NULL){
            // 格式化时间字符串
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
            printf("格式化时间: %s\n", buffer);
            
            // 更多格式选项
            strftime(buffer, sizeof(buffer), "%A, %B %d, %Y %I:%M:%S %p", timeinfo);
            printf("详细格式: %s\n", buffer);
        }
    }
    if (argc == 2){
        if (LWCLI_STRSTR(0, "set")){
            char time_buf[30];
            struct tm time_set;
            sscanf(argv[1], "\"%04d/%02d/%02d %02d:%02d:%02d\"", &time_set.tm_year, &time_set.tm_mon, &time_set.tm_mday, &time_set.tm_hour, &time_set.tm_min, &time_set.tm_sec);
            if (time_set.tm_year > 2000 && time_set.tm_mon < 13 && time_set.tm_hour < 24 && time_set.tm_min < 60 && time_set.tm_sec < 60){
                printf("date set success %s\n", argv[1]);
            }
            else{
                printf("date set error, time format invaild\n");
            }
        }
    }
}

int main(void)
{
    system("stty -icanon");
    system("stty -echo");
    lwcli_software_init();
    int command_fd = 0;
    command_fd = lwcli_regist_command("echo", "echo command", echo_func);
    lwcli_regist_command_help(command_fd,
    "echo [\"str\"]", 
    "echo string behind the \"echo\"");
    command_fd = lwcli_regist_command("date", "get or set time", date_func);
    lwcli_regist_command_help(command_fd,
    "    [get]:get date info\r\n"
    "    [set][\"2026/01/18 14:22:53\"]:change date info of system", 
    NULL);
    lwcli_regist_command("test2", "test command2", test_func);
    lwcli_regist_command("test3", "test command3", test_func);
    lwcli_regist_command("test4", "test command4", test_func);

    while(1)
    {
        lwcli_process_receive_char(getchar());
    }
    return 0;
}