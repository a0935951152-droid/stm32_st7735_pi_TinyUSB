/*
 * stm32f1xx.h — minimal CMSIS-style shim, just enough for TinyUSB's
 *               stm32_fsdev port to compile against this bare-metal project.
 *
 * Provides:
 *   - USB peripheral register layout + bit definitions (RM0008 §23)
 *   - IRQn_Type enum (only the entries TinyUSB references for F1)
 *   - Minimal NVIC inline helpers, __DSB / __ISB / __NOP
 *
 * Definitions for RCC/GPIO/SPI/USART etc. live in stm32f103xb.h.
 */
#ifndef STM32F1XX_H
#define STM32F1XX_H

#include <stdint.h>
#include "stm32f103xb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------- */
/* IRQn_Type — only the entries TinyUSB / our startup ever references.   */
/* Values match the STM32F103xB vector table (RM0008 §10.1.2).           */
/* --------------------------------------------------------------------- */

typedef enum IRQn {
    NonMaskableInt_IRQn          = -14,
    HardFault_IRQn               = -13,
    MemoryManagement_IRQn        = -12,
    BusFault_IRQn                = -11,
    UsageFault_IRQn              = -10,
    SVCall_IRQn                  =  -5,
    DebugMonitor_IRQn            =  -4,
    PendSV_IRQn                  =  -2,
    SysTick_IRQn                 =  -1,

    /* STM32F1 peripheral interrupts (just what we need) */
    USB_HP_CAN1_TX_IRQn          =  19,
    USB_LP_CAN1_RX0_IRQn         =  20,
    USBWakeUp_IRQn               =  42
} IRQn_Type;

/* --------------------------------------------------------------------- */
/* Minimal NVIC (Cortex-M3) — register layout from ARMv7-M ARM           */
/* --------------------------------------------------------------------- */

#define NVIC_BASE   0xE000E100UL

typedef struct {
    volatile uint32_t ISER[8];
    uint32_t          RESERVED0[24];
    volatile uint32_t ICER[8];
    uint32_t          RESERVED1[24];
    volatile uint32_t ISPR[8];
    uint32_t          RESERVED2[24];
    volatile uint32_t ICPR[8];
    uint32_t          RESERVED3[24];
    volatile uint32_t IABR[8];
    uint32_t          RESERVED4[56];
    volatile uint8_t  IP[240];
} NVIC_Type;

#define NVIC  ((NVIC_Type *)NVIC_BASE)

static inline void NVIC_EnableIRQ(IRQn_Type irq) {
    NVIC->ISER[((uint32_t)irq) >> 5] = (1UL << (((uint32_t)irq) & 0x1FUL));
}

static inline void NVIC_DisableIRQ(IRQn_Type irq) {
    NVIC->ICER[((uint32_t)irq) >> 5] = (1UL << (((uint32_t)irq) & 0x1FUL));
}

static inline void NVIC_SetPriority(IRQn_Type irq, uint32_t prio) {
    if ((int32_t)irq < 0) {
        /* System exceptions — not used by TinyUSB, skip */
        (void)prio;
    } else {
        NVIC->IP[(uint32_t)irq] = (uint8_t)((prio << 4) & 0xFFUL);
    }
}

#define __DSB()   __asm volatile ("dsb 0xF" ::: "memory")
#define __ISB()   __asm volatile ("isb 0xF" ::: "memory")
#define __NOP()   __asm volatile ("nop")

/* --------------------------------------------------------------------- */
/* USB Device peripheral (RM0008 §23, F103xB)                            */
/*   USB regs at 0x40005C00; PMA (packet memory) at 0x40006000           */
/*   Register stride is 4 bytes but only the low 16 bits are used.       */
/* --------------------------------------------------------------------- */

#define USB_BASE       (APB1PERIPH_BASE + 0x00005C00UL)
#define USB_PMAADDR    (APB1PERIPH_BASE + 0x00006000UL)

typedef struct {
    volatile uint32_t EP0R;
    volatile uint32_t EP1R;
    volatile uint32_t EP2R;
    volatile uint32_t EP3R;
    volatile uint32_t EP4R;
    volatile uint32_t EP5R;
    volatile uint32_t EP6R;
    volatile uint32_t EP7R;
    uint32_t          RESERVED[8];
    volatile uint32_t CNTR;
    volatile uint32_t ISTR;
    volatile uint32_t FNR;
    volatile uint32_t DADDR;
    volatile uint32_t BTABLE;
} USB_TypeDef;

#define USB  ((USB_TypeDef *)USB_BASE)

