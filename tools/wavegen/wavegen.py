#!/usr/bin/python3

# This utility generates wav files consisting of pure sine tones.

import argparse
import math
import matplotlib.pyplot as plt
import numpy as np
import wave

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description='Generate pure tone wav files.')

    ap.add_argument('wave_freqs', metavar='W', type=float, nargs='+',
                    help='List of wave frequencies to synthesize')
    ap.add_argument('--num-samps', dest='num_samps', type=int, action='store', default=48000,
                    help='Number of samples to synthesize')
    ap.add_argument('--show-graph', dest='show_graph', action='store_true',
                    help='Display a matplotlib graph of the signal that will be written to the wav file')
    ap.add_argument('--output-file', dest='output_file', action='store', default='a.wav',
                    help='Wav file to write to')
    ap.add_argument('--amplitude', dest='amplitude', type=float, action='store', default=32768,
                    help='Amplitude of each individual sine wave')
    ap.add_argument('--pcm', dest='pcm', action='store_true',
                    help='Omit wav headers, leaving a raw PCM file')

    args = ap.parse_args()

    FSAMP = 48000


    # Use scipy to synthesize sine waves
    output_wave = np.zeros(args.num_samps)
    for freq in args.wave_freqs:
        for samp_num in range(0, args.num_samps):
            samp = args.amplitude * math.sin((samp_num * 2 * math.pi * freq) / FSAMP)
            output_wave[samp_num] += samp

    # Clip
    for samp_num in range(0, args.num_samps):
        if (output_wave[samp_num] >= 32768): output_wave[samp_num] =  32767
        if (output_wave[samp_num] < -32768): output_wave[samp_num] = -32768

    # Convert to byte array
    output_wave_ints = output_wave.astype(np.int16)
    output_wave_bytes = output_wave_ints.tobytes()

    # Display
    if (args.show_graph):
        plt.plot(range(args.num_samps), output_wave_ints)
        plt.show()

    if (args.pcm):
        with open(args.output_file, "wb") as pcm_file:
            pcm_file.write(output_wave_bytes)
    else:
        with wave.open(args.output_file, "wb") as wav_file:
            # Configure wav file to be 16-bit mono at 48ksps
            wav_file.setnchannels(1)
            wav_file.setsampwidth(2)
            wav_file.setframerate(FSAMP)
            wav_file.setnframes(args.num_samps)

            # Write to file
            wav_file.writeframesraw(output_wave_bytes)
