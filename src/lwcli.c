/**
 * @file lwcli.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 核心函数实现文件
 * @version V0.0.3
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
typedef struct cmdList
{
    char cmdStr[LWCLI_COMMAND_STR_MAX_LENGTH];
    char helpStr[LWCLI_HELP_STR_MAX_LENGTH];
    uint8_t cmdLen;
    cliCmdFunc callBack;
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
static historyList_t *historyList;  // 历史记录表
static void lwcli_add_history_command(const char *cmdStr);
static void lwcli_history_list_update(void);
static void lwcli_history_command_down(char *cmdStr);
static void lwcli_history_command_up(char *cmdStr);
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

static void lwcli_output_string_withcolor(char *str, colorEnum_e color);
void lwcli_output_file_path(void);
#endif  // LWCLI_WITH_FILE_SYSTEM == true



/* 链表结构体定义  */
static cmdList_t *cmdListHead = NULL;   // 命令链表头
static bool findHistoryEnable = false;
static char printfBuffer[LWCLI_SHELL_OUTPUT_BUFFER_SIZE] = {0};

/** 静态函数声明 **/
static void lwcli_help(int argc, char* argv[]);
static void lwcli_clear(int argc, char* argv[]);
static uint8_t lwcli_find_parameters(const char *argv_str, char **parameter_arry, uint8_t parameter_num);
static uint8_t lwcli_get_parameter_number(const char *command_string);
static void lwcli_process_command(char *cmdStr);
static void lwcli_printf(const char *format, ...);

/** lwcli_port 接口 **/
extern void *lwcli_malloc(size_t size);
extern void lwcli_free(void *ptr);
extern void lwcli_output_char(char output_char);
extern void lwcli_output_string(const char *output_string, uint16_t string_len);
#if (LWCLI_WITH_FILE_SYSTEM == true)
extern char *lwcli_get_file_path(void);
#endif  // LWCLI_WITH_FILE_SYSTEM == true

/** 输入缓冲区 **/
static char inputBuffer[LWCLI_RECEIVE_BUFFER_SIZE] = {0};
static uint16_t inputBufferPos = 0;

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
                                lwcli_printf("%d %s", __LINE__, #x);\
                                return; \
                            }\
                        }while(0)
                        

/**
 * @brief lwcli 软件初始化
 * @note 初始化链表和历史记录缓冲区，注册默认命令
 */
void lwcli_software_init(void)
{
    if (cmdListHead == NULL)    // 仅第一次调用时 初始化
    {
        /** 初始化头节点并绑定帮助命令 */
        cmdListHead = (cmdList_t *)lwcli_malloc(sizeof(cmdList_t));
        if (cmdListHead == NULL)
        {
            lwcli_output_string("lwcli malloc error\r\n", strlen("lwcli malloc error\r\n"));
            return;
        }
        cmdListHead->cmdLen = strlen("help");
        memcpy(cmdListHead->cmdStr, "help", cmdListHead->cmdLen);
        cmdListHead->cmdStr[4] = '\0';
        cmdListHead->callBack = lwcli_help;
        cmdListHead->next = NULL;

        /** 注册清屏命令 */
        cmdList_t *clearNode = (cmdList_t *)lwcli_malloc(sizeof(cmdList_t));
        if (clearNode == NULL)
        {
            lwcli_output_string("lwcli malloc error\r\n", strlen("lwcli malloc error\r\n"));
            return;
        }
        clearNode->cmdLen = strlen("clear");
        memcpy(clearNode->cmdStr, "clear", clearNode->cmdLen);
        memcpy(clearNode->helpStr, "clear screen", strlen("clear screen"));
        clearNode->cmdStr[5] = '\0';
        clearNode->helpStr[12] = '\0';
        clearNode->callBack = lwcli_clear;
        clearNode->next = NULL;
        cmdListHead->next = clearNode;

        /** 初始化历史记录缓冲区 */
        #if (LWCLI_HISTORY_COMMAND_NUM > 0)
        historyList = (historyList_t *)lwcli_malloc(sizeof(historyList_t));
        if (historyList == NULL)
        {
            lwcli_output_string("lwcli malloc error\r\n", strlen("lwcli malloc error\r\n"));
            return;
        }
        historyList->commandStrSize = LWCLI_RECEIVE_BUFFER_SIZE;
        historyList->recordHistoryNum = LWCLI_HISTORY_COMMAND_NUM;
        historyList->writeMirror = 0;
        historyList->writePos = 0;
        historyList->readMirror = 0;
        historyList->readPos = 0;
        historyList->findPos = 0;

        memset(historyList->buffer, 0, LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE);
        #endif // LWCLI_HISTORY_COMMAND_NUM > 0
    }
}

