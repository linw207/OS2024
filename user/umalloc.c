#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;    // 指向下一个内存块的指针
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;   // 基础头部，用于初始化空闲链表
static Header *freep;   // 指向空闲链表的开始

// 释放已分配的内存块并将其插入到空闲链表中
void
free(void *ap)
{
  Header *bp, *p;   

  bp = (Header*)ap - 1;   // 将ap指针转换为 Header 指针，并将bp指向该内存块的Header
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){    // 向后合并相邻的空闲块
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){    // 向前合并相邻的空闲块
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

// 向操作系统请求更多内存，并将其添加到空闲链表中。
static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;

  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));  // 请求更多内存
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));  // 将新内存块添加到空闲链表
  return freep;
}

// 分配指定大小的内存块
void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;   // 计算所需的Header单元数
  if((prevp = freep) == 0){ // 初始化空闲链表
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){  // 找到足够大的空闲块，首次适应算法
      if(p->s.size == nunits) // 刚好大小合适
        prevp->s.ptr = p->s.ptr;
      else {    // 分割空闲块
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)    // 如果循环一圈没有找到合适的块，请求更多内存
      if((p = morecore(nunits)) == 0)
        return 0;
  }
}



