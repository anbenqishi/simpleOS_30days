#include <stdio.h>
#include "bootpack.h"

shtctl_t *shtctl_init(memman_t *memman, unsigned char *vram, int xsize, int ysize)
{
  shtctl_t *ctl;
  int i;
  ctl = (shtctl_t *)memman_alloc_4k(memman, sizeof (shtctl_t));
  if (ctl == NULL)
    goto err;

  ctl->vram = vram;
  ctl->xsize = xsize;
  ctl->ysize = ysize;
  ctl->top   = -1;
  for (i = 0; i < MAX_SHEETS; ++i) {
    ctl->sheets0[i].flags = 0;
  }

err:
  return ctl;
}

sheet_t *sheet_alloc(shtctl_t *ctl)
{
  sheet_t *sht;
  int i;
  for (i = 0; i < MAX_SHEETS; ++i) {
    if (ctl->sheets0[i].flags != 0)
      continue;

    sht = &ctl->sheets0[i];
    sht->flags = SHEET_USE;
    sht->height = -1; /* 隐藏 */
    return sht;
  }
  return NULL;
}

void sheet_setbuf(sheet_t *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
  sht->buf = buf;
  sht->bxsize = xsize;
  sht->bysize = ysize;
  sht->col_inv = col_inv;
  return;
}

void sheet_refreshsub(shtctl_t *ctl, int vx0, int vy0, int vx1, int vy1)
{
  int h, bx, by, vx, vy;
  int bx0, by0, bx1, by1;
  unsigned char *buf, c, *vram = ctl->vram;
  sheet_t *sht;

  for (h = 0; h <= ctl->top; ++h) {
    sht = ctl->sheets[h];
    buf = sht->buf;

    bx0 = vx0 - sht->vx0;
    by0 = vy0 - sht->vy0;
    bx1 = vx1 - sht->vx0;
    by1 = vy1 - sht->vy0;
    if (bx0 < 0)
      bx0 = 0;
    if (by0 < 0)
      by0 = 0;
    if (bx1 > sht->bxsize)
      bx1 = sht->bxsize;
    if (by1 > sht->bysize)
      by1 = sht->bysize;

    for (by = by0; by < by1; ++by) {
      vy = sht->vy0 + by;
      for (bx = bx0; bx < bx1; ++bx) {
        vx = sht->vx0 + bx;
        if (vx0 <= vx && vx < vx1 && vy0 <= vy && vy < vy1) {
          c = buf[by * sht->bxsize + bx];
          if (c != sht->col_inv)
            vram[vy * ctl->xsize + vx] = c;
        }
      }
    }
  }
}

void sheet_refresh(shtctl_t *ctl, sheet_t *sht, int bx0, int by0, int bx1, int by1)
{
  if (sht->height >= 0)
    sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
  return;
}

void sheet_updown(shtctl_t *ctl, sheet_t *sht, int height)
{
  int h;
  int old = sht->height;

  if (height > ctl->top + 1)
    height = ctl->top + 1;

  if (height < -1)
    height = -1;

  sht->height = height;

  if (old > height) {
    if (height >= 0) {
      for (h = old; h > height; --h) {
        ctl->sheets[h] = ctl->sheets[h - 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else {
      if (ctl->top > old) {
        for (h = old; h < ctl->top; ++h) {
          ctl->sheets[h] = ctl->sheets[h + 1];
          ctl->sheets[h]->height = h;
        }
      }
      --ctl->top;
    }
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize); /* 重绘 */
  } else if (old < height) {
    if (old >= 0) {
      for (h = old; h < height; ++h) {
        ctl->sheets[h] = ctl->sheets[h + 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else { //由隐藏状态转为显示状态
        for (h = ctl->top; h >= height; --h) {
          ctl->sheets[h + 1] = ctl->sheets[h];
          ctl->sheets[h + 1]->height = h + 1;
        }
        ctl->sheets[height] = sht;
        ctl->top++;
    }
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
  }
  return;
}

void sheet_slide(shtctl_t *ctl, sheet_t *sht, int vx0, int vy0)
{
  int old_vx0 = sht->vx0;
  int old_vy0 = sht->vy0;

  sht->vx0 = vx0;
  sht->vy0 = vy0;

  if (sht->height >= 0)
  {
    sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
    sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
  }
}

void sheet_free(shtctl_t *ctl, sheet_t *sht)
{
  if (sht->height >= 0)
    sheet_updown(ctl, sht, -1); //如果处于显示状态, 则先设定为隐藏

  sht->flags = 0;
  return;
}
