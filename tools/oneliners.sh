# Useful one-liners for copying and pasting to the terminal

# Compress a wav file to opus using very no-frills options
opusenc --comp 0 --hard-cbr --framesize 10 --bitrate 64.000 48ksps_1kHz.wav out.opus

opusenc --hard-cbr \        # constant bit-rate
        --comp 0 \          # encoding complexity of 0
        --framesize 10 \    # 10 millisecond frame size
        --bitrate 64.000 \  # 64kbps
        48ksps_1kHz.wav out.opus
