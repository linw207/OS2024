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
    struct block *next;//指向链表中的下一个内存块。
    size_t size;//当前内存块的大小。
    int free;       // 1空闲， 0分配
} blockNode;
static blockNode *heapList;//heapList 是指向链表头部的指针，表示堆的起始地址。
//整个堆内存空间被组织成一个由 blockNode 组成的单向链表，链表中的每个节点表示一个内存块。
static struct spinlock heapLock;//spinlock 结构体用于实现互斥锁，以确保在多核环境下进行内存分配和释放操作时的线程安全。

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
    size = ALIGN(size);// 对传入的 size 进行对齐
    blockNode *current = heapList;
    acquire(&heapLock);
    while (current) {
        if (current->free && current->size >= size) {// 找到一个空闲且大小足够的内存块
            if (current->size >= size + ALIGNMENT) {
                //大小大于等于 size + ALIGNMENT，则将其分割成两块，一块用于分配，另一块作为新的空闲块。
                blockNode *newBlock = (blockNode *)((char *)current + size);
                newBlock->size = current->size - size;
                newBlock->free = 1;//标记为未分配分配
                newBlock->next = current->next;
                current->next = newBlock;
                current->size = size;
            }
            current->free = 0;//已分配
            release(&heapLock);
            printf("完成分配！\n");
            return (void *)(current);//返回该内存块的指针。
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

    if (!ptr ) return;// 如果指针为 NULL，直接返回

    block->free = 1;//标记为未分配

    // 空闲区合并操作
    acquire(&heapLock);
    blockNode *current = heapList;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += current->next->size;//合并
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
    
    void *t1, *t2, *t3,*t4,*t5;
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
    
    printf("give 16MB\n");
    t4 = halloc(16*1024*1024);
    if(t4 != NULL){
    print_state();
    printf(t4);
    hfree(t4);
    printf("释放t4\n");
    }

    printf("give 12MB\n");
    t5 = halloc(12*1024*1024);
    if(t5 != NULL){
    print_state();
    printf(t5);
    hfree(t5);
    printf("释放t5\n");
    }
}