#include "bootpack.h"

void fifo8_init (fifo8 *fifo, int size, unsigned char *buf)
{
  fifo->size = size;
  fifo->buf  = buf;
  fifo->freed = size;
  fifo->flags = 0;
  fifo->p    = 0;
  fifo->q    = 0;
  return;
}

int fifo8_put (fifo8 *fifo, unsigned char data)
{
  if (fifo->freed == 0){
    fifo->flags |= FLAGS_OVERRUN;
    return -1;
  }

  fifo->buf[fifo->p] = data;
  fifo->p++;
  if (fifo->p == fifo->size)
    fifo->p = 0;
  fifo->freed--;
  return 0;
}

int fifo8_get (fifo8 *fifo)
{
  int data;

  if (fifo->freed == fifo->size)
    return -1;

  data = fifo->buf[fifo->q];
  fifo->q++;
  if (fifo->q == fifo->size)
    fifo->q = 0;
  fifo->freed++;
  return data;
}

int fifo8_status(fifo8 *fifo)
{
  return fifo->size - fifo->freed;
}