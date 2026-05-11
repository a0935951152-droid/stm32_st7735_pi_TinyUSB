#pragma once
#include <stdint.h>

#define __IO volatile

/* SysTick (Cortex-M3 core) */
#define SysTick_BASE     0xE000E010UL

typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t LOAD;
    __IO uint32_t VAL;
    __IO uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick  ((SysTick_TypeDef *)SysTick_BASE)

#define SysTick_CTRL_ENABLE     (1UL << 0)
#define SysTick_CTRL_CLKSOURCE  (1UL << 2)
#define SysTick_CTRL_COUNTFLAG  (1UL << 16)

/* Peripheral base addresses */
#define PERIPH_BASE      0x40000000UL
#define APB1PERIPH_BASE  PERIPH_BASE
#define APB2PERIPH_BASE  (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE   (PERIPH_BASE + 0x00020000UL)

#define FLASH_R_BASE     (AHBPERIPH_BASE + 0x00002000UL)
#define RCC_BASE         (AHBPERIPH_BASE + 0x00001000UL)
#define GPIOA_BASE       (APB2PERIPH_BASE + 0x00000800UL)
#define GPIOB_BASE       (APB2PERIPH_BASE + 0x00000C00UL)
#define GPIOC_BASE       (APB2PERIPH_BASE + 0x00001000UL)
#define SPI1_BASE        (APB2PERIPH_BASE + 0x00003000UL)
#define SPI2_BASE        (APB1PERIPH_BASE + 0x00003800UL)
#define USART1_BASE      (APB2PERIPH_BASE + 0x00003800UL)
#define USART2_BASE      (APB1PERIPH_BASE + 0x00004400UL)
#define USART3_BASE      (APB1PERIPH_BASE + 0x00004800UL)

/* Peripheral structs */
typedef struct {
    __IO uint32_t CRL, CRH, IDR, ODR, BSRR, BRR, LCKR;
} GPIO_TypeDef;

typedef struct {
    __IO uint32_t CR, CFGR, CIR, APB2RSTR, APB1RSTR;
    __IO uint32_t AHBENR, APB2ENR, APB1ENR, BDCR, CSR;
} RCC_TypeDef;

typedef struct {
    __IO uint32_t CR1, CR2, SR, DR, CRCPR, RXCRCR, TXCRCR, I2SCFGR, I2SPR;
} SPI_TypeDef;

typedef struct {
    __IO uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
} USART_TypeDef;

typedef struct {
    __IO uint32_t ACR, KEYR, OPTKEYR, SR, CR, AR, RESERVED, OBR, WRPR;
} FLASH_TypeDef;

#define SPI1    ((SPI_TypeDef *)SPI1_BASE)
#define SPI2    ((SPI_TypeDef *)SPI2_BASE)
#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define RCC     ((RCC_TypeDef *)RCC_BASE)
#define USART1  ((USART_TypeDef *)USART1_BASE)
#define USART2  ((USART_TypeDef *)USART2_BASE)
#define USART3  ((USART_TypeDef *)USART3_BASE)
#define FLASH   ((FLASH_TypeDef *)FLASH_R_BASE)

/* RCC_CR */
#define RCC_CR_HSEON    (1UL << 16)
#define RCC_CR_HSERDY   (1UL << 17)
#define RCC_CR_PLLON    (1UL << 24)
#define RCC_CR_PLLRDY   (1UL << 25)

/* RCC_CFGR */
#define RCC_CFGR_SW_PLL      (0x2UL << 0)
#define RCC_CFGR_SWS_PLL     (0x2UL << 2)
#define RCC_CFGR_HPRE_DIV1   (0x0UL << 4)
#define RCC_CFGR_PPRE1_DIV2  (0x4UL << 8)
#define RCC_CFGR_PPRE2_DIV1  (0x0UL << 11)
#define RCC_CFGR_PLLSRC_HSE  (1UL << 16)
#define RCC_CFGR_PLLMULL9    (0x7UL << 18)

/* RCC_APB2ENR */
#define RCC_APB2ENR_AFIOEN   (1UL << 0)
#define RCC_APB2ENR_IOPAEN   (1UL << 2)
#define RCC_APB2ENR_IOPBEN   (1UL << 3)
#define RCC_APB2ENR_IOPCEN   (1UL << 4)
#define RCC_APB2ENR_USART1EN (1UL << 14)

/* RCC_APB1ENR */
#define RCC_APB1ENR_SPI2EN   (1UL << 14)
#define RCC_APB1ENR_USART2EN (1UL << 17)
#define RCC_APB1ENR_USART3EN (1UL << 18)

/* SPI_CR1 */
#define SPI_CR1_CPHA    (1UL << 0)
#define SPI_CR1_CPOL    (1UL << 1)
#define SPI_CR1_MSTR    (1UL << 2)
#define SPI_CR1_BR_DIV4 (0x1UL << 3)
#define SPI_CR1_BR_DIV8 (0x2UL << 3)
#define SPI_CR1_SPE     (1UL << 6)
#define SPI_CR1_SSI     (1UL << 8)
#define SPI_CR1_SSM     (1UL << 9)

/* SPI_SR */
#define SPI_SR_RXNE     (1UL << 0)
#define SPI_SR_TXE      (1UL << 1)
#define SPI_SR_BSY      (1UL << 7)

/* FLASH_ACR */
#define FLASH_ACR_LATENCY_2  (0x2UL << 0)
#define FLASH_ACR_PRFTBE     (1UL << 4)

/* USART_SR */
#define USART_SR_RXNE  (1UL << 5)
#define USART_SR_TC    (1UL << 6)
#define USART_SR_TXE   (1UL << 7)

/* USART_CR1 */
#define USART_CR1_RE   (1UL << 2)
#define USART_CR1_TE   (1UL << 3)
#define USART_CR1_UE   (1UL << 13)
