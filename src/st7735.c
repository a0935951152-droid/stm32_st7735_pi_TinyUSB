#include "st7735.h"
#include "stm32f103xb.h"

/*
 * J6 (LCD inserted REVERSED):
 *   PB15 = DI  (SPI2_MOSI, AF)
 *   PB13 = SC  (SPI2_SCK,  AF)
 *   PB12 = CS  (software)
 *   PB1  = RS  (DC: low=cmd, high=data)
 *   NRST = RST (shared with MCU reset)
 *
 * 1.44" 128x128 red-PCB modules typically need COL+2, ROW+3.
 */
#define COL_OFFSET  2
#define ROW_OFFSET  3

#define CS_LOW()   (GPIOB->BRR  = (1UL << 12))
#define CS_HIGH()  (GPIOB->BSRR = (1UL << 12))
#define DC_CMD()   (GPIOB->BRR  = (1UL << 1))
#define DC_DATA()  (GPIOB->BSRR = (1UL << 1))

extern void delay_ms(uint32_t ms);

/* ------------------------------------------------------------------ */

static inline void spi_tx(uint8_t b) {
    while (!(SPI2->SR & SPI_SR_TXE)) {}
    SPI2->DR = b;
}

static inline void spi_flush(void) {
    while (!(SPI2->SR & SPI_SR_TXE)) {}
    while (SPI2->SR & SPI_SR_BSY) {}
}

/*
 * Each LCD transaction = CS-low for the WHOLE command + all its data bytes,
 * CS-high only after the last byte fully drains. DC is toggled mid-transaction
 * (around a flush so the command byte fully clocks out before DC changes).
 */
static void wr_cmd(uint8_t cmd) {
    CS_LOW();
    DC_CMD();
    spi_tx(cmd);
    spi_flush();
    CS_HIGH();
}

static void wr_cmd_d1(uint8_t cmd, uint8_t d0) {
    CS_LOW();
    DC_CMD();
    spi_tx(cmd);
    spi_flush();
    DC_DATA();
    spi_tx(d0);
    spi_flush();
    CS_HIGH();
}

static void wr_cmd_dn(uint8_t cmd, const uint8_t *data, uint8_t n) {
    CS_LOW();
    DC_CMD();
    spi_tx(cmd);
    spi_flush();
    DC_DATA();
    for (uint8_t i = 0; i < n; i++) {
        spi_tx(data[i]);
    }
    spi_flush();
    CS_HIGH();
}

static void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t caset[4] = { 0, (uint8_t)(x0 + COL_OFFSET), 0, (uint8_t)(x1 + COL_OFFSET) };
    uint8_t raset[4] = { 0, (uint8_t)(y0 + ROW_OFFSET), 0, (uint8_t)(y1 + ROW_OFFSET) };
    wr_cmd_dn(0x2A, caset, 4);  /* CASET */
    wr_cmd_dn(0x2B, raset, 4);  /* RASET */
}

/* ------------------------------------------------------------------ */

static void gpio_spi2_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    /* PB1 (DC): GP push-pull 50MHz → CRL bits [7:4] = 0x3 */
    GPIOB->CRL &= ~(0xFUL << 4);
    GPIOB->CRL |=  (0x3UL << 4);

    /*
     * PB12 (CS) : GP push-pull 50MHz → bits [19:16] = 0x3
     * PB13 (SCK): AF push-pull 50MHz → bits [23:20] = 0xB
     * PB14 (MISO): input floating    → bits [27:24] = 0x4
     * PB15 (MOSI): AF push-pull 50MHz → bits [31:28] = 0xB
     */
    GPIOB->CRH &= ~(0xFFFFUL << 16);
    GPIOB->CRH |= (0x3UL << 16) | (0xBUL << 20)
               |  (0x4UL << 24) | (0xBUL << 28);

    CS_HIGH();
    DC_DATA();
}

static void spi2_init(void) {
    /* Configure with SPE=0 first, then enable last (per RM0008 recommendation). */
    SPI2->CR1 = SPI_CR1_SSM | SPI_CR1_SSI
              | SPI_CR1_BR_DIV8 | SPI_CR1_MSTR;
    SPI2->CR1 |= SPI_CR1_SPE;
}

