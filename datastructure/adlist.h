/* adlist.h - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */

typedef struct listNode {
    struct listNode *prev;  //指向前驱结点的指针
    struct listNode *next;  //指向后继结点的指针
    void *value;            //指向结点值的指针
} listNode;

typedef struct listIter {
    listNode *next;	//迭代器指向的结点
    int direction;	//迭代器方向,AL_START_HEAD 或 AL_START_TAIL
} listIter;

typedef struct list {
    listNode *head;	//头结点
    listNode *tail;	//尾结点
    void *(*dup)(void *ptr);	//结点的复制函数
    void (*free)(void *ptr);	//结点的释放函数
    int (*match)(void *ptr, void *key);//结点的比较函数
    unsigned long len;	//整个链表的长度
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len)		//返回链表长度
#define listFirst(l) ((l)->head)		//返回链表的头结点
#define listLast(l) ((l)->tail)			//返回链表的尾结点
#define listPrevNode(n) ((n)->prev)		//返回当前结点的前驱
#define listNextNode(n) ((n)->next)		//返回当前结点的后继
#define listNodeValue(n) ((n)->value)		//返回当前结点的值

#define listSetDupMethod(l,m) ((l)->dup = (m))	//设置链表的dup函数
#define listSetFreeMethod(l,m) ((l)->free = (m))//设置链表的free函数
#define listSetMatchMethod(l,m) ((l)->match = (m))//设置链表的match函数

#define listGetDupMethod(l) ((l)->dup)		//获取链表的dup函数
#define listGetFree(l) ((l)->free)		//获取链表的free函数
#define listGetMatchMethod(l) ((l)->match)	//获取链表的match函数

/* Prototypes */
list *listCreate(void);			//创建链表
void listRelease(list *list);	//释放整个链表
void listEmpty(list *list);		//释放链表中的每一个结点
list *listAddNodeHead(list *list, void *value);//头部插入一个结点
list *listAddNodeTail(list *list, void *value);//尾部插入一个结点
list *listInsertNode(list *list, listNode *old_node, void *value, int after);//在某一结点前/后插入新结点
void listDelNode(list *list, listNode *node);//删除结点
listIter *listGetIterator(list *list, int direction);//获取链表的迭代器
listNode *listNext(listIter *iter);			//获取当前迭代器指向结点的后继结点
void listReleaseIterator(listIter *iter);	//释放迭代器
list *listDup(list *orig);					//拷贝链表
listNode *listSearchKey(list *list, void *key);	//在链表中查找key
listNode *listIndex(list *list, long index);	//查找链表的第index个元素
void listRewind(list *list, listIter *li);		//重置迭代器,指向链表头部
void listRewindTail(list *list, listIter *li);	//重置迭代器,指向链表尾部
void listRotate(list *list);					//将尾结点搬到头部
void listJoin(list *l, list *o);				//将链表o插入链表l

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
