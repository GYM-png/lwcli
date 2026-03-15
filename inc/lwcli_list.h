/**
 * @file lwcli_list.h
 * @brief 嵌入式单向链表 - 纯链表结构，可被其他结构体包含
 *
 * 用法示例:
 *   struct my_item {
 *       int data;
 *       list_node_t node;   // 嵌入链表节点
 *   };
 *   list_node_t head;       // 链表头（哨兵节点）
 *   list_head_init(&head);
 *   struct my_item *item = ...;
 *   list_add_tail(&head, &item->node);
 *   list_for_each_entry(pos, &head, node, struct my_item) { ... }
 */

#ifndef __LWCLI_LIST_H__
#define __LWCLI_LIST_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 链表节点 - 仅包含 next 指针（单向链表），可嵌入任意结构体
 */
typedef struct list_node {
    struct list_node *next;
} list_node_t;

/**
 * @brief 从链表节点获取包含它的结构体指针
 * @param node   链表节点指针
 * @param type   包含该节点的结构体类型
 * @param member 结构体中 list_node_t 成员的名称
 * @return 包含该节点的结构体指针
 */
#define list_entry(node, type, member) \
    ((type *)((char *)(node) - offsetof(type, member)))

/**
 * @brief 正向遍历链表（头 -> 尾）
 * @param pos   list_node_t* 循环变量
 * @param head  链表头指针
 */
#define list_for_each(pos, head) \
    for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

/**
 * @brief 正向遍历链表，获取包含节点的结构体
 * @param pos   结构体指针，作为循环变量
 * @param head  链表头指针
 * @param member 结构体中 list_node_t 成员的名称
 * @param type  结构体类型名（如 struct my_item）
 */
#define list_for_each_entry(pos, head, member, type) \
    for ((pos) = list_entry((head)->next, type, member); \
         &(pos)->member != (head); \
         (pos) = list_entry((pos)->member.next, type, member))

/**
 * @brief 安全遍历（遍历时可删除当前节点）
 * @param pos   结构体指针，作为循环变量
 * @param n     临时变量，用于保存下一个节点对应的结构体
 * @param head  链表头指针
 * @param member 结构体中 list_node_t 成员的名称
 * @param type  结构体类型名
 */
#define list_for_each_entry_safe(pos, n, head, member, type) \
    for ((pos) = list_entry((head)->next, type, member), \
         (n) = list_entry((pos)->member.next, type, member); \
         &(pos)->member != (head); \
         (pos) = (n), (n) = list_entry((n)->member.next, type, member))

/* ---------- API ---------- */

/**
 * @brief 初始化单个节点（使 next 指向自身，表示未链接状态）
 * @param node 节点指针
 */
void list_node_init(list_node_t *node);

/**
 * @brief 初始化链表头（循环哨兵：head->next = head）
 * @param head 链表头指针
 */
void list_head_init(list_node_t *head);

/**
 * @brief 将 node 插入到 pos 之后
 * @param pos  插入位置节点
 * @param node 待插入节点
 */
void list_add_after(list_node_t *pos, list_node_t *node);

/**
 * @brief 将 node 插入到链表头后（即链表最前）
 * @param head 链表头指针
 * @param node 待插入节点
 */
void list_add_front(list_node_t *head, list_node_t *node);

/**
 * @brief 将 node 插入到链表头前（即链表最后）
 * @param head 链表头指针
 * @param node 待插入节点
 */
void list_add_tail(list_node_t *head, list_node_t *node);

/**
 * @brief 从链表中移除 node（不释放内存，需从头遍历，O(n)）
 * @param head 链表头指针
 * @param node 待移除节点
 */
void list_remove(list_node_t *head, list_node_t *node);

/**
 * @brief 判断链表是否为空
 * @param head 链表头指针
 * @return 1 为空，0 非空
 */
int list_empty(const list_node_t *head);

/**
 * @brief 获取第一个节点
 * @param head 链表头指针
 * @return 第一个节点，空链表返回 head
 */
list_node_t *list_first(list_node_t *head);

/**
 * @brief 获取下一个节点
 * @param node 当前节点
 * @return 下一个节点
 */
static inline list_node_t *list_next(list_node_t *node) {
    return node->next;
}

/**
 * @brief 判断节点是否已链接到某链表
 * @param node 节点指针
 * @return 1 已链接，0 未链接
 */
static inline int list_node_linked(const list_node_t *node) {
    return node->next != node;
}

#ifdef __cplusplus
}
#endif

#endif /* __LWCLI_LIST_H__ */
