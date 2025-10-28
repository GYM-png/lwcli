/**
 * @file lwcli.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 核心函数实现文件
 * @version 0.1
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "lwCLI.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "lwcli_config.h"
typedef struct cmdList
{
    char cmdStr[LWCLI_COMMAND_STR_MAX_LENGTH];
    char helpStr[LWCLI_HELP_STR_MAX_LENGTH];
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
    uint16_t deleteLength;  // 输出delete的长度

    /** 存储命令历史记录索引 */
    /** 数组中存储的数值依次为第一条到最后一条命令在缓冲区中的位置 */
    uint16_t findPosIndex[LWCLI_HISTORY_COMMAND_NUM];
}historyList_t;
static historyList_t *historyList;  // 历史记录表
static void lwcli_add_history_command(char *cmdStr);
static void lwcli_history_list_update(void);
static void lwcli_history_command_down(char *cmdStr);
static void lwcli_history_command_up(char *cmdStr);
#endif  // LWCLI_HISTORY_COMMAND_NUM > 0

#if (LWCLI_WITH_FILE_SYSTEM == true)
typedef enum
{
    COLOR_RED = 0,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_NONE,
}colorEnum_e;

static char *colorTable[] = {
    "\x1B[31m",
    "\x1B[32m",
    "\x1B[33m",
    "\x1B[34m",
    "\x1B[35m",
    "\x1B[36m",
    "\x1B[37m",
    "\x1B[30m",
};
#define LWCLI_ANSI_COLOR_RESET "\x1B[0m"

static void lwcli_output_string_withcolor(char *str, colorEnum_e color);
void lwcli_output_file_path(void);
#endif  // LWCLI_WITH_FILE_SYSTEM == true



/* 链表结构体定义  */
static cmdList_t *cmdListHead = NULL;   // 命令链表头
static bool findHistoryEnable = false;

/** 静态函数声明 **/
static void lwcli_help(char **command_arry, uint8_t parameter_num);
static void lwcli_clear(char **command_arry, uint8_t parameter_num);
static uint8_t lwcli_find_parameters(const char *command_string, char **parameter_arry);
static uint8_t lwcli_get_parameter_number(const char *command_string);
static void lwcli_process_command(char *cmdStr);

/** lwcli_port 接口 **/
extern void *lwcli_malloc(size_t size);
extern void lwcli_free(void *ptr);
extern void lwcli_output_char(char output_char);
extern void lwcli_output_string(char *output_string, uint16_t string_len);
#if (LWCLI_WITH_FILE_SYSTEM == true)
extern char *lwcli_get_file_path(void);
#endif  // LWCLI_WITH_FILE_SYSTEM == true




/**
 * @brief 注册命令
 * @param cmdStr 命令字符串
 * @param helpStr 帮助字符串
 * @param userCallback 用户回调函数
 */
