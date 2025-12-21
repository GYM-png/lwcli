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
    printf("********lwcli example start********\n");
    system("stty -icanon");
    system("stty -echo");

    lwcli_regist_command("test", "test command", test_func);
    while(1)
    {
        lwcli_process_receive_char(getchar());
    }
    return 0;
}