#include "bootpack.h"

void memman_init(struct MEMMAN *man)
{
  man->frees = 0;    // 可用信息数目
  man->maxfrees = 0; // 用于观察可用状况, frees的最大值
  man->lostsize = 0; // 释放失败的内存的大小总和
  man->losts = 0;    // 释放失败的次数
  return;
}

// 报告空余内存大小的合计
unsigned int memman_total(struct MEMMAN *man)
{
  unsigned int i;
  unsigned int t = 0;

  for(i = 0; i < man->frees; ++i) {
    t += man->free[i].size;
  }
  return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
  unsigned int i, a;

  for (i = 0; i < man->frees; ++i)
    {
      if (man->free[i].size >= size) {
        // 找到了足够大的内存
        a = man->free[i].addr;
        man->free[i].addr += size;
        man->free[i].size -= size;
        if (man->free[i].size == 0) {
          man->frees--;
          for (; i < man->frees; ++i)
            man->free[i] = man->free[i + 1];
        }
        return a;
      }
    }
  return 0;
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
  unsigned int a;
  size = (size + 0xfff) & 0xfffff000;
  a = memman_alloc(man, size);
  return a;
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
  int i, j;
  for (i = 0; i < man->frees; ++i)
  {
    if (man->free[i].addr > addr)
      break;
  }

  // free[i-1].addr < addr < free[i].addr
  if (i > 0) {
    if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
      man->free[i - 1].size += size;
      if (i < man->frees) {
        if (addr + size == man->free[i].addr) {
          man->free[i - 1].size += man->free[i].size;
          man->frees--;
          for (; i < man->frees; ++i)
            man->free[i] = man->free[i + 1];
        }
      }
      return 0;
    }
  }

  if (i < man->frees) {
    if (addr + size == man->free[i].addr) {
      man->free[i].addr = addr;
      man->free[i].size += size;
      return 0;
    }
  }

  if (man->frees < MEMMAN_FREES) {
    for (j = man->frees; j > i; --j) {
      man->free[j] = man->free[j - 1];
    }
    ++man->frees;
    if (man->maxfrees < man ->frees)
      man->maxfrees = man->frees;
    man->free[i].addr = addr;
    man->free[i].size = size;
    return 0;
  }

  man->losts++;
  man->lostsize += size;

  return -1;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
  int i;
  size = (size + 0xfff) & 0xfffff000; /* round up. */
  i = memman_free(man, addr, size);
  return i;
}
