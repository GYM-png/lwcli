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

/**
 * @brief 初始化单个链表节点
 *
 * 将节点的 next 指针指向自身，表示该节点当前未链接到任何链表。
 * 这是节点的初始状态标记。
 *
 * @param[in] node 待初始化的链表节点指针，若为 NULL 则直接返回
 *
 * @note 节点初始化后，可通过 list_node_linked() 检查节点是否已链接到链表
 * @see list_node_linked()
 */
void list_node_init(list_node_t *node)
{
    if (node == NULL) {
        return;
    }
    node->next = node;
}

/**
 * @brief 初始化链表头节点（哨兵节点）
 *
 * 将链表头的 next 指针指向自身，形成一个空的循环链表。
 * 初始化后的链表头可作为链表操作的入口点。
 *
 * @param[in] head 链表头节点指针
 *
 * @note 链表头节点不存储实际数据，仅作为哨兵节点使用
 * @see list_node_init()
 */
void list_head_init(list_node_t *head)
{
    list_node_init(head);
}

/**
 * @brief 将节点插入到指定位置之后
 *
 * 在链表中指定节点 pos 的后方插入新节点 node。
 * 时间复杂度：O(1)
 *
 * @param[in] pos  插入位置的参考节点指针，若为 NULL 则不执行操作
 * @param[in] node 待插入的新节点指针，若为 NULL 则不执行操作
 *
 * @note 插入前 node 应已调用 list_node_init() 初始化
 * @note 插入后 node->next 指向原 pos->next 所指的节点
 * @see list_add_front() list_add_tail()
 */
void list_add_after(list_node_t *pos, list_node_t *node)
{
    if (pos == NULL || node == NULL) {
        return;
    }
    node->next = pos->next;
    pos->next = node;
}

/**
 * @brief 将节点插入到链表头部
 *
 * 在链表头部插入新节点，即插入到 head 节点之后。
 * 新节点成为链表的第一个实际数据节点。
 * 时间复杂度：O(1)
 *
 * @param[in] head 链表头节点指针
 * @param[in] node 待插入的新节点指针
 *
 * @note 此函数通过调用 list_add_after() 实现
 * @see list_add_after() list_add_tail()
 */
void list_add_front(list_node_t *head, list_node_t *node)
{
    list_add_after(head, node);
}

/**
 * @brief 将节点插入到链表尾部
 *
 * 遍历链表找到最后一个节点，在其后插入新节点。
 * 新节点的 next 指向链表头，保持循环链表结构。
 * 时间复杂度：O(n)，n 为链表节点数
 *
 * @param[in] head 链表头节点指针，若为 NULL 则不执行操作
 * @param[in] node 待插入的新节点指针，若为 NULL 则不执行操作
 *
 * @note 此操作需要遍历整个链表寻找尾节点，对于频繁尾插的场景
 *       建议考虑维护尾指针以优化到 O(1)
 * @warning 链表头节点（哨兵）不计入实际节点数
 * @see list_add_front()
 */
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

/**
 * @brief 从链表中移除指定节点
 *
 * 遍历链表查找目标节点，找到后将其从链表中摘除。
 * 被移除节点的 next 指针会被重置为指向自身。
 * 时间复杂度：O(n)，n 为链表节点数
 *
 * @param[in] head 链表头节点指针，若为 NULL 则不执行操作
 * @param[in] node 待移除的节点指针，若为 NULL 则不执行操作
 *
 * @note 此函数仅将节点从链表中断开，不会释放节点内存
 *       如需释放内存，调用者需自行处理
 * @note 若节点不在链表中，函数静默返回不作任何操作
 * @warning 不要在遍历链表时调用此函数移除当前节点，
 *          如需遍历时删除请使用 list_for_each_entry_safe() 宏
 * @see list_node_init() list_for_each_entry_safe()
 */
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

/**
 * @brief 检查链表是否为空
 *
 * 判断链表中是否包含除头节点外的实际数据节点。
 *
 * @param[in] head 链表头节点指针
 *
 * @retval 1  链表为空（仅包含头节点）
 * @retval 0  链表非空（包含至少一个数据节点）
 *
 * @note 头节点（哨兵）不计入实际节点
 * @note 若 head 为 NULL，返回 1 表示空
 * @see list_first()
 */
int list_empty(const list_node_t *head)
{
    if (head == NULL) {
        return 1;
    }
    return head->next == head;
}

/**
 * @brief 获取链表的第一个数据节点
 *
 * 返回链表中第一个实际数据节点（即头节点的下一个节点）。
 *
 * @param[in] head 链表头节点指针
 *
 * @return 第一个数据节点的指针
 * @return 若链表为空或 head 为 NULL，则返回 NULL
 *
 * @note 返回的节点可能是头节点自身（当链表为空时 head->next == head）
 *       因此调用者应配合 list_empty() 判断链表是否为空
 * @note 链表头节点不存储实际数据，返回的是其后继节点
 * @see list_empty() list_next()
 */
list_node_t *list_first(list_node_t *head)
{
    if (head == NULL) {
        return NULL;
    }
    return head->next;
}
