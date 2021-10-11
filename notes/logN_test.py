import math

eBands = [0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 34, 40, 48, 60, 78, 100]

band_widths = []
for i in range(0, len(eBands) - 1):
    band_widths.append(eBands[i + 1] - eBands[i])

# This is the value that mode->logN[i] has for i = [0, 21)
opus_logN = [0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 16, 16, 16, 21, 21, 24, 29, 34, 36]

their_pulse_cap_vals = []
my_pulse_cap_vals = []

for LM in range(0, 4):
    print(f"LM = {LM:2}\n", end='')
    for band_number in range(0, 21):
        print(f"{band_widths[band_number]:3}, ", end='')
    print("\n", end='')

    for band_number in range(0, 21):
        their_pulse_cap = opus_logN[band_number] + (LM * (1 << 3))
        their_pulse_cap_vals.append(their_pulse_cap)
        print(f"{their_pulse_cap:3}, ", end='')
    print("\n", end='')

    for band_number in range(0, 21):
        my_pulse_cap = math.ceil(8 * math.log2(band_widths[band_number] * (1 << LM)))
        my_pulse_cap_vals.append(my_pulse_cap)
        print(f"{my_pulse_cap:3}, ", end='')

    if (my_pulse_cap_vals != their_pulse_cap_vals):
        print("BAD")
    else:
        print("GOOD")
    print("\n\n", end='')
