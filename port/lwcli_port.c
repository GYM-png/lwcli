/**
 * @file lwcli_port.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 对外接口实现
 * @version V0.0.4
 * @date 2025-10-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */
/* library includes. */
#include "stdint.h"
#include "lwcli_config.h"

/* Your includes. */
#include "stdlib.h"
#include "stdio.h"

/**
 * @brief Memory allocation function
 * @param size  Size of memory to allocate
 * @return      Pointer to the allocated memory, or NULL on failure
 */
void *lwcli_malloc(size_t size)
{
    /* add your malloc function here */
    return malloc(size);
}

/**
 * @brief Memory free function
 * @param[in] ptr  Pointer to the memory block to free
 */
void lwcli_free(void *ptr)
{
    /* add your free function here */
    free(ptr);
}

/**
 * @brief Hardware initialization function
 * @note Initialize UART/serial, USB, or other I/O interfaces used by lwcli
 */
void lwcli_hardware_init(void)
{
    /* add your hardware init function here */
}

/**
 * @brief Output string to terminal
 * @param output_string  String to output
 * @param string_len     Length of the string
 */
void lwcli_output(const char *output_string, uint16_t string_len)
{
    /* add your output function here */
    if (string_len == 1) {
        putchar(*output_string);
    } else {
        printf("%s", output_string);
    }
}

#if (LWCLI_WITH_FILE_SYSTEM == true)
/**
 * @brief Get current filesystem path
 * @return Current path string (must be null-terminated)
 */
char *lwcli_get_file_path(void)
{
    /* add your get file path function here */
    return "/";
}
#endif
