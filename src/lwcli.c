/**
 * @file lwcli.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 核心函数实现文件
 * @version V0.0.4
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "lwcli.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "lwcli_config.h"
#include "stdarg.h"
#include "ctype.h"

#if (LWCLI_PARAMETER_COMPLETION == true)
typedef struct parameter_list
{
    char *data;
    uint8_t len;
    char *description;
    struct parameter_list *next;
}parameter_t;

typedef struct
{
    char data[LWCLI_PARAMETER_BUFFER_POOL_SIZE];
    uint32_t size;
    uint32_t pos;
}pool_manage_t;
static pool_manage_t parameter_pool = {.size = LWCLI_PARAMETER_BUFFER_POOL_SIZE, .pos = 0};
#endif // LWCLI_PARAMETER_COMPLETION == true

typedef struct cmdList
{
    char command[LWCLI_COMMAND_STR_MAX_LENGTH];
    char brief[LWCLI_BRIEF_MAX_LENGTH];

#if (LWCLI_PARAMETER_COMPLETION == true)
    parameter_t parameter_head;
#endif

    uint8_t cmd_len;
    user_callback_f callback;
    struct cmdList *next;
}cmdList_t;

#if (LWCLI_HISTORY_COMMAND_NUM > 0)
typedef struct
{
    /** 命令历史记录缓冲区 ringbuffer*/
    char buffer[LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE];
    uint16_t commandStrSize;
    uint16_t recordHistoryNum;
    uint16_t writeMirror : 1;
    uint16_t writePos : 15;
    uint16_t readMirror : 1;
    uint16_t readPos : 15;

    /** 当前命令行位置 */
    uint16_t findPos;

    /** 存储命令历史记录索引 */
    /** 数组中存储的数值依次为第一条到最后一条命令在缓冲区中的位置 */
    uint16_t findPosIndex[LWCLI_HISTORY_COMMAND_NUM];
}historyList_t;
#endif  // LWCLI_HISTORY_COMMAND_NUM > 0

#if (LWCLI_WITH_FILE_SYSTEM == true)
#define ANSI_COLOR_TABLE \
    COLOR_INFO( "\x1B[31m", COLOR_RED   )\
    COLOR_INFO( "\x1B[32m", COLOR_GREEN )\
    COLOR_INFO( "\x1B[33m", COLOR_YELLOW)\
    COLOR_INFO( "\x1B[34m", COLOR_BLUE  )\
    COLOR_INFO( "\x1B[35m", COLOR_PURPLE)\
    COLOR_INFO( "\x1B[36m", COLOR_CYAN  )\
    COLOR_INFO( "\x1B[37m", COLOR_WHITE )\
    COLOR_INFO( "\x1B[30m", COLOR_NONE  )
    
typedef enum
{
#define COLOR_INFO(ansi, color_enum) color_enum,
    ANSI_COLOR_TABLE
#undef COLOR_INFO
}colorEnum_e;

static char *colorTable[] = {
#define COLOR_INFO(ansi, color_enum) ansi,
    ANSI_COLOR_TABLE
#undef COLOR_INFO
};
#define LWCLI_ANSI_COLOR_RESET "\x1B[0m"

#endif  // LWCLI_WITH_FILE_SYSTEM == true

typedef struct 
{
    cmdList_t *cmdListHead;
    uint8_t command_num;
    /** 输入输出缓冲区 **/
    char inputBuffer[LWCLI_RECEIVE_BUFFER_SIZE];
    char ouputBuffer[LWCLI_SHELL_OUTPUT_BUFFER_SIZE];
    uint16_t inputBufferPos;
    uint16_t cursorPos;

#if (LWCLI_HISTORY_COMMAND_NUM > 0)
    historyList_t historyList; // 历史记录表
#endif  // LWCLI_HISTORY_COMMAND_NUM > 0

#if (LWCLI_PARAMETER_COMPLETION == true)
    int help_fd;
#endif 
}lwcli_t;

static lwcli_t lwcliObj = {0};

/** 静态函数声明 **/
static void lwcli_help(int argc, char* argv[]);
static void lwcli_clear(int argc, char* argv[]);
static uint8_t lwcli_find_parameters(const char *argv_str, char **parameter_arry, uint8_t parameter_num);
static uint8_t lwcli_get_parameter_number(const char *command_string);
static void lwcli_process_command(char *command);
static void lwcli_printf(const char *format, ...);
static void lwcli_table_process(void);
static void lwcli_fix_command(void);

#if (LWCLI_PARAMETER_COMPLETION == true)
static void lwcil_fix_parameter(cmdList_t *cmd_node);
static char *lwcli_parameter_malloc(uint32_t size);
#endif 

#if (LWCLI_WITH_FILE_SYSTEM == true)
static void lwcli_output_string_withcolor(const char *str, colorEnum_e color);
static void lwcli_output_file_path(void);
#endif 

#if (LWCLI_HISTORY_COMMAND_NUM > 0)
static void lwcli_add_history_command(void);
static void lwcli_history_command_down(void);
static void lwcli_history_command_up(void);
#endif 

/** lwcli_port 接口 **/
extern void *lwcli_malloc(size_t size);
extern void lwcli_free(void *ptr);
extern void lwcli_output(const char *output_string, uint16_t string_len);
#if (LWCLI_WITH_FILE_SYSTEM == true)
extern char *lwcli_get_file_path(void);
#endif  // LWCLI_WITH_FILE_SYSTEM == true



