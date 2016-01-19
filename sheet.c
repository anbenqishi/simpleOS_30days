#include <stdio.h>
#include "bootpack.h"

shtctl_t *shtctl_init(memman_t *memman, unsigned char *vram, int xsize, int ysize)
{
  shtctl_t *ctl;
  int i;
  ctl = (shtctl_t *)memman_alloc_4k(memman, sizeof (shtctl_t));
  if (ctl == NULL)
    goto err;

	ctl->map = (unsigned char *)memman_alloc_4k(memman, xsize * ysize);
	if (ctl->map == NULL)
		{
			memman_free_4k(memman, (unsigned int)ctl, sizeof(shtctl_t));
			goto err;
		}
	
  ctl->vram = vram;
  ctl->xsize = xsize;
  ctl->ysize = ysize;
  ctl->top   = -1; /* 没有一张sheet */
  for (i = 0; i < MAX_SHEETS; ++i) {
    ctl->sheets0[i].flags = 0; /* 未使用标记 */
    ctl->sheets0[i].ctl   = ctl; /* 记录所属 */
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

static void sheet_refreshmap(shtctl_t *ctl, int vx0, int vy0,
	                    int vx1, int vy1, int h0)
{
  int h, bx, by, vx, vy;
  int bx0, by0, bx1, by1;
  unsigned char *buf;
	unsigned char sheet_id;
	unsigned char *map;
  sheet_t *sht;

	map = ctl->map;

  if (vx0 < 0)
    vx0 = 0;
  if (vy0 < 0)
    vy0 = 0;
  if (vx1 > ctl->xsize)
    vx1 = ctl->xsize;
  if (vy1 > ctl->ysize)
    vy1 = ctl->ysize;

  for (h = h0; h <= ctl->top; ++h) {
    sht = ctl->sheets[h];
		sheet_id = sht - ctl->sheets0; /* 将此值作为图层号码使用. */
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
        if (buf[by * sht->bxsize + bx] != sht->col_inv) {
        	map[vy * ctl->xsize + vx] = sheet_id;
        }
      }
    }
  }
	return;
}

/**
 * h0: 只对此参数以上的图层进行刷新
 */
static void sheet_refreshsub(shtctl_t *ctl, int vx0, int vy0, 
                                            int vx1, int vy1, 
                                            int h0, int h1)
{
  int h, bx, by, vx, vy;
  int bx0, by0, bx1, by1;
  unsigned char *buf;
	unsigned char *vram;
	unsigned char sheet_id;
	unsigned char *map;
  sheet_t *sht;

	map = ctl->map;
	vram = ctl->vram;
	
  if (vx0 < 0)
    vx0 = 0;
  if (vy0 < 0)
    vy0 = 0;
  if (vx1 > ctl->xsize)
    vx1 = ctl->xsize;
  if (vy1 > ctl->ysize)
    vy1 = ctl->ysize;

  for (h = h0; h <= h1; ++h) {
    sht = ctl->sheets[h];
    buf = sht->buf;

		sheet_id = sht - ctl->sheets0;

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
        if (map[vy * ctl->xsize + vx] == sheet_id) {
          vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
        }
      }
    }
  }
}

void sheet_refresh(sheet_t *sht, int bx0, int by0, int bx1, int by1)
{
	/* 如果正在显示, 则按新图层的信息进行刷新. */
  if (sht->height >= 0)
    sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, 
                               sht->vx0 + bx1, sht->vy0 + by1,
                               sht->height, sht->height);
  return;
}

/*
 * 设定底板高度
 */
void sheet_updown(sheet_t *sht, int height)
{
  int h;
  int old = sht->height;
  shtctl_t *ctl = sht->ctl;

  if (height > ctl->top + 1)
    height = ctl->top + 1;

  if (height < -1)
    height = -1;

  sht->height = height;

  if (old > height) {
		/* 中间的提起. */
    if (height >= 0) {
      for (h = old; h > height; --h) {
        ctl->sheets[h] = ctl->sheets[h - 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0,
				               sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize,
				               height + 1);
	    sheet_refreshsub(ctl, sht->vx0, sht->vy0, 
	                    sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize,
	                    height + 1, old); /* 重绘 */
    } else {
      if (ctl->top > old) {
        for (h = old; h < ctl->top; ++h) {
          ctl->sheets[h] = ctl->sheets[h + 1];
          ctl->sheets[h]->height = h;
        }
      }
      --ctl->top;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0,
				               sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize,
				               0);
			
    	sheet_refreshsub(ctl, sht->vx0, sht->vy0, 
			                    sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize,
			                    0, old - 1); /* 重绘 */
    	}
		/* 比以前高. */
  } else if (old < height) {
    if (old >= 0) {
			/* 中间的图层往下降一层. */
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
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, 
			               sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize,
			               height);
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, 
			                    sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize,
			                    height, height);
  }
  return;
}

/*
 * 不改变图层高度而只上下左右移动图层
 */
void sheet_slide(sheet_t *sht, int vx0, int vy0)
{
  int old_vx0 = sht->vx0;
  int old_vy0 = sht->vy0;

  sht->vx0 = vx0;
  sht->vy0 = vy0;

	/* 如果正在显示, 则按新图层的信息进行刷新. */
  if (sht->height >= 0)
  {
  	sheet_refreshmap(sht->ctl, old_vx0, old_vy0,
			               old_vx0 + sht->bxsize, old_vy0 + sht->bysize,
			               0);
		sheet_refreshmap(sht->ctl, vx0, vy0,
			               vx0 + sht->bxsize, vy0 + sht->bysize,
			               sht->height);
		
    sheet_refreshsub(sht->ctl, old_vx0, old_vy0, 
			               old_vx0 + sht->bxsize, old_vy0 + sht->bysize,
			               0, sht->height - 1);
    sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize,
			               sht->height, sht->height);
  }
}

void sheet_free(sheet_t *sht)
{
  if (sht->height >= 0)
    sheet_updown(sht, -1); //如果处于显示状态, 则先设定为隐藏

  sht->flags = 0;
  return;
}
