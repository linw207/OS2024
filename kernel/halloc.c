#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "stddef.h"

#define HEAPSIZE (16 * 1024 * 1024) // 16MB for Heap
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & (~(ALIGNMENT - 1)))

typedef struct block {
    struct block *next;
    size_t size;
    int free; // 1空闲， 0分配
} blockNode;

static blockNode *heapList;
static struct spinlock heapLock;

// 堆内存分配器初始化函数
void hinit() {
    initlock(&heapLock, "heapLock");
    heapList = (blockNode *)HEAPSTART;
    heapList->size = HEAPSIZE - sizeof(blockNode); // 确保堆大小正确
    heapList->free = 1;
    heapList->next = NULL;
}

// 堆分配函数：首次适应
void *halloc(size_t size) {
    size = ALIGN(size);
    blockNode *current = heapList;
    acquire(&heapLock);
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size >= size + sizeof(blockNode) + ALIGNMENT) {
                blockNode *newBlock = (blockNode *)((char *)current + sizeof(blockNode) + size);
                newBlock->size = current->size - size - sizeof(blockNode);
                newBlock->free = 1;
                newBlock->next = current->next;
                current->next = newBlock;
                current->size = size;
            }
            current->free = 0;
            release(&heapLock);
            printf("完成分配！\n");
            return (void *)(current + 1);
        }
        current = current->next;
    }
    release(&heapLock);
    printf("空间不足！无法完成分配\n");
    return NULL;
}

// 块释放函数：每次释放会执行一次合并操作
void hfree(void *ptr) {
    if (!ptr) return;
    blockNode *block = (blockNode *)ptr - 1;
    block->free = 1;

    // 空闲区合并操作
    acquire(&heapLock);
    blockNode *current = heapList;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += current->next->size + sizeof(blockNode);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
    release(&heapLock);
}

// 调试函数
void print_state() {
    blockNode *current = heapList;
    while (current) {
        printf("内存块起始地址为： %p, 大小为： %d, 使用状态： %d\n", (void *)current, current->size, current->free);
        current = current->next;
    }
}

// 测试函数
void my_heap() {
    void *t1, *t2, *t3;
    hinit(); // 初始化堆
    printf("分配100字节内存\n");
    t1 = halloc(100);
    printf("分配200字节内存\n");
    t2 = halloc(200);
    printf("分配300字节内存\n");
    t3 = halloc(300);
    print_state();

    hfree(t1);
    hfree(t2);
    print_state();

    printf("分配16MB内存\n");
    void *t4 = halloc(16 * 1024 * 1024);
    print_state();

    hfree(t3);
    hfree(t4);
    print_state();
}