/** ANSI序列 **/
static const char ansi_delete = '\177';
static const char ansi_cursor_left[] = "\033[D";
static const char ansi_cursor_right[] = "\033[C";
static const char ansi_cursor_up[] = "\033[A";
static const char ansi_cursor_down[] = "\033[B";
static const char ansi_clear_screen[] = "\033[2J";
static const char ansi_claer_line[] = "\033[2K\r";
static const char ansi_clear_behind[] = "\033[K";
static const char ansi_cursor_save[] = "\033[s";
static const char ansi_cursor_restore[] = "\033[u";
static const char ansi_cursor_move_to[] = "\033[%d;%dH";

/** 其他字符串定义 **/
static const char lwcli_delete[] = "\b \b";
static const char lwcli_reminder[] = "Error: \"%s\" not registered.  Enter \"help\" to view a list of available commands.\r\n\r\n";

/** 辅助宏 **/
#define STR(x) #x
#define XSTR(x) STR(x)
#define LWCLI_COMMAND_STR_MAX_LENGTH_STR XSTR(LWCLI_COMMAND_STR_MAX_LENGTH)

#define lwcli_assert(x) do{             \
                            if(!(x)){   \
                                lwcli_printf("%d %s\r\n", __LINE__, #x);\
                                return; \
                            }\
                        }while(0)
                        
#define lwcli_assert_return(x, value)   do{             \
                                        if(!(x)){   \
                                            lwcli_printf("%d %s\r\n", __LINE__, #x);\
                                            return value; \
                                        }\
                                    }while(0)

/**
 * @brief lwcli 软件初始化
 * @note 初始化链表和历史记录缓冲区，注册默认命令
 */
void lwcli_software_init(void)
{
    /** 初始化头节点并绑定帮助命令 */
    lwcliObj.cmdListHead = (cmdList_t *)lwcli_malloc(sizeof(cmdList_t));
    if (lwcliObj.cmdListHead == NULL) {
        lwcli_printf("lwcli malloc error\r\n");
        return;
    }
    int command_fd = 0;
    command_fd = lwcli_regist_command("help", "list all commands", lwcli_help);
#if (LWCLI_PARAMETER_COMPLETION == true)
    lwcliObj.help_fd = command_fd;
#endif 
    lwcli_regist_command("clear", "clear screen", lwcli_clear);

    /** 初始化历史记录缓冲区 */
    #if (LWCLI_HISTORY_COMMAND_NUM > 0)
    lwcliObj.historyList.commandStrSize = LWCLI_RECEIVE_BUFFER_SIZE;
    lwcliObj.historyList.recordHistoryNum = LWCLI_HISTORY_COMMAND_NUM;
    lwcliObj.historyList.writeMirror = 0;
    lwcliObj.historyList.writePos = 0;
    lwcliObj.historyList.readMirror = 0;
    lwcliObj.historyList.readPos = 0;
    lwcliObj.historyList.findPos = 0;
    memset(lwcliObj.historyList.buffer, 0, LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE);
    #endif // LWCLI_HISTORY_COMMAND_NUM > 0

    lwcli_printf("%s\r\n"," ___                        ___           ");
    lwcli_printf("%s\r\n","/\\_ \\                      /\\_ \\    __    ");
    lwcli_printf("%s\r\n","\\//\\ \\    __  __  __    ___\\//\\ \\  /\\_\\   ");
    lwcli_printf("%s\r\n","  \\ \\ \\  /\\ \\/\\ \\/\\ \\  /'___\\\\ \\ \\ \\/\\ \\  ");
    lwcli_printf("%s\r\n","   \\_\\ \\_\\ \\ \\_/ \\_/ \\/\\ \\__/ \\_\\ \\_\\ \\ \\ ");
    lwcli_printf("%s\r\n","   /\\____\\\\ \\___x___/'\\ \\____\\/\\____\\\\ \\_\\");
    lwcli_printf("%s\r\n","   \\/____/ \\/__//__/   \\/____/\\/____/ \\/_/");
    lwcli_printf("lwcli version: "LWCLI_VERSION" Enter \"help\" to learn more infomation""\r\n");
    lwcli_printf("%s\r\n","                                          ");

#if (LWCLI_WITH_FILE_SYSTEM == true)
    lwcli_output_file_path();
#endif // LWCLI_WITH_FILE_SYSTEM == true
}

/**
 * @brief 注册命令
 * @param command 命令字符串
 * @param brief 帮助字符串
 * @param user_callback 用户回调函数
 * @return 命令描述符 command_fd
 */
