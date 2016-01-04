; haribote-ipl
; TAB=4

CYLS  EQU   10

ORG 0x7c00          ; 指明程序的装在位置

; FAT12 FLOOPY

JMP entry
DB  0x90

                    ; DB 0xeb, 0x4e, 0x90
DB "HARIBOTE"       ; 8BTYE, can be any string
DW 512              ; sector
DB 1                ; cluster
DW 1                ; start point
DB 2                ; FAT number
DW 224              ; root directory space
DW 2880             ; disk space = 2880 sectors
DB 0xf0             ; disk kind
DW 9                ; length of FAT = 9 sectors
DW 18               ; 1 track = 18 sectors
DW 2                ; headers
DD 0                ; 不使用分区
DD 2880             ;重写一次磁盘大小
DB 0, 0, 0x29       ; unknown
DD 0xffffffff       ; maybe 卷标号码
DB "HELLO-OS   "    ; 11BYTES disk name
DB "FAT12   "       ; 8BYTES 磁盘格式名称
RESB 18             ; 空出

; program

entry:
  MOV AX, 0         ; 初始化寄存器 DB 0xb8, 0x00, 0x00
  MOV SS, AX        ; 0x8e, 0xd0
  MOV SP, 0x7c00    ; 0xbc, 0x00, 0x7c
  MOV DS, AX        ; 0x8e, 0xd8

  mov AX, 0x0820
  MOV ES, AX        ; 0x8e, 0xc0
  mov CH, 0         ; cylinder 0
  mov DH, 0         ; disk header 0
  mov CL, 2         ; sector 2

readloop:
  mov SI, 0         ;记录次数

retry:
  mov AH, 0x02      ; 读盘
  mov AL, 1         ; 1 sector
  mov BX, 0         ;
  mov DL, 0x00      ; A驱动器
  INT 0x13          ; 调用磁盘BIOS
  JNC  next

  add SI, 1
  cmp SI, 5
  jae error
  mov AH, 0x00
  mov DL, 0x00
  INT 0x13
  jmp retry

next:
  mov AX, ES
  add AX, 0x0020
  mov ES, AX
  add CL, 1
  cmp CL, 18
  jbe readloop
  mov CL, 1
  add DH, 1
  cmp DH, 2
  JB  readloop
  mov DH, 0
  ADD CH, 1
  CMP CH, CYLS
  JB readloop

  MOV [0x0ff0],CH   ; XXX:漏掉这一句, 导致day4的颜色效果出不来
  jmp 0xc200        ; real program

error:
  MOV SI, msg       ; 0xbe, 0x74, 0x7c   msg的地址是0x7c74

putloop:
  MOV AL, [SI]      ; 0x8a, 0x04
  ADD SI, 1         ; 0x83, 0xc6, 0x01
  CMP AL, 0         ; 0x3c, 0x00

  JE fin            ; 0x74, 0x09
  MOV AH, 0x0e      ; 显示一个文字  0xb4, 0x0e
  MOV BX, 15        ; 指定字符颜色  0xbb, 0x0f, 0x00
  INT 0x10          ; 调用显卡BIOS  0xcd, 0x10
  JMP putloop       ; 0xeb, 0xee


fin:                
  HLT               ; 让CPU停止, 等待指令  0xf4
  JMP fin           ; loop   0xeb, 0xfd

msg:
; information

DB 0x0a, 0x0a            ; \n
DB "load error"
DB 0x0a                  ; \n
DB 0


RESB 0x7dfe-$            ; RESB 0x1fe-$   XXX：这个值与最初的值不同          ;填充0
DB 0x55, 0xaa            ; 启动代码