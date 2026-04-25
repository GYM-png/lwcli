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

#define LWCLI_MEMPOOL_DEFINE(name, buffer_size) \
    typedef struct { \
        char data[buffer_size]; \
        uint32_t size; \
        uint32_t pos; \
    } name##_pool_t; \
    static name##_pool_t name##_pool = {.size = buffer_size, .pos = 0};

#define LWCLI_POOL_ALLOC(pool_name, size) \
    (lwcli_pool_alloc(&(pool_name##_pool.data[0]), &(pool_name##_pool.size), &(pool_name##_pool.pos), (size)))
#define LWCLI_POOL_FREE(pool_name) ((pool_name##_pool.pos) = 0)

static inline char *lwcli_pool_alloc(char *data, uint32_t *pool_size, uint32_t *pool_pos, uint32_t size)
{
    char *ptr = NULL;
    if ((*pool_size - *pool_pos) >= size) {
        ptr = data + *pool_pos;
        *pool_pos += size;
    }
    return ptr;
}

LWCLI_MEMPOOL_DEFINE(dynamic, LWCLI_DYNAMIC_POOL_SIZE);

#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
typedef struct parameter_list
{
    char *data;
    uint8_t len;
    char *description;
    list_node_t node;
} parameter_t;

LWCLI_MEMPOOL_DEFINE(parameter, LWCLI_STATIC_POOL_SIZE);
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE

typedef struct command
{
    char command[LWCLI_COMMAND_STR_MAX_LENGTH];
    char brief[LWCLI_BRIEF_MAX_LENGTH];

#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
    parameter_t para;   /* 参数链表头，仅使用 para.node，data/len/description 未使用 */
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE

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

#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
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

#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE

typedef struct 
{
    const lwcli_opt_t *opt;   /**< 用户注入的接口（由 lwcli_hardware_init 注册）*/
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

#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
    int help_fd;
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE
}lwcli_t;

static lwcli_t lwcliObj = {0};

/** 静态函数声明 **/
#if (LWCLI_PARAMETER_SPLIT == LWCLI_TRUE)
static void lwcli_help(int argc, char *argv[]);
static void lwcli_clear(int argc, char *argv[]);
#else
static void lwcli_help(char *argvs);
static void lwcli_clear(char *argvs);
#endif  // LWCLI_PARAMETER_SPLIT == LWCLI_TRUE
static void lwcli_process_command(char *command);
#if (LWCLI_PARAMETER_SPLIT == LWCLI_TRUE)
static uint8_t lwcli_find_parameters(const char *argv_str, char **parameter_arry, uint8_t parameter_num);
static uint8_t lwcli_get_parameter_number(const char *command_string);
#endif  // LWCLI_PARAMETER_SPLIT == LWCLI_TRUE
static void lwcli_printf(const char *format, ...);
static void lwcli_table_process(void);
static void lwcli_fix_command(void);

#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
static void lwcli_fix_parameter(command_t *cmd);
static void lwcli_get_current_parameter_prefix(command_t *cmd, const char **prefix, int *prefix_len, uint16_t *prefix_start_pos);
static char *lwcli_parameter_malloc(uint32_t size);
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE

#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
static void lwcli_output_string_withcolor(const char *str, colorEnum_e color);
static void lwcli_output_file_path(void);
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE

#if (LWCLI_HISTORY_COMMAND_NUM > 0)
static void lwcli_add_history_command(void);
static void lwcli_history_command_down(void);
static void lwcli_history_command_up(void);
#endif  // LWCLI_HISTORY_COMMAND_NUM > 0

/** 通过 opt 调用的接口宏 **/
#define lwcli_opt_malloc(s)      (lwcliObj.opt->malloc(s))
#define lwcli_opt_free(p)         (lwcliObj.opt->free(p))
#define lwcli_opt_output(s, l)   (lwcliObj.opt->output(s, l))



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
 * @brief 硬件初始化，注册用户接口
 */
void lwcli_hardware_init(const lwcli_opt_t *opt)
{
    if (opt == NULL || opt->malloc == NULL || opt->free == NULL || opt->output == NULL) {
        return;
    }
    lwcliObj.opt = opt;
    if (opt->hardware_init != NULL) {
        opt->hardware_init();
    }
}

/**
 * @brief lwcli 软件初始化
 * @note 初始化链表和历史记录缓冲区，注册默认命令
 */
void lwcli_software_init(void)
{
    if (lwcliObj.opt == NULL) {
        return;  /* 需先调用 lwcli_hardware_init(opt) */
    }
    /** 初始化命令链表头节点 */
    lwcliObj.command = (command_t *)lwcli_opt_malloc(sizeof(command_t));
    if (lwcliObj.command == NULL) {
        lwcli_printf("lwcli malloc error\r\n");
        return;
    }
    list_head_init(&lwcliObj.command->node);
#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
    list_head_init(&lwcliObj.command->para.node);
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE
    int command_fd = 0;
    command_fd = lwcli_regist_command("help", "list all commands", lwcli_help);
#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
    lwcliObj.help_fd = command_fd;
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE
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
    #endif  // LWCLI_HISTORY_COMMAND_NUM > 0

    lwcli_printf("%s\r\n"," ___                        ___           ");
    lwcli_printf("%s\r\n","/\\_ \\                      /\\_ \\    __    ");
    lwcli_printf("%s\r\n","\\//\\ \\    __  __  __    ___\\//\\ \\  /\\_\\   ");
    lwcli_printf("%s\r\n","  \\ \\ \\  /\\ \\/\\ \\/\\ \\  /'___\\\\ \\ \\ \\/\\ \\  ");
    lwcli_printf("%s\r\n","   \\_\\ \\_\\ \\ \\_/ \\_/ \\/\\ \\__/ \\_\\ \\_\\ \\ \\ ");
    lwcli_printf("%s\r\n","   /\\____\\\\ \\___x___/'\\ \\____\\/\\____\\\\ \\_\\");
    lwcli_printf("%s\r\n","   \\/____/ \\/__//__/   \\/____/\\/____/ \\/_/");
    lwcli_printf("lwcli version: "LWCLI_VERSION" Enter \"help\" to learn more infomation""\r\n");
    lwcli_printf("%s\r\n","                                          ");

#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
    lwcli_output_file_path();
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
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
    command_t *new_cmd = (command_t *)lwcli_opt_malloc(sizeof(command_t));
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
#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
    list_head_init(&new_cmd->para.node);
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE
    list_node_init(&new_cmd->node);
    list_add_tail(&lwcliObj.command->node, &new_cmd->node);
    lwcliObj.command_num++;
    #if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
    if (lwcliObj.help_fd) {
        char buffer[LWCLI_COMMAND_STR_MAX_LENGTH + 30] = {0};
        snprintf(buffer, sizeof(buffer), "get the detail of [%s]", command);
        lwcli_regist_command_parameter(lwcliObj.help_fd, command, buffer);
    }
    #endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE
    return lwcliObj.command_num;
}

#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
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
        i++;
        if (i == command_fd) break;
    }
    if (i != command_fd) return;

    new_param = (parameter_t *)lwcli_opt_malloc(sizeof(parameter_t));
    if (new_param == NULL) {
        lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
        return;
    }
    uint32_t param_data_len = strlen(parameter) + 1;
    new_param->data = lwcli_parameter_malloc(param_data_len);
    if (new_param->data == NULL) {
        lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
        lwcli_opt_free(new_param);
        return;
    }
    if (description) {
        new_param->description = (char *)lwcli_opt_malloc(strlen(description) + 1);
        if (new_param->description == NULL) {
            lwcli_printf("%s %d ,malloc error ", __FILE__, __LINE__);
            parameter_pool.pos -= param_data_len;  /* 回滚 parameter 池 */
            lwcli_opt_free(new_param);
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
 * @brief 从参数注册内存池分配（用于 lwcli_regist_command_parameter）
 * @param size 字符串长度
 * @return 指针，失败返回 NULL
 */
static char *lwcli_parameter_malloc(uint32_t size)
{
    return LWCLI_POOL_ALLOC(parameter, size);
}
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE

/**
 * @brief 从 dynamic 内存池分配（用于 Tab 补全、参数分割等运行时临时分配）
 * @param size 分配长度
 * @return 指针，失败返回 NULL
 */
static char *lwcli_dynamic_malloc(uint32_t size)
{
    return LWCLI_POOL_ALLOC(dynamic, size);
}

/**
 * @brief 释放 dynamic 内存池（使用完毕后调用，重置池供下次使用）
 */
static void lwcli_dynamic_free(void)
{
    dynamic_pool.pos = 0;
}

/**
 * @brief 帮助命令
 */
#if (LWCLI_PARAMETER_SPLIT == LWCLI_TRUE)
static void lwcli_help(int argc, char *argv[])
{
    command_t *cmd = NULL;
    uint16_t output_len = 0;
    memset(lwcliObj.ouputBuffer, 0, sizeof(lwcliObj.ouputBuffer));
    if (argc == 0) {
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
                lwcli_opt_output(lwcliObj.ouputBuffer, output_len);
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
#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
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
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE
    }
}
#else
static void lwcli_help(char *argvs)
{
    command_t *cmd = NULL;
    uint16_t output_len = 0;
    const char *search = argvs;
    while (*search == ' ') search++;

    memset(lwcliObj.ouputBuffer, 0, sizeof(lwcliObj.ouputBuffer));
    if (*search == '\0') {
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
                lwcli_opt_output(lwcliObj.ouputBuffer, output_len);
            }
        }
    } else {
        const char *p = search;
        while (*p && *p != ' ') p++;
        uint16_t search_len = (uint16_t)(p - search);
        int found = 0;
        list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
            uint16_t cmp_len = (search_len > cmd->cmd_len) ? cmd->cmd_len : search_len;
            if (cmp_len > 0 && memcmp(cmd->command, search, cmp_len) == 0) {
                found = 1;
                break;
            }
        }
        if (!found || cmd == NULL) {
            lwcli_printf("Error: \"%s\" not found. Enter \"help\" to view available commands.\r\n", search);
            return;
        }
        lwcli_printf("%s  %s\r\n", cmd->command, cmd->brief);
    }
}
#endif  // LWCLI_PARAMETER_SPLIT == LWCLI_TRUE

/**
 * @brief 清屏命令
 */
#if (LWCLI_PARAMETER_SPLIT == LWCLI_TRUE)
static void lwcli_clear(int argc, char *argv[])
#else
static void lwcli_clear(char *argvs)
#endif  // LWCLI_PARAMETER_SPLIT == LWCLI_TRUE
{
    lwcli_opt_output(ansi_clear_screen, sizeof(ansi_clear_screen));
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
    lwcli_opt_output(lwcliObj.ouputBuffer, print_len);
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
        lwcli_opt_output("\r\n", 2);
        lwcli_process_command(lwcliObj.inputBuffer);
        #if (LWCLI_HISTORY_COMMAND_NUM > 0)
        if (lwcliObj.inputBufferPos > 0){
            lwcli_add_history_command();
        }
        #endif  // LWCLI_HISTORY_COMMAND_NUM > 0
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
                lwcli_opt_output(&recv_char, 1);
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
                    lwcli_opt_output(ansi_cursor_right, sizeof(ansi_cursor_right));
                }
            }
            else if (recv_char == 'D') {
                if (lwcliObj.cursorPos > 0) {
                    lwcliObj.cursorPos--;
                    lwcli_opt_output(ansi_cursor_left, sizeof(ansi_cursor_left));
                }
            }
            #if (LWCLI_HISTORY_COMMAND_NUM > 0)
            else if (recv_char == 'A') {
                lwcli_history_command_up();
            }
            else if (recv_char == 'B') {
                lwcli_history_command_down();
            }
            #endif  // LWCLI_HISTORY_COMMAND_NUM > 0
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
    list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
        if (lwcli_match_command(command, cmd->command, cmd->cmd_len)) {
#if (LWCLI_PARAMETER_SPLIT == LWCLI_TRUE)
            char *cmdParameter = command;
            uint8_t parameter_num = lwcli_get_parameter_number(cmdParameter);
            if (parameter_num == 0) {
                cmd->callback(0, NULL);
#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
                lwcli_output_file_path();
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
                return;
            }
            char **parameterArray = (char **)lwcli_dynamic_malloc(sizeof(char *) * parameter_num);
            if (parameterArray == NULL) {
                lwcli_printf("error malloc\r\n");
                return;
            }
            uint8_t findParameterNum = lwcli_find_parameters(cmdParameter + cmd->cmd_len, parameterArray, parameter_num);
            cmd->callback(findParameterNum, parameterArray);
            lwcli_dynamic_free();  /* 释放 dynamic 池 */
#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
            lwcli_output_file_path();
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
            return;
#else  // LWCLI_PARAMETER_SPLIT == LWCLI_TRUE
            {
                char *argvs = command + cmd->cmd_len;
                while (*argvs == ' ') argvs++;
                cmd->callback(argvs);
            }
#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
            lwcli_output_file_path();
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
            return;
#endif  // LWCLI_PARAMETER_SPLIT == LWCLI_TRUE
        }
    }

    lwcli_printf(lwcli_reminder, command);