int lwcli_regist_command(const char *command, const char *brief, user_callback_f user_callback)
{
    lwcli_assert_return(command != NULL, -1);
    lwcli_assert_return(brief != NULL, -1);
    lwcli_assert_return(user_callback != NULL, -1);
    if (strlen(command) > LWCLI_COMMAND_STR_MAX_LENGTH) {
        lwcli_printf("command string too long please modify LWCLI_COMMAND_STR_MAX_LENGTH \r\n");
        return -1;
    }
    if (strlen(brief) > LWCLI_BRIEF_MAX_LENGTH) {
        lwcli_printf("help string too long please modify LWCLI_BRIEF_MAX_LENGTH \r\n");
        return -1;
    }
    if (lwcliObj.cmdListHead == NULL) {
        lwcli_printf("please call lwcli_software_init before regist command \r\n");
        return -1;
    }
    cmdList_t *newNode = (cmdList_t *) lwcli_malloc(sizeof(cmdList_t));
    cmdList_t *pnode = lwcliObj.cmdListHead;
    if (newNode == NULL) {
        lwcli_printf("lwcli malloc error\r\n");
        return -1;
    }
    newNode->cmd_len = strlen(command);
    memcpy(newNode->command, command, newNode->cmd_len);
    newNode->command[newNode->cmd_len] = '\0';
    memcpy(newNode->brief, brief, strlen(brief));
    newNode->brief[strlen(brief)] = '\0';
    newNode->callback = user_callback;
    newNode->next = NULL;
    #if (LWCLI_PARAMETER_COMPLETION == true)
    newNode->parameter_head.next = NULL;
    #endif 
    while (pnode->next) {
        pnode = pnode->next;
    }
    pnode->next = newNode;
    lwcliObj.command_num++;
    #if (LWCLI_PARAMETER_COMPLETION == true)
    if (lwcliObj.help_fd) {
        char buffer[LWCLI_COMMAND_STR_MAX_LENGTH + 20] = {};
        snprintf(buffer, LWCLI_COMMAND_STR_MAX_LENGTH + 20, "get the detail of [%s]", command);
        lwcli_regist_command_parameter(lwcliObj.help_fd, command, buffer);
    }
    #endif
    return lwcliObj.command_num;
}

#if (LWCLI_PARAMETER_COMPLETION == true)
/**
 * @brief 注册命令参数 用于help 和 tab补全
 * @param command_fd 命令描述符 @brief lwcli_regist_command
 * @param parameter 参数 
 * @param description 详细的说明 可以为NULL
 */
void lwcli_regist_command_parameter(int command_fd, const char *parameter, const char *description)
{
    lwcli_assert(command_fd > 0);
    lwcli_assert(command_fd <= lwcliObj.command_num);
    lwcli_assert(parameter);
    cmdList_t *cmd_node = lwcliObj.cmdListHead;
    parameter_t *para_node = NULL, *new_para_node = NULL;

    for (int i = 0; i < command_fd; i++) {
        cmd_node = cmd_node->next;
    }
    para_node = &cmd_node->parameter_head;

    while (para_node->next) {
        para_node = para_node->next;
    }
    new_para_node = (parameter_t *)lwcli_malloc(sizeof(parameter_t));
    if (new_para_node == NULL) {
        lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
        return;
    }
    new_para_node->data = lwcli_parameter_malloc(strlen(parameter) + 1);
    if (new_para_node->data == NULL) {
        lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
        return;
    }
    if (description) {
        new_para_node->description = (char *)lwcli_malloc(strlen(description) + 1);
        if (new_para_node->description == NULL) {
            lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
            return;
        }
    }

    para_node->next = new_para_node;
    new_para_node->next = NULL;
    new_para_node->len = strlen(parameter);
    strcpy(new_para_node->data, parameter);
    if (description) {
        strcpy(new_para_node->description, description);
    }
}

/**
 * @brief 从参数内存池分配内存
 * @param size 字符串长度
 * @return 
 */
static char *lwcli_parameter_malloc(uint32_t size)
{
    char *ptr = NULL;
    uint32_t free_size = parameter_pool.size - parameter_pool.pos;
    if (free_size >= size){
        ptr = &parameter_pool.data[parameter_pool.pos];
        parameter_pool.pos += size;
    }
    return ptr;
}
#endif // (LWCLI_PARAMETER_COMPLETION == true)

/**
 * @brief 帮助命令
 * @param cmdParameter 命令参数
 */
static void lwcli_help(int argc, char* argv[])
{
    cmdList_t *node = lwcliObj.cmdListHead->next;
    uint16_t output_len = 0;
    memset(lwcliObj.ouputBuffer, 0, sizeof(lwcliObj.ouputBuffer));
    if (argc == 0){
        while(node) {
            if (node->brief[0] != '\0') {
                memcpy(lwcliObj.ouputBuffer, node->command, node->cmd_len);
                output_len = node->cmd_len;
                lwcliObj.ouputBuffer[output_len++] = ':';
                for (uint16_t i = 0; i < (LWCLI_COMMAND_STR_MAX_LENGTH + 3 - (node->cmd_len + 1) + 4); i++) {
                    lwcliObj.ouputBuffer[output_len++] = ' ';
                }
                uint16_t helpStrLen = strlen(node->brief);
                memcpy(lwcliObj.ouputBuffer + output_len, node->brief, helpStrLen);
                output_len += helpStrLen;
                lwcliObj.ouputBuffer[output_len++] = '\r';
                lwcliObj.ouputBuffer[output_len++] = '\n';
                lwcliObj.ouputBuffer[output_len++] = '\r';
                lwcliObj.ouputBuffer[output_len++] = '\n';
                lwcliObj.ouputBuffer[output_len++] = '\0';
                lwcli_output(lwcliObj.ouputBuffer, output_len);
            }
            node = node->next;
        }
    }
    else {
        uint16_t command_len = strlen(argv[0]);
        while(node){
            if (strncmp(node->command, argv[0], (command_len > node->cmd_len) ? node->cmd_len : command_len) == 0) {
                break;
            }
            node = node->next;
        }
        lwcli_printf("%s  %s\r\n", node->command, node->brief);
    #if (LWCLI_PARAMETER_COMPLETION == true)
        parameter_t *para_node = node->parameter_head.next;
        while (para_node)
        {
            if (para_node->description) {
                lwcli_printf("[%s]:   %s\r\n", para_node->data, para_node->description);
            }
            else {
                lwcli_printf("[%s]:   no description\r\n", para_node->data);
            }
            para_node = para_node->next;
        }
    #endif
    }
}

