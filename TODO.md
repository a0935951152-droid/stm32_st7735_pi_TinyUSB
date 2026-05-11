# TODO

## 🟥 必須驗證（next step）
- [x] ~~**USB enumeration 實機測試**：板子接到 Pi，`/dev/ttyACM0` 出現~~ ✅ 2026-05-12
- [x] ~~**D+ pull-up 電阻檢查**~~ ✅ enumeration 成功代表阻值正確，不用外掛
- [ ] **完整圖片傳輸測試**：`tools/send_image.py claude.jpg` 看圖是否正確顯示在 LCD（先解 dialout 權限）

## 🟧 短期可優化
- [ ] **傳輸進度顯示**：在 LCD 邊緣畫一條進度條（用前幾個 row 當進度條，圖片內容從第 4 row 開始畫）
- [ ] **開機 splash screen**：顯示版本號或 Claude logo（把 `claude.jpg` 預先轉成 const 陣列存 flash）
- [ ] **DMA-driven SPI2**：目前是 CPU polling SPI TXE，改成 DMA 可以讓 CPU 同時處理下一段 USB 資料
- [ ] **加 checksum / framing**：協議加上 CRC16 或 magic-end footer，傳輸過程出錯能自動 resync
- [ ] **支援更大圖片 + 自動裁切顯示**：>128×128 的圖在 host 端就裁好，但裝置端可以接受並只顯示中心 128×128

## 🟨 中期功能
- [ ] **多圖佇列 / slideshow 模式**：host 連續推多張，裝置端可控制顯示時間
- [ ] **PC 端 GUI（樹莓派桌面）**：拖一張圖到視窗就推送，省掉 CLI
- [ ] **支援 ANSI escape 在 USB 通道做控制指令**：例如 `CLEAR`、`FILL <color>`、`TEXT <x> <y> <str>`
- [ ] **加上 bitmap 字型**：可以從 host 推文字命令過來顯示，做出真正的 USB 外接小螢幕

## 🟩 長期 / 探索
- [ ] **SD 卡 fallback**：脫機顯示，外接 SPI1 SD card + FatFS
- [ ] **USB MSC**：把裝置變成 USB 隨身碟，host 丟檔到 LCD\_IMG.BIN，按鈕觸發顯示
- [ ] **PNG 解碼 on MCU**：用 uPNG 之類小函式庫，host 直接傳 PNG 就好
- [ ] **更換 LCD 模組**：1.8" / 2.4" / ST7789 等等
- [ ] **加裝按鈕 (S2 = PA0)**：本地切換顯示模式

## 🛠 程式碼整理
- [ ] **`include/stm32f1xx.h` 註解補完**：每組 USB 暫存器位元說明 RM0008 章節編號
- [ ] **`usb_cdc_app.c` 跟 `st7735.c` 抽共用 SPI 低階函數**：目前 `wr_cmd_dn` 在兩個檔複製了
- [ ] **CMakeLists.txt 加 debug / release 兩種 build type**：debug 帶 `-Og -g3` 方便用 GDB

## 📝 文件
- [ ] **加幾張實體照片**：板子+LCD+SWD 的接線、USB 接上 Pi 之後的樣子、實際顯示效果
- [ ] **錄個 demo 影片連結**：放 README 最上面
- [ ] **板子的中文/英文雙語 README**：目前是中文為主
