#include "bootpack.h"
#include <stdio.h>

extern fifo8 keyfifo;
extern fifo8 mousefifo;

extern timer_t timerctl;

void HariMain(void)
{
  char s[40];
  struct MOUSE_DEC mdec;
  int mx, my;
  int i;
  unsigned int memtotal;
	unsigned int count;
	fifo8 timerfifo;

  char keybuf[KEYBUF_SIZE] = {0};
  char mousebuf[MOUSEBUF_SIZE] = {0};
	char timerbuf[8] = {0};
  struct BOOTINFO *binfo;
  struct MEMMAN *memman;

  shtctl_t *shtctl;
  sheet_t *sht_back, *sht_mouse, *sht_win;
  unsigned char *buf_back, buf_mouse[256], *buf_win;

  binfo = (struct BOOTINFO *) ADR_BOOTINFO;
  memman = (struct MEMMAN *)MEMMAN_ADDR;
	count = 0;

  init_gdtidt ();
  init_pic();
  io_sti();
  fifo8_init(&keyfifo, KEYBUF_SIZE, keybuf); /* keyboard */
  fifo8_init(&mousefifo, MOUSEBUF_SIZE, mousebuf); /* mouse */
	
	init_pit();

	fifo8_init(&timerfifo, 8, timerbuf);       /* timer */
	settimer(100, &timerfifo, 1);
	
  io_out8(PIC0_IMR, 0xf8); /* PIC1/keyboard/PIT设置为许可(11111000) */
  io_out8(PIC1_IMR, 0xef); /* 鼠标设置为许可(11101111) */

  init_keyboard ();
  enable_mouse(&mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

  init_palette();

  shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
  sht_back = sheet_alloc(shtctl);
  sht_mouse = sheet_alloc(shtctl);
	sht_win  = sheet_alloc(shtctl);
	
  buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win  = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	
  sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
  sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);

  init_screen8(buf_back, binfo->scrnx, binfo->scrny);
  init_mouse_cursor8(buf_mouse, 99);
	make_window8(buf_win, 160, 52, "counter");
	
  sheet_slide(sht_back, 0, 0);

	/* make it in the middle. */
  mx = (binfo->scrnx - 16) / 2;
  my = (binfo->scrny - 28 - 16) / 2;
  //init_mouse_cursor8(mcursor, COL8_008484);
  sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	
  sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
  sheet_updown(sht_mouse, 2);

  //putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
  sprintf(s, "(%3d, %3d)", mx, my);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	sprintf(s, "memory %dMB   free : %dKB",
			    memtotal / (1024 * 1024), memman_total(memman) / 1024);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
  sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

  for (;;) {
		++count;
		sprintf(s, "%u, %u", timerctl.count, timerctl.timeout);
		boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
		putfonts8_asc(buf_win, 160, 40, 28, COL8_000000, s);
		sheet_refresh(sht_win, 40, 28, 120, 44);
		
    io_cli();
    if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo) == 0){
      io_sti();
    } else {
      if (fifo8_status(&keyfifo) != 0) {
        i = fifo8_get(&keyfifo);
        io_sti();
        sprintf(s, "%02X", i);
				boxfill8(buf_back, binfo->scrnx, COL8_008484,  0, 16, 15, 31);
				putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        sheet_refresh(sht_back, 0, 16, 16, 32);
      } else if (fifo8_status(&mousefifo) != 0){
        i = fifo8_get(&mousefifo);
        io_sti();
        if (mouse_decode(&mdec, i) != 0) {
          sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
          if ((mdec.btn & 0x01) != 0)
            s[1] = 'L';
          if ((mdec.btn & 0x02) != 0)
            s[3] = 'R';
          if ((mdec.btn & 0x04) != 0)
            s[2] = 'C';
          boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
          putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
          sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);

          /* 鼠标指针的移动 */
          //boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* 隐藏鼠标 */
          mx += mdec.x;
          my += mdec.y;
          if (mx < 0)
            mx = 0;
          if (my < 0)
            my = 0;
          if (mx > binfo->scrnx - 1)
            mx = binfo->scrnx - 1;
          if (my > binfo->scrny - 1)
            my = binfo->scrny - 1;

          sprintf(s, "(%3d, %3d)", mx, my);
          boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); /* 隐藏坐标 */
          putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); /* 显示坐标 */
          sheet_refresh(sht_back, 0, 0, 80, 16);
          sheet_slide(sht_mouse, mx, my);
          //putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /* 描画鼠标 */
        }
      } else if (fifo8_status(&timerfifo) != 0) {
				i = fifo8_get(&timerfifo);
				io_sti();
				putfonts8_asc(buf_back, binfo->scrnx, 0, 64, COL8_FFFFFF, "10[sec]");
				sheet_refresh(sht_back, 0, 64, 56, 80);
			}
    }
  }
}




