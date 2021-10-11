#!/usr/bin/python3

# Given an array representing the PDF of a symbol, this small utility gives back fl and fh.

# 480-sample intra-frame coarse energy predictor error PDF.
# Wait... this is definately wrong.
arr = [21, 178,  59, 110,  71,  86,  75,  85,  84,  83,  91,  66,  88,  73, 87,  72,  92,  75,  98,  72, 105,  58, 107,  54, 115,  52, 114,  55, 112,  56, 129,  51, 132,  40, 150,  33, 140,  29,  98,  35,  77,  42]

fl = []
fh = []
total = 0
for i in range(len(arr)):
    fl.append(total)
    total += arr[i]
    fh.append(total)

print(fl)
print(fh)
print("ft = " + str(total))
