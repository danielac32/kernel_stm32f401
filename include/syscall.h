/* Xinu system calls */


extern int __syscall(int a,...);
/*
extern int syscall0(int code);
extern int syscall1(int code, int arg1);
extern int syscall2(int code, int arg1, int arg2);
extern int syscall3(int code, int arg1, int arg2, int arg3);
extern int syscall4(int code, int arg1, int arg2, int arg3, int arg4);
*/

extern void xinu_syscall(int nr, uint32 arg1, uint32 arg2, uint32 arg3);
extern int xinu_putc(int dev, char c);



#define XINU_NULLPROCESS  1
#define XINU_PUTC 2
#define XINU_PIN_MODE 3
#define XINU_PIN_SET  4
#define XINU_PIN_GET  5
#define XINU_TEST1    6
#define XINU_TEST2    7

#define XINU_PRINTF    100
//#define XINU_PUTC    8
#define XINU_SLEEP    9

