# Opus uses a triangular distribution for one of its range-coded symbols. I'm trying to work out
# how they arrived at the formulae they use to decode the symbol.

import math

def calculate_ft(qn):
    return ((qn >> 1) + 1) * ((qn >> 1) + 1)

def calculate_theta_from_triangular_distribution(qn, fm):
    """
    fm   - Decoded uniform symbol that represents a point on the PDF
    qn   - max value for theta (inclusive)
    """
    ft  = calculate_ft(qn)
    mid = ((qn >> 1) * ((qn >> 1) + 1)) >> 1
    #print(f"ft = {ft}, mid = {mid}")

    try:
        if (fm < mid):
            return (math.floor(math.sqrt((8 * fm) + 1)) - 1) >> 1
        else:
            maximum = 2 * (qn + 1)
            return (maximum - math.floor(math.sqrt(8 * (ft - fm - 1) + 1))) >> 1
    except:
        return float('Nan')

def generate_my_histogram_guess(qn):
    mid = qn >> 1
    ret = {}
    if (qn % 2 == 0):
        for i in range(0, mid):
            ret[i] = (i + 1)

        for i in range(mid, qn + 1):
            ret[i] = (qn - i + 1)
    else:
        for i in range(0, mid):
            ret[i] = (i + 1)

        for i in range(mid, qn):
            ret[i + 1] = (qn - i)


    return ret

def prettyprint_histogram(histo):
    for key in histo:
        print(f"{key:3}: {histo[key]:3}")

qn = 7
ft = calculate_ft(qn)
vals = [calculate_theta_from_triangular_distribution(qn, fm) for fm in range(0, ft)]


histo = {}
for v in vals:
    try:
        histo[v] += 1
    except KeyError:
        histo[v] = 1

print(f"histogram for qn = {qn:3}:")
prettyprint_histogram(histo)
print("")
prettyprint_histogram(generate_my_histogram_guess(qn))

print(histo == generate_my_histogram_guess(qn))

print("sanity testing values of qn in [4, 255]:")
for qn in range(4, 256):
    ft = calculate_ft(qn)
    vals = [calculate_theta_from_triangular_distribution(qn, fm) for fm in range(0, ft)]

    histo = {}
    for v in vals:
        try:
            histo[v] += 1
        except KeyError:
            histo[v] = 1

    print(f"{qn:3}: {histo == generate_my_histogram_guess(qn)}")
