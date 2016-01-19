#ifndef BOOTPACK_H
#define BOOTPACK_H

/* color */
#define COL8_000000   0
#define COL8_FF0000   1
#define COL8_00FF00   2
#define COL8_FFFF00   3
#define COL8_0000FF   4
#define COL8_FF00FF   5
#define COL8_00FFFF   6
#define COL8_FFFFFF   7
#define COL8_C6C6C6   8
#define COL8_840000   9
#define COL8_008400   10
#define COL8_848400   11
#define COL8_000084   12
#define COL8_840084   13
#define COL8_008484   14
#define COL8_848484   15

#define ADR_BOOTINFO  0x00000ff0

/* interrupt */
//#define PORT_KEYDAT   0x0060

#define PIC0_ICW1   0x0020
#define PIC0_OCW2   0x0020
#define PIC0_IMR    0x0021
#define PIC0_ICW2   0x0021
#define PIC0_ICW3   0x0021
#define PIC0_ICW4   0x0021
#define PIC1_ICW1   0x00a0
#define PIC1_OCW2   0x00a0
#define PIC1_IMR    0x00a1
#define PIC1_ICW2   0x00a1
#define PIC1_ICW3   0x00a1
#define PIC1_ICW4   0x00a1

/* gdt/idt */
#define ADR_IDT     0x0026f800
#define LIMIT_IDT   0x000007ff
#define ADR_GDT     0x00270000
#define LIMIT_GDT   0x0000ffff
#define ADR_BOTPAK    0x00280000
#define LIMIT_BOTPAK  0x0007ffff
#define AR_DATA32_RW  0x4092
#define AR_CODE32_ER  0x409a
#define AR_INTGATE32  0x008e

#define PORT_KEYDAT   0x0060
#define PORT_KEYSTA   0x0064
#define PORT_KEYCMD   0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE      0x47
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

#define FLAGS_OVERRUN (0x0001)
#define KEYBUF_SIZE   (32)
#define MOUSEBUF_SIZE (128)

#define EFLAGS_AC_BIT (0x00040000)
#define CR0_CACHE_DISABLE (0x60000000)

#define MEMMAN_FREES (4090) /* ABOUT 32KB */
#define MEMMAN_ADDR  (0x003c0000)

#define MAX_SHEETS   (256)
#define SHEET_USE    1
struct MOUSE_DEC {
  unsigned char buf[3];
  unsigned char phase;
  int x;
  int y;
  int btn;
};

typedef struct BOOTINFO {
  char cyls;
  char leds;
  char vmode;
  char reserve;
  short scrnx;
  short scrny;
  char *vram;
}bootinfo;

struct SEGMENT_DESCRIPTOR {
  short limit_low, base_low;
  char base_mid, access_right;
  char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
  short offset_low, selector;
  char dw_count, access_right;
  short offset_high;
};

typedef struct FIFO8 {
  unsigned char *buf;
  int p; /* next_w */
  int q; /* next_r */
  int size;
  int freed;
  int flags;
}fifo8;

struct FREEINFO
{
  unsigned int addr;
  unsigned int size;
};

typedef struct MEMMAN
{
  int frees;
  int maxfrees;
  int lostsize;
  int losts;
  struct FREEINFO free[MEMMAN_FREES];
}memman_t;

typedef struct SHEET {
  unsigned char *buf; /* 图层内容 */
  int bxsize, bysize; /* 图层大小 */
  int vx0, vy0;       /* 图层坐标 */
  int col_inv;        /* color && invisible */
  int height;         /* 图层高度 */
  int flags;
  struct SHTCTL *ctl;
}sheet_t;

typedef struct SHTCTL {
  unsigned char *vram;
	unsigned char *map; /* 记录当前画面上点的图层 */
  int xsize, ysize, top;
  sheet_t *sheets[MAX_SHEETS];
  sheet_t sheets0[MAX_SHEETS];
}shtctl_t;

/* io control */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

/* graphic */
void
init_screen8 (char *vram, int xsize, int ysize);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
                 int pysize, int px0, int py0, char *buf, int bxsize);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

/* gdt/idt */
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

/* interrupt */
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);

/* fifo */
void fifo8_init (fifo8 *fifo, int size, unsigned char *buf);
int fifo8_put (fifo8 *fifo, unsigned char data);
int fifo8_get (fifo8 *fifo);
int fifo8_status(fifo8 *fifo);

/* mouse */
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* keyboard */
void init_keyboard(void);

/* memtest */
int load_cr0(void);
void store_cr0(int cr0);
unsigned int memtest_sub(unsigned int start, unsigned int end);
unsigned int memtest(unsigned int start, unsigned int end);

/* memory */
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet */
shtctl_t *shtctl_init(memman_t *memman, unsigned char *vram, int xsize, int ysize);
sheet_t *sheet_alloc(shtctl_t *ctl);
void sheet_setbuf(sheet_t *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_refresh(sheet_t *sht, int bx0, int by0, int bx1, int by1);
void sheet_updown(sheet_t *sht, int height);
void sheet_slide(sheet_t *sht, int vx0, int vy0);
void sheet_free(sheet_t *sht);

#endif // BOOTPACK_H
