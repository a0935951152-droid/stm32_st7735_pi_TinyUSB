#include <stdint.h>
#include "stm32f103xb.h"
#include "stm32f1xx.h"     /* USB peripheral + RCC USB bits */
#include "st7735.h"
#include "tusb.h"

static void clock_init(void);
static void led_init(void);
static void usb_pins_init(void);
static void usb_renumerate(void);

void delay_ms(uint32_t ms) {
    SysTick->LOAD = 72000UL - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_ENABLE;
    while (ms--) {
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG));
    }
    SysTick->CTRL = 0;
}

int main(void) {
    clock_init();
    led_init();
    st7735_init();
    st7735_fill(ST7735_BLACK);

    usb_pins_init();
    usb_renumerate();          /* force host re-enumeration after reset */

    /* Enable USB IRQs (peripheral itself is brought up by TinyUSB) */
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 2);
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn,  2);

    tud_init(0);

    while (1) {
        tud_task();
        /* All work happens in the CDC rx callback in usb_cdc_app.c */
    }
}

/* --------------------------------------------------------------------- */
/* IRQ handlers — bridge STM32 vectors to TinyUSB's dcd_int_handler.     */
/* --------------------------------------------------------------------- */

void USB_LP_CAN1_RX0_IRQHandler(void) { tud_int_handler(0); }
void USB_HP_CAN1_TX_IRQHandler(void)  { tud_int_handler(0); }
void USBWakeUp_IRQHandler(void)       { tud_int_handler(0); }

/* --------------------------------------------------------------------- */

static void led_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(0xFUL << 20);
    GPIOC->CRH |=  (0x2UL << 20);
    GPIOC->BSRR = (1UL << 13);
}

/*
 * PA11 / PA12 are dedicated USB pins once the peripheral is enabled.
 * We only configure GPIOA's clock here; the alternate function is wired
 * inside the chip when USB is on, no AF remap or AFIO config needed.
 */
static void usb_pins_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
}

/*
 * Many F103 dev boards (Blue Pill, STM32 Smart V2.0) tie a 1.5k pull-up
 * permanently from D+ to 3V3, so the host sees us as connected the
 * moment we power on — which is BEFORE our firmware is ready. The fix is
 * to drive D+ low for a few ms at startup, then release it, forcing the
 * host to re-enumerate while we are actually awake.
 */
static void usb_renumerate(void) {
    /* PA12 = output push-pull 50MHz, drive low */
    GPIOA->CRH &= ~(0xFUL << 16);
    GPIOA->CRH |=  (0x3UL << 16);
    GPIOA->BRR  = (1UL << 12);
    delay_ms(20);
    /* Release: back to input floating; USB will reclaim it on tud_init() */
    GPIOA->CRH &= ~(0xFUL << 16);
    GPIOA->CRH |=  (0x4UL << 16);
}

static void clock_init(void) {
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    /*
     * SYSCLK = HSE 8 MHz * PLL9 = 72 MHz
     * USB    = SYSCLK / 1.5      = 48 MHz   (USBPRE = 0)
     * AHB    = SYSCLK            = 72 MHz
     * APB1   = SYSCLK / 2        = 36 MHz
     * APB2   = SYSCLK            = 72 MHz
     */
    RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1
              | RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9;
    /* USBPRE bit cleared = /1.5 — the default-zero already gives 48MHz */

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & (3UL << 2)) != RCC_CFGR_SWS_PLL);

    /* Enable USB peripheral clock on APB1 */
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
}
