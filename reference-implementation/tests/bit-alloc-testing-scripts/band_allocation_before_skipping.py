#!/usr/bin/python3

import os
import random
import subprocess
import sys

# This test script expects the opus reference implementation decoder to print bit allocations in
# 1/8th bits after fractional quality factor determination, but before band skipping and bit
# redistribution. e.g.
#     1/8th bit allocations before band skipping: {  514  514  514  514  501  491  482  ....  773 1111}
#
# The reference implementation decoder should be located

random.seed(1)

# usage: test_band_allocation_before_skipping.py opusfile.opusdemo

target_string = "1/8th bit allocations before band skipping:"
LILAC_BASE = os.path.expanduser("~/projects/lilac/")
OPUS_DEMO = os.path.expanduser('~/projects/opus/opus-annotated/opus_demo')

for testnum in range(10):
    # generate wav file
    freq = random.randint(50, 5000)
    base_filename = '48ksps_' + str(freq)
    pcm_filename = base_filename + '.pcm'
    wavegen_args = ['--num-samps', '48000', '--output-file', pcm_filename, '--pcm',
                    '--amplitude', '1000', str(freq)]
    wavegen_path = LILAC_BASE + 'tools/wavegen/wavegen.py'
    subprocess.run([wavegen_path] + wavegen_args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    # generate opusdemo file
    bitrate = random.randint(0x0c000, 0x30000)
    opusdemo_filename = base_filename + '_' + str(testnum) + '.opusdemo'
    subprocess.run([OPUS_DEMO, '-e', 'audio', '48000', '1', str(bitrate), '-celt_force_intra',
                    '-celt_disable_pf', pcm_filename, opusdemo_filename],
                   stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)


    # decode file with opus_demo
    ground_truth_output_filename = 'groundtruth.pcmout'
    ground_truth  = subprocess.run([OPUS_DEMO, '-d', '48000', '1', opusdemo_filename,
                                    ground_truth_output_filename],
                                   stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    ground_truth_filtered = [l for l in ground_truth.stdout.decode('utf-8').split('\n') if l.startswith(target_string)]

    # decode file with lilac reference impl
    TEST_PROGRAM = os.path.expanduser('~/projects/lilac/reference-implementation/celt-decode-frame')
    test_output_collated = []
    for i in range(len(ground_truth_filtered)):
        test_output = subprocess.run([TEST_PROGRAM, opusdemo_filename, str(i)], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        test_output_filtered = [l for l in test_output.stdout.decode('utf-8').split('\n') if l.startswith(target_string)]
        test_output_collated.append(test_output_filtered[0])

    print(f"test {testnum}, bitrate {bitrate}:")
    for i in range(len(ground_truth_filtered)):
        if (ground_truth_filtered[i] == test_output_collated[i]):
            print(f"frame {i}: \x1b[32mpass\x1b[0m")
        else:
            print(f"frame {i}: \x1b[31mfail\x1b[0m")
            print(ground_truth_filtered[i])
            print(test_output_collated[i])
    print()
