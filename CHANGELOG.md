# Changelog

本專案使用 [Keep a Changelog](https://keepachangelog.com/zh-TW/1.1.0/) 格式，版本號遵循 [Semantic Versioning](https://semver.org/lang/zh-TW/)。

## [Unreleased]

### Verified — 實機驗證結果
- ✅ **USB CDC enumeration 成功**：板上 D+ pull-up 阻值正確，`/dev/ttyACM0` 在 Pi 上自動出現
- 📝 **Linux 權限**：使用者需加入 `dialout` 群組才能不靠 sudo 開 ACM 裝置（README 已補步驟）

## [0.1.0] — 2026-05-12

第一個可用版本：從零開始 bare-metal、跑通 LED → ST7735 LCD → USB CDC 收圖整條流水線。

### Added — 基礎框架
- CMake + arm-none-eabi-gcc 交叉編譯工具鏈設定（`cmake/toolchain-arm-none-eabi.cmake`）
- STM32F103C8T6 最小 Register 定義（`include/stm32f103xb.h`）
- 向量表 + Reset Handler（`startup/startup_stm32f103xb.c`）
- Linker script，64K Flash @ 0x08000000，20K RAM @ 0x20000000（`ld/STM32F103C8TX_FLASH.ld`）
- 副廠 J-Link SWD 燒錄（`openocd.cfg`、`flash.sh`、CMake `flash` target）

### Added — LED 測試
- `clock_init()`：HSE 8 MHz + PLL×9 = 72 MHz SYSCLK
- `delay_ms()`：SysTick 軟體延遲
- PC13 active-low 板載使用者燈閃爍

### Added — ST7735 1.44" LCD（128×128 紅版）驅動
- SPI2 driver：PB15 MOSI / PB13 SCK / PB12 CS / PB1 DC，4.5 MHz (BR_DIV8)
- ST7735R 初始化序列（frame rate、power control、gamma、INVOFF、DISPON、NORON）
- MADCTL=`0xC8`（MY+MX+BGR），COL_OFFSET=2、ROW_OFFSET=3
- API：`st7735_fill()` / `st7735_draw_pixel()` / `st7735_draw_rect()`
- RGB565 顏色常數

### Added — USB CDC（TinyUSB）
- TinyUSB v0.17 整合（device mode、CDC class、`stm32_fsdev` port）
- 自寫的 `stm32f1xx.h` CMSIS-style shim：USB 暫存器、IRQn enum、NVIC 函數、`__DSB`/`__ISB`
- USB descriptors：VID `0xCafe` / PID `0x4001`，product string `STM32 ImgRecv`
- `clock_init()` 加入 USB 48 MHz（CFGR USBPRE=0，SYSCLK/1.5）
- `usb_renumerate()`：開機時 PA12 拉 low 20 ms 強迫 host 重新枚舉
- USB IRQ handlers：`USB_LP_CAN1_RX0`、`USB_HP_CAN1_TX`、`USBWakeUp` → `tud_int_handler()`

### Added — 圖像接收
- CDC rx 狀態機：4-byte header (`0xA5 0x5A W H`) + W×H 像素 RGB565
- **流式像素**：byte 一進來就直接 `SPI2->DR = byte`，沒有 framebuffer（RAM 不夠放 32K 圖）
- 整張圖在同一次 CS-LOW transaction 內完成
- LED 每寫完一 row toggle 一次作為 liveness indicator
- Pi 端 Python 腳本（`tools/send_image.py`）：PIL 自動 resize/quantize → RGB565 → 寫 `/dev/ttyACM0`

### Fixed — 開發過程中踩到的關鍵 bug
- **ST7735 開機全白**：每送 byte 就 toggle CS 導致 LCD 把 RAMWR 跟像素切成多段 transaction。修法：整個 command + 全部 data bytes 共用一次 CS-LOW。沿用此規則寫流式接收也通了。
- **`-Wmisleading-indentation` 警告**：`while(...);` 後接 macro 展開造成假錯覺。改用 `while(...) {}` 分行。
- **LCD 反插方向**：J6 連接器 pinout 跟 LCD 模組順序相反，必須 180° 翻轉插入。

### Notes — 還未在實體 USB 上驗證
USB CDC 程式碼編譯成功（14.7 KB text、1.5 KB BSS）並已燒進板子，但 enumeration 在硬體上是否成功還沒測過。如果 `lsusb` 看不到裝置，常見原因是板上 D+ pull-up 電阻錯成 10 kΩ（應為 1.5 kΩ），需要硬體確認。

[Unreleased]: https://github.com/a0935951152-droid/stm32_st7735_pi_TinyUSB/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/a0935951152-droid/stm32_st7735_pi_TinyUSB/releases/tag/v0.1.0
