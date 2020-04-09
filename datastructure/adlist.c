/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */
list *listCreate(void)
{
    struct list *list;

    if ((list = zmalloc(sizeof(*list))) == NULL)//调用zmalloc
        return NULL;	//创建失败返回NULL
    list->head = list->tail = NULL;//头尾结点均设置为NULL
    list->len = 0;	//长度设置为0
    list->dup = NULL;	//结点复制函数设置为NULL
    list->free = NULL;	//结点释放函数设置为NULL
    list->match = NULL; //结点比较函数设置为NULL
    return list;
}

/* Remove all the elements from the list without destroying the list itself. */
void listEmpty(list *list)
{
    unsigned long len;
    listNode *current, *next;

    current = list->head;
    len = list->len;
    while(len--) {		//遍历链表中的所有结点
        next = current->next;
        if (list->free) list->free(current->value);//如果定义了结点的free函数,则调用free函数释放结点的值
        zfree(current);	//释放当前结点
        current = next;
    }
    list->head = list->tail = NULL;//头尾结点置为NULL
    list->len = 0;
}

/* Free the whole list.
 *
 * This function can't fail. */
void listRelease(list *list)
{
    listEmpty(list);	//先调用listEmpty释放链表中的所有结点
    zfree(list);	//调用zfree释放链表
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
list *listAddNodeHead(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;	//内存分配失败返回NULL
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;//如果链表长度为0,则将头尾结点指向新插入的结点
        node->prev = node->next = NULL;//链表中只有这一个结点,前驱后继均为NULL
    } else {
        node->prev = NULL;	//链表不为空,该结点成为链表新的头结点,前驱为NULL
        node->next = list->head;//后继为原来的head
        list->head->prev = node;//原来的head前驱变为新结点
        list->head = node;	//链表的头结点变为新结点
    }
    list->len++;//长度加一
    return list;//返回
}

/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = list->tail;//新的尾结点的前驱　指向　旧的尾结点
        node->next = NULL;	//新的尾结点后继为NULL
        list->tail->next = node;//旧的尾结点后继为新的尾结点
        list->tail = node;
    }
    list->len++;
    return list;
}

list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;	//内存分配失败返回NULL
    node->value = value;
    if (after) {	//after非0则在old_node后面插入新结点
        node->prev = old_node;
        node->next = old_node->next;
        if (list->tail == old_node) {
            list->tail = node;//old_node为尾结点,则新的尾结点变为node
        }
    } else {		//after为0则在old_node前面插入新结点
        node->next = old_node;
        node->prev = old_node->prev;
        if (list->head == old_node) {
            list->head = node;//old_node为头结点,则新的头结点变为node
        }
    }
    if (node->prev != NULL) {
        node->prev->next = node;
    }
    if (node->next != NULL) {
        node->next->prev = node;
    }
    list->len++;
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */
void listDelNode(list *list, listNode *node)
{
    if (node->prev)
        node->prev->next = node->next;	//有前驱,前驱的next指向node的next
    else
        list->head = node->next;	//没前驱,node为头结点，新的头结点为node->next
    if (node->next)
        node->next->prev = node->prev;	//有后继,后继的prev指向node的prev
    else
        list->tail = node->prev;	//没后继,node为尾结点，新的尾结点为node->prev
    if (list->free) list->free(node->value);//定义了free函数则使用结点的free函数释放结点
    zfree(node);
    list->len--;//长度减一
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */
listIter *listGetIterator(list *list, int direction)
{
    listIter *iter;

    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;//内存分配失败返回NULL
    if (direction == AL_START_HEAD)
        iter->next = list->head;//返回从前向后的迭代器
    else
        iter->next = list->tail;//返回从后向前的迭代器
    iter->direction = direction;//设置方向
    return iter;
}

/* Release the iterator memory */
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure */
void listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;//将li变为前向迭代器
}

void listRewindTail(list *list, listIter *li) {
    li->next = list->tail;
    li->direction = AL_START_TAIL;//将li变为后向迭代器
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * */
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;//当前迭代器指向的结点

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next;//前向迭代器,往后移动
        else
            iter->next = current->prev;//后向迭代器,往前移动
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */
list *listDup(list *orig)
{
    list *copy;		//拷贝后的链表
    listIter iter;	//迭代器
    listNode *node;

    if ((copy = listCreate()) == NULL)//内存分配失败返回NULL
        return NULL;
    copy->dup = orig->dup;	//拷贝和结点相关的函数
    copy->free = orig->free;
    copy->match = orig->match;
    listRewind(orig, &iter);	//将迭代器指向原链表头
    while((node = listNext(&iter)) != NULL) {
        void *value;

        if (copy->dup) {
            value = copy->dup(node->value);//如果定义了dup函数则使用dup函数复制每一个结点
            if (value == NULL) {
                listRelease(copy);//复制整个链表期间,如果出错,释放已经复制的内容,然后返回NULL
                return NULL;
            }
        } else
            value = node->value;	//没定义dup函数,新的value和旧的结点的value指向相同
        if (listAddNodeTail(copy, value) == NULL) {
            listRelease(copy);//将value加入链表尾部期间,如果失败,释放已经复制的内容,然后返回NULL
            return NULL;
        }
    }
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */
listNode *listSearchKey(list *list, void *key)
{
    listIter iter;
    listNode *node;

    listRewind(list, &iter);	//获取迭代器
    while((node = listNext(&iter)) != NULL) {
        if (list->match) {	//如果定义了match函数则使用match函数比较两个value
            if (list->match(node->value, key)) {
                return node;
            }
        } else {		//没有定义match函数则直接比较两者是否指向同一地址
            if (key == node->value) {
                return node;
            }
        }
    }
    return NULL;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */
listNode *listIndex(list *list, long index) {
    listNode *n;

    if (index < 0) {	//index小于0,从后往前找
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {		//index大于0,从前往后找
        n = list->head;
        while(index-- && n) n = n->next;
    }
    return n;
}

/* Rotate the list removing the tail node and inserting it to the head. */
void listRotate(list *list) {
    listNode *tail = list->tail;//链表尾结点

    if (listLength(list) <= 1) return;//链表为空或只有一个结点,直接返回,没必要搬移

    /* Detach current tail */
    list->tail = tail->prev;	//将尾结点的前驱变为新的尾结点
    list->tail->next = NULL;
    /* Move it as head */
    list->head->prev = tail;	//将尾结点变为头结点
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}

/* Add all the elements of the list 'o' at the end of the
 * list 'l'. The list 'other' remains empty but otherwise valid. */
void listJoin(list *l, list *o) {
    if (o->head)
        o->head->prev = l->tail;//o链表头结点的前驱设置为l的尾结点

    if (l->tail)
        l->tail->next = o->head;//l不为空,l尾结点的后继设置为o的头结点
    else
        l->head = o->head;//l为空,l的头就是o的头

    if (o->tail) l->tail = o->tail;//o不为空,l的尾结点设置为o的尾结点
    l->len += o->len;//新链表的长度

    /* Setup other as an empty list. */
    o->head = o->tail = NULL;
    o->len = 0;
}
