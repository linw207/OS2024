#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "stddef.h"

#define HEAPSIZE 16*1024*1024
#define ALIGNMENT 8     // 8字节对齐
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & (~(ALIGNMENT - 1)))

typedef struct block {
    struct block *next;  // 指向链表中的下一个内存块
    size_t size;         // 当前内存块的大小
    int free;            // 1空闲，0分配
    int id;              // 内存块的唯一序号
} blockNode;

static blockNode *heapList;  // heapList 是指向链表头部的指针，表示堆的起始地址
static struct spinlock heapLock;  // 用于实现互斥锁，以确保线程安全
static int next_id = 1;  // 下一个要分配的内存块的序号

//自定义函数
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void itoa(int n, char s[]) {
    int i, sign;
    if ((sign = n) < 0) n = -n;  // 记录符号
    i = 0;
    do {  // 生成数字的逆序
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';
}

int atoi(const char *str) {
    int res = 0;
    int sign = 1;
    int i = 0;
    if (str[0] == '-') {
        sign = -1;
        i++;
    }
    for (; str[i] != '\0'; ++i)
        res = res * 10 + str[i] - '0';
    return sign * res;
}

long strtol(const char *str, char **endptr, int base) {
    long result = 0;
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }
    while (*str) {
        int digit = *str - '0';
        if (digit < 0 || digit >= base) break;
        result = result * base + digit;
        str++;
    }
    if (endptr) *endptr = (char *)str;
    return sign * result;
}


void hinit() {
    initlock(&heapLock, "heapLock");
    heapList = (blockNode *)HEAPSTART;
    heapList->size = HEAPSIZE;
    heapList->free = 1;
    heapList->id = 0;  // id为0表示初始的整个堆块
    heapList->next = NULL;
}

void *halloc(size_t size) {
    size = ALIGN(size);  // 对传入的 size 进行对齐
    blockNode *current = heapList;
    acquire(&heapLock);
    while (current) {
        if (current->free && current->size >= size) {  // 找到一个空闲且大小足够的内存块
            if (current->size >= size + sizeof(blockNode)) {
                blockNode *newBlock = (blockNode *)((char *)current + size);
                newBlock->size = current->size - size;
                newBlock->free = 1;
                newBlock->id = 0;  // 初始的id为0
                newBlock->next = current->next;
                current->next = newBlock;
                current->size = size;
            }
            current->free = 0;  // 标记为已分配
            current->id = next_id++;  // 分配唯一的序号
            release(&heapLock);
            return (void *)(current);  // 返回分配的内存块
        }
        current = current->next;
    }
    release(&heapLock);
    return NULL;  // 没有找到足够大的空闲内存块，返回 NULL
}

void hfree_by_id(int id) {
    if (id == 0) return;  // 不允许释放初始的整个堆块
    acquire(&heapLock);
    blockNode *current = heapList;
    blockNode *prev = NULL;
    while (current) {
        if (current->id == id) {
            current->free = 1;  // 将内存块标记为空闲
            // 尝试合并相邻的空闲块
            if (prev && prev->free) {
                prev->size += current->size;
                prev->next = current->next;
                current = prev;
            }
            if (current->next && current->next->free) {
                current->size += current->next->size;
                current->next = current->next->next;
            }
            release(&heapLock);
            return;
        }
        prev = current;
        current = current->next;
    }
    release(&heapLock);
}

void print_state() {
    blockNode *current = heapList;
    while (current) {
        printf("内存块序号: %d, 起始地址: %p, 大小: %d, 使用状态: %d\n", current->id, current, current->size, current->free);
        current = current->next;
    }
}

void my_heap() {
    char buf[128];
    int n;
    hinit();
    while (1) {
        printf("输入命令 (1: 分配内存块, 2: 释放内存块, 3: 退出): ");
        n = consoleread(0, (uint64)buf, sizeof(buf));
        buf[n] = '\0';  // 确保字符串结束
        if (strcmp(buf, "1\n") == 0) {
            printf("输入要分配的大小: ");
            n = consoleread(0, (uint64)buf, sizeof(buf));
            buf[n] = '\0';
            int size = (int)strtol(buf, NULL, 10);
            void *ptr = halloc(size);
            if (ptr != NULL) {
                blockNode *block = (blockNode *)ptr;
                printf("分配成功，地址: %p, 序号: %d\n", ptr, block->id);
            } else {
                printf("分配失败\n");
            }
            print_state();
        } else if (strcmp(buf, "2\n") == 0) {
            printf("输入要释放的序号: ");
            n = consoleread(0, (uint64)buf, sizeof(buf));
            buf[n] = '\0';
            int id = (int)strtol(buf, NULL, 10);
            hfree_by_id(id);
            printf("释放成功\n");
            print_state();
        } else if (strcmp(buf, "3\n") == 0) {
            printf("退出程序\n");
            break;
        } else {
            printf("无效的命令\n");
        }
    }
}