#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
    lwcli_output_file_path();
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
}

#if (LWCLI_PARAMETER_SPLIT == LWCLI_TRUE)
/**
 * @brief 寻找参数总数量
 * @note 支持识别双引号
 * @param command_string 命令字符串
 * @return 参数数量
 */
static uint8_t lwcli_get_parameter_number(const char *command_string)
{
    size_t len = strlen(command_string);
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
    size_t len = strlen(argv_str);
    if (len <= 1) {
        return 0;
    }
    uint8_t found_num = 0;
    bool in_quotes = false; // 是否处于引号中

    uint8_t *index = (uint8_t*) lwcli_dynamic_malloc(sizeof(uint8_t) * (parameter_num + 1));  // 增加一个元素用于最后一个参数
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

        parameter_arry[i] = (char *)lwcli_dynamic_malloc(param_len + 1);
        if (parameter_arry[i] == NULL) {
            return 0;  /* 由调用方 lwcli_process_command 的 lwcli_dynamic_free 统一释放 */
        }
        memcpy(parameter_arry[i], argv_str + start, param_len);
        parameter_arry[i][param_len] = '\0';  // 添加字符串结束符 避免乱码
    }
    // 此处不能释放 index 内存，因为参数可能被用户回调函数使用
    return found_num;
}
#endif  // LWCLI_PARAMETER_SPLIT == LWCLI_TRUE

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
        #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
        lwcli_opt_output("\r\n", 2);
        lwcli_output_file_path();
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;
        lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        uint16_t output_len = snprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), "\r\n\r\n%s", lwcliObj.inputBuffer);
        lwcli_opt_output(lwcliObj.ouputBuffer, output_len);
        #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
    }
    else if (match_num == 1) {
        list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
            if (lwcliObj.inputBufferPos < cmd->cmd_len) {
                if (memcmp(cmd->command, prefix, lwcliObj.inputBufferPos) == 0) {
                    memcpy(lwcliObj.inputBuffer, cmd->command, cmd->cmd_len);
                    lwcliObj.inputBufferPos = cmd->cmd_len;
                    lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
                    lwcliObj.cursorPos = lwcliObj.inputBufferPos;
                    lwcli_opt_output(ansi_clear_line, sizeof(ansi_clear_line));
#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
                    lwcli_output_file_path();
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
                    lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
                    break;
                }
            }
        }
    }
    else {
        char **match_arr = NULL;
        uint16_t match_index = 0;
        match_arr = (char **)lwcli_dynamic_malloc(sizeof(char *) * match_num);
        if (match_arr == NULL) {
            return;
        }

        list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
            if (memcmp(cmd->command, prefix, lwcliObj.inputBufferPos) == 0) {
                match_arr[match_index++] = cmd->command;
            }
        }

        uint16_t match_max_len = lwcli_longest_common_prefix_length(match_num, match_arr);
        lwcli_opt_output("\r\n", 2);
        for (uint16_t i = 0; i < match_num; i++) {
            lwcli_opt_output(match_arr[i], strlen(match_arr[i]));
            lwcli_opt_output("    ", 4);
        }
        
        while (lwcliObj.inputBufferPos < match_max_len) {
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos] = match_arr[0][lwcliObj.inputBufferPos];
            lwcliObj.inputBufferPos++;
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;

        #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
        lwcli_opt_output("\r\n", 2);
        lwcli_output_file_path();
        lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        uint16_t output_len = snprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), "\r\n\r\n%s", lwcliObj.inputBuffer);
        lwcli_opt_output(lwcliObj.ouputBuffer, output_len);
        #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE

        lwcli_dynamic_free();
    }
}

