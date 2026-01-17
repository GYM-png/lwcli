#include "lwcli.h"
#include <stdio.h>
#include <stdlib.h>



void test_func(int argc, char *argv[])
{
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        printf("%s, ", argv[i]);
    }
    printf("\n");
}

int main(void)
{
    system("stty -icanon");
    system("stty -echo");
    lwcli_software_init();
    lwcli_regist_command("test12", "test command1", test_func);
    lwcli_regist_command("test2", "test command2", test_func);
    lwcli_regist_command("test3", "test command3", test_func);
    lwcli_regist_command("test4", "test command4", test_func);

    while(1)
    {
        lwcli_process_receive_char(getchar());
    }
    return 0;
}