/**
 * @brief 注册命令
 * @param cmdStr 命令字符串
 * @param helpStr 帮助字符串
 * @param userCallback 用户回调函数
 */
void lwcli_regist_command(const char *cmdStr, const char *helpStr, cliCmdFunc userCallback)
{
    lwcli_assert(cmdStr != NULL);
    lwcli_assert(helpStr != NULL);
    lwcli_assert(userCallback != NULL);
    if (strlen(cmdStr) > LWCLI_COMMAND_STR_MAX_LENGTH)
    {
        lwcli_output_string("command string too long please modify LWCLI_COMMAND_STR_MAX_LENGTH \r\n", strlen("command string too long please modify LWCLI_COMMAND_STR_MAX_LENGTH \r\n"));
        return;
    }
    if (strlen(helpStr) > LWCLI_HELP_STR_MAX_LENGTH)
    {
        lwcli_output_string("help string too long please modify LWCLI_HELP_STR_MAX_LENGTH \r\n", strlen("help string too long please modify LWCLI_HELP_STR_MAX_LENGTH \r\n"));
        return;
    }
    lwcli_software_init(); // 保证能正确初始化
    cmdList_t *newNode = (cmdList_t *) lwcli_malloc(sizeof(cmdList_t));
    cmdList_t *pnode = cmdListHead;
    if (newNode == NULL)
    {
        lwcli_output_string("lwcli malloc error\r\n", strlen("lwcli malloc error\r\n"));
        return;
    }
    newNode->cmdLen = strlen(cmdStr);
    memcpy(newNode->cmdStr, cmdStr, newNode->cmdLen);
    newNode->cmdStr[newNode->cmdLen] = '\0';
    memcpy(newNode->helpStr, helpStr, strlen(helpStr));
    newNode->helpStr[strlen(helpStr)] = '\0';
    newNode->callBack = userCallback;
    newNode->next = NULL;
    while (pnode->next)
    {
        pnode = pnode->next;
    }
    pnode->next = newNode;
}

/**
 * @brief 帮助命令
 * @param cmdParameter 命令参数
 */
static void lwcli_help(int argc, char* argv[])
{
    cmdList_t *node = cmdListHead->next;
    while(node)
    {
        if (node->helpStr[0] != '\0')
        {
            lwcli_output_string(node->cmdStr, node->cmdLen);
            lwcli_output_char(':');
            for (uint16_t i = 0; i < (LWCLI_COMMAND_STR_MAX_LENGTH - (node->cmdLen + 1) + 4); i++)  // 打印空格对齐
            {
                lwcli_output_char(' ');
            }
            lwcli_output_string(node->helpStr, strlen(node->helpStr));
            lwcli_output_string("\r\n\r\n", strlen("\r\n\r\n"));
        }
        node = node->next;
    }
}

/**
 * @brief 清屏命令
 */
static void lwcli_clear(int argc, char* argv[])
{
    lwcli_output_string(ansi_clear_screen, sizeof(ansi_clear_screen));
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
    uint16_t print_len = vsnprintf(printfBuffer, sizeof(printfBuffer), format, args);
    va_end(args);
    lwcli_output_string(printfBuffer, print_len);
}

/**
 * @brief 接收处理字符
 * @param revChar 接收到的字符
 */