#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
/**
 * @brief 获取当前光标位置所在“参数token”的前缀
 * @details
 *  - 支持双引号包裹：token 内部的空格不参与分隔
 *  - 支持多个参数：只匹配“最后一个token”的前缀
 *  - 若光标位于token后的空格处，则当前前缀视为长度为0
 *
 * @param cmd 参数所属的命令
 * @param prefix 输出：指向当前token前缀起始地址
 * @param prefix_len 输出：当前token前缀长度（不含分隔空格；尾随空格场景为0）
 * @param prefix_start_pos 输出：token在 inputBuffer 中的起始下标
 */
static void lwcli_get_current_parameter_prefix(command_t *cmd, const char **prefix, int *prefix_len, uint16_t *prefix_start_pos)
{
    const char *buf = lwcliObj.inputBuffer;
    uint16_t cursor = lwcliObj.inputBufferPos;
    uint8_t cmd_end = (size_t)cmd->cmd_len;

    /* 保持与原实现一致：cmd后无字符时 prefix_len 为负，后续逻辑会走 prefix_len < 0 分支补空格 */
    if (cursor <= cmd_end) {
        *prefix_start_pos = cmd_end + 1;
        *prefix = buf + cmd_end + 1;
        *prefix_len = (int)cursor - (int)cmd_end - 1;
        return;
    }

    /* 1) 判断光标处是否仍在引号中；若不在引号中且末尾有空格，则当前token前缀为空 */
    bool in_quotes = false;
    for (uint16_t i = cmd_end; i < cursor; i++) {
        if (buf[i] == '\"') {
            in_quotes = !in_quotes;
        }
    }
    if (!in_quotes) {
        uint16_t end = cursor;
        while (end > cmd_end && buf[end - 1] == ' ') {
            end--;
        }
        if (end < cursor) {
            *prefix_start_pos = cursor;
            *prefix = buf + cursor;
            *prefix_len = 0;
            return;
        }
    }

    /* 2) 跳过命令后的前导空格，找到token起点 */
    uint16_t start = cmd_end;
    while (start < cursor && buf[start] == ' ') {
        start++;
    }
    if (start >= cursor) {
        *prefix_start_pos = cursor;
        *prefix = buf + cursor;
        *prefix_len = 0;
        return;
    }

    /* 3) 扫描分隔位置：空格且不在引号中，且后一个不是空格 */
    uint16_t token_start = start;
    in_quotes = false;
    for (uint16_t i = start; i < cursor; i++) {
        char c = buf[i];
        if (c == '\"') {
            in_quotes = !in_quotes;
        }
        if (!in_quotes && c == ' ' && (i + 1) < cursor && buf[i + 1] != ' ') {
            token_start = i + 1;
        }
    }

    *prefix_start_pos = token_start;
    *prefix = buf + token_start;
    *prefix_len = (int)(cursor - token_start);
}

