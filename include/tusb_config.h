/*
 * TinyUSB configuration for STM32F103C8T6 — Device mode, CDC class only.
 */
#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* MCU / port selection */
#define CFG_TUSB_MCU              OPT_MCU_STM32F1
#define CFG_TUSB_OS               OPT_OS_NONE
#define CFG_TUSB_DEBUG            0

/* Allocate USB DMA buffers in regular SRAM (F103 has no separate USB RAM) */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN        __attribute__((aligned(4)))
#endif

/* Device mode, one root hub port, full-speed */
#define CFG_TUD_ENABLED           1
#define CFG_TUD_MAX_SPEED         OPT_MODE_FULL_SPEED
#define CFG_TUD_ENDPOINT0_SIZE    64

/* Class drivers we enable */
#define CFG_TUD_CDC               1
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

/* CDC endpoint buffer sizes (host->device path needs to keep up with the
 * SPI feed; bigger RX buffer means fewer NAKs while we drain to LCD).    */
#define CFG_TUD_CDC_RX_BUFSIZE    512
#define CFG_TUD_CDC_TX_BUFSIZE    64
#define CFG_TUD_CDC_EP_BUFSIZE    64

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
