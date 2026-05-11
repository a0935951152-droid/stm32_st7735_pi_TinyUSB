<p align="center">
  <img src="claude.jpg" width="120" alt="logo">
</p>

# STM32 ST7735 + Raspberry Pi + TinyUSB

純 bare-metal STM32F103C8T6 專案：把樹莓派透過 mini-USB 推過來的圖片，即時顯示在 1.44" 128×128 ST7735 LCD 上。

- **MCU**：STM32F103C8T6（Cortex-M3，72 MHz）
- **板子**：STM32 Smart V2.0（Blue Pill 衍生板）
- **LCD**：1.44" ST7735 紅版 128×128（接在板上 J6 TFT header）
- **USB**：mini-USB → 走 TinyUSB device CDC，Pi 看到 `/dev/ttyACM0`
- **編譯**：CMake + arm-none-eabi-gcc，**完全無 HAL/CubeMX**
- **燒錄**：副廠 J-Link / OpenOCD，走 SWD

---

## 架構一覽

```
Pi  ──USB──▶  STM32F103  ──SPI2──▶  ST7735 LCD
        CDC               9MHz 4.5MHz
   /dev/ttyACM0           CS-per-transaction
   4-byte header
   + raw RGB565
```

像素是 **streaming** 從 USB 收到就直接寫 SPI2，沒有 framebuffer（RAM 只有 20K，全螢幕 32K 放不下）。

## 線材格式（Pi → STM32）

```
offset  size   content
0       1      magic 0xA5
1       1      magic 0x5A
2       1      width  (1..128)
3       1      height (1..128)
4       W*H*2  RGB565 像素（hi-byte 先）
```

128×128 一張總共 **32,772 bytes**。

## 編譯與燒錄

### 1. 安裝工具鏈（Raspberry Pi）
```bash
sudo apt install gcc-arm-none-eabi cmake openocd python3-serial python3-pil
```

### 2. 取得 TinyUSB（被 .gitignore 排除，需自行 clone）
```bash
git clone --depth=1 --branch=0.17.0 \
    https://github.com/hathach/tinyusb.git lib/tinyusb
```

### 3. Build
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

輸出：`stm32f103_uart.bin` / `.hex` / `.elf`。

### 4. Flash（J-Link SWD）
**接線**：

| J-Link | Blue Pill |
|--------|-----------|
| SWDIO  | PA13      |
| SWDCLK | PA14      |
| GND    | GND       |
| 3V3    | 3V3（可選）|

```bash
make flash
```

副廠 J-Link 若 OpenOCD 無法識別，把 `openocd.cfg` 的 `adapter speed` 從 2000 改成 500。

### 5. 接 LCD（J6 TFT header，**反插**）
LCD pin1 (BLK) 對 J6 pin8 (BL)；LCD pin8 (VCC) 對 J6 pin1 (3V3)。詳見 `CLAUDE.md`。

### 6. 傳圖片（Pi → STM32）
拔掉 SWD，把 mini-USB 接到 Pi：
```bash
lsusb | grep Cafe          # 應該看到 "Anthropic Demo STM32 ImgRecv"
ls /dev/ttyACM*            # 應該出現 ttyACM0
```

**權限**：第一次跑會碰到 `Permission denied: '/dev/ttyACM0'`，把使用者加進 `dialout` 群組：
```bash
sudo usermod -aG dialout $USER
# 登出再登入（或重開機）讓群組生效
```
若只想先測一次：直接 `sudo ./tools/send_image.py ...`。

```bash
./tools/send_image.py avatar.png            # 預設 /dev/ttyACM0
./tools/send_image.py avatar.jpg /dev/ttyACM1
```

任何 PIL 看得懂的圖（PNG/JPG/BMP）會自動 resize 成 128×128 → RGB565 → 推送。

## 專案結構

```
├── CMakeLists.txt                         # build + flash target
├── openocd.cfg / flash.sh                 # J-Link SWD 設定
├── cmake/toolchain-arm-none-eabi.cmake    # 交叉編譯工具鏈
├── ld/STM32F103C8TX_FLASH.ld              # linker script
├── startup/startup_stm32f103xb.c          # 向量表 + Reset Handler
├── include/
│   ├── stm32f103xb.h                      # 最小 Register 定義
│   ├── stm32f1xx.h                        # CMSIS-style shim（給 TinyUSB）
│   ├── tusb_config.h                      # TinyUSB 設定
│   └── st7735.h                           # LCD API
├── src/
│   ├── main.c                             # clock + LED + USB init + main loop
│   ├── st7735.c                           # ST7735 driver (SPI2)
│   ├── usb_descriptors.c                  # USB descriptors (VID 0xCafe / PID 0x4001)
│   └── usb_cdc_app.c                      # CDC rx 狀態機 → 直接 stream 到 LCD
├── tools/
│   └── send_image.py                      # Pi 端 host script
└── lib/tinyusb/                           # 第三方（.gitignore，需自行 clone）
```

## 已踩過的坑（重要）

1. **ST7735 全白 = CS toggle 錯誤**：每送一個 byte 就 toggle CS，會讓 LCD 把指令切成多段獨立 transaction，特別是 `RAMWR (0x2C)` 之後的像素 — 整張圖必須在**同一次** CS-LOW 內。
2. **1.44" 紅版 ST7735 的 magic 數字**：MADCTL=`0xC8`、COL_OFFSET=2、ROW_OFFSET=3、SPI BR_DIV8 (4.5 MHz)、SPI mode 0。
3. **USB pull-up always-on**：F103 沒有內建 D+ pull-up，板上 1.5kΩ 寫死接 3V3，host 在 MCU 起來前就「看到」我們。`usb_renumerate()` 在開機時把 PA12 拉 low 20 ms 強迫 host 重新枚舉。
4. **USB clock 必須 48 MHz**：CFGR USBPRE=0 → SYSCLK/1.5 → 72MHz/1.5 = 48MHz。
5. **LCD 必須反插 J6**：LCD pin1 (BLK) 對 J6 pin8 (BL)。

詳細請看 [`CLAUDE.md`](CLAUDE.md)。

## License

本專案自寫的程式碼採 MIT License。第三方部分：
- **TinyUSB** — MIT，未含於本 repo，請依步驟自行 clone

## Credits

- 開發協作：Claude（Anthropic）
- TinyUSB：[hathach/tinyusb](https://github.com/hathach/tinyusb)
- 板子文件：[STM32-Base](https://stm32-base.org/boards/STM32F103C8T6-STM32-Smart-V2.0)
