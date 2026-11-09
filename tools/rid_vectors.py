#!/usr/bin/env python3
"""Reference 31-byte BLE Remote ID service-data payloads, for host tests."""
import struct

def messages():
    basic = bytearray(25)
    basic[0] = 0x02
    basic[1] = 0x12
    basic[2:22] = b'SQUACHEMIT-TEST-0001'
    loc = bytearray(25)
    loc[0] = 0x12
    loc[5:9] = struct.pack('<i', 407128000)
    loc[9:13] = struct.pack('<i', -740060000)
    loc[15:17] = struct.pack('<H', 2240)
    system = bytearray(25)
    system[0] = 0x42
    system[2:6] = struct.pack('<i', 407580000)
    system[6:10] = struct.pack('<i', -739855000)
    return [bytes(basic), bytes(loc), bytes(system)]

def advertisements():
    return [bytes([30, 0x16, 0xFA, 0xFF, 0x0D, i+1]) + m
            for i, m in enumerate(messages())]

if __name__ == '__main__':
    for b in advertisements(): print(b.hex())
