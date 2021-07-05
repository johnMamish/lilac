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

    time.sleep(0.02)

    p = pyaudio.PyAudio()

    frame = b''
    for i in range(args.chunk_size):
        frame += struct.pack('b', int(32 * math.sin(i * (2 * math.pi * 960) / args.fsamp)))

    global t_prev
    t_prev  = time.perf_counter()
    def audio_callback(in_data, frame_count, time_info, status):
        print("audio callback")
        global t_prev
        t_now = time.perf_counter()
        print(f"callback fsamp = {frame_count / (t_now - t_prev)}")
        t_prev = t_now

        if (ser.in_waiting < frame_count):
            # If the serial port doesn't have enough data in it, just send some zeros
            print(f"audio buffering: {ser.in_waiting: 6d} / {frame_count: 6d} bytes ready")
            return (b'\x00' * frame_count, pyaudio.paContinue)
        else:
            data = ser.read(frame_count)
            print(data.hex())
            return (data, pyaudio.paContinue)


    stream = p.open(format=p.get_format_from_width(1, unsigned=False),
                    channels=1,
                    rate=args.fsamp,
                    output=True,
                    frames_per_buffer=args.chunk_size,
                    stream_callback=audio_callback)

    while (1):
        time.sleep(0.1)
        print(f"{ser.in_waiting}")

    stream.stop_stream()
    stream.close()

    p.terminate()
