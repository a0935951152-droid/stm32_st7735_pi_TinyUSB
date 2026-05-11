#!/usr/bin/env python3
"""
Send an image to the STM32 ST7735 over USB CDC.

Wire format (matches src/usb_cdc_app.c):
    [0xA5] [0x5A] [W] [H] then W*H*2 bytes RGB565 (hi-byte first)

Usage:
    pip install pyserial pillow
    ./send_image.py path/to/image.png            # default /dev/ttyACM0
    ./send_image.py path/to/image.jpg /dev/ttyACM0
"""
import sys
import struct
from PIL import Image
import serial

W, H = 128, 128

def to_rgb565(im):
    im = im.convert("RGB").resize((W, H), Image.LANCZOS)
    out = bytearray(W * H * 2)
    px = im.load()
    i = 0
    for y in range(H):
        for x in range(W):
            r, g, b = px[x, y]
            v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            out[i]     = (v >> 8) & 0xFF   # hi first
            out[i + 1] = v & 0xFF
            i += 2
    return bytes(out)

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    img_path = sys.argv[1]
    port = sys.argv[2] if len(sys.argv) > 2 else "/dev/ttyACM0"

    im = Image.open(img_path)
    pixels = to_rgb565(im)
    header = struct.pack("BBBB", 0xA5, 0x5A, W, H)
    payload = header + pixels

    with serial.Serial(port, 115200, timeout=2) as s:
        s.write(payload)
        s.flush()

    print(f"Sent {len(payload)} bytes to {port} "
          f"({W}x{H} RGB565 + 4-byte header)")

if __name__ == "__main__":
    main()
