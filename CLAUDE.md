# STM32F103C8T6 UART Project

## Hardware
- **Board**: STM32F103C8T6 STM32 Smart V2.0
- MCU: STM32F103C8T6, Cortex-M3
- Flash: 64K @ 0x08000000, RAM: 20K @ 0x20000000
- Clock: 72MHz (HSE 8MHz + PLL x9)
- USART1: PA9=TX, PA10=RX, 115200 8N1

## Onboard LEDs
| 編號 | 連接 | 行為 |
|------|------|------|
| D1   | 3V3 via R8 (510Ω) | 電源燈，不可控 |
| D2   | PC13 via R15 (510Ω) | 使用者燈，**active low** |

## Other Onboard Peripherals
- EEPROM: AT24C04 on I2C1 (PB6=SCL, PB7=SDA)
- User button (S2): PA0
- Reset button (S1): NRST

## ST7735 1.44" LCD (TFT header J6) — 已驗證可用

### J6 連接器（板上 8-pin TFT header）
從左到右 pin1→pin8：`3V3 | GND | PB15(MOSI) | PB13(SCK) | PB12(CS) | NRST | PB1(DC) | BL`

### LCD 模組（1.44" 128×128 紅板 ST7735）
LCD 引腳左→右：`BLK | RS | RST | CS | SC | DI | GND | VCC`

### ⚠️ 插法：LCD 必須**反插**進 J6
LCD pin1 (BLK) 對 J6 pin8 (BL)、LCD pin8 (VCC) 對 J6 pin1 (3V3)。
若反插後背光仍亮但畫面全白，先檢查驅動程式（見下方關鍵設定），最後才考慮 BLK 額外拉 3V3。

### 關鍵驅動設定（不要亂改，這套已驗證成功）
| 項目 | 值 | 原因 |
|------|----|----|
| SPI2 速率 | `BR_DIV8` (36MHz/8 = 4.5MHz) | 9MHz 會打到 ST7735 規格上限，部分副廠模組不穩 |
| MADCTL (0x36) | `0xC8` (MY\|MX\|BGR) | 1.44" 紅板正向擺放的標準值 |
| COLMOD (0x3A) | `0x05` | 16-bit RGB565 |
| COL_OFFSET | `2` | 1.44" 模組可視區從 col 2 開始 |
| ROW_OFFSET | `3` | 1.44" 模組可視區從 row 3 開始 |
| SPI mode | mode 0 (CPOL=0, CPHA=0) | ST7735 預設 |

### ⚠️ Protocol 重點：CS 必須包住整個 transaction
**這是「全白」最常見的原因 —— 不要每送一個 byte 就 toggle CS。**

正確流程：
```
CS_LOW
  DC=CMD;  spi_tx(cmd);   spi_flush
  DC=DATA; spi_tx(d0..dn); spi_flush
CS_HIGH
```

特別是 `RAMWR (0x2C)` 之後的像素資料 —— RAMWR + 全部像素必須在**同一次** CS-LOW transaction 內，否則 LCD 不會把後續 byte 當成 GRAM data。

### GPIO 設定速查（src/st7735.c 內 `gpio_spi2_init`）
- PB1  DC : GP push-pull 50MHz (CRL 設 `0x3 << 4`)
- PB12 CS : GP push-pull 50MHz (CRH 設 `0x3 << 16`)
- PB13 SCK: **AF push-pull** 50MHz (CRH 設 `0xB << 20`)
- PB14    : input floating (CRH 設 `0x4 << 24`)
- PB15 MOSI: **AF push-pull** 50MHz (CRH 設 `0xB << 28`)
- 需 enable `RCC_APB2ENR_IOPBEN` + `RCC_APB1ENR_SPI2EN`
- SPI2 預設腳位無 remap，**不需要** AFIO clock

### 初始化順序
1. `gpio_spi2_init()` → `spi2_init()` (CR1 先設不含 SPE，再 OR SPE)
2. `delay_ms(20)` → `SWRESET(0x01)` → `delay_ms(150)`
3. `SLPOUT(0x11)` → `delay_ms(255)`
4. Frame rate (B1/B2/B3) → INVCTR (B4=0x07)
5. Power control (C0~C5)
6. INVOFF (0x20) → MADCTL=0xC8 → COLMOD=0x05
7. Gamma (E0/E1，各 16 bytes)
8. NORON (0x13) → DISPON (0x29)

## Toolchain Install (Raspberry Pi)
```bash
sudo apt install gcc-arm-none-eabi cmake openocd
```

## Build
```bash
cd /home/test/stm32/build
cmake ..
make -j$(nproc)
# Output: stm32f103_uart.bin / .hex / .elf
```

## Flash via J-Link SWD
**接線（Blue Pill SWD header）：**
| J-Link 腳 | Blue Pill 腳 |
|-----------|-------------|
| SWDIO     | PA13 (DIO)  |
| SWDCLK    | PA14 (CLK)  |
| GND       | GND         |
| 3.3V      | 3V3（可選）  |

```bash
# 方法一：make flash（直接在 build 目錄）
make flash

# 方法二：手動腳本
./flash.sh
```

