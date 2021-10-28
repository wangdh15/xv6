#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

uint64
sys_mmap(void) {
  // void *addr, int length, int prot, int flags, int fd, int offset
  // return void*

  // 获取对应的参数
  uint64 addr_src;
  int length, prot, flags, fd, offset;
  struct file *f;

  struct proc *p = myproc();

  if (argaddr(0, &addr_src) < 0 ||
      argint(1, &length) < 0 ||
      argint(2, &prot) < 0 ||
      argint(3, &flags) < 0 ||
      argfd(4, &fd, &f) < 0 ||
      argint(5, &offset) < 0) {
    return -1;
  }

  // 校验参数
  // 如果打开的文件描述符不可读，或者映射的页可写但是文件不可写，且是SHARED模式
  // 则之间返回错误
  if (f->readable == 0 || (f->writable == 0 && (prot & PROT_WRITE) != 0 && (flags & MAP_SHARED) != 0)) {
    return -1;
  }

  // 分配一个空闲的vma
  struct vma* vma_ = proc_getfreevma(myproc());
  if (vma_ == 0) return -1;
  if (addr_src == 0) {

    // 从虚拟地址空间的后面分配一些地址给这个映射的文件
    int alloc_length = PGROUNDUP(length);
    vma_->addr_src = p->mmap_end - alloc_length;
    p->mmap_end -= alloc_length;
  } else {
    vma_->addr_src = addr_src;
  }
  vma_->length = length;
  vma_->prot = prot;
  vma_->flags = flags;
  vma_->f = f;
  vma_->offset = offset;

  // 增加对f的引用计数
  filedup(f);

  return vma_->addr_src;

}



uint64
sys_munmap(void) {
  // void *addr, int length
  // return int
  // return -1;

  uint64 addr_src;
  int length;
  if (argaddr(0, &addr_src) < 0 || argint(1, &length) < 0) {
    printf("sys_munmap: parse parameters error!\n");
    return -1;
  }
  return munmap(addr_src, length);
}