void lwcli_process_receive_char(char revChar)
{
    static uint16_t cursorPosNow = 0;
    static uint8_t ansiKey = 0;
    if (revChar == '\r' || revChar == '\n')
    {
        inputBuffer[inputBufferPos] = '\0';
        lwcli_output_string("\r\n", strlen("\r\n"));
        lwcli_process_command(inputBuffer);
        #if (LWCLI_HISTORY_COMMAND_NUM > 0)
        if (inputBufferPos > 0){
            lwcli_add_history_command(inputBuffer);
        }
        #endif // LWCLI_HISTORY_COMMAND_NUM > 0
        memset(inputBuffer, 0, sizeof(inputBuffer));
        inputBufferPos = 0;
        cursorPosNow = 0;
        cursorPosNow = inputBufferPos;
        findHistoryEnable = true;
        #if (LWCLI_HISTORY_COMMAND_NUM > 0)
        lwcli_history_list_update();
        #endif
    }
    else if ((revChar == '\b' || revChar == ansi_delete))
    {
        if (inputBufferPos == 0)
        {
            return;
        }
        if (cursorPosNow == inputBufferPos)
        {
            inputBuffer[--inputBufferPos] = '\0';
            lwcli_printf(lwcli_delete);
            findHistoryEnable = (inputBufferPos == 0) ? true : false;
            cursorPosNow = inputBufferPos;
        }
        else if (cursorPosNow > 0)
        {
            for (size_t i = cursorPosNow - 1; i < inputBufferPos; i++) // 字符串缓存处理，保证cmdStrBuffer中存储的字符串和屏幕一致
            {
                inputBuffer[i] = inputBuffer[i + 1];
            }
            lwcli_printf("%s%s%s%s%s", ansi_clear_behind, lwcli_delete, ansi_cursor_save, &inputBuffer[cursorPosNow - 1], ansi_cursor_restore);
            inputBufferPos--;
            cursorPosNow--;
        }
    }
    else if (revChar == '\033')
    {
        ansiKey = 1;
    }
    else
    {
        if (ansiKey == 0)
        {
            if (cursorPosNow == inputBufferPos)  // 普通字符且光标处于最后
            {
                inputBuffer[inputBufferPos++] = revChar;
                cursorPosNow = inputBufferPos;
                findHistoryEnable = false;
                lwcli_output_char(revChar);
            }
            else    // 普通字符但光标不是在最后
            {
                lwcli_printf("%c%s%s%s", revChar, ansi_cursor_save, inputBuffer + cursorPosNow, ansi_cursor_restore);
                
                for (size_t i = inputBufferPos; i > cursorPosNow; i--) // 字符串缓存处理，保证cmdStrBuffer中存储的字符串和屏幕一致
                {
                    inputBuffer[i] = inputBuffer[i - 1];
                }
                inputBuffer[cursorPosNow] = revChar;
                inputBufferPos++;
                cursorPosNow++;
            }
        }
        else if (ansiKey == 1)
        {
            ansiKey++;
        }
        else if (ansiKey == 2)
        {
            ansiKey = 0;
            if (revChar == 'C')
            {
                if (cursorPosNow < inputBufferPos)
                {
                    cursorPosNow++;
                    lwcli_output_string(ansi_cursor_right, sizeof(ansi_cursor_right));
                }
            }
            else if (revChar == 'D')
            {
                if (cursorPosNow > 0)
                {
                    cursorPosNow--;
                    lwcli_output_string(ansi_cursor_left, sizeof(ansi_cursor_left));
                }
            }
            #if (LWCLI_HISTORY_COMMAND_NUM > 0)
            else if (revChar == 'A')
            {
                lwcli_history_command_up(inputBuffer);
                lwcli_output_string(inputBuffer, strlen(inputBuffer));
                cursorPosNow = inputBufferPos;
            }
            else if (revChar == 'B')
            {
                lwcli_history_command_down(inputBuffer);
                lwcli_output_string(inputBuffer, strlen(inputBuffer));
                cursorPosNow = inputBufferPos;
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
 * @param cmdStr 命令字符串
 */
static void lwcli_process_command(char *cmdStr)
{
    cmdList_t *node = cmdListHead;
    char *cmdParameter = NULL;
    while (node)
    {
        if (lwcli_match_command(cmdStr, node->cmdStr))
        {
            cmdParameter = cmdStr;
            // 寻找参数
            uint8_t parameter_num = lwcli_get_parameter_number(cmdParameter);
            if (parameter_num == 0)
            {
                node->callBack(0, NULL);
                #if (LWCLI_WITH_FILE_SYSTEM == true)
                lwcli_output_file_path();
                #endif  
                return;
            }
            else
            {
                char **parameterArray = (char **) lwcli_malloc(sizeof(char *) * parameter_num);
                if (parameterArray == NULL)
                {
                    lwcli_output_string("error malloc\r\n", strlen("error malloc\r\n"));
                    return;
                }
                uint8_t findParameterNum = lwcli_find_parameters(cmdParameter + node->cmdLen, parameterArray, parameter_num);
                node->callBack(findParameterNum, parameterArray);
                for (int i = 0; i < findParameterNum; ++i)
                {
                    lwcli_free(parameterArray[i]);
                }
                lwcli_free(parameterArray);
                #if (LWCLI_WITH_FILE_SYSTEM == true)
                lwcli_output_file_path();
                #endif   
                return;
            }         
        }
        node = node->next;
    }
#if (LWCLI_WITH_FILE_SYSTEM == false)
    lwcli_printf(lwcli_reminder, cmdStr);
#else
    lwcli_output_file_path();
#endif
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
    for (uint8_t i = 0; i < len - 1; i++)
    {
        if (command_string[i] == ' ' && command_string[i + 1] != ' ' && !in_quotes)
        {
            parameter_num++;
        }
        else if (command_string[i] == '\"') // 引号
        {
            quotes_num++;
            in_quotes = !in_quotes;
        }
    }
    if (command_string[len - 1] == '\"')
    {
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
    if (index == NULL)
    {
        lwcli_output_string("error malloc\r\n", strlen("error malloc\r\n"));
        return 0;
    }

    // 找到参数位置
    for (uint8_t i = 0; i < len - 1; i++)
    {
        if (argv_str[i] == ' ' && argv_str[i + 1] != ' ' && !in_quotes)
        {
            index[found_num++] = i + 1;
            if (found_num == parameter_num)
                break;
        }
        else if (argv_str[i] == '\"')
        {
            in_quotes = !in_quotes;
        }
    }

    // 提取处理参数
    for (uint8_t i = 0; i < found_num; i++)
    {
        uint8_t start = index[i];
        uint8_t end = 0;
        end = (i + 1 < found_num) ? index[i + 1] - 1: len;  // 确保最后一个参数处理正确
        uint8_t param_len = end - start;

        parameter_arry[i] = (char *)lwcli_malloc(param_len + 1);  // 分配内存
        if (parameter_arry[i] == NULL)//错误处理
        {
            lwcli_output_string("error malloc\r\n", strlen("error malloc\r\n"));
            for (uint8_t j = 0; j < i; j++)
            {
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

#if (LWCLI_HISTORY_COMMAND_NUM > 0)
#define HISTORY_UPDATE_READ_INDEX()      do {historyList->readPos = (historyList->readPos + 1) % historyList->recordHistoryNum; if (!historyList->readPos) historyList->readMirror ^= 1;} while(0)
#define HISTORY_UPDATE_WRITE_INDEX()     do {historyList->writePos = (historyList->writePos + 1) % historyList->recordHistoryNum; if (!historyList->writePos) historyList->writeMirror ^= 1;} while(0)


/**
 * @brief 判断历史命令列表是否为满
 * @return true:为空 false:不为空
 */
static bool lwcli_history_is_full()
{
    return ((historyList->writePos == historyList->readPos) && (historyList->writeMirror != historyList->readMirror));
}

/**
 * @brief 判断历史命令列表是否为空
 * @return true:为空 false:不为空
 */
static bool lwcli_history_is_empty()
{
    return ((historyList->writePos == historyList->readPos) && (historyList->writeMirror == historyList->readMirror));
}

/**
 * @brief 添加一条历史命令
 * @param cmdStr 命令字符串
 */
static void lwcli_add_history_command(const char *cmdStr)
{
    if (lwcli_history_is_full())
    {
        HISTORY_UPDATE_READ_INDEX();
    }
    memcpy(historyList->buffer + (historyList->writePos * historyList->commandStrSize), cmdStr, strlen(cmdStr));
    historyList->buffer[historyList->writePos * historyList->commandStrSize + strlen(cmdStr)] = '\0';
    HISTORY_UPDATE_WRITE_INDEX();
    historyList->findPos = historyList->writePos;

    if (lwcli_history_is_full())
    {
        uint16_t readPosTemp = historyList->readPos;
        for (uint16_t i = 0; i < LWCLI_HISTORY_COMMAND_NUM; i++)
        {
            historyList->findPosIndex[i] = readPosTemp;
            readPosTemp = (readPosTemp + 1) % historyList->recordHistoryNum;
        }
        historyList->findPos = LWCLI_HISTORY_COMMAND_NUM;
    }
    else
    {
        for (uint16_t i = 0; i < historyList->writePos; i++)
        {
            historyList->findPosIndex[i] = i;
        }
        historyList->findPos = historyList->writePos;
    }
}

/**
 * @brief 更新历史命令中的一些状态
 */
static void lwcli_history_list_update(void)
{
    if (lwcli_history_is_full())
    {
        historyList->findPos = LWCLI_HISTORY_COMMAND_NUM;
    }
    else
    {
        historyList->findPos = historyList->writePos;
    }
    memset(inputBuffer, 0, LWCLI_RECEIVE_BUFFER_SIZE);
    inputBufferPos = 0;
}


/**
 * @brief 读取上一条历史命令
 * @param cmdStr 读取到的命令字符串
 */
static void lwcli_history_command_up(char *cmdStr)
{
    uint16_t historyCmdPos = historyList->findPosIndex[0]; 

    if (lwcli_history_is_empty())
    {
        return;
    }

    if (historyList->findPos > 0)
    {
        historyList->findPos --;
        historyCmdPos = historyList->findPosIndex[historyList->findPos];
    }

    uint8_t historyCommandLen = 0;
    char cmdChar = historyList->buffer[historyCmdPos * historyList->commandStrSize];
    while (cmdChar != '\0' && historyCommandLen < historyList->commandStrSize)
    {
        historyCommandLen++;
        cmdChar = historyList->buffer[historyCmdPos * historyList->commandStrSize + historyCommandLen];
    }
    memcpy(cmdStr, historyList->buffer + (historyCmdPos * historyList->commandStrSize), historyList->commandStrSize);
    memcpy(inputBuffer, historyList->buffer + (historyCmdPos * historyList->commandStrSize), historyList->commandStrSize);
    cmdStr[historyCommandLen] = '\0';
    inputBuffer[historyCommandLen] = '\0';
    inputBufferPos = historyCommandLen;

    // 删除之前打印的命令
    lwcli_output_string(ansi_claer_line, sizeof(ansi_claer_line));
}

/**
 * @brief 获取历史命令的下一个
 * @param cmdStr 存放提取出的参数
 */
static void lwcli_history_command_down(char *cmdStr)
{
    uint16_t historyCmdPos = historyList->findPosIndex[0]; 

    if (lwcli_history_is_empty())
    {
        return;
    }

    if (lwcli_history_is_full())
    {
        if (historyList->findPos >= LWCLI_HISTORY_COMMAND_NUM - 1) // 还没有使用UP键，则不允许使用Down 或者 Down到最后一个了
        {
            lwcli_output_string(ansi_claer_line, sizeof(ansi_claer_line));
            cmdStr[0] = '\0';
            lwcli_history_list_update();
            return;
        }
        else
        {
            if (historyList->findPos < LWCLI_HISTORY_COMMAND_NUM - 1)
            {
                historyList->findPos++;
                historyCmdPos = historyList->findPosIndex[historyList->findPos];
            }
        }
    }
    else
    {
        if (historyList->findPos >= historyList->writePos - 1)   // 还没有使用UP键，则不允许使用Down 或者 Down到最后一个了
        {
            lwcli_output_string(ansi_claer_line, sizeof(ansi_claer_line));
            cmdStr[0] = '\0';
            lwcli_history_list_update();
            return;
        }
        else
        {
            if (historyList->findPos < historyList->writePos)
            {
                historyList->findPos++;
            }
            historyCmdPos = historyList->findPosIndex[historyList->findPos];
        }
    }
    uint8_t historyCommandLen = 0;
    char cmdChar = historyList->buffer[historyCmdPos * historyList->commandStrSize];
    while (cmdChar != '\0' && historyCommandLen < historyList->commandStrSize)
    {
        historyCommandLen++;
        cmdChar = historyList->buffer[historyCmdPos * historyList->commandStrSize + historyCommandLen];
    }
    memcpy(cmdStr, historyList->buffer + (historyCmdPos * historyList->commandStrSize), historyList->commandStrSize);
    memcpy(inputBuffer, historyList->buffer + (historyCmdPos * historyList->commandStrSize), historyList->commandStrSize);
    cmdStr[historyCommandLen] = '\0';
    inputBuffer[historyCommandLen] = '\0';
    inputBufferPos = historyCommandLen;

    // 删除之前打印的命令
    lwcli_output_string(ansi_claer_line, sizeof(ansi_claer_line));
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
    lwcli_output_string(colorTable[color], strlen(colorTable[color]));
    lwcli_output_string(str, strlen(str));
    lwcli_output_string(LWCLI_ANSI_COLOR_RESET, strlen(LWCLI_ANSI_COLOR_RESET));
}

/**
 * @brief 输出当前路径
 */
void lwcli_output_file_path(void)
{
    char *filePath = lwcli_get_file_path();
    lwcli_output_string_withcolor(LWCLI_USER_NAME ":", COLOR_GREEN);
    lwcli_output_string_withcolor(filePath, COLOR_BLUE);
    lwcli_output_string("$ ", 2);
}
#endif // LWCLI_WITH_FILE_SYSTEM == true