副廠 J-Link 若 OpenOCD 無法識別，調低速度：
在 `openocd.cfg` 將 `adapter speed 2000` 改為 `adapter speed 500`

## UART 通訊（USB-to-UART）
接線：adapter-TX → PA10, adapter-RX → PA9, GND → GND

```bash
minicom -D /dev/ttyUSB0 -b 115200
# 或
screen /dev/ttyUSB0 115200
```

## Project Structure
```
├── CMakeLists.txt                         # build + flash target；含 TinyUSB sources
├── openocd.cfg                            # J-Link SWD 設定
├── flash.sh                               # OpenOCD 燒錄腳本
├── cmake/toolchain-arm-none-eabi.cmake    # 交叉編譯工具鏈
├── ld/STM32F103C8TX_FLASH.ld              # linker script
├── startup/startup_stm32f103xb.c          # 向量表 + Reset Handler
├── lib/tinyusb/                           # 第三方：TinyUSB v0.17（device-only CDC）
├── include/
│   ├── stm32f103xb.h                      # 最小 Register 定義（RCC/GPIO/SPI/USART/FLASH）
│   ├── stm32f1xx.h                        # CMSIS-style shim：USB regs + IRQn + NVIC（給 TinyUSB 用）
│   ├── tusb_config.h                      # TinyUSB 配置（CDC class, F1 port, EP buf 大小）
│   └── st7735.h                           # LCD API + RGB565 顏色常數
├── src/
│   ├── main.c                             # clock_init (USB 48MHz) + USB IRQ handlers + tud_task loop
│   ├── st7735.c                           # ST7735 driver (SPI2)
│   ├── usb_descriptors.c                  # USB device/config/string descriptors (VID 0xCafe / PID 0x4001)
│   └── usb_cdc_app.c                      # CDC rx 狀態機 + 直接 stream 到 LCD（無 framebuffer）
└── tools/
    └── send_image.py                      # Pi 端：PIL → RGB565 → /dev/ttyACM0
```

## USB CDC 圖像接收 — 已可編譯

### 主機 ↔ 裝置 通訊
**主機端**：Pi 看到 `/dev/ttyACM0`（CDC ACM virtual COM port）。
**裝置端**：STM32F103 USB 周邊（PA11=D-、PA12=D+），TinyUSB fsdev port。

### 線材格式（Pi → STM32）
```
[0xA5] [0x5A] [W] [H]   then   W*H*2 bytes of RGB565（hi-byte 先）
```
W、H 各 1 byte，最大 128。128×128 = 32,768 像素 + 4 byte header = **32,772 bytes**。

### 關鍵時脈設定（**勿動**）
- SYSCLK = HSE 8MHz × PLL9 = 72MHz
- USBPRE = 0（CFGR bit 22）→ USB 48MHz = SYSCLK/1.5
- RCC_APB1ENR_USBEN（bit 23）要 enable

### USB pull-up re-enumeration trick
F103 沒有內建 D+ pull-up，板上 1.5kΩ 是寫死接 3V3 — 也就是 host 在 MCU 跑起來前就「看到」我們了，會錯過第一輪 enumeration。`usb_renumerate()` 在開機時把 PA12 拉 low 20ms，host 看到斷線，釋放後重新 enumerate。**這個函數一定要在 tud_init() 之前呼叫。**

### 流式像素：不要在 RAM 開 framebuffer
RAM 只有 20K，一張全螢幕 32K 根本不夠。CDC rx callback 拿到 byte 後直接 `SPI2->DR = byte` 推給 LCD。整張圖在「同一次 CS-LOW transaction」內完成（沿用 ST7735 那條規則）。

### 流程
1. `clock_init()` → 72MHz + USB 48MHz + enable USB peripheral clock
2. `led_init()`、`st7735_init()`、`st7735_fill(BLACK)`
3. `usb_renumerate()` — 強迫 host 重新枚舉
4. `tud_init(0)`
5. main loop：`tud_task()`
6. rx callback：`feed_byte()` 走 magic→W→H→pixels 狀態機

### Pi 端使用
```bash
sudo apt install python3-serial python3-pil
./tools/send_image.py avatar.png            # 預設 /dev/ttyACM0
./tools/send_image.py avatar.jpg /dev/ttyACM1
```
任何 PIL 看得懂的圖（PNG/JPG/BMP）會自動 resize 成 128×128 → RGB565 → 推送。

### 燒進去之後檢查清單
```bash
lsusb | grep Cafe                # 應該看到 Anthropic Demo / STM32 ImgRecv
ls /dev/ttyACM*                   # 應該出現 ttyACM0
dmesg | tail -20 | grep -i usb   # 看 enumeration 訊息
```
若 `lsusb` 看不到裝置：
1. 板上的 D+ pull-up 是否為 1.5kΩ（部分 Blue Pill clone 是錯的 10kΩ）
2. PA12 接腳是否被 LCD 或其他電路佔用（這板子沒有，但要排除）
3. USB 線是 data 線（不是只能充電的）

## Key Notes
- 純 bare-metal，無 HAL/CubeMX
- printf 透過 `_write` 導向 USART1
- 多數 C8T6 實際有 128K flash，可修改 linker script 的 `LENGTH = 128K`
