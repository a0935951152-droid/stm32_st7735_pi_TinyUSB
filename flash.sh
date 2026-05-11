#!/bin/bash
set -e

BIN="build/stm32f103_uart.bin"

if [ ! -f "$BIN" ]; then
    echo "Error: $BIN not found. Build first:"
    echo "  cd build && cmake .. && make -j\$(nproc)"
    exit 1
fi

echo "Flashing $BIN via J-Link SWD..."
openocd -f openocd.cfg \
    -c "program $BIN verify reset exit 0x08000000"
