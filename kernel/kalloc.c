// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define HEAP_SIZE (8 * 1024 * 1024) // 8MB堆

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel, defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// 堆的起始地址和大小
char *heap_start;
char *heap_end;
char *heap_free_start;

void
kinit()
{
  initlock(&kmem.lock, "kmem");

  // 扣减8MB作为堆
  heap_start = (char*)PHYSTOP - HEAP_SIZE;
  heap_end = (char*)PHYSTOP;
  heap_free_start = heap_start;

  freerange(end, (void*)heap_start);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= (uint64)heap_start)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// 简单的字节级分配函数
void *
malloc(uint64 size)
{
  char *ptr;

  if (size == 0 || heap_free_start + size > heap_end)
    return 0;

  ptr = heap_free_start;
  heap_free_start += size;

  return (void*)ptr;
}

// 简单的字节级释放函数（暂不实现复杂的空闲列表管理）
void
free(void *ptr)
{
  // 不实现复杂的释放逻辑
}

// 演示函数
void
heap_demo()
{
  printf("Heap demo start\n");

  void *p1 = malloc(1024);
  printf("Allocated 1024 bytes at %p\n", p1);

  void *p2 = malloc(2048);
  printf("Allocated 2048 bytes at %p\n", p2);

  free(p1);
  printf("Freed 1024 bytes at %p\n", p1);

  void *p3 = malloc(4096);
  printf("Allocated 4096 bytes at %p\n", p3);

  free(p2);
  printf("Freed 2048 bytes at %p\n", p2);

  free(p3);
  printf("Freed 4096 bytes at %p\n", p3);

  printf("Heap demo end\n");
}
