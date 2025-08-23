#!/usr/bin/env python3
import os
import sys

def generate_key_parts(key: str):
    key_bytes = key.encode("utf-8")
    mask_bytes = os.urandom(len(key_bytes))  # 随机掩码
    part1 = [b ^ m for b, m in zip(key_bytes, mask_bytes)]
    part2 = list(mask_bytes)

    print("// Key length:", len(key_bytes))
    print("static const unsigned char key_part1[] = {", ", ".join(f"0x{b:02X}" for b in part1), "};")
    print("static const unsigned char key_part2[] = {", ", ".join(f"0x{b:02X}" for b in part2), "};")

def main():
    if len(sys.argv) < 2:
        print("Usage: python gen_key_parts.py <YourSecretKey>")
        sys.exit(1)

    key = sys.argv[1]
    generate_key_parts(key)

if __name__ == "__main__":
    main()
