cache50 = [
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   0,   0,   0,  41,  41,  41,  82,  82, 123, 164, 200, 222,
     0,   0,   0,   0,   0,   0,   0,   0,  41,  41,  41,  41, 123, 123, 123, 164, 164, 240, 266, 283, 295,
    41,  41,  41,  41,  41,  41,  41,  41, 123, 123, 123, 123, 240, 240, 240, 266, 266, 305, 318, 328, 336,
   123, 123, 123, 123, 123, 123, 123, 123, 240, 240, 240, 240, 305, 305, 305, 318, 318, 343, 351, 358, 364,
   240, 240, 240, 240, 240, 240, 240, 240, 305, 305, 305, 305, 343, 343, 343, 351, 351, 370, 376, 382, 387
]

band_sizes_0 = [1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 6, 6, 8, 12, 18, 22]

band_sizes_all = []

band_sizes_all += [(round(b / 2) if (b > 1) else -1) for b in band_sizes_0]
band_sizes_all += [b * 1 for b in band_sizes_0]
band_sizes_all += [b * 2 for b in band_sizes_0]
band_sizes_all += [b * 4 for b in band_sizes_0]
band_sizes_all += [b * 8 for b in band_sizes_0]

print(band_sizes_all)

size_to_idx_dict = {cache50[i]: band_sizes_all[i] for i in range(len(cache50))}
for k in sorted(size_to_idx_dict.keys()):
    print(f"{k}: {size_to_idx_dict[k]}")

for k in sorted(size_to_idx_dict.keys()):
    print(f"{size_to_idx_dict[k]:4},", end='')

print("")


ours = [   -1,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
   -1,   15,   23,   28,   31,   34,   36,   38,   39,   41,   42,   43,   44,   45,   46,   47,   47,   49,   50,   51,   52,   53,   54,   55,   55,   57,   58,   59,   60,   61,   62,   63,   63,   65,   66,   67,   68,   69,   70,   71,   71,
   -1,   20,   33,   41,   48,   53,   57,   61,   64,   66,   69,   71,   73,   75,   76,   78,   80,   82,   85,   87,   89,   91,   92,   94,   96,   98,  101,  103,  105,  107,  108,  110,  112,  114,  117,  119,  121,  123,  124,  126,  128,
   -1,   23,   39,   51,   60,   67,   73,   79,   83,   87,   91,   94,   97,  100,  102,  105,  107,  111,  115,  118,  121,  124,  126,  129,  131,  135,  139,  142,  145,  148,  150,  153,  155,  159,  163,  166,  169,  172,  174,  177,  179,
   -1,   28,   49,   65,   78,   89,   99,  107,  114,  120,  126,  132,  136,  141,  145,  149,  153,  159,  165,  171,  176,  180,  185,  189,  192,  199,  205,  211,  216,  220,  225,  229,  232,  239,  245,  251,
   -1,   33,   58,   79,   97,  112,  125,  137,  148,  157,  166,  174,  182,  189,  195,  201,  207,  217,  227,  235,  243,  251,
   -1,   35,   63,   86,  106,  123,  139,  152,  165,  176,  187,  197,  206,  214,  222,  230,  237,  250,
   -1,   31,   55,   75,   91,  105,  117,  128,  138,  146,  154,  161,  168,  174,  180,  185,  190,  200,  208,  215,  222,  229,  235,  240,  245,  255,
   -1,   36,   65,   89,  110,  128,  144,  159,  173,  185,  196,  207,  217,  226,  234,  242,  250,
   -1,   41,   74,  103,  128,  151,  172,  191,  209,  225,  241,  255,
   -1,   43,   79,  110,  138,  163,  186,  207,  227,  246,
   -1,   39,   71,   99,  123,  144,  164,  182,  198,  214,  228,  241,  253,
   -1,   44,   81,  113,  142,  168,  192,  214,  235,  255,
   -1,   49,   90,  127,  160,  191,  220,  247,
   -1,   51,   95,  134,  170,  203,  234,
   -1,   47,   87,  123,  155,  184,  212,  237,
   -1,   52,   97,  137,  174,  208,  240,
   -1,   57,  106,  151,  192,  231,
   -1,   59,  111,  158,  202,  243,
   -1,   55,  103,  147,  187,  224,
   -1,   60,  113,  161,  206,  248,
   -1,   65,  122,  175,  224,
   -1,   67,  127,  182,  234]


theirs = [40, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    40, 15, 23, 28, 31, 34, 36, 38, 39, 41, 42, 43, 44, 45, 46, 47, 47, 49, 50, 51, 52, 53, 54, 55, 55, 57, 58, 59, 60, 61, 62, 63, 63, 65, 66, 67, 68, 69, 70, 71, 71,
    40, 20, 33, 41, 48, 53, 57, 61, 64, 66, 69, 71, 73, 75, 76, 78, 80, 82, 85, 87, 89, 91, 92, 94, 96, 98, 101, 103, 105, 107, 108, 110, 112, 114, 117, 119, 121, 123, 124, 126, 128,
    40, 23, 39, 51, 60, 67, 73, 79, 83, 87, 91, 94, 97, 100, 102, 105, 107, 111, 115, 118, 121, 124, 126, 129, 131, 135, 139,
142, 145, 148, 150, 153, 155, 159, 163, 166, 169, 172, 174, 177, 179, 35,
28, 49, 65, 78, 89, 99, 107, 114, 120, 126, 132, 136, 141, 145, 149,
153, 159, 165, 171, 176, 180, 185, 189, 192, 199, 205, 211, 216, 220, 225,
229, 232, 239, 245, 251, 21, 33, 58, 79, 97, 112, 125, 137, 148, 157,
166, 174, 182, 189, 195, 201, 207, 217, 227, 235, 243, 251, 17, 35, 63,
86, 106, 123, 139, 152, 165, 177, 187, 197, 206, 214, 222, 230, 237, 250,
25, 31, 55, 75, 91, 105, 117, 128, 138, 146, 154, 161, 168, 174, 180,
185, 190, 200, 208, 215, 222, 229, 235, 240, 245, 255, 16, 36, 65, 89,
110, 128, 144, 159, 173, 185, 196, 207, 217, 226, 234, 242, 250, 11, 41,
74, 103, 128, 151, 172, 191, 209, 225, 241, 255, 9, 43, 79, 110, 138,
163, 186, 207, 227, 246, 12, 39, 71, 99, 123, 144, 164, 182, 198, 214,
228, 241, 253, 9, 44, 81, 113, 142, 168, 192, 214, 235, 255, 7, 49,
90, 127, 160, 191, 220, 247, 6, 51, 95, 134, 170, 203, 234, 7, 47,
87, 123, 155, 184, 212, 237, 6, 52, 97, 137, 174, 208, 240, 5, 57,
106, 151, 192, 231, 5, 59, 111, 158, 202, 243, 5, 55, 103, 147, 187,
224, 5, 60, 113, 161, 206, 248, 4, 65, 122, 175, 224, 4, 67, 127,
182, 234]

bad_rows = [(i, ours[i], theirs[i]) for i in range(len(ours)) if ((ours[i] != theirs[i]) and (ours[i] != -1))]

print(f"len bad_rows = {len(bad_rows)}")
print(bad_rows)