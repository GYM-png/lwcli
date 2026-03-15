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
#include "lwcli_list.h"
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
    list_node_t node;
} parameter_t;

typedef struct
{
    char data[LWCLI_PARAMETER_BUFFER_POOL_SIZE];
    uint32_t size;
    uint32_t pos;
}pool_manage_t;
static pool_manage_t parameter_pool = {.size = LWCLI_PARAMETER_BUFFER_POOL_SIZE, .pos = 0};
#endif // LWCLI_PARAMETER_COMPLETION == true

typedef struct command
{
    char command[LWCLI_COMMAND_STR_MAX_LENGTH];
    char brief[LWCLI_BRIEF_MAX_LENGTH];

#if (LWCLI_PARAMETER_COMPLETION == true)
    parameter_t para;   /* 参数链表头，仅使用 para.node，data/len/description 未使用 */
#endif

    uint8_t cmd_len;
    user_callback_f callback;
    list_node_t node;
} command_t;

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
    command_t *command;
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
static void lwcil_fix_parameter(command_t *cmd);
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
static const char ansi_clear_line[] = "\033[2K\r";
static const char ansi_clear_behind[] = "\033[K";
static const char ansi_cursor_save[] = "\033[s";
static const char ansi_cursor_restore[] = "\033[u";
static const char ansi_cursor_move_to[] = "\033[%d;%dH";

/** 其他字符串定义 **/
static const char lwcli_delete[] = "\b \b";
static const char lwcli_reminder[] = "Error: \"%s\" not registered.  Enter \"help\" to view a list of available commands.\r\n\r\n";


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
    /** 初始化命令链表头节点 */
    lwcliObj.command = (command_t *)lwcli_malloc(sizeof(command_t));
    if (lwcliObj.command == NULL) {
        lwcli_printf("lwcli malloc error\r\n");
        return;
    }
    list_head_init(&lwcliObj.command->node);
#if (LWCLI_PARAMETER_COMPLETION == true)
    list_head_init(&lwcliObj.command->para.node);
#endif
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
    if (lwcliObj.command == NULL) {
        lwcli_printf("please call lwcli_software_init before regist command \r\n");
        return -1;
    }
    command_t *new_cmd = (command_t *)lwcli_malloc(sizeof(command_t));
    if (new_cmd == NULL) {
        lwcli_printf("lwcli malloc error\r\n");
        return -1;
    }
    new_cmd->cmd_len = strlen(command);
    memcpy(new_cmd->command, command, new_cmd->cmd_len);
    new_cmd->command[new_cmd->cmd_len] = '\0';
    memcpy(new_cmd->brief, brief, strlen(brief));
    new_cmd->brief[strlen(brief)] = '\0';
    new_cmd->callback = user_callback;
#if (LWCLI_PARAMETER_COMPLETION == true)
    list_head_init(&new_cmd->para.node);
