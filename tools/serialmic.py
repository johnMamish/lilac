#!/usr/bin/env python3

# Requires:
#     # (Thanks https://stackoverflow.com/questions/20023131/cannot-install-pyaudio-gcc-error)
#     sudo apt install libasound-dev portaudio19-dev libportaudio2 libportaudiocpp0
#
#     pip install pyaudio

# Reads raw, 8-bit, signed pcm audio data from uart and plays it.
# TODO: how to frame endianness? We'd like to extend to 16-bit.

import argparse
import math
import pyaudio
import serial
import struct
import sys
import time
import wave

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description='Stream raw PCM audio over uart')

    ap.add_argument('--port', dest='port', metavar='P', type=str, action='store', default='/dev/ttyUSB0',
                    help='Path to serial port.')
    ap.add_argument('--baud-rate', dest='baud_rate', action='store', default=2000000,
                    help='Baud rate.')
    ap.add_argument('--fsamp', dest='fsamp', action='store', default=48000,
                    help='Sample frequency.')
    ap.add_argument('--chunk-size', dest='chunk_size', action='store', default=2000,
                    help='Chunk size. Smaller chunk gives lower latency, but more opportunities for tearing.')

    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud_rate, timeout=0, parity=serial.PARITY_NONE)

    p = pyaudio.PyAudio()

    stream = p.open(format=p.get_format_from_width(1, unsigned=False),
                    channels=1,
                    rate=args.fsamp,
                    output=True,
                    frames_per_buffer=args.chunk_size)

    while (1):
        if (ser.inWaiting() > 0):
            stream.write(ser.read(ser.inWaiting()))

    stream.stop_stream()
    stream.close()

    p.terminate()
