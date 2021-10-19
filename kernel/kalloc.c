// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  void *vm_start;             // the start of free memory.
  uint8 rec_cnt[8 * PGSIZE];   // maintain the reference count of physical memory.
} kmem;

// compute the index of pm pages.
int
get_index(void* addr) {
  return (PGROUNDDOWN((uint64)addr) - (uint64)kmem.vm_start) / PGSIZE;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  for (int i = 0; i < 8 * PGSIZE; ++i) kmem.rec_cnt[i] = 1;
  kmem.vm_start = (void*)PGROUNDUP((uint64)end);
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// 增加对一个物理内存的引用
void
increase_reference(void *pa) {
  acquire(&kmem.lock);
  int idx = get_index(pa);
  if (idx < 0 || idx >= 8 * PGSIZE || kmem.rec_cnt[idx] == 0)
    panic("increase reference: error!, out of bound!");
  ++kmem.rec_cnt[idx];
  release(&kmem.lock);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);

  // 查看当前物理页的引用计数
  int idx = get_index(pa);
  if (idx < 0 || idx >= 8 * PGSIZE || kmem.rec_cnt[idx] == 0)
    panic("real_kfree: error!");
  --kmem.rec_cnt[idx];
  // 引用计数不是零
  if (kmem.rec_cnt[idx] != 0) {
    release(&kmem.lock);
    return;
  }

  struct run *r;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
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
  if(r) {
    kmem.freelist = r->next;
    ++kmem.rec_cnt[get_index(r)];
    if (kmem.rec_cnt[get_index(r)] != 1)
      panic("kalloc: the refcount shoule be 1!");
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