void lwcli_regist_command(char *cmdStr, char *helpStr, cliCmdFunc userCallback)
{
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
    if (cmdStr == NULL || helpStr == NULL || userCallback == NULL)
    {
        lwcli_output_string("command string or help string or callback function is null\r\n", strlen("command string or help string or callback function is null\r\n"));
        return;
    }
    if (cmdListHead == NULL)    // 第一次调用时 做初始化
    {
        /** 初始化头节点并绑定帮助命令 */
        cmdListHead = (cmdList_t *)lwcli_malloc(sizeof(cmdList_t));
        if (cmdListHead == NULL)
        {
            lwcli_output_string("lwcli malloc error\r\n", strlen("lwcli malloc error\r\n"));
            return;
        }
        memcpy(cmdListHead->cmdStr, "help", strlen("help"));
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
        memcpy(clearNode->cmdStr, "clear", strlen("clear"));
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

        historyList->deleteLength = 0;
        memset(historyList->buffer, 0, LWCLI_HISTORY_COMMAND_NUM * LWCLI_RECEIVE_BUFFER_SIZE);
        #endif // LWCLI_HISTORY_COMMAND_NUM > 0
    }
    cmdList_t *newNode = (cmdList_t *) lwcli_malloc(sizeof(cmdList_t));
    cmdList_t *pnode = cmdListHead;
    if (newNode == NULL)
    {
        lwcli_output_string("lwcli malloc error\r\n", strlen("lwcli malloc error\r\n"));
        return;
    }
    memcpy(newNode->cmdStr, cmdStr, strlen(cmdStr));
    newNode->cmdStr[strlen(cmdStr)] = '\0';
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
static void lwcli_help(char **command_arry, uint8_t parameter_num)
{
    cmdList_t *node = cmdListHead->next;
    while(node)
    {
        if (node->helpStr[0] != '\0')
        {
            lwcli_output_string(node->cmdStr, strlen(node->cmdStr));
            lwcli_output_char(':');
            for (uint16_t i = 0; i < (LWCLI_COMMAND_STR_MAX_LENGTH - (strlen(node->cmdStr) + 1) + 4); i++)  // 打印空格对齐
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
static void lwcli_clear(char **command_arry, uint8_t parameter_num)
{
    lwcli_output_string(LWCLI_SHELL_CLEAR_STRING, strlen(LWCLI_SHELL_CLEAR_STRING));
    lwcli_output_string(LWCLI_SHELL_CURSOR_TO_ZERO_STRING, strlen(LWCLI_SHELL_CURSOR_TO_ZERO_STRING));
}

static char cmdStrBuffer[LWCLI_RECEIVE_BUFFER_SIZE] = {0};
static uint16_t cmdStrBufferPos = 0;

/**
 * @brief 接收处理字符
 * @param revChar 接收到的字符
 */
void lwcli_process_receive_char(char revChar)
{
#if (LWCLI_HISTORY_COMMAND_NUM > 0)
    static char findHistoryKey[3] = {0, 0, 0};
    static uint8_t findHistoryKeyPos = 0;
#endif
    static char cursorMoveKey[3] = {0, 0, 0};
    static uint8_t cursorMoveKeyPos = 0;
    static uint16_t cursorPosNow = 0;
    if (revChar == '\r' || revChar == '\n')
    {
        cmdStrBuffer[cmdStrBufferPos] = '\0';
        lwcli_output_string("\r\n", strlen("\r\n"));
        lwcli_process_command(cmdStrBuffer);
        memset(cmdStrBuffer, 0, LWCLI_RECEIVE_BUFFER_SIZE);
        cmdStrBufferPos = 0;
        cursorPosNow = cmdStrBufferPos;
        findHistoryEnable = true;
        #if (LWCLI_HISTORY_COMMAND_NUM > 0)
        lwcli_history_list_update();
        #endif
    }
    else if ((revChar == '\b' || revChar == LWCLI_SHELL_BACK_CHAR))
    {
        if (cmdStrBufferPos == 0)
        {
            return;
        }
        if (cursorPosNow == cmdStrBufferPos)
        {
            cmdStrBuffer[--cmdStrBufferPos] = '\0';
            lwcli_output_char(revChar);
            findHistoryEnable = (cmdStrBufferPos == 0) ? true : false;
            cursorPosNow = cmdStrBufferPos;
        }
        else if (cursorPosNow > 0)
        {
            for (size_t i = cursorPosNow - 1; i < cmdStrBufferPos; i++) // 字符串缓存处理，保证cmdStrBuffer中存储的字符串和屏幕一致
            {
                cmdStrBuffer[i] = cmdStrBuffer[i + 1];
            }
            for (size_t i = 0; i < cmdStrBufferPos - cursorPosNow; i++) // 光标移动到最后一个字符后面
            {
                lwcli_output_string(LWCLI_SHELL_CURSOR_RIGHT_STRING, strlen(LWCLI_SHELL_CURSOR_RIGHT_STRING));
            }
            for (size_t i = 0; i < cmdStrBufferPos - cursorPosNow + 1; i++) // 控制屏幕删除，直到删除到需要删除的字符
            {
                lwcli_output_char(LWCLI_SHELL_DELETE_CHAR);
            }
            for (size_t i = cursorPosNow - 1; i < cmdStrBufferPos; i++) // 将多删的字符输出
            {
                lwcli_output_char(cmdStrBuffer[i]);
            }
            for (size_t i = 0; i < cmdStrBufferPos - cursorPosNow; i++) // 移动光标到正确位置
            {
                lwcli_output_string(LWCLI_SHELL_CURSOR_LEFT_STRING, strlen(LWCLI_SHELL_CURSOR_LEFT_STRING));
            }
            cmdStrBufferPos--;
            cursorPosNow--;
        }
    }
    else if (revChar == 0x1B || revChar == 0x5B || revChar == 0x41 || revChar == 0x42 || revChar == 0x43 || revChar == 0x44)
    {
        if (revChar == 0x1B || revChar == 0x5B)
        {
            #if (LWCLI_HISTORY_COMMAND_NUM > 0)
            findHistoryKey[findHistoryKeyPos++] = revChar;
            #endif
            cursorMoveKey[cursorMoveKeyPos++] = revChar;
        }
        else
        {
            if (revChar == 0x43 || revChar == 0x44)
            {
                cursorMoveKey[cursorMoveKeyPos++] = revChar;
                if (cursorMoveKeyPos == 3)
                {
                    #if (LWCLI_HISTORY_COMMAND_NUM > 0)
                    findHistoryKeyPos = 0;
                    #endif
                    if (cursorMoveKey[0] == 0x1B && cursorMoveKey[1] == 0x5B && cursorMoveKey[2] == 0x44)
                    {
                        if (cursorPosNow > 0)
                        {
                            cursorPosNow--;
                            lwcli_output_string(LWCLI_SHELL_CURSOR_LEFT_STRING, strlen(LWCLI_SHELL_CURSOR_LEFT_STRING));
                        }
                    }
                    else if (cursorMoveKey[0] == 0x1B && cursorMoveKey[1] == 0x5B && cursorMoveKey[2] == 0x43)
                    {
                        if (cursorPosNow < cmdStrBufferPos)
                        {
                            cursorPosNow++;
                            lwcli_output_string(LWCLI_SHELL_CURSOR_RIGHT_STRING, strlen(LWCLI_SHELL_CURSOR_RIGHT_STRING));
                        }
                    }
                    memset(cursorMoveKey, 0, 3);
                    cursorMoveKeyPos = 0;
                }
            }
#if (LWCLI_HISTORY_COMMAND_NUM > 0)
            else
            {
                if (findHistoryEnable)
                {
                    findHistoryKey[findHistoryKeyPos++] = revChar;
                    if (findHistoryKeyPos == 3)
                    {
                        cursorMoveKeyPos = 0;
                        if (findHistoryKey[0] == 0x1B && findHistoryKey[1] == 0x5B && findHistoryKey[2] == 0x41 && findHistoryEnable)
                        {
                            lwcli_history_command_up(cmdStrBuffer);
                            lwcli_output_string(cmdStrBuffer, strlen(cmdStrBuffer));
                        }
                        else if (findHistoryKey[0] == 0x1B && findHistoryKey[1] == 0x5B && findHistoryKey[2] == 0x42 && findHistoryEnable)
                        {
                            lwcli_history_command_down(cmdStrBuffer);
                            lwcli_output_string(cmdStrBuffer, strlen(cmdStrBuffer));
                        }
                        memset(findHistoryKey, 0, 3);
                        findHistoryKeyPos = 0;
                        cursorPosNow = cmdStrBufferPos;
                    }
                }
            }
#endif // LWCLI_HISTORY_COMMAND_NUM > 0
        }
    }
    else
    {
        if (cursorPosNow == cmdStrBufferPos)  // 普通字符且光标处于最后
        {
            cmdStrBuffer[cmdStrBufferPos++] = revChar;
            cursorPosNow = cmdStrBufferPos;
            findHistoryEnable = false;
            lwcli_output_char(revChar);
        }
        else    // 普通字符但光标不是在最后
        {
            lwcli_output_char(revChar);
            for (size_t i = cursorPosNow; i < cmdStrBufferPos; i++) // 输出原本光标后面的字符
            {
                lwcli_output_char(cmdStrBuffer[i]);
            }
            
            for (size_t i = 0; i < cmdStrBufferPos - cursorPosNow; i++) // 将光标移动到正确的位置
            {
                lwcli_output_string(LWCLI_SHELL_CURSOR_LEFT_STRING, strlen(LWCLI_SHELL_CURSOR_LEFT_STRING));
            }

            for (size_t i = cmdStrBufferPos; i > cursorPosNow; i--) // 字符串缓存处理，保证cmdStrBuffer中存储的字符串和屏幕一致
            {
                cmdStrBuffer[i] = cmdStrBuffer[i - 1];
            }
            cmdStrBuffer[cursorPosNow] = revChar;
            cmdStrBufferPos++;
            cursorPosNow++;
        }
    }
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
        if ((cmdParameter = strstr(cmdStr, node->cmdStr)) != NULL)
        {
            #if (LWCLI_HISTORY_COMMAND_NUM > 0)
            /** 添加命令历史记录 */
            lwcli_add_history_command(cmdStr);
            #endif // LWCLI_HISTORY_COMMAND_NUM > 0
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
                uint8_t findParameterNum = lwcli_find_parameters(cmdParameter + strlen(node->cmdStr), parameterArray);
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
    lwcli_output_string(LWCLI_COMMAND_FIND_FAIL_MESSAGE, strlen(LWCLI_COMMAND_FIND_FAIL_MESSAGE));
#else
    lwcli_output_file_path();
#endif
}

/**
 * @brief 寻找参数总数量
 * @param command_string 命令字符串
 * @return 参数数量
 */
static uint8_t lwcli_get_parameter_number(const char *command_string)
{
    uint8_t len = strlen(command_string);
    uint8_t parameter_num = 0;
    for (uint8_t i = 0; i < len - 1; i++)
    {
        if (command_string[i] == ' ' && command_string[i + 1] != ' ')
        {
            parameter_num++;
        }
    }
    return parameter_num;
}

/**
 * @brief 提取参数放入字符串数组
 * @param command_string 整个参数字符串
 * @param parameter_arry 字符串数组，存放提取出的参数
 * @return 找到的参数个数
 */
static uint8_t lwcli_find_parameters(const char *command_string, char **parameter_arry)
{
    uint8_t len = strlen(command_string);
    uint8_t found_num = 0;
    uint8_t parameter_num = 0;//总参数数量
    for (uint8_t i = 0; i < len - 1; i++)//寻找参数总数量
    {
        if (command_string[i] == ' ' && command_string[i + 1] != ' ')
        {
            parameter_num++;
        }
    }
    if (parameter_num == 0)
    {
        return 0;
    }

    uint8_t *index = (uint8_t*) lwcli_malloc(sizeof(uint8_t) * (parameter_num + 1));  // 增加一个元素用于最后一个参数
    if (index == NULL)
    {
        return 0;
    }

    // 找到参数位置
    for (uint8_t i = 0; i < len - 1; i++)
    {
        if (command_string[i] == ' ' && command_string[i + 1] != ' ')
        {
            index[found_num++] = i + 1;
            if (found_num == parameter_num)
                break;
        }
    }

    // 提取处理参数
    for (uint8_t i = 0; i < found_num; i++)
    {
        uint8_t start = index[i];
        uint8_t end = 0;
        if (i % 2 == 0)
            end = (i + 1 < found_num) ? index[i + 1] - 1: len;  // 确保最后一个参数处理正确
        else
            end = (i + 1 < found_num) ? index[i + 1] - 2: len;  // 确保最后一个参数处理正确
        uint8_t param_len = end - start;

        parameter_arry[i] = (char *)lwcli_malloc(param_len + 1);  // 分配内存
        if (parameter_arry[i] == NULL)//错误处理
        {
            for (uint8_t j = 0; j < i; j++)
            {
                lwcli_free(parameter_arry[j]);
            }
            lwcli_free(index);
            return 0;
        }
        memcpy(parameter_arry[i], command_string + start, param_len);
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
static void lwcli_add_history_command(char *cmdStr)
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
    // historyList->findPos = historyList->writePos;
    if (lwcli_history_is_full())
    {
        historyList->findPos = LWCLI_HISTORY_COMMAND_NUM;
    }
    else
    {
        historyList->findPos = historyList->writePos;
    }
    historyList->deleteLength = 0;
    memset(cmdStrBuffer, 0, LWCLI_RECEIVE_BUFFER_SIZE);
    cmdStrBufferPos = 0;
}

/**
 * @brief 删除当前输出
 * @param nowCmooandLength 当前命令长度，用于保存，下次调用时使用 
 */
static void lwcli_delete_output(uint16_t nowCmooandLength)
{
    if (historyList->deleteLength)
    {
        for (int i = 0; i < historyList->deleteLength; i++)
        {
            lwcli_output_char(LWCLI_SHELL_DELETE_CHAR);
        }
    }
    historyList->deleteLength = nowCmooandLength;
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
    memcpy(cmdStrBuffer, historyList->buffer + (historyCmdPos * historyList->commandStrSize), historyList->commandStrSize);
    cmdStr[historyCommandLen] = '\0';
    cmdStrBuffer[historyCommandLen] = '\0';
    cmdStrBufferPos = historyCommandLen;

    // 删除之前打印的命令
    lwcli_delete_output(historyCommandLen);
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
            lwcli_delete_output(0);
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
            lwcli_delete_output(0);
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
    memcpy(cmdStrBuffer, historyList->buffer + (historyCmdPos * historyList->commandStrSize), historyList->commandStrSize);
    cmdStr[historyCommandLen] = '\0';
    cmdStrBuffer[historyCommandLen] = '\0';
    cmdStrBufferPos = historyCommandLen;

    // 删除之前打印的命令
    lwcli_delete_output(historyCommandLen);
}

#endif // LWCLI_HISTORY_COMMAND_NUM > 0

#if (LWCLI_WITH_FILE_SYSTEM == true)

/**
 * @brief 通过ANSI编码输出带颜色的字符串
 * @param str 字符串
 * @param color 颜色
 */
static void lwcli_output_string_withcolor(char *str, colorEnum_e color)
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