#endif
    list_node_init(&new_cmd->node);
    list_add_tail(&lwcliObj.command->node, &new_cmd->node);
    lwcliObj.command_num++;
    #if (LWCLI_PARAMETER_COMPLETION == true)
    if (lwcliObj.help_fd) {
        char buffer[LWCLI_COMMAND_STR_MAX_LENGTH + 30] = {0};
        snprintf(buffer, sizeof(buffer), "get the detail of [%s]", command);
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
    command_t *cmd = NULL;
    parameter_t *new_param = NULL;
    int i = 0;

    list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
        if (++i == command_fd) break;
    }
    if (i != command_fd) return;

    new_param = (parameter_t *)lwcli_malloc(sizeof(parameter_t));
    if (new_param == NULL) {
        lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
        return;
    }
    new_param->data = lwcli_parameter_malloc(strlen(parameter) + 1);
    if (new_param->data == NULL) {
        lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
        lwcli_free(new_param);
        return;
    }
    if (description) {
        new_param->description = (char *)lwcli_malloc(strlen(description) + 1);
        if (new_param->description == NULL) {
            lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
            lwcli_free(new_param);
            return;
        }
    }

    new_param->len = strlen(parameter);
    strcpy(new_param->data, parameter);
    if (description) {
        strcpy(new_param->description, description);
    }
    list_node_init(&new_param->node);
    list_add_tail(&cmd->para.node, &new_param->node);
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
    command_t *cmd = NULL;
    uint16_t output_len = 0;
    memset(lwcliObj.ouputBuffer, 0, sizeof(lwcliObj.ouputBuffer));
    if (argc == 0){
        list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
            if (cmd->brief[0] != '\0') {
                memcpy(lwcliObj.ouputBuffer, cmd->command, cmd->cmd_len);
                output_len = cmd->cmd_len;
                lwcliObj.ouputBuffer[output_len++] = ':';
                for (uint16_t i = 0; i < (LWCLI_COMMAND_STR_MAX_LENGTH + 3 - (cmd->cmd_len + 1) + 4); i++) {
                    lwcliObj.ouputBuffer[output_len++] = ' ';
                }
                uint16_t helpStrLen = strlen(cmd->brief);
                memcpy(lwcliObj.ouputBuffer + output_len, cmd->brief, helpStrLen);
                output_len += helpStrLen;
                lwcliObj.ouputBuffer[output_len++] = '\r';
                lwcliObj.ouputBuffer[output_len++] = '\n';
                lwcliObj.ouputBuffer[output_len++] = '\r';
                lwcliObj.ouputBuffer[output_len++] = '\n';
                lwcliObj.ouputBuffer[output_len++] = '\0';
                lwcli_output(lwcliObj.ouputBuffer, output_len);
            }
        }
    }
    else {
        uint16_t command_len = strlen(argv[0]);
        cmd = NULL;
        int found = 0;
        list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
            uint16_t cmp_len = (command_len > cmd->cmd_len) ? cmd->cmd_len : command_len;
            if (cmp_len > 0 && memcmp(cmd->command, argv[0], cmp_len) == 0) {
                found = 1;
                break;
            }
        }
        if (!found || cmd == NULL) {
            lwcli_printf("Error: \"%s\" not found. Enter \"help\" to view available commands.\r\n", argv[0]);
            return;
        }
        lwcli_printf("%s  %s\r\n", cmd->command, cmd->brief);
#if (LWCLI_PARAMETER_COMPLETION == true)
        {
            parameter_t *param = NULL;
            list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
                if (param->description) {
                    lwcli_printf("[%s]:   %s\r\n", param->data, param->description);
                } else {
                    lwcli_printf("[%s]:   no description\r\n", param->data);
                }
            }
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
    int ret = vsnprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), format, args);
    va_end(args);
    uint16_t print_len = (ret < 0 || (size_t)ret >= sizeof(lwcliObj.ouputBuffer))
                        ? (uint16_t)(sizeof(lwcliObj.ouputBuffer) - 1) : (uint16_t)ret;
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
 * @brief 匹配命令（高效实现：手动解析 + 长度预检 + memcmp）
 * @param inputStr 输入字符串
 * @param expected_command 期望的命令字符串
 * @param expected_len 期望命令长度（避免 strlen）
 * @return 1 匹配成功
 * @return 0 匹配失败
 */
static uint8_t lwcli_match_command(const char *inputStr, const char *expected_command, uint8_t expected_len)
{
    uint8_t input_len = 0;
    while (inputStr[input_len] && inputStr[input_len] != ' ' && input_len < LWCLI_COMMAND_STR_MAX_LENGTH) {
        input_len++;
    }
    if (input_len == 0) {
        return 0;
    }
    if (input_len != expected_len) {
        return 0;
    }
    return memcmp(inputStr, expected_command, expected_len) == 0;
}

/**
 * @brief 命令处理
 * @param command 命令字符串
 */
