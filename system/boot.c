#include <memory.h>
#include <interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stm32.h>
#include <gpio.h>
#include <spi.h>
#include <w25qxxx.h>
#include <usb_cdc_conf.h>


extern void (*vector_table[])(void); 
extern void nulluser();
extern  void meminit(void); /* Initializes the free memory list */

int ready_preemptive=0;
#if !defined  (HSE_VALUE) 
  #define HSE_VALUE    ((uint32_t)25000000) /*!< Default value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

uint32_t SystemCoreClock = 16000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};


#define PLL_M      8
#define PLL_N      336
#define PLL_P      2
#define PLL_Q      8


static void SetSysClock(void)
{

    //irq_disable();
    // HSE clock enable
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Power interface clock enable
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    // Scale 1 mode (default value at reset):
    // the device does operate at the maximum frequency 168 MHz
    PWR->CR |= PWR_CR_VOS;

    // AHB = SYSCLK / 1
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

    // APB2 = AHB / 2
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

    // APB1 = AHB / 4
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

    // configure the main PLL
    // SYSCLK = HSE / PLL_M * PLL_N / PLL_P
    RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) - 1) << 16) | RCC_PLLCFGR_PLLSRC_HSE | (PLL_Q << 24);

    // enable the main PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // configure Flash prefetch, Instruction cache, Data cache and wait state
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS;

    // select the main PLL as system clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}



 void init_systick(uint32_t s)
{
    // Clear CTRL register
    SysTick->CTRL = 0x00000;
    // Main clock source is running with HSI by default which is at 8 Mhz.
    // SysTick clock source can be set with CTRL register (Bit 2)
    // 0: Processor clock/8 (AHB/8)
    // 1: Processor clock (AHB)
    SysTick->CTRL |= (1 << 2);
    // Enable callback (bit 1)
    SysTick->CTRL |= (1 << 1);
    // Load the value
    SysTick->LOAD = (uint32_t)(s-1);
    // Set the current value to 0
    SysTick->VAL = 0;
    // Enable SysTick (bit 0)
    SysTick->CTRL |= (1 << 0);
}




void SystemCoreClockUpdate(void)
{
  uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;
  
  /* Get SYSCLK source -------------------------------------------------------*/
  tmp = RCC->CFGR & RCC_CFGR_SWS;

  switch (tmp)
  {
    case 0x00:  /* HSI used as system clock source */
      SystemCoreClock = HSI_VALUE;
      break;
    case 0x04:  /* HSE used as system clock source */
      SystemCoreClock = HSE_VALUE;
      break;
    case 0x08:  /* PLL used as system clock source */

      /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
         SYSCLK = PLL_VCO / PLL_P
         */    
      pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
      pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
      
      if (pllsource != 0)
      {
        /* HSE used as PLL clock source */
        pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      }
      else
      {
        /* HSI used as PLL clock source */
        pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      }

      pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >>16) + 1 ) *2;
      SystemCoreClock = pllvco/pllp;
      break;
    default:
      SystemCoreClock = HSI_VALUE;
      break;
  }
  /* Compute HCLK frequency --------------------------------------------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
  /* HCLK frequency */
  SystemCoreClock >>= tmp;
}




static void DWTInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; 
}


void systemInit(void) {
     disable();
    _BCL(RCC->AHB2ENR, RCC_AHB2ENR_OTGFSEN);
    _BCL(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
    _BCL(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
    _BCL(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

    RCC->CR |= 1UL << 0;
    RCC->CFGR = 0;
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON | RCC_CR_HSEBYP);
    RCC->PLLCFGR = 0x24003010;   // Documented reset value
    RCC->CIR = 0;  // disable all rcc interrupts
    ready_preemptive=0;
    

 #if 1
    _BMD(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_3WS);
    /* setting up PLL 16MHz HSI, VCO=144MHz, PLLP = 72MHz PLLQ = 48MHz  */
    _BMD(RCC->PLLCFGR,
        RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLQ | RCC_PLLCFGR_PLLP,
        _VAL2FLD(RCC_PLLCFGR_PLLM, 8) | _VAL2FLD(RCC_PLLCFGR_PLLN, 72) | _VAL2FLD(RCC_PLLCFGR_PLLQ, 3));
    /* enabling PLL */
    _BST(RCC->CR, RCC_CR_PLLON);
    _WBS(RCC->CR, RCC_CR_PLLRDY);
    while ((RCC->CR & RCC_CR_PLLRDY) == 0)
        __NOP();
    /* switching to PLL */
    _BMD(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    _WVL(RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL);
    #endif
    /* enabling GPIOA and setting PA11 and PA12 to AF10 (USB_FS) */
    _BST(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
    _BST(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
    _BST(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

    _BST(GPIOA->AFR[1], (0x0A << 12) | (0x0A << 16));
    _BMD(GPIOA->MODER, (0x03 << 22) | (0x03 << 24), (0x02 << 22) | (0x02 << 24));
    
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { /*  */ }
    
    
    //irq_enable();
}



void Reset_Handler(void) {
    unsigned int* src = &_sidata;
    unsigned int* dst = &_sdata;

    while (dst < &_edata)
        *dst++ = *src++;

    for (dst = &_sbss; dst < &_ebss; dst++)
        *dst = 0;
     disable();
    

    _BST(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
    _BST(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
    _BST(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

    SCB->VTOR = (uintptr_t)&vector_table; /* Vector Table Relocation in Internal FLASH. */
    SetSysClock();//systemInit();
    SystemCoreClockUpdate();

    init_systick(100000000 / 1000);
    DWTInit();
 
    nulluser();
    for (;;)
        __NOP(); // hang
}