/* USB_CNTR — Control register */
#define USB_CNTR_CTRM       (1U <<  15)
#define USB_CNTR_PMAOVRM    (1U <<  14)
#define USB_CNTR_ERRM       (1U <<  13)
#define USB_CNTR_WKUPM      (1U <<  12)
#define USB_CNTR_SUSPM      (1U <<  11)
#define USB_CNTR_RESETM     (1U <<  10)
#define USB_CNTR_SOFM       (1U <<   9)
#define USB_CNTR_ESOFM      (1U <<   8)
#define USB_CNTR_RESUME     (1U <<   4)
#define USB_CNTR_FSUSP      (1U <<   3)
#define USB_CNTR_LP_MODE    (1U <<   2)
#define USB_CNTR_PDWN       (1U <<   1)
#define USB_CNTR_FRES       (1U <<   0)

/* USB_ISTR — Interrupt status register */
#define USB_ISTR_CTR        (1U <<  15)
#define USB_ISTR_PMAOVR     (1U <<  14)
#define USB_ISTR_ERR        (1U <<  13)
#define USB_ISTR_WKUP       (1U <<  12)
#define USB_ISTR_SUSP       (1U <<  11)
#define USB_ISTR_RESET      (1U <<  10)
#define USB_ISTR_SOF        (1U <<   9)
#define USB_ISTR_ESOF       (1U <<   8)
#define USB_ISTR_DIR        (1U <<   4)
#define USB_ISTR_EP_ID      (0x0FU)

/* USB_FNR */
#define USB_FNR_FN          (0x07FFU)

/* USB_DADDR */
#define USB_DADDR_EF        (1U <<   7)
#define USB_DADDR_ADD       (0x7FU)

/* USB_EPnR — endpoint registers */
#define USB_EP_CTR_RX       (1U << 15)
#define USB_EP_DTOG_RX      (1U << 14)
#define USB_EPRX_STAT       (3U << 12)
#define USB_EP_SETUP        (1U << 11)
#define USB_EP_T_FIELD      (3U <<  9)
#define USB_EP_KIND         (1U <<  8)
#define USB_EP_CTR_TX       (1U <<  7)
#define USB_EP_DTOG_TX      (1U <<  6)
#define USB_EPTX_STAT       (3U <<  4)
#define USB_EPADDR_FIELD    (0x0FU)

/* Endpoint type field values */
#define USB_EP_BULK         (0U <<  9)
#define USB_EP_CONTROL      (1U <<  9)
#define USB_EP_ISOCHRONOUS  (2U <<  9)
#define USB_EP_INTERRUPT    (3U <<  9)
#define USB_EP_T_MASK       USB_EP_T_FIELD
#define USB_EP_TYPE_MASK    USB_EP_T_FIELD

/* Mask for the "write-1-to-toggle / write-0-to-clear" semantics.
 * Per RM0008: writing to EPnR, toggle bits (DTOG_*, STAT_*) flip when 1
 * is written; CTR bits clear when 0 is written; type/kind/addr are normal.
 * USB_EPREG_MASK selects the bits that must NOT be touched (preserve them
 * when modifying other fields). */
#define USB_EPREG_MASK      (USB_EP_CTR_RX | USB_EP_SETUP | USB_EP_T_FIELD | \
                             USB_EP_KIND   | USB_EP_CTR_TX | USB_EPADDR_FIELD)

/* TX/RX status field values (must be XOR'd with current to set, per RM) */
#define USB_EP_RX_DIS       (0U << 12)
#define USB_EP_RX_STALL     (1U << 12)
#define USB_EP_RX_NAK       (2U << 12)
#define USB_EP_RX_VALID     (3U << 12)
#define USB_EP_TX_DIS       (0U <<  4)
#define USB_EP_TX_STALL     (1U <<  4)
#define USB_EP_TX_NAK       (2U <<  4)
#define USB_EP_TX_VALID     (3U <<  4)

/* Bit position helpers used by TinyUSB */
#define USB_EPTX_STAT_Pos       4U
#define USB_EPRX_STAT_Pos       12U
#define USB_EP_DTOG_TX_Pos      6U
#define USB_EP_DTOG_RX_Pos      14U
#define USB_EP_CTR_TX_Pos       7U
#define USB_EP_CTR_RX_Pos       15U

/* Direction (USB standard, used by TinyUSB) */
#define USB_DIR_OUT             0U
#define USB_DIR_IN              0x80U
#define USB_DIR_IN_MASK         0x80U

/* Misc USB request constants used in the stack */
#define USB_REQ_TYPE_STANDARD   0U
#define USB_REQ_RCPT_DEVICE     0U
#define USB_REQ_SET_ADDRESS     5U
#define USB_SPEED_FULL          1U
#define USB_XFER_CONTROL        0U
#define USB_XFER_ISOCHRONOUS    1U
#define USB_XFER_BULK           2U
#define USB_XFER_INTERRUPT      3U

/* --------------------------------------------------------------------- */
/* RCC additions for USB (beyond what stm32f103xb.h covers)              */
/* --------------------------------------------------------------------- */

#ifndef RCC_APB1ENR_USBEN
#define RCC_APB1ENR_USBEN    (1UL << 23)
#endif
#ifndef RCC_CFGR_USBPRE
#define RCC_CFGR_USBPRE      (1UL << 22)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32F1XX_H */