/**
 * @brief 补全参数
 * @param cmd 参数所属的命令
 */
static void lwcli_fix_parameter(command_t *cmd)
{
    if (list_empty(&cmd->para.node)) {
        return;
    }
    const char *prefix = NULL;
    int prefix_len = 0;
    uint16_t prefix_start_pos = 0;
    lwcli_get_current_parameter_prefix(cmd, &prefix, &prefix_len, &prefix_start_pos);
    uint16_t match_num = 0;
    parameter_t *param = NULL;
    if (!lwcliObj.inputBufferPos) {
        return;
    }
    list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
        if (prefix_len > 0 && (uint16_t)prefix_len <= param->len) {
            if (memcmp(param->data, prefix, (uint16_t)prefix_len) == 0) {
                match_num++;
            }
        }
    }

    if (match_num == 0) {
        lwcli_opt_output("\r\n", 2);
        if (prefix_len == 0) {
            /* 用户仅输入 "cmd " 未输入前缀时，列出所有参数 */
            list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
                lwcli_printf("%s    ", param->data);
            }
        }
        #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
        lwcli_opt_output("\r\n", 2);
        lwcli_output_file_path();
        if (prefix_len < 0){
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;
        lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        lwcli_opt_output("\r\n", 2);
        if (prefix_len < 0){
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;
        lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
    }
    else if (match_num == 1) {
        list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
            if ((size_t)prefix_len <= param->len && memcmp(param->data, prefix, (size_t)prefix_len) == 0) {
                memcpy(lwcliObj.inputBuffer + prefix_start_pos, param->data, param->len);
                lwcliObj.inputBufferPos = prefix_start_pos + param->len;
                lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = ' ';
                lwcliObj.cursorPos = lwcliObj.inputBufferPos;
                lwcli_opt_output(ansi_clear_line, sizeof(ansi_clear_line));
#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
                lwcli_output_file_path();
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
                lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
                break;
            }
        }
    }
    else {
        char **match_arr = NULL;
        uint16_t match_index = 0;
        match_arr = (char **)lwcli_dynamic_malloc(sizeof(char *) * match_num);
        if (match_arr == NULL) {
            return;
        }

        list_for_each_entry(param, &cmd->para.node, node, parameter_t) {
            if ((size_t)prefix_len <= param->len && memcmp(param->data, prefix, (size_t)prefix_len) == 0) {
                match_arr[match_index++] = param->data;
            }
        }

        uint16_t match_max_len = lwcli_longest_common_prefix_length(match_num, match_arr);
        lwcli_opt_output("\r\n", 2);
        for (uint16_t i = 0; i < match_num; i++) {
            lwcli_opt_output(match_arr[i], strlen(match_arr[i]));
            lwcli_opt_output("    ", 4);
        }
        
        while (prefix_len < match_max_len) {
            lwcliObj.inputBuffer[lwcliObj.inputBufferPos++] = match_arr[0][prefix_len++];
        }
        lwcliObj.cursorPos = lwcliObj.inputBufferPos;

        #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
        lwcli_opt_output("\r\n", 2);
        lwcli_output_file_path();
        lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
        #else
        uint16_t output_len = snprintf(lwcliObj.ouputBuffer, sizeof(lwcliObj.ouputBuffer), "\r\n\r\n%s", lwcliObj.inputBuffer);
        lwcli_opt_output(lwcliObj.ouputBuffer, output_len);
        #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE

        lwcli_dynamic_free();
    }
}
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE

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
#if (LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE)
    list_for_each_entry(cmd, &lwcliObj.command->node, node, command_t) {
        if (lwcli_match_command(lwcliObj.inputBuffer, cmd->command, cmd->cmd_len)) {
            compelter_par = true;
            break;
        }
    }
    if (compelter_par) {
        lwcli_fix_parameter(cmd);
    }
    else {
        lwcli_fix_command();
    }
