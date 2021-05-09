# Useful one-liners for copying and pasting to the terminal

# Compress a wav file to opus using very no-frills options
opusenc --comp 0 --hard-cbr --framesize 10 --bitrate 64.000 48ksps_1kHz.wav out.opus

opusenc --hard-cbr \        # constant bit-rate
        --comp 0 \          # encoding complexity of 0
        --framesize 10 \    # 10 millisecond frame size
        --bitrate 64.000 \  # 64kbps
        48ksps_1kHz.wav out.opus

# Dump frame from an opus file in a format that can be pasted into a C array.
# e.g. yields
# 0xf0, 0x5f, 0x48, 0x95, ....
hexdump -n $((0x50)) -s $((0x3c8)) -ve '1/1 "0x%02x, "'  opusfile.opus
