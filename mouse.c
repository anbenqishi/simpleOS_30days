#include "bootpack.h"

fifo8 mousefifo;

void inthandler2c(int *esp)
{
  unsigned char data;

  io_out8(PIC1_OCW2, 0x64);
  io_out8(PIC0_OCW2, 0x62);
  data = io_in8(PORT_KEYDAT);
  fifo8_put(&mousefifo, data);

  return;
}

void enable_mouse(struct MOUSE_DEC *mdec)
{
  wait_kbc_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_kbc_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);

  mdec->phase = 0;
  return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
  switch (mdec->phase){
  case 0:
    /* 等待鼠标的0xfa的阶段 */
    if (dat == 0xfa) /* wait for 0xfa status */
      mdec->phase = 1;
    return 0;
  case 1: /* 等待鼠标第一字节的阶段 */
    if ((dat & 0xc8) == 0x08){
      mdec->buf[0] = dat;
      mdec->phase = 2;
    }
    return 0;
  case 2: /* 等待鼠标第二字节的阶段 */
    mdec->buf[1] = dat; /* left/right */
    mdec->phase = 3;
    return 0;
  case 3: /* 等待鼠标第三字节的阶段 */
    mdec->buf[2] = dat; /* up/down */
    mdec->phase = 1;

    mdec->btn = mdec->buf[0] & 0x07; /* 低三位, 鼠标点击状态 */
    mdec->x   = mdec->buf[1];
    mdec->y   = mdec->buf[2];

    if ((mdec->buf[0] & 0x10) != 0)
      mdec->x |= 0xffffff00;
    if ((mdec->buf[0] & 0x20) != 0)
      mdec->y |= 0xffffff00;

    mdec->y = -mdec->y;
    return 1;
  default:
    break;
  }
  return -1;
}