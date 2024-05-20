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

/* initialize.c - nulluser, sysinit */

/* Handle system initialization and become the null process */

#include <xinu.h>
#include <gpio.h>
//#include <syscall.h>
#include <usb_cdc_conf.h>

extern	void	start(void);	/* Start of Xinu code			*/
extern	void	*_end;		/* End of Xinu code			*/

/* Function prototypes */

extern	void main(void);	/* Main is the first process created	*/
static	void sysinit(); 	/* Internal system initialization	*/
extern	void meminit(void);	/* Initializes the free memory list	*/
extern int ready_preemptive;
extern void usbTask();
/* Declarations of major kernel variables */

struct	procent	proctab[NPROC];	/* Process table			*/
struct	sentry	semtab[NSEM];	/* Semaphore table			*/
struct	memblk	memlist;	/* List of free memory blocks		*/

/* Active system status */

int	prcount;		/* Total number of live processes	*/
pid32	currpid;		/* ID of currently executing process	*/
//bool start_null=false;
/* Control sequence to reset the console colors and cusor positiion	*/

#define	CONSOLE_RESET	" \033[0m\033[2J\033[;H"
 


 void blink(){
	//write(CONSOLE,"write test\n",12);
    hw_cfg_pin(GPIOx(GPIO_C),     13,     GPIOCFG_MODE_OUT | GPIOCFG_OSPEED_VHIGH  | GPIOCFG_OTYPE_PUPD | GPIOCFG_PUPD_PUP);
	for (;;)
	{
		hw_set_pin(GPIOx(GPIO_C), 13, 1);//pin_set(pin_c13);
		sleepms(500);
		hw_set_pin(GPIOx(GPIO_C), 13, 0);//pin_reset(pin_c13);
		sleep(1);
	}
}

int blink2(int nargs, char *args[])
{
	//resume(create(blink, 1024, 50, "blink", 0));
    for(int i=0;i<nargs;i++){
    	printf("%s\n",args[i]);
    }
	printf("blink2\n");
	return OK;
}


void blink1(){
	int c=0;
	int ulen;
	char	buf[32];
    kprintf("blink1\n");
	resume(create(blink2, INITSTK, 60, "blink2", 3,"daniel","quintero","kernel"));
	receive();
    
    kprintf("\nprocess has completed.\n");
	while(1){
        kprintf("nullp %d\n",c++);
        sleep(3);
        /*while(!usb_available()){
              PEND_SV();
        }
        while(usb_available()){
              ulen = readBytes(buf,sizeof(buf));
        }
        buf[ulen] = SH_NEWLINE;
        kprintf("->%s\n",buf);*/
      
	}
    
}



void nullprocess(void) {
	//resume(create(shell, 4096/2, 50, "shell", 1, 0));

	kprintf("nullprocess\n");
	//putc('D', CONSOLE);
	//resume(create(usbTask, 1024*2, 50, "blink", 0));
	//receive();
    //resume(create(blink1, INITSTK, 50, "blink1", 0));
    
    resume(create(blink, 1024, 51, "blink", 0));
    resume(create(blink1, INITSTK, 50, "blink1", 0));
	//resume(create(shell, 4096, 52, "shell", 1, 0));
	for(;;){

	}
}


 



void	nulluser()
{	
	struct	memblk	*memptr;	/* Ptr to memory block		*/
	uint32	free_mem;		/* Total amount of free memory	*/

    hw_cfg_pin(GPIOx(GPIO_C),13,GPIOCFG_MODE_OUT | GPIOCFG_OSPEED_VHIGH  | GPIOCFG_OTYPE_PUPD | GPIOCFG_PUPD_PUP);
    hw_cfg_pin(GPIOx(GPIO_A),0,GPIOCFG_MODE_INP | GPIOCFG_OSPEED_VHIGH  | GPIOCFG_OTYPE_OPEN | GPIOCFG_PUPD_PUP);

    meminit();
	platinit();
    /* Enable interrupts */
	enable();

	/* Initialize the system */
   
    while(1){
        if(!hw_get_pin(GPIOx(GPIO_A),0)){
            break;
        }
        hw_toggle_pin(GPIOx(GPIO_C),13);
        delay(50);
    }
	
    sysinit();
	/* Output Xinu memory layout */
	free_mem = 0;
	for (memptr = memlist.mnext; memptr != NULL;
						memptr = memptr->mnext) {
		free_mem += memptr->mlength;
	}
	kprintf ("Build date: %s %s\n\n", __DATE__, __TIME__);
	kprintf("%10d bytes of free memory.  Free list:\n", free_mem);
	for (memptr=memlist.mnext; memptr!=NULL;memptr = memptr->mnext) {
	    kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)memptr, ((uint32)memptr) + memptr->mlength - 1);
	}

	kprintf("%10d bytes of Xinu code.\n",
		(uint32)&_etext - (uint32)&_text);
	kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)&_text, (uint32)&_etext - 1);
	kprintf("%10d bytes of data.\n",
		(uint32)&_ebss - (uint32)&_sdata);
	kprintf("           [0x%08X to 0x%08X]\n\n",
		(uint32)&_sdata, (uint32)&_ebss - 1);

     
	/* Initialize the Null process entry */	
	int pid = create((void *)nullprocess, 256, 10, "Null process", 0, NULL);
	struct procent * prptr = &proctab[pid];
	prptr->prstate = PR_CURR;

	
	/* Initialize the real time clock */
	clkinit();

    TIM2->CR1 |= (1 << 0); 
    ready_preemptive=1;
    NVIC_EnableIRQ(TIM2_IRQn);
    asm volatile ("mov r0, %0\n" : : "r" (prptr->prstkptr));
	asm volatile ("msr psp, r0");
	asm volatile ("ldmia r0!, {r4-r11} ");
    asm volatile ("msr psp, r0");
	asm volatile ("mov r0, #2");
	asm volatile ("msr control, r0");
	asm volatile ("isb");
    nullprocess();

	for(;;);

}

/* Startup does a system call, the processor switches to handler 
 * mode and prepares for executing the null process (see syscall.c) 
 * This is also where a kernel mode to user mode switch can
 * take place */


/*------------------------------------------------------------------------
 *
 * sysinit  -  Initialize all Xinu data structures and devices
 *
 *------------------------------------------------------------------------
 */


 
static	void	sysinit()
{
	int32	i;
	struct	procent	*prptr;		/* Ptr to process table entry	*/
	struct	sentry	*semptr;	/* Ptr to semaphore table entry	*/


	/* Platform Specific Initialization */
    //meminit();
	  //platinit();

	//kprintf(CONSOLE_RESET);
	for (int i = 0; i < 10; ++i)
	{
		kprintf("\n");
	}
	/* Initialize free memory list */
	
	

	/* Initialize system variables */

	/* Count the Null process as the first process in the system */
	prcount = 0;
//	prcount = 1;

	/* Scheduling is not currently blocked */

	Defer.ndefers = 0;

	/* Initialize process table entries free */

	for (i = 0; i < NPROC; i++) {
		prptr = &proctab[i];
		prptr->prstate = PR_FREE;
		prptr->prname[0] = NULLCH;
		prptr->prstkbase = NULL;
		prptr->prprio = 0;
	}


	/* Initialize semaphores */

	for (i = 0; i < NSEM; i++) {
		semptr = &semtab[i];
		semptr->sstate = S_FREE;
		semptr->scount = 0;
		semptr->squeue = newqueue();
	}

	
	readylist = newqueue();

	
	for (i = 0; i < NDEVS; i++) {
		init(i);
	}
	return;
}


 
int32	stop(char *s)
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
	;
}

 
 