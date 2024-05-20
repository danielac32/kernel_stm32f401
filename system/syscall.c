/* Xinu for STM32
 *
 * Original license applies
 * Modifications for STM32 by Robin Krens
 * Please see LICENSE and AUTHORS 
 * 
 * $LOG$
 * 2019/11/11 - ROBIN KRENS
 * Initial version 
 * 
 * $DESCRIPTION$
 *
 * */

#include <xinu.h>

/* System call handler 
 * The __attribute__((naked)) makes sure the compiler doesn't
 * places registers on the stack
 *
 * A SVC is similar to an exception or interrupt
 * A stack frame is pushed on entry and popped
 * on exit BY PROCESSOR. Please see in-function command */

 

/*
int syscall0(int code)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("svc #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

 int syscall1(int code, int arg1)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg1));
	__asm__ volatile("svc #1");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

int syscall2(int code, int arg1, int arg2)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg1));
	__asm__ volatile("ldr r2, %0" : : "m" (arg2));
	__asm__ volatile("svc #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

int syscall3(int code, int arg1, int arg2, int arg3)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg1));
	__asm__ volatile("ldr r2, %0" : : "m" (arg2));
	__asm__ volatile("ldr r3, %0" : : "m" (arg3));
	__asm__ volatile("svc #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

int syscall4(int code, int arg1, int arg2, int arg3, int arg4)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg1));
	__asm__ volatile("ldr r2, %0" : : "m" (arg2));
	__asm__ volatile("ldr r3, %0" : : "m" (arg3));
	__asm__ volatile("ldr r4, %0" : : "m" (arg4));
	__asm__ volatile("svc #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}
*/


 __attribute__((naked))
void xinu_syscall(int nr, uint32 arg1, uint32 arg2, uint32 arg3) 
{
	uint32 result;
    asm volatile("push {r0-r11}");
    asm volatile("push {lr}");
    asm volatile("svc 1"); /* Hack: For all service call use 1, arg1 got number */
    asm volatile("pop {lr}");
    asm volatile("pop {r0-r11}");
    asm volatile("bx lr");
   // __asm__ volatile("str r0, %0" : "=m" (result));
    //return result;
}

int xinu_putc(int dev, char c) {
    xinu_syscall(XINU_PUTC, dev, c, 0 );
    return OK;
}



__attribute__ ((naked))
void _svc_handler(void) {

	uint32 * args;
	/* Test whether system call was invoked from supervisor (use MSP) or
	 * user (use PSP) mode (needed for future purposes) */
	//uint32 q=disable();
	asm volatile (
	"tst lr, #4" "\n\t"
	"ite eq" "\n\t"
	"mrseq %0, msp" "\n\t"
	"mrsne %0, psp" "\n\t"
	"push {lr}" "\n\t"
	"push {r4-r11}" : "=r" (args));

	/* An exception (or interrupt) before entering this handler
	 * places the following on the stack 
	 * 
	 * R0 <- args[0]
	 * R1 <- args[1]
	 * R2 
	 * R3
	 * R12
	 * LR <- Not a real LR, A certain 'mask'
	 * PC <- placed at args[6], see description below
	 * PSR <- Status of processor before call
	 *
	 * PC contains the return address that will continue after this SVC handler
	 * is finised. The previous address (the svc # call) is at PC - 2, the
	 * first byte contains the svc number.
	 * */

	//uint8 svc_num = ((char *) args[6])[-2];
	uint32 svc_nr = args[0];
	
    //svc_nr = svc_num==XINU_PRINTF?XINU_PRINTF:svc_nr;

	int ret = -1;
	struct procent * p;
  

	switch(svc_nr) {
		case XINU_NULLPROCESS:
			p = (struct procent *) args[1]; 
	  		asm volatile ("mov r0, %0" : : "r" (p->prstkptr));
                	asm volatile("ldmia r0!, {r4-r11} ");
                 	asm volatile ("msr psp, r0");
                 	asm volatile (
                          "ldr r0, =0xFFFFFFFD" "\n\t"
                          "mov lr, r0" "\n\t"
                          "bx lr");
			break;
		 
        case XINU_PUTC:
        		{
        			ret = putc(args[1], args[2]);
        			//kputc(args[2]);
        		}
        break;
		case XINU_PIN_MODE://pin mode
 			 {
 			 	 
 			 }
			break;
		case XINU_PIN_SET://pin set
 			 {
 			 	 
 			 }
		break;

		case XINU_PIN_GET://pin mode
 			 {
 			 	 
 			 }
		break;
		case XINU_SLEEP:
			{
				//kprintf("sleep\n");
				sleep(args[1]);
			}
		break;
		/* case 5:
		 		{
		 			kputc((int)args[1]);
		 		}
		 break;*/
        case XINU_PRINTF:
        		{
        			//printf((char *)args[0],args[1],args[2],args[3] );
        			//printf("%s\n",(char *)args[0]);
        			//ret = putc(CONSOLE, 'A');
        		}
        break;
        case XINU_TEST2:
			{
				//extern void blink_syscall();
				//resume(create((void *)blink_syscall, 1024/2, INITPRIO, "blink", 0, NULL));
				

				//kprintf("%d %d %d \n",args[0],args[1],args[2]);
				//char *s=args[1];
				//while(*s){
				//	putc(CONSOLE,*s++);
				//}
				//printf((char *)args[1],args[2],args[3],args[4],args[5] );
                //printf("-->\n");

                int pid = create(args[1], args[2], 50, args[3], 1,0);
                ready(pid);
                ret = 777;
			}
		break;

		/*case XINU_TEST1:
		     {
		     	extern void button();
				resume(create((void *)button, 1024/2, INITPRIO, "button", 0, NULL));
		     }
		break;

		
		case XINU_TEST3:
			{
				resume(create(shell, 4096/2, 50, "shell", 1, 0));
			}
		break;*/
		default:
			kprintf("System call not implemented\n");	
			for (int i = 0; i < 15; ++i)
			{
				kprintf("[r%d]=%08x\n",i,args[i]);
			}
			break;
	}

	/* Return value in r0 for callee */
	args[0] = ret;

	asm volatile ("pop {r4-r11}");
	/* exception return, 0xFFFFFFFX (<-- last value is flag
	 * and gives the processor information to return to what */
	asm volatile ("pop {lr}"); 
	//restore(q);//asm volatile("cpsie	i");
	asm volatile ("bx lr");


}
