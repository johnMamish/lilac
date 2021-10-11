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
# The reference implementation encoder/decoder's location should be in the OPUS_DEMO variable
#
# The reference implementation encoder/decoder should have 2 extra flags added:
#     celt_force_intra      disable usage of intra frames
#     celt_disable_pf       disable usage of post-filtering

random.seed(1)

# usage: test_band_allocation_before_skipping.py opusfile.opusdemo

target_string = "final bit allocations:"
LILAC_BASE = os.path.expanduser("~/projects/lilac/")
OPUS_DEMO = os.path.expanduser('~/projects/opus/opus-annotated/opus_demo')

for testnum in range(53600, 60000):
    # generate wav file with several different frequencies
    freqs = [random.randint(500, 2000) for i in range(10)]

    base_filename = '48ksps_' + '_'.join([str(f) for f in freqs])
    pcm_filename = base_filename + '.pcm'
    wavegen_args = ['--num-samps', '48000', '--output-file', pcm_filename, '--pcm',
                    '--amplitude', '1000']
    wavegen_args += [str(f) for f in freqs]
    wavegen_path = LILAC_BASE + 'tools/wavegen/wavegen.py'
    subprocess.run([wavegen_path] + wavegen_args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    # generate opusdemo file
    # bitrate = random.randint(53000, 72000)
    bitrate = testnum
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
    test_output_collated = b""
    for i in range(len(ground_truth_filtered)):
        test_output = subprocess.run([TEST_PROGRAM, opusdemo_filename, str(i)], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        test_output_collated += test_output.stdout

    test_output_filtered = [l for l in test_output_collated.decode('utf-8').split('\n') if l.startswith(target_string)]

    # determine whether test passed or failed
    print(f"test {testnum}, bitrate {bitrate}: ", end='')
    try:
        bad_frames = [i for i in range(len(ground_truth_filtered)) if (ground_truth_filtered[i] != test_output_filtered[i])]
    except:
        print(f"ground truth records: {len(ground_truth_filtered)}; test output records: {len(test_output_filtered)}")
        with open(f"ground_truth_stdout_test_{testnum}.log", 'w') as outfile:
            outfile.write(ground_truth.stdout.decode('utf-8'))
        with open(f"lilac_ref_stdout_test_{testnum}.log", 'w') as outfile:
            outfile.write(test_output_collated.decode('utf-8'))

        continue

    if (len(bad_frames) == 0):
        print("\x1b[32mpass\x1b[0m")
        os.remove(pcm_filename)
        os.remove(opusdemo_filename)
        os.remove(ground_truth_output_filename)
        if (False):
            for idx in range(len(ground_truth_filtered)):
                print(f"frame {idx: 3}/{len(ground_truth_filtered): 3}:")
                print(ground_truth_filtered[idx])
                print(test_output_filtered[idx])

    else:
        print("\x1b[31mfail\x1b[0m. Retaining files.")
        for badidx in bad_frames:
                print(f"frame {badidx: 3}/{len(ground_truth_filtered): 3}: \x1b[31mfail\x1b[0m")
                print(ground_truth_filtered[badidx])
                print(test_output_filtered[badidx])

        with open(f"ground_truth_stdout_test_{testnum}.log", 'w') as outfile:
            outfile.write(ground_truth.stdout.decode('utf-8'))
        with open(f"lilac_ref_stdout_test_{testnum}.log", 'w') as outfile:
            outfile.write(test_output_collated.decode('utf-8'))
