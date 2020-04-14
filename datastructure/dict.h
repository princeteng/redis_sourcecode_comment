/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
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

#include <stdint.h>

#ifndef __DICT_H
#define __DICT_H

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

typedef struct dictEntry {
    void *key; //key
    union {    //value
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct dictEntry *next;//指向下一个结点的指针，由于使用的是拉链法，所以有指向下一个value的指针
} dictEntry;

typedef struct dictType {
    uint64_t (*hashFunction)(const void *key);			//哈希函数
    void *(*keyDup)(void *privdata, const void *key);	//key的复制函数
    void *(*valDup)(void *privdata, const void *obj);	//value的复制函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);//key的比较函数
    void (*keyDestructor)(void *privdata, void *key);	//key的"析构函数"
    void (*valDestructor)(void *privdata, void *obj);	//value的"析构函数"
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
typedef struct dictht {
    dictEntry **table;	//哈希表的bucket，结点指针数组，结点类型为dictEntry
    unsigned long size; //指针数组的大小，即bucket的大小
    unsigned long sizemask;//指针数组的长度掩码，用于计算索引值
    unsigned long used; //结点个数
} dictht;

typedef struct dict {
    dictType *type; //和哈希表key value相关的一些函数
    void *privdata; //上面函数的私有数据
    dictht ht[2];   //两张哈希表，用于渐进式rehash
    long rehashidx; /* -1表示没在进行rehash */
    unsigned long iterators; /* 当前运作的迭代器的数量 */
} dict;

/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
typedef struct dictIterator {
    dict *d;
    long index;
    int table, safe;
    dictEntry *entry, *nextEntry;
    /* unsafe iterator fingerprint for misuse detection. */
    long long fingerprint;
} dictIterator;

typedef void (dictScanFunction)(void *privdata, const dictEntry *de);
typedef void (dictScanBucketFunction)(void *privdata, dictEntry **bucketref);

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4 //每个哈希表的默认大小是4，就是bucket中有4个位置table[0]/table[1]/table[2]/table[3]

/* ------------------------------- Macros ------------------------------------*/
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)//如果定义了valDestructor，则使用valDestructor释放value

#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        (entry)->v.val = (_val_); \
} while(0)//如果定义了valDup，则使用valDup复制value

#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)//设置有符号的value

#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)//设置无符号的value

#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)//设置double的value

#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)//如果定义了keyDestructor则使用keyDestructor释放key

#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)//如果定义了keyDup则使用keyDup复制key

#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))//如果定义了keyCompare则使用keyCompare比较两个key

#define dictHashKey(d, key) (d)->type->hashFunction(key)	//计算key的哈希值
#define dictGetKey(he) ((he)->key)							//返回结点的key
#define dictGetVal(he) ((he)->v.val)						//返回结点的value
#define dictGetSignedIntegerVal(he) ((he)->v.s64)			//返回结点有符号的value
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)			//返回结点无符号的value
#define dictGetDoubleVal(he) ((he)->v.d)					//返回结点double型的value
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)		//返回字典槽的数量
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)		//返回字典哈希表中使用的槽的数量
#define dictIsRehashing(d) ((d)->rehashidx != -1)			//返回是否在rehash

/* API */
dict *dictCreate(dictType *type, void *privDataPtr); //创建字典
int dictExpand(dict *d, unsigned long size);//扩大字典
int dictAdd(dict *d, void *key, void *val); //往字典中加一个值
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);//
dictEntry *dictAddOrFind(dict *d, void *key);
int dictReplace(dict *d, void *key, void *val);		//将字典d中的key的value替换成val
int dictDelete(dict *d, const void *key);			//在字典中删除key
dictEntry *dictUnlink(dict *ht, const void *key);	//
void dictFreeUnlinkedEntry(dict *d, dictEntry *he);	//
void dictRelease(dict *d);							//
dictEntry * dictFind(dict *d, const void *key);
void *dictFetchValue(dict *d, const void *key);
int dictResize(dict *d);
dictIterator *dictGetIterator(dict *d);			//创建一个不安全的迭代器
dictIterator *dictGetSafeIterator(dict *d);		//创建一个安全的迭代器
dictEntry *dictNext(dictIterator *iter);		//迭代器当前指向的结点
void dictReleaseIterator(dictIterator *iter);	//释放迭代器
dictEntry *dictGetRandomKey(dict *d);			//随机返回一个结点
unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);//获取count个key，结点存在dictEntry中
void dictGetStats(char *buf, size_t bufsize, dict *d);
uint64_t dictGenHashFunction(const void *key, int len);//根据key生成哈希值
uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);//同上
void dictEmpty(dict *d, void(callback)(void*));	//destroy整个dict
void dictEnableResize(void);	//允许resize
void dictDisableResize(void);	//禁止resize
int dictRehash(dict *d, int n);	//rehash
int dictRehashMilliseconds(dict *d, int ms);//rehash ms毫秒
void dictSetHashFunctionSeed(uint8_t *seed);
uint8_t *dictGetHashFunctionSeed(void);
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);
uint64_t dictGetHash(dict *d, const void *key);
dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, uint64_t hash);

/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