/**
 * @brief 清屏命令
 */
static void lwcli_clear(int argc, char* argv[])
{
    lwcli_output(ansi_clear_screen, sizeof(ansi_clear_screen));
    lwcli_printf(ansi_cursor_move_to, 0, 0);
}

/**
 * @brief printf 函数
 * @param format 
 * @param  
 */
static void lwcli_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    uint16_t print_len = vsnprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), format, args);
    va_end(args);
    lwcli_output(lwcliObj.ouputBuffer, print_len);
}

/**
 * @brief 接收处理字符
 * @param recv_char 接收到的字符
 */
void lwcli_process_receive_char(char recv_char)
{
    static uint8_t ansiKey = 0;
    if (recv_char == '\r' || recv_char == '\n') {
        lwcliObj.inputBuffer[lwcliObj.inputBufferPos] = '\0';
        lwcli_output("\r\n", 2);
        lwcli_process_command(lwcliObj.inputBuffer);
        #if (LWCLI_HISTORY_COMMAND_NUM > 0)
        if (lwcliObj.inputBufferPos > 0){
            lwcli_add_history_command();
        }
        #endif // LWCLI_HISTORY_COMMAND_NUM > 0
        memset(lwcliObj.inputBuffer, 0, sizeof(lwcliObj.inputBuffer));
        lwcliObj.inputBufferPos = 0;
        lwcliObj.cursorPos = 0;
    }
    else if ((recv_char == '\b' || recv_char == ansi_delete)) {
        if (lwcliObj.inputBufferPos == 0) {
            return;
        }
        if (lwcliObj.cursorPos == lwcliObj.inputBufferPos) {
            lwcliObj.inputBuffer[--lwcliObj.inputBufferPos] = '\0';
            lwcli_printf(lwcli_delete);
            lwcliObj.cursorPos = lwcliObj.inputBufferPos;
        }
        else if (lwcliObj.cursorPos > 0) {
            for (size_t i = lwcliObj.cursorPos - 1; i < lwcliObj.inputBufferPos; i++) // 字符串缓存处理，保证cmdStrBuffer中存储的字符串和屏幕一致
            {
                lwcliObj.inputBuffer[i] = lwcliObj.inputBuffer[i + 1];
            }
            lwcli_printf("%s%s%s%s%s", ansi_clear_behind, lwcli_delete, ansi_cursor_save, &lwcliObj.inputBuffer[lwcliObj.cursorPos - 1], ansi_cursor_restore);
            lwcliObj.inputBufferPos--;
            lwcliObj.cursorPos--;
        }
    }
    else if (recv_char == '\t') {
        lwcli_table_process();
    }
    else if (recv_char == '\033') {
        ansiKey = 1;
    }
    else {
        if (ansiKey == 0) {
            lwcli_assert(lwcliObj.inputBufferPos < sizeof(lwcliObj.inputBuffer) - 1);
            if (lwcliObj.cursorPos == lwcliObj.inputBufferPos) { // 普通字符且光标处于最后
                lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = recv_char;
                lwcliObj.cursorPos = lwcliObj.inputBufferPos;
                lwcli_output(&recv_char, 1);
            }
            else {   // 普通字符但光标不是在最后
                lwcli_printf("%c%s%s%s", recv_char, ansi_cursor_save, lwcliObj.inputBuffer + lwcliObj.cursorPos, ansi_cursor_restore);
                
                for (size_t i = lwcliObj.inputBufferPos; i > lwcliObj.cursorPos; i--) {// 字符串缓存处理，保证cmdStrBuffer中存储的字符串和屏幕一致
                    lwcliObj.inputBuffer[i] = lwcliObj.inputBuffer[i - 1];
                }
                lwcliObj.inputBuffer[lwcliObj.cursorPos] = recv_char;
                lwcliObj.inputBufferPos++;
                lwcliObj.cursorPos++;
            }
        }
        else if (ansiKey == 1) {
            ansiKey++;
        }
        else if (ansiKey == 2) {
            ansiKey = 0;
            if (recv_char == 'C') {
                if (lwcliObj.cursorPos < lwcliObj.inputBufferPos) {
                    lwcliObj.cursorPos++;
                    lwcli_output(ansi_cursor_right, sizeof(ansi_cursor_right));
                }
            }
            else if (recv_char == 'D') {
                if (lwcliObj.cursorPos > 0) {
                    lwcliObj.cursorPos--;
                    lwcli_output(ansi_cursor_left, sizeof(ansi_cursor_left));
                }
            }
            #if (LWCLI_HISTORY_COMMAND_NUM > 0)
            else if (recv_char == 'A') {
                lwcli_history_command_up();
            }
            else if (recv_char == 'B') {
                lwcli_history_command_down();
            }
            #endif // LWCLI_HISTORY_COMMAND_NUM > 0
        }
    }
}

/**
 * @brief 匹配命令
 * @param inputStr 输入字符串
 * @param expected_command 匹配命令
 * @return 1 匹配成功
 * @return 0 匹配失败
 */
