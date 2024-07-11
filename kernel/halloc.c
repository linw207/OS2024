#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "stddef.h"
// #include "halloc.h"

#define HEAPSIZE 16*1024*1024
#define ALIGNMENT 8     // 8字节对齐
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & (~(ALIGNMENT - 1)))

typedef struct block {
    struct block *next;
    size_t size;
    int free;       // 1空闲， 0分配
} blockNode;
static blockNode *heapList;
static struct spinlock heapLock;

// 堆内存分配器初始化函数
void hinit()
{
    initlock(&heapLock, "heapLock");
    heapList = (blockNode *)HEAPSTART;
    heapList->size = HEAPSIZE;
    heapList->free = 1;
    heapList->next = NULL;
}

// 堆分配函数：首次适应  
void *halloc(size_t size)
{
    size = ALIGN(size);
    blockNode *current = heapList;
    acquire(&heapLock);
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size >= size + ALIGNMENT) {
                blockNode *newBlock = (blockNode *)((char *)current + size);
                newBlock->size = current->size - size;
                newBlock->free = 1;
                newBlock->next = current->next;
                current->next = newBlock;
                current->size = size;
            }
            current->free = 0;
            release(&heapLock);
            printf("完成分配！\n");
            return (void *)(current);
        }
        current = current->next;
    }
    release(&heapLock);
    printf("空间不足！无法完成分配\n");
    return NULL; 
}

// 块释放函数：每次释放会执行一次合并操作
void hfree(void *ptr)
{

    blockNode *block = (blockNode *)ptr;

    if (!ptr ) return;

    block->free = 1;

    // 空闲区合并操作
    acquire(&heapLock);
    blockNode *current = heapList;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
    release(&heapLock);
}

// 调试函数
void print_state()
{
    blockNode *current = heapList;
    while (current) {
        printf("内存块起始地址为： %p, 大小为： %d, 使用状态： %d\n", current, current->size, current->free);
        current = current->next;
    }
}


// 测试函数
void
my_heap() {
// void *test1, *test2, *test3, *test4;
    
    // printf("分配100B的内存\n");
    // test1 = halloc(100);

    // printf("分配123B的内存\n");
    // test2 = halloc(123);


    // printf("分配200B的内存\n");
    // test3 = halloc(200);

    // print_state();

    // hfree(test2);
    // printf("释放%p起123B的内存分配\n");
    // print_state();

    // printf("在 %p位置处重新进行60B内存的分配\n");
    // test2 = halloc(60);
    // print_state();

    // printf("分配16MB内存\n");
    // test4 = halloc(16*1024*1024);


    // hfree(test1);
    // hfree(test2);
    // hfree(test3);
    // printf("释放所有的内存分配\n",test4);
    // print_state();
    void *t1, *t2, *t3;
    printf("give 100\n");
    t1 = halloc(100);
    print_state();

    printf("give 200\n");
    t2 = halloc(200);
    print_state();
    hfree(t1);
    printf("释放t1\n");
    hfree(t2);
    printf("释放t2\n");

    printf("give 300\n");
    t3 = halloc(300);
    print_state();
    printf(t3);
    hfree(t3);
}


