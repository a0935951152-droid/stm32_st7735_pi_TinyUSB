/*
 * USB CDC image receiver.
 *
 * Wire protocol from host (Pi):
 *   [0xA5] [0x5A] [W] [H]  followed by  W*H*2 bytes RGB565 (hi-byte first)
 *
 * Implementation:
 *   - State machine consumes bytes from tud_cdc_read_char()
 *   - Once header is locked in, address window is set and RAMWR opened
 *   - Each subsequent pair of bytes is streamed straight into SPI2->DR
 *   - No RAM framebuffer (we only have 20K — image is 32K)
 *   - PC13 LED toggles every transferred row as a liveness indicator
 */
#include <stdint.h>
#include "stm32f103xb.h"
#include "tusb.h"
#include "st7735.h"

extern void delay_ms(uint32_t ms);

/* Re-declare a couple of pieces from st7735.c that we need direct access
 * to in order to stream pixels without buffering. The public header keeps
 * these private; rather than expose them, the implementation below uses
 * the same low-level register sequence the driver itself uses. */
#define CS_LOW()    (GPIOB->BRR  = (1UL << 12))
#define CS_HIGH()   (GPIOB->BSRR = (1UL << 12))
#define DC_CMD()    (GPIOB->BRR  = (1UL << 1))
#define DC_DATA()   (GPIOB->BSRR = (1UL << 1))
#define LED_ON()    (GPIOC->BRR  = (1UL << 13))   /* active low */
#define LED_OFF()   (GPIOC->BSRR = (1UL << 13))

#define COL_OFFSET  2
#define ROW_OFFSET  3

static inline void spi_tx(uint8_t b) {
    while (!(SPI2->SR & SPI_SR_TXE)) {}
    SPI2->DR = b;
}
static inline void spi_flush(void) {
    while (!(SPI2->SR & SPI_SR_TXE)) {}
    while (SPI2->SR & SPI_SR_BSY) {}
}
static void wr_cmd_dn(uint8_t cmd, const uint8_t *data, uint8_t n) {
    CS_LOW();
    DC_CMD();  spi_tx(cmd); spi_flush();
    DC_DATA();
    for (uint8_t i = 0; i < n; i++) spi_tx(data[i]);
    spi_flush();
    CS_HIGH();
}

/* --------------------------------------------------------------------- */
/* Receiver state machine                                                 */
/* --------------------------------------------------------------------- */

typedef enum {
    ST_WAIT_MAGIC0,
    ST_WAIT_MAGIC1,
    ST_WAIT_W,
    ST_WAIT_H,
    ST_PIXEL_HI,
    ST_PIXEL_LO
} rx_state_t;

static rx_state_t state = ST_WAIT_MAGIC0;
static uint8_t  img_w, img_h;
static uint32_t pix_remaining;
static uint8_t  pix_hi;
static uint32_t row_pix_left;  /* used only for the LED tick */

static void start_image(uint8_t w, uint8_t h) {
    if (w == 0 || h == 0 || w > ST7735_WIDTH || h > ST7735_HEIGHT) {
        state = ST_WAIT_MAGIC0;
        return;
    }
    img_w = w; img_h = h;
    pix_remaining = (uint32_t)w * h;
    row_pix_left  = w;

    uint8_t caset[4] = {0, COL_OFFSET, 0, (uint8_t)(w - 1 + COL_OFFSET)};
    uint8_t raset[4] = {0, ROW_OFFSET, 0, (uint8_t)(h - 1 + ROW_OFFSET)};
    wr_cmd_dn(0x2A, caset, 4);
    wr_cmd_dn(0x2B, raset, 4);

    /* Open RAMWR — CS stays low until the entire frame is shifted out */
    CS_LOW();
    DC_CMD();  spi_tx(0x2C);  spi_flush();
    DC_DATA();

    state = ST_PIXEL_HI;
}

static void finish_image(void) {
    spi_flush();
    CS_HIGH();
    LED_OFF();
    state = ST_WAIT_MAGIC0;
}

static void feed_byte(uint8_t b) {
    switch (state) {
    case ST_WAIT_MAGIC0:
        if (b == 0xA5) state = ST_WAIT_MAGIC1;
        break;
    case ST_WAIT_MAGIC1:
        state = (b == 0x5A) ? ST_WAIT_W : ST_WAIT_MAGIC0;
        break;
    case ST_WAIT_W:
        img_w = b;
        state = ST_WAIT_H;
        break;
    case ST_WAIT_H:
        img_h = b;
        start_image(img_w, img_h);
        break;
    case ST_PIXEL_HI:
        pix_hi = b;
        state = ST_PIXEL_LO;
        break;
    case ST_PIXEL_LO:
        while (!(SPI2->SR & SPI_SR_TXE)) {}
        SPI2->DR = pix_hi;
        while (!(SPI2->SR & SPI_SR_TXE)) {}
        SPI2->DR = b;
        pix_remaining--;
        if (--row_pix_left == 0) {
            row_pix_left = img_w;
            GPIOC->ODR ^= (1UL << 13);   /* toggle LED per row */
        }
        if (pix_remaining == 0) {
            finish_image();
        } else {
            state = ST_PIXEL_HI;
        }
        break;
    }
}

/* --------------------------------------------------------------------- */
/* TinyUSB callback — drain CDC RX into the state machine                 */
/* --------------------------------------------------------------------- */

void tud_cdc_rx_cb(uint8_t itf) {
    (void)itf;
    uint8_t buf[64];
    uint32_t n;
    while ((n = tud_cdc_read(buf, sizeof(buf))) > 0) {
        for (uint32_t i = 0; i < n; i++) {
            feed_byte(buf[i]);
        }
    }
}

void usb_cdc_app_task(void) {
    /* TinyUSB drains via the callback above; nothing else to do here. */
}
