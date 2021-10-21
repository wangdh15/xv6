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
} kmem[NCPU];

char lock_names[NCPU][8];


void
kinit()
{
  for (int i = 0; i < NCPU; ++i) {
    snprintf(&lock_names[i][0], 8, "kmem%d", i);
    initlock(&kmem[i].lock, &lock_names[i][0]);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  struct run* r;
  p = (char*)PGROUNDUP((uint64)pa_start);
  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmem[id].lock);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    r = (struct run*)p;
    r->next = kmem[id].freelist;
    kmem[id].freelist = r;
  }
  release(&kmem[id].lock);

}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.

void*
alloc_from_id(int id) {

  struct run *r;
  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if (r)
    kmem[id].freelist = r -> next;
  release(&kmem[id].lock);
  return r;
}

void *
kalloc(void)
{

  push_off();
  int id = cpuid();
  pop_off();

  void *r;
  for (int i = 0; i < NCPU; ++i) {
    r = alloc_from_id(id);
    if (r) break;
    else id = (id + 1) % NCPU;
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
