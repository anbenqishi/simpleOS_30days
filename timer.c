#include "bootpack.h"
#include <stdio.h>

timer_t timerctl;

/* Programmable interval timer. */
void init_pit(void)
{
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c); /* low 8bit. */
	io_out8(PIT_CNT0, 0x2e); /* high 8bit. */

	timerctl.count = 0;
	timerctl.timeout = 0;
	return;
}

void inthandler20(int *esp)
{
	/* ��IRQ-00�źŽ������˵���Ϣ֪ͨ��PIC. */
	io_out8(PIC0_OCW2, 0x60);
	++timerctl.count;
	if (timerctl.timeout > 0) 
		{
			timerctl.timeout--;
			if (timerctl.timeout == 0)
				fifo8_put(timerctl.fifo, timerctl.data);
		}
	return;
}

void settimer(unsigned int timeout, fifo8 *fifo, unsigned char data)
{
	int eflags;

	eflags = io_load_eflags();
	io_cli();
	timerctl.timeout = timeout;
	timerctl.fifo    = fifo;
	timerctl.data    = data;
	io_store_eflags(eflags);
	return;
}