static uint8_t lwcli_match_command(const char *inputStr, const char *expected_command)
{
    static char cmd_buffer[LWCLI_COMMAND_STR_MAX_LENGTH + 1] = {0};
    if (sscanf(inputStr, "%"LWCLI_COMMAND_STR_MAX_LENGTH_STR"s", cmd_buffer) != 1) {
        return 0;  // 输入为空
    }

    return strcmp(cmd_buffer, expected_command) == 0;
}

/**
 * @brief 命令处理
 * @param command 命令字符串
 */
static void lwcli_process_command(char *command)
{
    cmdList_t *node = lwcliObj.cmdListHead;
    char *cmdParameter = NULL;
    while (node) {
        if (lwcli_match_command(command, node->command)) {
            cmdParameter = command;
            // 寻找参数
            uint8_t parameter_num = lwcli_get_parameter_number(cmdParameter);
            if (parameter_num == 0) {
                node->callback(0, NULL);
                #if (LWCLI_WITH_FILE_SYSTEM == true)
                lwcli_output_file_path();
                #endif  // LWCLI_WITH_FILE_SYSTEM == true
                return;
            }
            else {
                char **parameterArray = (char **) lwcli_malloc(sizeof(char *) * parameter_num);
                if (parameterArray == NULL) {
                    lwcli_printf("error malloc\r\n");
                    return;
                }
                uint8_t findParameterNum = lwcli_find_parameters(cmdParameter + node->cmd_len, parameterArray, parameter_num);
                node->callback(findParameterNum, parameterArray);
                for (int i = 0; i < findParameterNum; ++i) {
                    lwcli_free(parameterArray[i]);
                }
                lwcli_free(parameterArray);
                #if (LWCLI_WITH_FILE_SYSTEM == true)
                lwcli_output_file_path();
                #endif // LWCLI_WITH_FILE_SYSTEM == true 
                return;
            }         
        }
        node = node->next;
    }

    lwcli_printf(lwcli_reminder, command);
#if (LWCLI_WITH_FILE_SYSTEM == true)
    lwcli_output_file_path();
#endif // LWCLI_WITH_FILE_SYSTEM == true
}

/**
 * @brief 寻找参数总数量
 * @note 支持识别双引号
 * @param command_string 命令字符串
 * @return 参数数量
 */
static uint8_t lwcli_get_parameter_number(const char *command_string)
{
    uint8_t len = strlen(command_string);
    bool in_quotes = false; // 是否处于引号中
    uint8_t parameter_num = 0, quotes_num = 0;
    for (uint8_t i = 0; i < len - 1; i++) {
        if (command_string[i] == ' ' && command_string[i + 1] != ' ' && !in_quotes) {
            parameter_num++;
        }
        else if (command_string[i] == '\"') {// 引号
            quotes_num++;
            in_quotes = !in_quotes;
        }
    }
    if (command_string[len - 1] == '\"') {
        quotes_num++;
    }
    parameter_num = (quotes_num % 2 == 0) ? parameter_num : 0; // 奇数个引号则参数无效
    return parameter_num;
}

/**
 * @brief 提取参数放入字符串数组
 * @param argv_str 整个参数字符串
 * @param parameter_arry 字符串数组，存放提取出的参数
 * @return 找到的参数个数
 */
static uint8_t lwcli_find_parameters(const char *argv_str, char **parameter_arry, uint8_t parameter_num)
{
    uint8_t len = strlen(argv_str);
    uint8_t found_num = 0;
    bool in_quotes = false; // 是否处于引号中

    uint8_t *index = (uint8_t*) lwcli_malloc(sizeof(uint8_t) * (parameter_num + 1));  // 增加一个元素用于最后一个参数
    if (index == NULL) {
        lwcli_printf("error malloc\r\n");
        return 0;
    }

    // 找到参数位置
    for (uint8_t i = 0; i < len - 1; i++) {
        if (argv_str[i] == ' ' && argv_str[i + 1] != ' ' && !in_quotes) {
            index[found_num++] = i + 1;
            if (found_num == parameter_num)
                break;
        }
        else if (argv_str[i] == '\"') {
            in_quotes = !in_quotes;
        }
    }

    // 提取处理参数
    for (uint8_t i = 0; i < found_num; i++) {
        uint8_t start = index[i];
        uint8_t end = 0;
        end = (i + 1 < found_num) ? index[i + 1] - 1: len;  // 确保最后一个参数处理正确
        uint8_t param_len = end - start;

        parameter_arry[i] = (char *)lwcli_malloc(param_len + 1);  // 分配内存
        if (parameter_arry[i] == NULL) { //错误处理
            lwcli_printf("error malloc\r\n");
            for (uint8_t j = 0; j < i; j++) {
                lwcli_free(parameter_arry[j]);
            }
            lwcli_free(index);
            return 0;
        }
        memcpy(parameter_arry[i], argv_str + start, param_len);
        parameter_arry[i][param_len] = '\0';  // 添加字符串结束符 避免乱码
    }

    lwcli_free(index);
    return found_num;
}

/**
 * @brief 计算最大补全长度
 * @param argc 匹配的命令数量
 * @param argv 匹配的命令
 * @return 最大补全长度
 */
int lwcli_longest_common_prefix_length(int argc, char *argv[])
{
    if (argc <= 0 || !argv) return 0;
    if (argc == 1) return argv[0] ? strlen(argv[0]) : 0;

    const char *base = argv[0];
    size_t len = 0;

    while (*base) { // 只要基准字符串没结束
        for (int i = 1; i < argc; i++) {
            if (!argv[i][len] || argv[i][len] != *base) {
                return (int)len;
            }
        }
        len++;
        base++;
    }

    // 走到这里说明 base 字符串是所有字符串的前缀
    return (int)len;
}

