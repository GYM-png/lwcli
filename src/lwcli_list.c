/**
 * @file lwcli_list.c
 * @author GYM (48060945@qq.com)
 * @brief lwcli 单向链表实现
 * @version V0.0.4
 * @date 2026-03-15
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "lwcli_list.h"

void list_node_init(list_node_t *node)
{
    if (node == NULL) {
        return;
    }
    node->next = node;
}

void list_head_init(list_node_t *head)
{
    list_node_init(head);
}

void list_add_after(list_node_t *pos, list_node_t *node)
{
    if (pos == NULL || node == NULL) {
        return;
    }
    node->next = pos->next;
    pos->next = node;
}

void list_add_front(list_node_t *head, list_node_t *node)
{
    list_add_after(head, node);
}

void list_add_tail(list_node_t *head, list_node_t *node)
{
    list_node_t *cur = NULL;
    if (head == NULL || node == NULL) {
        return;
    }
    cur = head;
    while (cur->next != head) {
        cur = cur->next;
    }
    node->next = head;
    cur->next = node;
}

void list_remove(list_node_t *head, list_node_t *node)
{
    list_node_t *cur = NULL;
    if (head == NULL || node == NULL) {
        return;
    }
    cur = head;
    while (cur->next != head && cur->next != node) {
        cur = cur->next;
    }
    if (cur->next == node) {
        cur->next = node->next;
        node->next = node;
    }
}

int list_empty(const list_node_t *head)
{
    if (head == NULL) {
        return 1;
    }
    return head->next == head;
}

list_node_t *list_first(list_node_t *head)
{
    if (head == NULL) {
        return NULL;
    }
    return head->next;
}
