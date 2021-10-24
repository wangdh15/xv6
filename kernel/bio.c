// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct buf buf[NBUF];
  struct buf head[BUCKETNUM];
  struct spinlock locks[BUCKETNUM];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

void
binit(void) {

  struct buf *b;

  for (int i = 0; i < BUCKETNUM; ++i) {
    initlock(&bcache.locks[i], "bcache.bucket");
    bcache.head[i].next = &bcache.head[i];
    bcache.head[i].prev = &bcache.head[i];
  }

  for (int i = 0; i < NBUF; ++i) {
    int idx = i % BUCKETNUM;
    b = bcache.buf + i;
    b->next = bcache.head[idx].next;
    b->prev = &bcache.head[idx];
    b->next->prev = b;
    b->prev->next = b;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = blockno % BUCKETNUM;
  struct buf *find = 0;

  acquire(&bcache.locks[idx]);

  // 查找已经使用的列表中是否有
  for (b = bcache.head[idx].next; b != &bcache.head[idx]; b = b -> next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt ++;
      release(&bcache.locks[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // acquire(&bcache.locks_free[idx]);

  for (int i = 0; i < BUCKETNUM / 2; ++i) {
    int new_idx = (idx + i) % BUCKETNUM;
    if (i != 0) acquire(&bcache.locks[new_idx]);
    for (b = bcache.head[new_idx].next; b != &bcache.head[new_idx]; b = b -> next) {
      if (b->refcnt == 0) {
        find = b;
        b->prev->next = b->next;
        b->next->prev = b->prev;
        if (i != 0) release(&bcache.locks[new_idx]);
        goto find_out;
      }
    }
    if (i != 0) release(&bcache.locks[new_idx]);
  }

  // 没找到的话直接panic
  if (find == 0) {
    panic("bget: no buffers");
    // return find;
  } else  {
    find_out:
        // 将找到的快插入到used链表中
    find->dev = dev;
    find->blockno = blockno;
    find->valid = 0;
    find->refcnt = 1;
    find->next = bcache.head[idx].next;
    find->prev = &bcache.head[idx];
    find->prev->next = find;
    find->next->prev = find;
    release(&bcache.locks[idx]);
    acquiresleep(&find->lock);
    return find;
  }

}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int idx = (b->blockno) % BUCKETNUM;
  // 维护引用计数
  acquire(&bcache.locks[idx]);
  b->refcnt--;
  // 如果引用计数为零的话，就将其放回到对应的free列表中
  release(&bcache.locks[idx]);
}

void
bpin(struct buf *b) {
  int idx = (b->blockno) % BUCKETNUM;
  acquire(&bcache.locks[idx]);
  b->refcnt++;
  release(&bcache.locks[idx]);
}

void
bunpin(struct buf *b) {
  int idx = (b->blockno) % BUCKETNUM;
  acquire(&bcache.locks[idx]);
  b->refcnt--;
  release(&bcache.locks[idx]);
}