/**
 * @brief 补全命令
 */
static void lwcli_fix_command(void)
{
    const char *prefix = lwcliObj.inputBuffer;
    cmdList_t *node = lwcliObj.cmdListHead;
    uint16_t match_num = 0;
    if (!lwcliObj.inputBufferPos) {
        return;
    }
    while (node) {
        if (lwcliObj.inputBufferPos < node->cmd_len) {
            if (strncmp(node->command, prefix, lwcliObj.inputBufferPos) == 0) 
            {
                match_num++;
            }
        }
        node = node->next;
    }

    node = lwcliObj.cmdListHead;
    if (match_num == 0) {
        #if (LWCLI_WITH_FILE_SYSTEM == true)
        lwcli_output("\r\n", 2);
        lwcli_output_file_path();
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;
        lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        uint16_t output_len = snprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), "\r\n\r\n%s", lwcliObj.inputBuffer);
        lwcli_output(lwcliObj.ouputBuffer, output_len);
        #endif // LWCLI_WITH_FILE_SYSTEM == true
    }
    else if (match_num == 1) {
        while (node) {
            if (lwcliObj.inputBufferPos < node->cmd_len) {
                if (strncmp(node->command, prefix, lwcliObj.inputBufferPos) == 0) {
                    memcpy(lwcliObj.inputBuffer, node->command, node->cmd_len);
                    lwcliObj.inputBufferPos = node->cmd_len;
                    lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
                    lwcliObj.cursorPos = lwcliObj.inputBufferPos;
                    lwcli_output(ansi_claer_line, sizeof(ansi_claer_line));
                    #if (LWCLI_WITH_FILE_SYSTEM == true)
                    lwcli_output_file_path();
                    #endif // LWCLI_WITH_FILE_SYSTEM == true
                    lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
                }
            }
            node = node->next;
        }

    }
    else {
        char **match_arr = NULL;
        uint16_t match_index = 0;
        match_arr = (char **)lwcli_malloc(sizeof(char *)  * match_num);
        if (match_arr == NULL){
            return;
        }

        while (node) {
            if (strncmp(node->command, prefix, lwcliObj.inputBufferPos) == 0) {
                match_arr[match_index++] = node->command;
            }
            node = node->next;
        }

        uint16_t match_max_len = lwcli_longest_common_prefix_length(match_num, match_arr);
        lwcli_output("\r\n", 2);
        for (uint16_t i = 0; i < match_num; i++) {
            lwcli_output(match_arr[i], strlen(match_arr[i]));
            lwcli_output("    ", 4);
        }
        
        while (lwcliObj.inputBufferPos < match_max_len) {
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos] = match_arr[0][lwcliObj.inputBufferPos];
            lwcliObj.inputBufferPos++;
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;

        #if (LWCLI_WITH_FILE_SYSTEM == true)
        lwcli_output("\r\n", 2);
        lwcli_output_file_path();
        lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        uint16_t output_len = snprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), "\r\n\r\n%s", lwcliObj.inputBuffer);
        lwcli_output(lwcliObj.ouputBuffer, output_len);
        #endif // LWCLI_WITH_FILE_SYSTEM == true
        
    }
}

#if (LWCLI_PARAMETER_COMPLETION == true)
/**
 * @brief 补全参数
 * @param cmd_node 参数所属的命令
 */
