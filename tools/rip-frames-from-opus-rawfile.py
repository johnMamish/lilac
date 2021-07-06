#!/usr/bin/python3

# rips a given frame number from an opus raw binary file output by opus_demo

import argparse
import textwrap
import struct

def read_frame(f):
    """
    Given a raw opus binary file, reads the next frame
    Returns a tuple containing (frame length, ending 'rng' value, [data])
    """
    length, = struct.unpack(">i", f.read(4))
    rng = f.read(4)
    data = f.read(length)

    return length, rng, data

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('input_file', type=str, help='input filename')
    parser.add_argument('frame_number', type=int, help='frame number to dump')

    args = parser.parse_args()

    print(args.input_file)
    print(args.frame_number)

    with open(args.input_file, "rb") as f:
        idx = 0
        while (idx < args.frame_number):
            length, rng, values = read_frame(f)
            print(f"Read frame {idx} with {length} bytes")
            idx += 1

        length, rng, values = read_frame(f)
        print(f"Outputting frame {idx} with {length} bytes")
        print("\n    ".join(textwrap.wrap(", ".join(["0x{:02x}".format(int(b)) for b in values]))))