static void lwcli_process_command(char *command)
{
    command_t *cmd = NULL;
    char *cmdParameter = NULL;
    list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
        if (lwcli_match_command(command, cmd->command, cmd->cmd_len)) {
            cmdParameter = command;
            // 寻找参数
            uint8_t parameter_num = lwcli_get_parameter_number(cmdParameter);
            if (parameter_num == 0) {
                cmd->callback(0, NULL);
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
                uint8_t findParameterNum = lwcli_find_parameters(cmdParameter + cmd->cmd_len, parameterArray, parameter_num);
                cmd->callback(findParameterNum, parameterArray);
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
    if (len <= 1) {
        return 0;
    }
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
    if (len <= 1) {
        return 0;
    }
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
    command_t *cmd = NULL;
    uint16_t match_num = 0;
    if (!lwcliObj.inputBufferPos) {
        return;
    }
    list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
        if (lwcliObj.inputBufferPos < cmd->cmd_len) {
            if (memcmp(cmd->command, prefix, lwcliObj.inputBufferPos) == 0) {
                match_num++;
            }
        }
    }

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
        list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
            if (lwcliObj.inputBufferPos < cmd->cmd_len) {
                if (memcmp(cmd->command, prefix, lwcliObj.inputBufferPos) == 0) {
                    memcpy(lwcliObj.inputBuffer, cmd->command, cmd->cmd_len);
                    lwcliObj.inputBufferPos = cmd->cmd_len;
                    lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
                    lwcliObj.cursorPos = lwcliObj.inputBufferPos;
                    lwcli_output(ansi_clear_line, sizeof(ansi_clear_line));
#if (LWCLI_WITH_FILE_SYSTEM == true)
                    lwcli_output_file_path();
#endif
                    lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
                    break;
                }
            }
        }
    }
    else {
        char **match_arr = NULL;
        uint16_t match_index = 0;
        match_arr = (char **)lwcli_malloc(sizeof(char *) * match_num);
        if (match_arr == NULL) {
            return;
        }

        list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
            if (memcmp(cmd->command, prefix, lwcliObj.inputBufferPos) == 0) {
                match_arr[match_index++] = cmd->command;
            }
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

        lwcli_free(match_arr);
    }
}

#if (LWCLI_PARAMETER_COMPLETION == true)
/**
 * @brief 补全参数
 * @param cmd 参数所属的命令
 */
static void lwcil_fix_parameter(command_t *cmd)
{
    if (list_empty(&cmd->para.node)) {
        return;
    }
    const char *prefix = lwcliObj.inputBuffer + cmd->cmd_len + 1;
    int prefix_len = lwcliObj.inputBufferPos - cmd->cmd_len - 1;
    uint16_t match_num = 0;
    parameter_t *param = NULL;
    if (!lwcliObj.inputBufferPos) {
        return;
    }
    list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
        if (prefix_len > 0 && prefix_len < param->len) {
            if (memcmp(param->data, prefix, (size_t)prefix_len) == 0) {
                match_num++;
            }
        }
    }

    if (match_num == 0) {
        lwcli_output("\r\n", 2);
        list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
            lwcli_printf("%s    ", param->data);
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
        list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
            if (prefix_len < param->len) {
                if (memcmp(param->data, prefix, (size_t)prefix_len) == 0) {
                    memcpy(lwcliObj.inputBuffer + cmd->cmd_len + 1, param->data, param->len);
                    lwcliObj.inputBufferPos = param->len + cmd->cmd_len + 1;
                    lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
                    lwcliObj.cursorPos = lwcliObj.inputBufferPos;
                    lwcli_output(ansi_clear_line, sizeof(ansi_clear_line));
#if (LWCLI_WITH_FILE_SYSTEM == true)
                    lwcli_output_file_path();
#endif
                    lwcli_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
                    break;
                }
            }
        }
    }
    else {
        char **match_arr = NULL;
        uint16_t match_index = 0;
        match_arr = (char **)lwcli_malloc(sizeof(char *) * match_num);
        if (match_arr == NULL) {
            return;
        }

        list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
            if (memcmp(param->data, prefix, (size_t)prefix_len) == 0) {
                match_arr[match_index++] = param->data;
            }
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
        #endif // LWCLI_PARAMETER_COMPLETION == true

        lwcli_free(match_arr);
    }
}
#endif // LWCLI_PARAMETER_COMPLETION == true

/**
 * @brief tab 补全处理
 */
static void lwcli_table_process(void)
{
    if (lwcliObj.command == NULL) {
        return;
    }
    command_t *cmd = NULL;
    bool compelter_par = false;
    if (!lwcliObj.inputBufferPos) {
        return;
    }
#if (LWCLI_PARAMETER_COMPLETION == true)
    list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
        if (lwcli_match_command(lwcliObj.inputBuffer, cmd->command, cmd->cmd_len)) {
            compelter_par = true;
            break;
        }
    }
    if (compelter_par) {
        lwcil_fix_parameter(cmd);
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

    lwcli_output(ansi_clear_line, sizeof(ansi_clear_line));
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
            lwcli_output(ansi_clear_line, sizeof(ansi_clear_line));
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
        if (lwcliObj.historyList.findPos >= lwcliObj.historyList.writePos){  // 处于当前输入位置，Down 应清空
            lwcli_output(ansi_clear_line, sizeof(ansi_clear_line));
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

    lwcli_output(ansi_clear_line, sizeof(ansi_clear_line));
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