static void lwcil_fix_parameter(cmdList_t *cmd_node)
{
    parameter_t *para_node = &cmd_node->parameter_head;
    if (para_node->next == NULL) {
        return;
    }
    const char *prefix = lwcliObj.inputBuffer + cmd_node->cmd_len + 1;
    int prefix_len = lwcliObj.inputBufferPos - cmd_node->cmd_len - 1;
    uint16_t match_num = 0;
    if (!lwcliObj.inputBufferPos) {
        return;
    }
    while (para_node && prefix_len > 0) {
        if (prefix_len < para_node->len) {
            if (strncmp(para_node->data, prefix, prefix_len) == 0) 
            {
                match_num++;
            }
        }
        para_node = para_node->next;
    }

    para_node = cmd_node->parameter_head.next;
    if (match_num == 0) {
        lwcli_output("\r\n", 2);
        while (para_node) {
            lwcli_printf("%s    ", para_node->data);
            para_node = para_node->next;
        }
        #if (LWCLI_WITH_FILE_SYSTEM == true)
        lwcli_output("\r\n", 2);
        lwcli_output_file_path();
        if (prefix_len < 0){
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;
        lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        lwcli_output("\r\n", 2);
        if (prefix_len < 0){
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;
        lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #endif // LWCLI_WITH_FILE_SYSTEM == true
    }
    else if (match_num == 1) {
        while (para_node) {
            if (prefix_len < para_node->len) {
                if (strncmp(para_node->data, prefix, prefix_len) == 0) {
                    memcpy(lwcliObj.inputBuffer + cmd_node->cmd_len + 1, para_node->data, para_node->len);
                    lwcliObj.inputBufferPos = para_node->len + cmd_node->cmd_len + 1;
                    lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
                    lwcliObj.cursorPos = lwcliObj.inputBufferPos;
                    lwcli_output(ansi_claer_line, sizeof(ansi_claer_line));
                    #if (LWCLI_WITH_FILE_SYSTEM == true)
                    lwcli_output_file_path();
                    #endif // LWCLI_WITH_FILE_SYSTEM == true
                    lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
                }
            }
            para_node = para_node->next;
        }

    }
    else {
        char **match_arr = NULL;
        uint16_t match_index = 0;
        match_arr = (char **)lwcli_malloc(sizeof(char *)  * match_num);
        if (match_arr == NULL){
            return;
        }

        while (para_node) {
            if (strncmp(para_node->data, prefix, prefix_len) == 0) {
                match_arr[match_index++] = para_node->data;
            }
            para_node = para_node->next;
        }

        uint16_t match_max_len = lwcli_longest_common_prefix_length(match_num, match_arr);
        lwcli_output("\r\n", 2);
        for (uint16_t i = 0; i < match_num; i++) {
            lwcli_output(match_arr[i], strlen(match_arr[i]));
            lwcli_output("    ", 4);
        }
        
        while (prefix_len < match_max_len) {
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = match_arr[0][prefix_len++];
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;

        #if (LWCLI_WITH_FILE_SYSTEM == true)
        lwcli_output("\r\n", 2);
        lwcli_output_file_path();
        lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        uint16_t output_len = snprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), "\r\n\r\n%s", lwcliObj.inputBuffer);
        lwcli_output(lwcliObj.ouputBuffer, output_len);
        #endif // LWCLI_WITH_FILE_SYSTEM == true
        
    }
}
#endif // LWCLI_PARAMETER_COMPLETION == true

/**
 * @brief tab 补全处理
 */
static void lwcli_table_process(void)
{
    cmdList_t *node = lwcliObj.cmdListHead;
    bool compelter_par = false;
    if (!lwcliObj.inputBufferPos) {
        return;
    }
#if (LWCLI_PARAMETER_COMPLETION == true)
    while (node)
    {
        if(lwcli_match_command(lwcliObj.inputBuffer, node->command)){
            compelter_par = true;
            break;
        }
        node = node->next;
    }
    if (compelter_par) {
        lwcil_fix_parameter(node);
    }
    else {
        lwcli_fix_command();
    }
#else
    lwcli_fix_command();
#endif 
}

#if (LWCLI_HISTORY_COMMAND_NUM > 0)
#define HISTORY_UPDATE_READ_INDEX()      do {lwcliObj.historyList.readPos = (lwcliObj.historyList.readPos + 1) % lwcliObj.historyList.recordHistoryNum; if (!lwcliObj.historyList.readPos) lwcliObj.historyList.readMirror ^= 1;} while(0)
#define HISTORY_UPDATE_WRITE_INDEX()     do {lwcliObj.historyList.writePos = (lwcliObj.historyList.writePos + 1) % lwcliObj.historyList.recordHistoryNum; if (!lwcliObj.historyList.writePos) lwcliObj.historyList.writeMirror ^= 1;} while(0)


/**
 * @brief 判断历史命令列表是否为满
 * @return true:为空 false:不为空
 */
static bool lwcli_history_is_full()
{
    return ((lwcliObj.historyList.writePos == lwcliObj.historyList.readPos) && (lwcliObj.historyList.writeMirror != lwcliObj.historyList.readMirror));
}

/**
 * @brief 判断历史命令列表是否为空
 * @return true:为空 false:不为空
 */
static bool lwcli_history_is_empty()
{
    return ((lwcliObj.historyList.writePos == lwcliObj.historyList.readPos) && (lwcliObj.historyList.writeMirror == lwcliObj.historyList.readMirror));
}

/**
 * @brief 添加一条历史命令
 */
static void lwcli_add_history_command(void)
{
    if (lwcli_history_is_full()) {
        HISTORY_UPDATE_READ_INDEX();
    }
    memcpy(lwcliObj.historyList.buffer + (lwcliObj.historyList.writePos * lwcliObj.historyList.commandStrSize), lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
    lwcliObj.historyList.buffer[lwcliObj.historyList.writePos * lwcliObj.historyList.commandStrSize + lwcliObj.inputBufferPos] = '\0';
    HISTORY_UPDATE_WRITE_INDEX();
    lwcliObj.historyList.findPos = lwcliObj.historyList.writePos;

    if (lwcli_history_is_full()) {
        uint16_t readPosTemp = lwcliObj.historyList.readPos;
        for (uint16_t i = 0; i < LWCLI_HISTORY_COMMAND_NUM; i++) {
            lwcliObj.historyList.findPosIndex[i] = readPosTemp;
            readPosTemp = (readPosTemp + 1) % lwcliObj.historyList.recordHistoryNum;
        }
        lwcliObj.historyList.findPos = LWCLI_HISTORY_COMMAND_NUM;
    }
    else {
        for (uint16_t i = 0; i < lwcliObj.historyList.writePos; i++) {
            lwcliObj.historyList.findPosIndex[i] = i;
        }
        lwcliObj.historyList.findPos = lwcliObj.historyList.writePos;
    }
}



/**
 * @brief 读取上一条历史命令
 * @param command 读取到的命令字符串
 */
static void lwcli_history_command_up(void)
{
    uint16_t historyCmdPos = lwcliObj.historyList.findPosIndex[0]; 

    if (lwcli_history_is_empty()) {
        return;
    }

    if (lwcliObj.historyList.findPos > 0) {
        lwcliObj.historyList.findPos --;
        historyCmdPos = lwcliObj.historyList.findPosIndex[lwcliObj.historyList.findPos];
    }

    uint8_t historyCommandLen = 0;
    char cmdChar = lwcliObj.historyList.buffer[historyCmdPos * lwcliObj.historyList.commandStrSize];
    while (cmdChar != '\0' && historyCommandLen < lwcliObj.historyList.commandStrSize) {
        historyCommandLen++;
        cmdChar = lwcliObj.historyList.buffer[historyCmdPos * lwcliObj.historyList.commandStrSize + historyCommandLen];
    }
    memcpy(lwcliObj.inputBuffer, lwcliObj.historyList.buffer + (historyCmdPos * lwcliObj.historyList.commandStrSize), lwcliObj.historyList.commandStrSize);
    lwcliObj.inputBuffer[historyCommandLen] = '\0';
    lwcliObj.inputBufferPos = historyCommandLen;

    lwcli_output(ansi_claer_line, sizeof(ansi_claer_line));
    #if (LWCLI_WITH_FILE_SYSTEM == true)
    lwcli_output_file_path();
    #endif // LWCLI_WITH_FILE_SYSTEM == true
    lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
    lwcliObj.cursorPos = lwcliObj.inputBufferPos;
}

/**
 * @brief 获取历史命令的下一个
 */
static void lwcli_history_command_down(void)
{
    uint16_t historyCmdPos = lwcliObj.historyList.findPosIndex[0]; 

    if (lwcli_history_is_empty()) {
        return;
    }

    if (lwcli_history_is_full()) {
        if (lwcliObj.historyList.findPos >= LWCLI_HISTORY_COMMAND_NUM - 1) { // 还没有使用UP键，则不允许使用Down 或者 Down到最后一个了
            lwcli_output(ansi_claer_line, sizeof(ansi_claer_line));
            #if (LWCLI_WITH_FILE_SYSTEM == true)
            lwcli_output_file_path();
            #endif // LWCLI_WITH_FILE_SYSTEM == true
            lwcliObj.inputBuffer[0] = '\0';
            lwcliObj.inputBufferPos = 0;
            lwcliObj.cursorPos = 0;
            return;
        }
        else {
            if (lwcliObj.historyList.findPos < LWCLI_HISTORY_COMMAND_NUM - 1) {
                lwcliObj.historyList.findPos++;
                historyCmdPos = lwcliObj.historyList.findPosIndex[lwcliObj.historyList.findPos];
            }
        }
    }
    else {
        if (lwcliObj.historyList.findPos >= lwcliObj.historyList.writePos - 1){  // 还没有使用UP键，则不允许使用Down 或者 Down到最后一个了
            lwcli_output(ansi_claer_line, sizeof(ansi_claer_line));
            #if (LWCLI_WITH_FILE_SYSTEM == true)
            lwcli_output_file_path();
            #endif // LWCLI_WITH_FILE_SYSTEM == true
            lwcliObj.inputBuffer[0] = '\0';
            lwcliObj.inputBufferPos = 0;
            lwcliObj.cursorPos = 0;
            return;
        }
        else {
            if (lwcliObj.historyList.findPos < lwcliObj.historyList.writePos) {
                lwcliObj.historyList.findPos++;
            }
            historyCmdPos = lwcliObj.historyList.findPosIndex[lwcliObj.historyList.findPos];
        }
    }
    uint8_t historyCommandLen = 0;
    char cmdChar = lwcliObj.historyList.buffer[historyCmdPos * lwcliObj.historyList.commandStrSize];
    while (cmdChar != '\0' && historyCommandLen < lwcliObj.historyList.commandStrSize) {
        historyCommandLen++;
        cmdChar = lwcliObj.historyList.buffer[historyCmdPos * lwcliObj.historyList.commandStrSize + historyCommandLen];
    }
    memcpy(lwcliObj.inputBuffer, lwcliObj.historyList.buffer + (historyCmdPos * lwcliObj.historyList.commandStrSize), lwcliObj.historyList.commandStrSize);
    lwcliObj.inputBuffer[historyCommandLen] = '\0';
    lwcliObj.inputBufferPos = historyCommandLen;

    lwcli_output(ansi_claer_line, sizeof(ansi_claer_line));
    #if (LWCLI_WITH_FILE_SYSTEM == true)
    lwcli_output_file_path();
    #endif // LWCLI_WITH_FILE_SYSTEM == true
    lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
    lwcliObj.cursorPos = lwcliObj.inputBufferPos;

}

#endif // LWCLI_HISTORY_COMMAND_NUM > 0

#if (LWCLI_WITH_FILE_SYSTEM == true)

/**
 * @brief 通过ANSI编码输出带颜色的字符串
 * @param str 字符串
 * @param color 颜色
 */
static void lwcli_output_string_withcolor(const char *str, colorEnum_e color)
{
    lwcli_printf("%s%s%s", colorTable[color], str, LWCLI_ANSI_COLOR_RESET);
}

/**
 * @brief 输出当前路径
 */
static void lwcli_output_file_path(void)
{
    char *filePath = lwcli_get_file_path();
    lwcli_output_string_withcolor(LWCLI_USER_NAME":", COLOR_GREEN);
    lwcli_output_string_withcolor(filePath, COLOR_BLUE);
    lwcli_output("$ ", 2);
}
#endif // LWCLI_WITH_FILE_SYSTEM == true