/* ------------------------------------------------------------------ */

void st7735_init(void) {
    gpio_spi2_init();
    spi2_init();
    delay_ms(20);

    wr_cmd(0x01);                       /* SWRESET */
    delay_ms(150);
    wr_cmd(0x11);                       /* SLPOUT  */
    delay_ms(255);

    /* Frame rate control (Rcmd1 from Adafruit) */
    {
        uint8_t v[] = {0x01, 0x2C, 0x2D};
        wr_cmd_dn(0xB1, v, 3);
        wr_cmd_dn(0xB2, v, 3);
    }
    {
        uint8_t v[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
        wr_cmd_dn(0xB3, v, 6);
    }
    wr_cmd_d1(0xB4, 0x07);              /* INVCTR: no inversion */

    /* Power control */
    { uint8_t v[] = {0xA2, 0x02, 0x84};  wr_cmd_dn(0xC0, v, 3); }
    wr_cmd_d1(0xC1, 0xC5);
    { uint8_t v[] = {0x0A, 0x00};        wr_cmd_dn(0xC2, v, 2); }
    { uint8_t v[] = {0x8A, 0x2A};        wr_cmd_dn(0xC3, v, 2); }
    { uint8_t v[] = {0x8A, 0xEE};        wr_cmd_dn(0xC4, v, 2); }
    wr_cmd_d1(0xC5, 0x0E);               /* VMCTR1 */

    wr_cmd(0x20);                        /* INVOFF */

    /* MADCTL: MY|MX|BGR — common for 1.44" red-PCB */
    wr_cmd_d1(0x36, 0xC8);
    /* COLMOD: 16-bit RGB565 */
    wr_cmd_d1(0x3A, 0x05);

    /* Gamma */
    {
        uint8_t gp[] = {0x0F,0x1A,0x0F,0x18,0x2F,0x28,0x20,0x22,
                        0x1F,0x1B,0x23,0x37,0x00,0x07,0x02,0x10};
        wr_cmd_dn(0xE0, gp, 16);
        uint8_t gn[] = {0x0F,0x1B,0x0F,0x17,0x33,0x2C,0x29,0x2E,
                        0x30,0x30,0x39,0x3F,0x00,0x07,0x03,0x10};
        wr_cmd_dn(0xE1, gn, 16);
    }

    wr_cmd(0x13);                        /* NORON */
    delay_ms(10);
    wr_cmd(0x29);                        /* DISPON */
    delay_ms(100);
}

/* ------------------------------------------------------------------ */
/*
 * Fill / draw: address window first, then RAMWR + pixel data inside ONE
 * CS-low transaction so the controller treats every byte as GRAM data.
 */

void st7735_fill(uint16_t color) {
    uint8_t hi = color >> 8, lo = color & 0xFF;
    set_addr_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);

    CS_LOW();
    DC_CMD();  spi_tx(0x2C);  spi_flush();   /* RAMWR */
    DC_DATA();
    for (uint32_t i = 0; i < (uint32_t)ST7735_WIDTH * ST7735_HEIGHT; i++) {
        while (!(SPI2->SR & SPI_SR_TXE)) {}
        SPI2->DR = hi;
        while (!(SPI2->SR & SPI_SR_TXE)) {}
        SPI2->DR = lo;
    }
    spi_flush();
    CS_HIGH();
}

void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color) {
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;
    set_addr_window(x, y, x, y);

    CS_LOW();
    DC_CMD();  spi_tx(0x2C);  spi_flush();
    DC_DATA();
    spi_tx(color >> 8);
    spi_tx(color & 0xFF);
    spi_flush();
    CS_HIGH();
}

void st7735_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    uint8_t hi = color >> 8, lo = color & 0xFF;
    set_addr_window(x, y, x + w - 1, y + h - 1);

    CS_LOW();
    DC_CMD();  spi_tx(0x2C);  spi_flush();
    DC_DATA();
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        while (!(SPI2->SR & SPI_SR_TXE)) {}
        SPI2->DR = hi;
        while (!(SPI2->SR & SPI_SR_TXE)) {}
        SPI2->DR = lo;
    }
    spi_flush();
    CS_HIGH();
}