#else
    lwcli_fix_command();
#endif  // LWCLI_PARAMETER_COMPLETION == LWCLI_TRUE
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

    lwcli_opt_output(ansi_clear_line, sizeof(ansi_clear_line));
    #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
    lwcli_output_file_path();
    #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
    lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
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
            lwcli_opt_output(ansi_clear_line, sizeof(ansi_clear_line));
            #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
            lwcli_output_file_path();
            #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
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
            lwcli_opt_output(ansi_clear_line, sizeof(ansi_clear_line));
            #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
            lwcli_output_file_path();
            #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
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

    lwcli_opt_output(ansi_clear_line, sizeof(ansi_clear_line));
    #if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)
    lwcli_output_file_path();
    #endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
    lwcli_opt_output(lwcliObj.inputBuffer, lwcliObj.inputBufferPos);
    lwcliObj.cursorPos = lwcliObj.inputBufferPos;

}

            #endif  // LWCLI_HISTORY_COMMAND_NUM > 0

#if (LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE)

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
    char *filePath = (lwcliObj.opt->get_file_path != NULL)
                    ? lwcliObj.opt->get_file_path() : "/";
    lwcli_output_string_withcolor(LWCLI_USER_NAME":", COLOR_GREEN);
    lwcli_output_string_withcolor(filePath, COLOR_BLUE);
    lwcli_opt_output("$ ", 2);
}
#endif  // LWCLI_WITH_FILE_SYSTEM == LWCLI_TRUE
