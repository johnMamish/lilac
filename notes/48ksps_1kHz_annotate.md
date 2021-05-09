It's little endian!!!

The first few frames of an Opus stream encapusulated by an Ogg container are not valid Opus frames, instead, they're metadata frames described by RFC7845.

The actual Ogg bitstream on the disk / wire consists of one or more 'logical' Ogg bitstreams. In our usecase, we've just got one bitstream, so the 'physical' and 'logical' bitstreams are the same.

Bitstreams are multiplexed on the "page" level.

Payload information within pages is entirely up to codec designers. It's expected that the first few bytes of the payload in the BOS page will let decoders identify the codec. This isn't a hard-and-fast rule, but is rather a social standard  that any sane codec is expected to conform to, e.g. the first 4 bytes of every Opus bitstream will be "Opus".

Apparently "pages" contain "segments". What's the reason for this

Terms:
  * Packet: How data is broken up by the codec. AFAICT, Ogg is agnostic to packet boundaries.
  * Page:   Ogg's basic level of data granularity. Up to 64kB of arbitrary data is put into an Ogg
            page. Each page has a header which identifies which bitstream it belongs to.
  * Segment: Encoder packets are made up of zero or more segments. Each segment has a maximum length
             of 255 bytes. Ogg was designed with the expectation that encoder packets would be
             nominally 100 - 200 Bytes, so each encoder packet should end up being 1 segment.
             AFAICT, the breakup of packets into segments is a mechanism to give Ogg a way to
             describe the length of encoder packets.


### Ogg Page 0
###### Starts at offset $0000
----------------------------------------------------------------
0000  4f 67 67 53
    Magic start string: "OggS"

0004  00
    uint8_t version = 0
    Uses Ogg Version 0

0005  02
    uint8_t header = 2
    2 indicates that this is the first page of a logical bitstream, designates it as a
    BOS (beginning-of-stream) page.

0006  00 00 00 00 00 00 00 00
    uint64_t granule_position = 0
    Value MAY contain total number of PCM samples coded by the end of this page. Acts as a hint to
    the decoder; could be useful for debugging.

000e  2d 4b 46 66
    uint32_t bitstream_serial_number
    Just a random number, I guess. Used for disentangling multiple bitstreams crammed into the same
    Ogg file?

0012  00 00 00 00
    uint32_t page_sequence_number
    Sequence number for identifying dropped pages.

0016  b1 b4 0e 35
    uint32_t CRC_checksum = 0x350e_b4b1
    32-bit CRC checksum of the page; generator poly of 0x04c11db7

001a  01
    uint8_t number_page_segments = 1
    Number of segment entries encoded in the segment table (????)

001b  13
    uint8_t lacing_value_for_segment_0 = 0x13
    "Lacing value" is just a fancy word for "segment size", according to the Ogg RFC.

### Opus Packet 0: ID Header
###### Starts at offset $001c
----------------------------------------------------------------
The 0th Opus packet should take up exactly 1 Ogg page.

4f 70 75 73 48 65 61 64
    Magic start string for Opus ID Header: "OpusHead"

01
    uint8_t version = 1
    Version number of the Opus Ogg ENCAPSULATION

01
    uint8_t output_channel_count = 1
    Number of output channels

38 01
    uint16_t pre_skip = 0x0138 = 312
    Number of samples to discard from the decoder output when starting playback.

80 bb 00 00
   uint32_t input_sample_rate = 0xbb80 = 48000
   Sample rate of the original input (before encoding) in Hz. This information is useful to make
   sure that the sample rate on the playback hardware matches the sample rate on the recording
   hardware.

00 00
   uint16_t output_gain = 0
   Q7.8 fixed point number that's 20 * log_10(G) for some gain G that should be applied to all
   samples. A typical value for this will be 0. It can take values in the range [-128, 128).

00
   uint8_t channel_mapping_family = 0
   An enum with a few fixed values defined in RFC7845.

### Ogg Page 1
###### Starts at offset $002f
----------------------------------------------------------------
"OggS" magic number            4f 67 67 53
version                        00
header: not BOS                00
granule position:              00 00 00 00 00 00 00 00
bitstream serial number:       2d 4b 46 66
page sequence number:          01 00 00 00
CRC_checksum:                  af 05 5a af
number_page_segments           03
lacing values
    lacing value 0:            ff
    lacing value 1:            ff
    lacing value 2:            fe


### Opus packet 1
###### Starts at offset $004d
----------------------------------------------------------------
This is a Comment Header packet. It's basically useless to us.

4f 70 75 73 54 61 67 73
    Magic start string: "OpusTags" ????

0d 00 00 00
    uint32_t vendor_string_length = 0x0d = 13

6c 69 62 6f 70 75 73 20 31 2e 31 2e 32
    uint8_t[] vendor_string = "libopus 1.1.2"

02 00 00 00
    uint32_t user_comment_list_length = 2

26 00 00 00
    uint32_t user_comment_list_length[0] = 0x26

45 4e ..... 44 45 52 5f
    uint8_t[] user_comment_string[0] = "ENCODER=opusenc from opus-tools 0.1.10"

43 00 00 00
    uint32_t user_comment_list_length[0] = 0x43

<a bunch of other bytes>
    uint8_t[] user_comment_string[1] = "ENCODER_OPTIONS=--comp 0 --hard-cbr --framesize 10 --bitrate 64.000"


### Ogg Page 2
----------------------------------------------------------------
"OggS" magic number                4f 67 67 53
version:                           00
header: not BOS                    00
granule position 0x80bb (48000):   80 bb 00 00 00 00 00 00
    From this, we can tell that the first Ogg page contains 48000 samples, but we also have to remember
    to account for the pre_skip defined in the Opus ID header.
bitstream serial number:           2d 4b 46 66
page_sequence_number = 2:          02 00 00 00
CRC_checksum = 0xce9f_6ebc:        bc 6e 9f ce
number_page_segments = 0x64:       64
uint8_t[] lacing_values = {50, 50, ..., 50} 50 50 50 ..... 50


### This is where the REAL SHIT starts.

### Opus packet 2
----------------------------------------------------------------
f0 = 0b11110_0_00
   TOC<7:3> config = 0x1e = 30
   Packet is a 10 millisecond CELT packet

   TOC<2> hybrid = 0
   This packet is not a SILK/CELT hybrid

   TOC<1:0> frame_count_code = 0
   There is 1 frame in this packet.


5f 48 95 5a 27 38 72
17 e0 c2 34 30 13 37 82
63 2d c4 26 75 ce b3 4d
72 02 31 e9 93 94 a7 e5
84 33 8c 18 e7 a5 ae cd
1d 45 77 b4 39 fb 0f d2
79 03 4a f0 43 1a 25 e7
07 d6 96 4c 42 c8 af ec
ed 9b 72 0e 22 a8 96 c8
76 d9 f1 4f b2 4c 42 ae


### Opus packet 3
----------------------------------------------------------------
f0 6e 95 29 dc 0e 76 02
dc 75 7e c5 9c 10 4a 20
44 6e 5d 06 a3 3b 5a 17
3f d9 c2 31 ce 34 f2 11
be b0 82 f0 fd 6c b1 65
73 15 8c 83 e4 3b 1e 0d
e0 4a ad d1 a3 5a 58 91
f1 71 28 d2 af 0f 8d 0c
14 de 2d df 07 78 db 58
16 18 c7 f5 ed 83 02 64


f0 6e 95 29 dc 0e 75 d9  |.......d.n.)..u.|
00000470  41 3d fe 86 cf 78 fc f3  d9 6b cb 33 bb 16 53 8d  |A=...x...k.3..S.|
00000480  4b 4f 3e 02 11 28 a1 a8  b7 06 3b 8d 1c ba b7 4c  |KO>..(....;....L|
00000490  b4 65 54 a6 44 ec 78 31  d2 b0 b8 f3 ab 39 5b 21  |.eT.D.x1.....9[!|
000004a0  11 29 b0 1b 1d 77 10 65  29 bc ad df 07 78 db 58  |.)...w.e)....x.X|
000004b0  15 17 b8 e7 8d 83 02 63  f0 59 a1 ec a9 14 06 6c  |.......c.Y.....l|
000004c0  28 e2 cf 3e 33 f1 58 0b  2b cf 79 7c 35 2f 1b 65  |(..>3.X.+.y|5/.e|
000004d0  3d 24 35 45 bb 2a 4e cb  94 2e 21 40 b5 e7 ee e4  |=$5E.*N...!@....|
000004e0  68 bc f4 2c 6e a0 fd b1  c1 d6 99 7f b4 08 cf 21  |h..,n..........!|
000004f0  34 fa 53 81 e6 a8 5a 6a  6f 2c e7 e7 69 c6 8e e0  |4.S...Zjo,..i...|
00000500  b0 a6 47 b4 54 9f ed 56  f0 59 a2 7e d8 0d d9 69  |..G.T..V.Y.~...i|
00000510  10 4b 09 af 30 ed 6b 2f  95 ac 47 16 46 fa 90 d0  |.K..0.k/..G.F...|
00000520  bc 63 19 bf 7b 25 be e2  b0 67 86 1a c8 47 dc ca  |.c..{%...g...G..|
00000530  03 2d ce c4 7f 18 24 a7  89 67 87 16 20 c7 92 27  |.-....$..g.. ..'|
00000540  eb 53 58 03 ff 18 b3 54  de 59 09 ce d3 5c 0e e0  |.SX....T.Y...\..|
00000550  e5 9d c9 9b ff ff ed 96  f0 59 a2 7e 60 71 82 24  |.........Y.~`q.$|
00000560  aa a3 08 aa 7e 14 60 55  35 1e d0 6d 2d 86 61 31  |....~.`U5..m-.a1|
00000570  a7 e2 67 6b 63 36 0f b8  c1 fa 4a 50 53 ed 23 a4  |..gkc6....JPS.#.|
00000580  35 9a 72 43 b1 e0 22 4a  a7 e7 0d fa 7d 7b 29 c5  |5.rC.."J....}{).|
00000590  98 bf 10 27 53 19 69 29  bc b3 9f 9d a5 34 3b b3  |...'S.i).....4;.|
000005a0  b7 cc 23 b0 7b ff e1 56  f0 59 a2 7e 60 72 47 06  |..#.{..V.Y.~`rG.|
000005b0  91 e6 4a b6 4a a1 3a 4a  f6 3a 26 ad bc da f5 84  |..J.J.:J.:&.....|
000005c0  8b 70 7c 61 27 68 32 50  64 15 f8 8f af 0d 93 ff  |.p|a'h2Pd.......|
000005d0  38 0c 24 ac 1a 48 40 e0  92 95 1f bd 9c 37 c8 76  |8.$..H@......7.v|
000005e0  b9 08 00 94 b8 1f f8 d7  59 53 79 67 3d 5c 0e e0  |........YSyg=\..|
000005f0  f8 9d 83 70 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |...p.....Y.~`q.$|
00000600  ab 3c 1c 79 f2 55 20 a2  a9 45 7b 0f fc 65 1e 70  |.<.y.U ..E{..e.p|
00000610  dc 76 c1 9b da bc 19 3a  5c 72 cd 16 96 b2 1b a0  |.v.....:\r......|
00000620  81 9e 8b 2d 8d d4 1c e9  58 5f 82 d2 f8 b1 06 5b  |...-....X_.....[|
00000630  4d 3c fe 65 4f 38 6c e5  9a c6 12 cf ca b8 1d c1  |M<.eO8l.........|
00000640  1f bc a7 b0 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |.......U.Y.~`rG.|
00000650  91 e6 4a c2 dd 87 d6 4a  f6 3a 26 ad bc eb f4 d3  |..J....J.:&.....|
00000660  ca 6a 16 5d 0f 0d f2 68  6d f2 ca 4f 3d 57 df 1a  |.j.]...hm..O=W..|
00000670  29 d1 cd f8 f1 6a 29 d2  9d 6d 15 b7 fc 58 81 c5  |)....j)..m...X..|
00000680  29 34 00 40 d0 1e 1b 2f  87 63 09 67 3d 5c 0e e0  |)4.@.../.c.g=\..|
00000690  92 9d 83 b8 7f ff ed 95  f0 59 a1 ec a9 13 70 cb  |.........Y....p.|
000006a0  9e 18 b8 f5 44 d5 cc 27  ea e8 e9 7a 1f 43 be 39  |....D..'...z.C.9|
000006b0  d6 6c 75 ac d8 b5 2c 2f  4f 15 d4 8e c9 d0 67 a4  |.lu...,/O.....g.|
000006c0  f9 9d a2 0a 81 22 75 60  5c 12 70 d0 fa 7d 82 84  |....."u`\.p..}..|
000006d0  da ab 60 ab 27 ba 6a 85  9a c6 12 ce 7a b8 1d c1  |..`.'.j.....z...|
000006e0  12 cc b3 88 85 3f d1 55  f0 59 a2 7c ed 28 ce 8e  |.....?.U.Y.|.(..|
000006f0  f5 07 1a f6 4a b1 1b a3  b0 c8 d0 df d5 71 32 33  |....J........q23|
00000700  d4 87 6e c6 c8 4f 39 79  df 85 d9 a3 e7 fb c8 20  |..n..O9y....... |
00000710  f3 8c 2e cc 6e 73 ae 94  ef 3e 72 c9 3d a0 39 88  |....ns...>r.=.9.|
00000720  a1 1e 27 3f cc 4e 0e 57  67 53 79 67 3d 5c 0e e0  |..'?.N.WgSyg=\..|
00000730  f1 1d 87 b9 1f ff cd 95  f0 59 a2 7e 60 71 82 24  |.........Y.~`q.$|
00000740  ab 3c 1d 05 2e a1 20 a2  a9 45 7b 0f fc 65 1e 85  |.<.... ..E{..e..|
00000750  d7 65 be 16 4d dc 51 45  96 fe 13 a2 68 b3 7e 15  |.e..M.QE....h.~.|
00000760  c8 5d 41 03 2d c2 79 d2  b0 b8 9c 20 f8 b0 f4 5f  |.]A.-.y.... ..._|
00000770  45 ee de 6a a0 1e 16 a5  9a c6 12 cf ca b8 1d c1  |E..j............|
00000780  b4 cb 03 72 7f ff ed 55  f0 59 a2 7e 7c 2d 0d 52  |...r...U.Y.~|-.R|
00000790  81 84 aa 52 3f 9d 06 c6  24 d2 6d 20 58 39 e9 5b  |...R?...$.m X9.[|
000007a0  c6 9d 75 e0 86 a5 cb 9e  84 ed a9 9c 72 35 5e a4  |..u.........r5^.|
000007b0  e0 9a b7 39 e2 d0 f1 d2  b0 bc e3 2c 7d 3e cb f8  |...9.......,}>..|
000007c0  cc 7a 80 b2 9c 0f fc 62  cd 63 09 64 25 5c 0e e0  |.z.....b.c.d%\..|
000007d0  a7 1b ff 66 7f ff cd 95  f0 59 a1 ec a9 13 70 cb  |...f.....Y....p.|
000007e0  9e 18 b8 ce cc 54 9c 27  ea e8 e9 7a 1f 17 8d be  |.....T.'...z....|
000007f0  bb 18 38 cf 46 28 7c b8  bb 3a c6 0d 7c 8e 47 ef  |..8.F(|..:..|.G.|
00000800  be ab 70 36 37 4a 7a 9d  f7 fa 20 d5 fa 7d ca 5b  |..p67Jz... ..}.[|
00000810  14 ce 57 12 b8 9c 6a 85  a4 a6 f2 ce 7a b8 1d c1  |..W...j.....z...|
00000820  7c cc a7 28 a5 3f cd 55  f0 59 a2 7e 60 72 47 06  ||..(.?.U.Y.~`rG.|
00000830  91 e6 4a dc 62 01 7c 4a  f6 3a 26 ad bc da f6 60  |..J.b.|J.:&....`|
00000840  8a 69 81 8d 2f 85 23 9e  b0 a0 a7 37 f5 8e ea 5f  |.i../.#....7..._|
00000850  ec 71 db 9d 24 20 98 e9  58 5f a5 ed 5c 58 7b 8d  |.q..$ ..X_..\X{.|
00000860  33 5d 85 80 7f 38 31 d7  6f d3 79 67 3d 5c 0e e0  |3]...81.o.yg=\..|
00000870  be 1d 87 08 7f ff cd 95  f0 59 a2 7e 60 78 a2 e2  |.........Y.~`x..|
00000880  4b 10 9d 3b ed 46 84 61  ad 56 aa e3 a2 7d a8 b9  |K..;.F.a.V...}..|
00000890  d0 84 e0 c6 69 63 2a d0  dd ef ed 50 4b b0 96 78  |....ic*....PK..x|
000008a0  92 5d d1 ed b4 c4 ca 9d  2b 0b f3 88 27 16 30 d0  |.]......+...'.0.|
000008b0  9b b5 19 98 5b ec 06 12  cd 53 79 67 3d 5c 0e e0  |....[....Syg=\..|
000008c0  b2 5e 3f 62 7f ff f1 55  f0 59 a2 7e 7c 01 9e b8  |.^?b...U.Y.~|...|
000008d0  cf eb 10 59 ef e7 a9 4e  d5 86 cc c4 22 2a cf ba  |...Y...N...."*..|
000008e0  50 5a eb f0 f2 51 1e dd  db dc b4 59 f5 e7 06 cc  |PZ...Q.....Y....|
000008f0  9f 6a 14 d3 4d 89 19 35  d2 9d 5c 50 1d 3d a0 51  |.j..M..5..\P.=.Q|
00000900  ba 70 f0 35 94 0f 0d 92  cd 53 79 67 3d 8d 1d c1  |.p.5.....Syg=...|
00000910  64 4c 8f 26 7f ff cd 95  f0 59 a1 ec a9 13 70 cb  |dL.&.....Y....p.|
00000920  9e 18 b8 c7 1a a1 2c 27  ea e8 e9 7a 1f 43 bc 4f  |......,'...z.C.O|
00000930  33 bb a2 2d bf 11 9d b6  28 5e 39 01 ec 40 cf bb  |3..-....(^9..@..|
00000940  8d ae 9b 62 48 41 03 a5  3b 38 ea c3 fa 7d 7b 27  |...bHA..;8...}{'|
00000950  be 22 02 13 90 1f f8 c5  a4 c6 12 ce 7a b8 1d c1  |."..........z...|
00000960  64 bb 33 aa a5 3f cd 55  f0 59 a2 7e 60 72 47 06  |d.3..?.U.Y.~`rG.|
00000970  91 e6 4a d6 d5 e7 0a 4a  f6 3a 26 ad bc eb f4 cc  |..J....J.:&.....|
00000980  69 5b 98 62 82 02 e1 42  8c c5 74 14 0f 16 57 1a  |i[.b...B..t...W.|
00000990  83 98 6a 03 1b a7 e3 a5  3b 7f 9c 22 7c 58 81 4d  |..j.....;.."|X.M|
000009a0  81 44 f9 3c 40 1e 6a 97  59 63 09 67 3d 5c 0e e0  |.D.<@.j.Yc.g=\..|
000009b0  93 1e 47 38 7f ff d1 95  f0 59 a2 7e 60 71 82 24  |..G8.....Y.~`q.$|
000009c0  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 65 1c 69  |.<..iW ..E{..e.i|
000009d0  42 8b 46 0e ef 0f a9 73  0c b9 7b d5 78 0c ee bb  |B.F....s..{.x...|
000009e0  a3 0d 39 23 c5 a9 25 7d  e0 20 14 62 f8 b1 03 bd  |..9#..%}. .b....|
000009f0  a6 72 ee 0a e7 39 f8 c5  a4 c6 12 cf ca b8 1d c1  |.r...9..........|
00000a00  26 ca b3 a0 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |&......U.Y.~`rG.|
00000a10  91 e6 4a d6 d5 e7 0a 4a  f6 3a 26 ad bc da f6 5a  |..J....J.:&....Z|
00000a20  69 bb d2 c3 c2 02 98 c0  1c f7 76 99 35 fa 65 9c  |i.........v.5.e.|
00000a30  ca f1 3e 9b 63 1b a4 e8  92 94 d4 cc 7c 58 78 b6  |..>.c.......|Xx.|
00000a40  16 3e ad 4e 30 1e 6a 97  6e d3 79 67 3d 5c 0e e0  |.>.N0.j.n.yg=\..|
00000a50  93 1e 4b a8 7f ff ed 95  f0 59 a2 7e 60 71 82 24  |..K......Y.~`q.$|
00000a60  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 36 96 ed  |.<..iW ..E{..6..|
00000a70  c8 a3 a3 ad a8 85 89 43  ec c2 90 47 43 87 4d 7f  |.......C...GC.M.|
00000a80  65 82 4f 05 88 fe 31 d2  9d 5e 95 33 f8 b1 97 2f  |e.O...1..^.3.../|
00000a90  2f 54 83 0c d8 1e 1b 25  9a a6 f2 cf ca b8 1d c1  |/T.....%........|
00000aa0  26 bc ab 68 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |&..h{..U.Y.....l|
00000ab0  29 04 d1 c1 21 10 0a 05  07 4d 9b a0 18 08 a0 26  |)...!....M.....&|
00000ac0  3b 7b 8b 7f ff ea 10 db  48 c1 ba f0 02 7e b3 83  |;{......H....~..|
00000ad0  58 1a 6a 8c 10 f1 67 8c  49 54 0c 08 be 2c 40 9b  |X.j...g.IT...,@.|
00000ae0  39 76 4f 1f b0 0f a6 32  cd 63 09 67 3d 5c 0e e0  |9vO....2.c.g=\..|
00000af0  93 1d 8b a0 89 3f cd 95  f0 59 a2 7e 60 71 82 24  |.....?...Y.~`q.$|
00000b00  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 36 96 e1  |.<..iW ..E{..6..|
00000b10  af a0 41 f7 4d 14 18 ea  37 5d 98 ab 60 00 45 3b  |..A.M...7]..`.E;|
00000b20  be 73 6e 7e c3 d7 07 4a  75 7d 9c ab d0 e0 c6 1a  |.sn~...Ju}......|
00000b30  3b 6a 6b 76 77 b0 6c 85  9a a6 f2 cf ca b8 1d c1  |;jkvw.l.........|
00000b40  26 cc a3 60 7f ff f1 56  f0 59 a2 7e 60 72 47 06  |&..`...V.Y.~`rG.|
00000b50  91 e6 4a d6 d5 e7 0a 4a  f6 3a 26 ad bc da f6 5a  |..J....J.:&....Z|
00000b60  69 65 d7 40 c8 54 94 b0  18 4e 71 e5 ea a6 13 83  |ie.@.T...Nq.....|
00000b70  05 3b c0 db 11 fc f3 a6  46 45 44 c3 3c 37 cb d4  |.;......FED.<7..|
00000b80  79 fa 74 8d 00 1e 6a 97  67 53 79 67 3d 5c 0e e0  |y.t...j.gSyg=\..|
00000b90  93 1d 87 2a 7f ff cd 95  f0 59 a2 7e 86 2c 7e 2e  |...*.....Y.~.,~.|
00000ba0  46 27 cf 81 37 4f 24 66  18 5f 00 8d bf fc a6 bd  |F'..7O$f._......|
00000bb0  32 85 ab 8a 85 86 a8 cd  be ad 33 47 7e 50 c5 66  |2.........3G~P.f|
00000bc0  46 07 54 36 84 8b 1a ed  12 52 69 3b 6f 4f af 22  |F.T6.....Ri;oO."|
00000bd0  be ee 69 0c 6a e7 3e 31  69 29 bc b3 9e c6 8e e0  |..i.j.>1i)......|
00000be0  93 5e 43 3e 7b ff d1 55  f0 59 a1 ec a9 14 06 6c  |.^C>{..U.Y.....l|
00000bf0  29 04 d1 c1 21 10 0a 05  07 4d 9b a0 17 fb b3 d9  |)...!....M......|
00000c00  5f 13 a0 e0 47 d4 73 6b  e9 b0 9d 88 26 e7 a0 fe  |_...G.sk....&...|
00000c10  90 19 2e a0 51 8d ce 3a  e5 21 bf 05 f4 38 31 6c  |....Q..:.!...81l|
00000c20  d5 21 e3 fd 54 4f a6 32  cd 53 79 67 3d 5c 0e e0  |.!..TO.2.Syg=\..|
00000c30  93 26 53 68 a9 3f ed 95  f0 59 a2 7e 60 71 82 24  |.&Sh.?...Y.~`q.$|
00000c40  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 65 1c 6c  |.<..iW ..E{..e.l|
00000c50  38 d2 bd 2b ee 43 61 a2  d9 9a b1 ad 8a 1b 7b d2  |8..+.Ca.......{.|
00000c60  e2 a5 ce db e1 7e 69 d2  9d 5d f2 d8 f8 6f 93 68  |.....~i..]...o.h|
00000c70  3e d4 98 20 f0 9f fa c5  a6 c6 12 cf ca b8 1d c1  |>.. ............|
00000c80  26 bb 03 fa ff ff cd 55  f0 59 a2 7e cd 39 c4 2d  |&......U.Y.~.9.-|
00000c90  5a fd 74 31 49 97 37 6d  fc a6 6a 4e 77 7c 0a 3b  |Z.t1I.7m..jNw|.;|
00000ca0  69 52 63 06 6a 1c 74 3a  ab 1d ff 77 b3 2c 86 7c  |iRc.j.t:...w.,.||
00000cb0  6c dd ba f7 39 26 9a b7  4a 75 7a 64 65 6e 2c 40  |l...9&..Juzden,@|
00000cc0  37 af 42 0b 66 f1 0d 92  cd 53 79 67 3d 8d 1d c1  |7.B.f....Syg=...|
00000cd0  26 4c ab 4d ff ff cd 95  f0 59 a2 7e 60 71 82 24  |&L.M.....Y.~`q.$|
00000ce0  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 65 1c 6c  |.<..iW ..E{..e.l|
00000cf0  38 58 58 f6 3c a1 27 d1  ae 71 57 4f 51 4e 79 be  |8XX.<.'..qWOQNy.|
00000d00  65 e5 1b 9d 8d ce 79 d2  9d a8 fe 6a 90 e0 c2 96  |e.....y....j....|
00000d10  0b d2 f0 ad a0 1f f8 c5  a4 c6 12 cf ca b8 1d c1  |................|
00000d20  26 ba 8b 28 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |&..({..U.Y.....l|
00000d30  29 04 d1 c1 21 10 0a 05  07 4d 9b a0 18 08 a0 b4  |)...!....M......|
00000d40  60 06 d3 72 9b 4b 25 64  bb 9a 08 b7 5e 5b 32 bd  |`..r.K%d....^[2.|
00000d50  0b 3f 11 4e 48 f1 67 c8  49 4c 4e e2 be 2c 41 e3  |.?.NH.g.ILN..,A.|
00000d60  a3 ca 54 53 b6 f0 35 42  cd 63 09 67 3d 5c 0e e0  |..TS..5B.c.g=\..|
00000d70  93 1d 87 6a a9 3f cd 95  f0 59 a2 7e 7b fc 46 4d  |...j.?...Y.~{.FM|
00000d80  48 05 9e d6 0d 18 55 8d  88 9e 5e fc fe c5 0c fc  |H.....U...^.....|
00000d90  91 5b f9 2f 93 84 4f f6  5d 56 2d a3 49 19 00 b4  |.[./..O.]V-.I...|
00000da0  ee 8a c9 cb 84 8d b8 c7  4b 1d 19 a5 03 7a 7d 78  |........K....z}x|
00000db0  25 0f 06 06 43 a4 6a b7  5a 63 09 67 3d 8d 1d c1  |%...C.j.Zc.g=...|
00000dc0  26 cc ab 8e 7f ff ed 55  f0 59 a2 7e 60 72 47 06  |&......U.Y.~`rG.|
00000dd0  91 e6 4a d6 d5 e7 0a 4a  f6 3a 26 ad bc eb f4 d3  |..J....J.:&.....|
00000de0  c9 23 ef ce 6b e5 2b 62  04 37 c1 94 d0 5c 8a 2e  |.#..k.+b.7...\..|
00000df0  f2 82 6b 64 5f e3 1b 48  b3 53 28 5c bc 58 7b ec  |..kd_..H.S(\.X{.|
00000e00  93 0c 7a d1 38 9e 1b 57  59 63 09 67 3d 5c 0e e0  |..z.8..WYc.g=\..|
00000e10  93 1e 47 aa 7f ff d1 95  f0 59 a2 7e 7b fc 46 4d  |..G......Y.~{.FM|
00000e20  48 05 9e d6 0d 18 55 8d  88 9e 5e fc fd b0 5a c1  |H.....U...^...Z.|
00000e30  7a c1 cd ed 60 12 27 d1  15 73 83 bb c0 ae 35 ad  |z...`.'..s....5.|
00000e40  f6 56 8e a7 69 21 04 ce  94 ea f3 34 ac fa 7d c9  |.V..i!.....4..}.|
00000e50  8e 4e bc 0e 30 1e 04 af  81 53 79 67 3d 8d 1d c1  |.N..0....Syg=...|
00000e60  26 ba a7 66 7b ff cd 55  f0 59 a2 7e cd 39 c4 2d  |&..f{..U.Y.~.9.-|
00000e70  5a fd 74 31 49 97 37 6d  fc a6 6a 4e 77 d2 15 5f  |Z.t1I.7m..jNw.._|
00000e80  62 71 ba c3 ea de 90 a0  ae 07 b0 cc e1 e2 96 f5  |bq..............|
00000e90  fe 1c e6 8b cd 8d d4 09  d1 eb c3 ef 55 7d 3e da  |............U}>.|
00000ea0  2b 5b 32 cf 3b 9c fc 62  d3 63 09 67 3d 8d 1d c1  |+[2.;..b.c.g=...|
00000eb0  26 3d 07 17 ff ff cd 95  f0 59 a1 ec a9 13 70 cb  |&=.......Y....p.|
00000ec0  9e 18 b8 c0 5f 24 2a 27  ea e8 e9 7a 1f 17 8d cf  |...._$*'...z....|
00000ed0  c0 20 ea 89 f3 7c fd 2a  a1 c4 e4 5f 7a e7 8c bc  |. ...|.*..._z...|
00000ee0  f8 27 9b 91 e2 d1 b9 d2  c7 14 31 80 7a 7d 9a 9c  |.'........1.z}..|
00000ef0  f1 02 76 7a 08 1e 1b 25  9a a6 f2 ce 7a b8 1d c1  |..vz...%....z...|
00000f00  26 ca ab b9 29 3f cd 55  f0 59 a2 7e 60 72 47 06  |&...)?.U.Y.~`rG.|
00000f10  91 e6 4a d6 d5 e7 0a 4a  f6 3a 26 ad bc da f6 61  |..J....J.:&....a|
00000f20  c9 06 4e 12 33 a2 64 e0  5e bd 6e 02 ee bf 51 9e  |..N.3.d.^.n...Q.|
00000f30  6f 05 a6 9a d3 1b a5 28  92 a8 52 ad 9c 58 7b b7  |o......(..R..X{.|
00000f40  4e 67 91 2d 48 1e 0f 57  5a 53 79 67 3d 5c 0e e0  |Ng.-H..WZSyg=\..|
00000f50  93 1e 83 70 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |...p.....Y.~`q.$|
00000f60  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 36 94 d4  |.<..iW ..E{..6..|
00000f70  3b 84 41 52 ca 02 10 98  c7 f6 cf 80 dd 60 0b d8  |;.AR.........`..|
00000f80  00 f9 8f 6d 24 1e 7d d2  9d ca 9a eb 78 6f 90 34  |...m$.}.....xo.4|
00000f90  ad 74 2d b6 88 1f fa c5  a4 a6 f2 cf ca b8 1d c1  |.t-.............|
00000fa0  26 ca 87 f0 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |&...{..U.Y.....l|
00000fb0  29 04 d1 c1 21 10 0a 05  07 4d 9b a0 17 fb b4 6a  |)...!....M.....j|
00000fc0  ea d3 77 b5 59 0c e7 6c  00 17 06 c9 2d a8 34 f9  |..w.Y..l....-.4.|
00000fd0  a6 a2 45 e7 24 d3 ea e9  4e bf 51 78 7e 2c 41 b8  |..E.$...N.Qx~,A.|
00000fe0  41 04 31 18 c6 f1 35 42  d6 d3 79 67 3d 5c 0e e0  |A.1...5B..yg=\..|
00000ff0  93 25 83 98 89 3f f1 95  f0 59 a2 7e 60 71 82 24  |.%...?...Y.~`q.$|
00001000  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 36 94 d4  |.<..iW ..E{..6..|
00001010  33 28 44 4e 17 7d 48 89  06 b2 57 48 9d ed 48 f5  |3(DN.}H...WH..H.|
00001020  b0 e8 9b 9d 8d d3 fc e9  58 5c b6 37 50 e0 c4 ed  |........X\.7P...|
00001030  7a 2a 47 5c 78 1f f8 c5  a4 a6 f2 cf ca b8 1d c1  |z*G\x...........|
00001040  26 ac ab b8 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |&......U.Y.~`rG.|
00001050  91 e6 4a d6 d5 e7 0a 4a  f6 3a 26 ad bc da f6 61  |..J....J.:&....a|
00001060  d0 12 20 9d 47 37 22 76  d4 98 3a d6 82 f5 5f 3f  |.. .G7"v..:..._?|
00001070  08 f8 da 11 8d 48 45 d2  9d 7f 45 ed 9c 58 78 6c  |.....HE...E..Xxl|
00001080  ff 54 be 6d 78 1e 1b 37  59 53 79 67 3d 5c 0e e0  |.T.mx..7YSyg=\..|
00001090  93 25 83 60 7f ff cd 95  f0 59 a2 7e 4a 40 aa 7f  |.%.`.....Y.~J@..|
000010a0  66 ee 41 1a d8 73 23 e4  bc d7 5c 65 04 96 98 fb  |f.A..s#...\e....|
000010b0  d6 6a e9 af 31 3b c7 d5  d0 7c ca 0d 85 12 84 f7  |.j..1;...|......|
000010c0  99 fa a7 24 81 a7 c6 e9  4e f3 bf 3e de 2c 3d d0  |...$....N..>.,=.|
000010d0  45 d6 5c 96 d8 0f fb 52  d2 53 79 67 3d 5c 0e e0  |E.\....R.Syg=\..|
000010e0  93 66 3f c8 7b ff f1 55  f0 59 a1 ec a9 14 06 6c  |.f?.{..U.Y.....l|
000010f0  29 04 d1 c1 21 10 0a 05  07 4d 9b a0 18 08 a0 bb  |)...!....M......|
00001100  88 2f e5 08 4f 10 f6 3e  dd 6e bc 15 f0 72 9b 87  |./..O..>.n...r..|
00001110  af 39 01 4f 0d 89 17 16  e6 b3 b6 8c 3e 2c 3d 40  |.9.O........>,=@|
00001120  ae 79 49 19 a8 4f 0d 92  cd 63 09 67 3d 5c 0e e0  |.yI..O...c.g=\..|
00001130  93 1e 53 b0 a9 3f cd 95  f0 59 a2 7e 60 71 82 24  |..S..?...Y.~`q.$|
00001140  ab 3c 1c db 69 57 20 a2  a9 45 7b 0f fc 65 1c 6c  |.<..iW ..E{..e.l|
00001150  33 0b 4d 14 1e 6a 5d bc  34 b5 90 49 5d 56 be cf  |3.M..j].4..I]V..|
00001160  cb ac 4f 6d 8d d6 ed d2  9d 5f 3a ec f8 6f 93 76  |..Om....._:..o.v|
00001170  ad d0 70 a3 60 1f f8 c5  9a c6 12 cf ca b8 1d c1  |..p.`...........|
00001180  26 bb 07 a0 7f ff cd 55  f0 59 a2 7e 46 96 52 61  |&......U.Y.~F.Ra|
00001190  28 0f 68 16 59 b5 a0 4a  c6 fa 69 c1 2a fd 7e 30  |(.h.Y..J..i.*.~0|
000011a0  92 f1 10 19 5f 5d 4e 00  80 71 64 d3 9c 37 a2 07  |...._]N..qd..7..|
000011b0  76 44 dd 39 26 37 5c 77  4a 77 95 51 6d fd 3e bd  |vD.9&7\wJw.Qm.>.|
000011c0  ae ab 89 2c bc 0f 35 42  d3 53 79 67 3d 8d 1d c1  |...,..5B.Syg=...|
000011d0  64 4c 94 00 7f ff d1 95  f0 59 a2 7e 60 71 82 24  |dL.......Y.~`q.$|
000011e0  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1c 6c  |.<..E. ..E{..e.l|
000011f0  38 68 81 64 a2 0b 41 4d  cc 22 8d 05 81 a5 e3 92  |8h.d..AM."......|
00001200  81 ae 8b 25 24 1f c1 d1  eb c0 d3 bd b8 6f 91 0e  |...%$........o..|
00001210  95 b6 92 25 a0 1f fa c5  a4 c6 12 cf ca b8 1d c1  |...%............|
00001220  64 bb 2f 70 7b ff ed 55  f0 59 a1 ec a9 14 06 6c  |d./p{..U.Y.....l|
00001230  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 17 fb b4 69  |)........M.....i|
00001240  b2 0a 45 d5 5a 5c 21 f9  60 59 06 f8 6f eb 8c 5a  |..E.Z\!.`Y..o..Z|
00001250  f5 75 e0 8b 2d 8d e2 78  49 49 3f 3b 9e 2c 3d d6  |.u..-..xII?;.,=.|
00001260  43 38 91 ba 34 0f 35 42  d2 53 79 67 3d 5c 0e e0  |C8..4.5B.Syg=\..|
00001270  b2 26 47 30 89 3f cd 95  f0 59 a2 7e 60 ad fe 39  |.&G0.?...Y.~`..9|
00001280  a2 1f 44 3d 84 ab 79 b5  eb 52 44 24 5d 7f a9 95  |..D=..y..RD$]...|
00001290  ab 67 b7 58 7a b7 c4 f3  d5 b4 93 1a e1 8f cf 46  |.g.Xz..........F|
000012a0  cf 30 13 dd 03 48 e0 e9  58 5d a6 f2 4f 4f b0 26  |.0...H..X]..OO.&|
000012b0  63 85 4a b6 60 03 c1 29  66 a9 bc b2 12 c6 8e e0  |c.J.`..)f.......|
000012c0  b2 55 4a e8 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |.UJ....U.Y.~`rG.|
000012d0  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc eb f4 cc  |..J.l..J.:&.....|
000012e0  69 69 41 92 89 ea 70 eb  d0 73 b4 dd df b3 e3 e1  |iiA...p..s......|
000012f0  ff bf 2a b7 3b 1b a5 90  92 93 4d d7 5c 58 79 9b  |..*.;.....M.\Xy.|
00001300  91 38 55 e0 60 1e 6a 97  59 63 09 67 3d 5c 0e e0  |.8U.`.j.Yc.g=\..|
00001310  b2 26 93 78 ff ff cd 95  f0 59 a2 7e cd 34 13 4d  |.&.x.....Y.~.4.M|
00001320  fd de 6e 5c 95 3d ef 2b  ae 80 b2 d0 2d 37 5f 8d  |..n\.=.+....-7_.|
00001330  b1 a9 32 80 d3 8d 10 e7  0d f4 d1 32 73 25 43 14  |..2........2s%C.|
00001340  6c fe 4d aa e3 31 1f c6  09 2a 9d ed 50 70 df 29  |l.M..1...*..Pp.)|
00001350  63 8b de 0a 48 9e 68 77  59 63 09 67 3d 8d 1d c1  |c...H.hwYc.g=...|
00001360  26 bc 93 9f fb ff f1 55  f0 59 a2 7e 4a 1f 97 42  |&......U.Y.~J..B|
00001370  c8 24 04 66 e7 23 de 40  3d 21 ad d2 2a de 8a 13  |.$.f.#.@=!..*...|
00001380  f4 cf 99 2f f7 59 1e 55  c7 4b c4 ec 05 b7 cc a9  |.../.Y.U.K......|
00001390  fc ed fa 6d 8c 48 bf 4e  94 ef 37 8d 9c 7a 7d 78  |...m.H.N..7..z}x|
000013a0  24 8d 9b 39 90 1f 02 85  a4 c6 12 ce 7a b8 1d c1  |$..9........z...|
000013b0  64 4a af c0 7f ff cd 95  f0 59 a1 ec a9 13 70 cb  |dJ.......Y....p.|
000013c0  9e 18 b8 c7 1a a1 2c 27  ea e8 e9 7a 1f 43 be 51  |......,'...z.C.Q|
000013d0  ed 56 96 c1 ab 6a cf 4f  f8 98 34 15 03 0e cf f1  |.V...j.O..4.....|
000013e0  f3 06 b7 3a 48 41 21 d2  b0 bf a8 b6 7a 7d 7a 67  |...:HA!.....z}zg|
000013f0  b6 4b 38 f0 dd e3 5a a5  9a c6 12 ce 7a b8 1d c1  |.K8...Z.....z...|
00001400  64 bd 0f 60 a9 3f d1 55  f0 59 a2 7e 60 72 47 06  |d..`.?.U.Y.~`rG.|
00001410  91 e6 4a d6 d5 e7 0a 4a  f6 3a 26 ad bc eb f4 cc  |..J....J.:&.....|
00001420  6a 25 64 85 b0 24 c6 fd  fe 79 a4 32 ac 76 ae 60  |j%d..$...y.2.v.`|
00001430  79 b7 b1 10 58 dd cf b2  e0 5b fa 19 5c 58 93 10  |y...X....[..\X..|
00001440  5b 24 69 13 38 1e 6a 8f  86 63 09 67 3d 5c 0e e0  |[$i.8.j..c.g=\..|
00001450  93 1d 4b 78 7f ff ed 95  f0 59 a2 7e cd 34 13 4d  |..Kx.....Y.~.4.M|
00001460  fd de 6e 9e 25 99 ef 2b  ae 80 b2 d0 2d 37 60 0d  |..n.%..+....-7`.|
00001470  86 6a bf 18 0f 33 74 68  3d e3 9d f8 c6 af e8 85  |.j...3th=.......|
00001480  f3 a2 d2 ea 6d 8c 6a 42  22 4a a7 b8 40 f1 62 07  |....m.jB"J..@.b.|
00001490  5e e3 03 7a e8 1e 1b 37  5a 63 09 67 3d 8d 1d c1  |^..z...7Zc.g=...|
000014a0  64 cc a7 8d fb ff cd 55  f0 59 a1 ec a9 14 06 6c  |d......U.Y.....l|
000014b0  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 18 08 a0 bc  |)........M......|
000014c0  c2 68 d2 11 08 dc d6 b2  8a 28 00 6e cf 78 a0 7b  |.h.......(.n.x.{|
000014d0  52 d2 65 39 24 90 79 d1  25 28 09 74 3e 2c 59 5c  |R.e9$.y.%(.t>,Y\|
000014e0  43 16 a2 d9 9e f1 0d a2  d3 63 09 67 3d 5c 0e e0  |C........c.g=\..|
000014f0  b2 1e 47 b1 09 3f cd 95  f0 59 a2 7e 60 71 82 24  |..G..?...Y.~`q.$|
00001500  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 85  |.<..E. ..E{..e..|
00001510  c8 d2 ed 84 5e 70 ae 77  50 10 ef 59 8d 53 6f ef  |....^p.wP..Y.So.|
00001520  c7 fe 6e 74 90 82 37 4a  75 f4 19 2a 38 b0 f3 7b  |..nt..7Ju..*8..{|
00001530  7b b5 90 ff c0 1e 16 a5  a6 c6 12 cf ca b8 1d c1  |{...............|
00001540  64 bb 2b 78 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |d.+x...U.Y.~`rG.|
00001550  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc eb f4 cb  |..J.l..J.:&.....|
00001560  2a 98 86 aa a2 18 d7 9b  d4 8d d1 ce 0b ce c4 d1  |*...............|
00001570  81 ea 27 6c 92 1f 91 d2  b0 b9 64 85 bc 58 81 69  |..'l......d..X.i|
00001580  1b 74 ee cb 2f 38 6a 97  5a 63 09 67 3d 5c 0e e0  |.t../8j.Zc.g=\..|
00001590  b2 1e 47 78 7f ff f1 95  f0 59 a1 ec a9 13 70 cb  |..Gx.....Y....p.|
000015a0  9e 18 b8 c7 1a a1 2c 27  ea e8 e9 7a 1f 17 8d be  |......,'...z....|
000015b0  bb 43 cc 1b 42 2c 06 71  07 4d bf 80 67 c2 39 ff  |.C..B,.q.M..g.9.|
000015c0  1b 1a 6d 8c 6e 98 8e 94  eb ff 1e e2 fa 7d 79 3c  |..m.n........}y<|
000015d0  62 32 6d 70 08 1e 6a 85  a6 a6 f2 ce 7a b8 1d c1  |b2mp..j.....z...|
000015e0  64 ba af 68 a5 3f cd 55  f0 59 a2 7e 60 72 47 06  |d..h.?.U.Y.~`rG.|
000015f0  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc da f5 84  |..J.l..J.:&.....|
00001600  8b ab 8c 66 48 a0 29 ed  85 10 bf af 5a e2 3c e8  |...fH.).....Z.<.|
00001610  27 9e 9b 63 1b a5 ab a5  3b 91 5b 8d 1c 58 7b 61  |'..c....;.[..X{a|
00001620  60 f8 08 f8 d0 1f f8 d7  5a 53 79 67 3d 5c 0e e0  |`.......ZSyg=\..|
00001630  b2 26 83 b8 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |.&.......Y.~`q.$|
00001640  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 85  |.<..E. ..E{..e..|
00001650  c8 77 58 ff d2 16 15 2d  42 65 7c c9 4f 36 9e 48  |.wX....-Be|.O6.H|
00001660  07 1d 39 23 c5 9e a3 a5  61 78 0b cf 38 6f 97 3e  |..9#....ax..8o.>|
00001670  63 23 b0 8a a8 1e 16 a5  a6 c6 12 cf ca b8 1d c1  |c#..............|
00001680  64 ba 83 68 7f ff cd 55  f0 59 a2 7e 60 78 85 48  |d..h...U.Y.~`x.H|
00001690  ac 7b f6 90 f7 76 d6 d0  81 bd ad 9e 93 15 75 31  |.{...v........u1|
000016a0  c0 2b e6 5d 16 b2 f3 ed  76 24 7c 83 a5 77 e3 cb  |.+.]....v$|..w..|
000016b0  2c 40 c2 b6 7c b1 ba e6  dc d6 64 83 47 16 1e a6  |,@..|.....d.G...|
000016c0  5e 52 60 f4 fe 07 86 12  d3 53 79 67 3d 5c 0e e0  |^R`......Syg=\..|
000016d0  e1 23 ff f0 7f ff d1 95  f0 59 a1 ec 98 19 9a 31  |.#.......Y.....1|
000016e0  55 09 70 59 a1 4b a3 f0  14 79 c9 54 29 43 1d be  |U.pY.K...y.T)C..|
000016f0  15 e3 d9 96 76 74 90 03  3b 42 5d 98 c5 cc 8b e5  |....vt..;B].....|
00001700  47 b1 a2 22 dc ec 6e db  02 4a 56 18 65 71 62 c6  |G.."..n..JV.eqb.|
00001710  cd 13 5a 0a f0 1e 1b 25  a6 a6 f2 cf ca b8 1d c1  |..Z....%........|
00001720  64 ab 27 80 a5 3f cd 55  f0 59 a2 7e 60 78 ab 43  |d.'..?.U.Y.~`x.C|
00001730  ce 07 37 88 00 ae a4 5b  7d 80 dc 39 df 39 cd fa  |..7....[}..9.9..|
00001740  d6 97 69 01 3f 61 69 db  f5 6b 22 57 77 b7 3f dd  |..i.?ai..k"Ww.?.|
00001750  ef 90 c4 61 8d 48 3d d2  9d c4 e3 a0 3e 2c 3c 44  |...a.H=.....>,<D|
00001760  a6 42 e7 62 30 0f fc 62  d3 53 79 67 3d 5c 0e e0  |.B.b0..b.Syg=\..|
00001770  e1 26 3b b0 7f ff ed 95  f0 59 a2 7e 60 71 82 24  |.&;......Y.~`q.$|
00001780  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 36 96 db  |.<..E. ..E{..6..|
00001790  dd c8 8e 27 48 4d 8e 74  a9 11 7b d7 ec aa 86 bb  |...'HM.t..{.....|
000017a0  2b ed 93 86 37 39 97 4a  77 91 57 3d f8 b1 02 9e  |+...79.Jw.W=....|
000017b0  8a b2 7b c5 08 1e 6a 85  a6 a6 f2 cf ca b8 1d c1  |..{...j.........|
000017c0  64 bc 8b b8 7f ff d1 55  f0 59 a2 7e 60 72 47 06  |d......U.Y.~`rG.|
000017d0  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc da f5 84  |..J.l..J.:&.....|
000017e0  8b 9c 14 63 b8 b0 07 5e  23 4b 2d fd 40 fa e5 b0  |...c...^#K-.@...|
000017f0  89 c4 ae 33 1a 93 3b a5  3b 97 9f 33 fc 58 83 b9  |...3..;.;..3.X..|
00001800  8f 17 fd 42 00 1f f8 d7  5a 53 79 67 3d 5c 0e e0  |...B....ZSyg=\..|
00001810  b2 1d 87 b0 7f ff cd 95  f0 59 a1 ec a9 13 70 cb  |.........Y....p.|
00001820  9e 18 b8 c7 1a a1 2c 27  ea e8 e9 7a 1f 43 bc 53  |......,'...z.C.S|
00001830  66 b5 17 94 c5 3a 1d 54  e5 b4 c1 0d e6 88 7c ee  |f....:.T......|.|
00001840  58 04 59 69 21 03 47 4a  c2 ee ce b3 7b 40 72 f8  |X.Yi!.GJ....{@r.|
00001850  77 a9 79 8e cd e3 4c 65  a6 c6 12 ce 7a b8 1d c1  |w.y...Le....z...|
00001860  64 bc b3 98 85 3f ed 55  f0 59 a2 7e 60 72 47 06  |d....?.U.Y.~`rG.|
00001870  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc da f5 83  |..J.l..J.:&.....|
00001880  4c e0 f2 19 83 34 e8 db  a3 49 cc 01 7d 41 3c d7  |L....4...I..}A<.|
00001890  71 8a bd 0b 1b a8 01 d2  b0 bd f9 1f fc 37 c9 f3  |q............7..|
000018a0  28 ad ac ee 37 39 f8 d7  67 53 79 67 3d 5c 0e e0  |(...79..gSyg=\..|
000018b0  b2 1d 47 68 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |..Gh.....Y.~`q.$|
000018c0  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 36 94 d4  |.<..E. ..E{..6..|
000018d0  38 c0 41 91 ea 5a 6b 3d  e8 b9 7c c6 c2 f1 3e b8  |8.A..Zk=..|...>.|
000018e0  6e f5 39 23 c5 9f 37 4a  75 90 40 f0 38 6f 92 37  |n.9#..7Ju.@.8o.7|
000018f0  6d 26 73 f7 b0 1f f8 c5  ad a6 f2 cf ca b8 1d c1  |m&s.............|
00001900  64 bd 2f 28 7f ff d1 55  f0 59 a2 7e 60 72 47 06  |d./(...U.Y.~`rG.|
00001910  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc da f5 84  |..J.l..J.:&.....|
00001920  92 60 e2 de 3b cb eb c1  f7 10 a8 82 ec 99 19 5e  |.`..;..........^|
00001930  9f d5 5e 9b 93 11 fc 60  92 a9 4f 4d 7c 58 7a 75  |..^....`..OM|Xzu|
00001940  31 9a f3 08 08 1f f8 cf  81 53 79 67 3d 5c 0e e0  |1........Syg=\..|
00001950  b2 1e 4b a8 7f ff cd 95  f0 59 a2 7e 4a 1b 98 24  |..K......Y.~J..$|
00001960  86 af 69 6d ae 38 70 e0  5e 2c 3c f4 b2 87 2a 87  |..im.8p.^,<...*.|
00001970  41 01 fe 29 68 a3 81 6d  64 1b 28 f1 5f 9d 0d 4e  |A..)h..md.(._..N|
00001980  44 f3 c2 9b 63 12 2f d5  cd 66 3c 14 71 61 e0 6b  |D...c./..f<.qa.k|
00001990  0b b5 45 a0 20 3e 98 ee  b2 a6 f2 cf ca b8 1d c1  |..E. >..........|
000019a0  64 cb 07 c2 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |d...{..U.Y.....l|
000019b0  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 17 fb b3 d9  |)........M......|
000019c0  5d a0 4e b5 20 d2 0c 9a  9f 4f 23 9c dc d6 61 e6  |].N. ....O#...a.|
000019d0  a0 dd a2 f3 49 08 1a 25  b1 60 d3 11 de 1b e4 a4  |....I..%.`......|
000019e0  61 75 4e 9b 2c 0f fc 62  d2 53 79 67 3d 5c 0e e0  |auN.,..b.Syg=\..|
000019f0  b2 1e 5b 70 a9 3f f1 95  f0 59 a2 7e 4a 1b 98 24  |..[p.?...Y.~J..$|
00001a00  86 af 69 6d ae 38 70 e0  5e 2c 3c f4 b2 87 2a a1  |..im.8p.^,<...*.|
00001a10  25 e2 1e 8c 55 ce f4 8e  50 84 ad 2a db 2a 9a 37  |%...U...P..*.*.7|
00001a20  cc 38 37 a1 49 08 22 bc  e7 68 b7 41 f1 62 00 d0  |.87.I."..h.A.b..|
00001a30  44 63 27 f2 eb c7 f1 ae  bb a6 f2 cf ca b8 1d c1  |Dc'.............|
00001a40  64 bb 0b 80 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |d......U.Y.~`rG.|
00001a50  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc da f6 61  |..J.l..J.:&....a|
00001a60  d0 16 8b ff 67 51 28 d8  c5 5b 5d fe 6a 35 5e 7b  |....gQ(..[].j5^{|
00001a70  6e b2 ae 33 1a 90 a1 d2  b0 bf 1e ca 9c 37 ca ec  |n..3.........7..|
00001a80  72 c6 24 87 98 1e 1b 37  5d d3 79 67 3d 5c 0e e0  |r.$....7].yg=\..|
00001a90  b2 1e 4b 60 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |..K`.....Y.~`q.$|
00001aa0  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 36 94 d4  |.<..E. ..E{..6..|
00001ab0  38 39 b7 62 ff 98 ac 23  6f 6b d8 6b 27 ba 86 5f  |89.b...#ok.k'.._|
00001ac0  95 fd 37 26 23 f9 07 4a  77 93 53 e8 b8 b0 f5 ff  |..7&#..Jw.S.....|
00001ad0  14 5f da c0 50 1f 4c 65  a4 a6 f2 cf ca b8 1d c1  |._..P.Le........|
00001ae0  64 ba b3 b0 7b ff d1 55  f0 59 a1 ec a9 14 06 6c  |d...{..U.Y.....l|
00001af0  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 17 fb b4 6e  |)........M.....n|
00001b00  ac 09 bb 5b d6 85 f9 c9  b8 7a 3b f6 8c 67 43 f0  |...[.....z;..gC.|
00001b10  4e 05 16 de 62 3f 8e 5f  45 6a 25 d9 3e 2c 3d f1  |N...b?._Ej%.>,=.|
00001b20  19 46 69 d3 3c 4f 08 a2  d2 53 79 67 3d 5c 0e e0  |.Fi.<O...Syg=\..|
00001b30  b2 26 43 68 a9 3f cd 95  f0 59 a2 7e 60 71 82 24  |.&Ch.?...Y.~`q.$|
00001b40  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 85  |.<..E. ..E{..e..|
00001b50  c3 1c a4 04 da 16 15 52  a3 fd 58 0e b6 80 9b 7f  |.......R..X.....|
00001b60  8c f4 9b 9d 24 20 89 d1  eb c1 39 13 38 6f 93 ef  |....$ ....9.8o..|
00001b70  12 29 8f 3f 08 1e 1b 25  ad c6 12 cf ca b8 1d c1  |.).?...%........|
00001b80  64 bb 37 70 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |d.7p...U.Y.~`rG.|
00001b90  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc eb f4 cb  |..J.l..J.:&.....|
00001ba0  2a 9a ef d7 f7 1c 8c e6  8a 3e d4 71 07 98 8b f4  |*........>.q....|
00001bb0  62 4f 69 b6 31 a9 0c 13  25 61 85 51 3c 58 80 9c  |bOi.1...%a.Q<X..|
00001bc0  db 55 f8 22 cf 38 6a 97  59 63 09 67 3d 5c 0e e0  |.U.".8j.Yc.g=\..|
00001bd0  b2 1e 47 60 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |..G`.....Y.~`q.$|
00001be0  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 85  |.<..E. ..E{..e..|
00001bf0  d9 23 7d 0d ae 35 f5 b2  c0 6c f1 7b 18 26 06 7d  |.#}..5...l.{.&.}|
00001c00  97 63 6e 76 5b 84 f3 a5  61 7e 58 b7 f8 6f 97 96  |.cnv[...a~X..o..|
00001c10  6d 7d ac b6 30 1e 1b 25  a4 c6 12 cf ca b8 1d c1  |m}..0..%........|
00001c20  64 ba 8b 68 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |d..h{..U.Y.....l|
00001c30  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 18 08 a0 b6  |)........M......|
00001c40  8d f7 d5 db 9e d4 d1 16  d3 6a bc 88 dd 42 70 f1  |.........j...Bp.|
00001c50  f4 f9 e0 0d c9 8d d6 fc  49 54 04 a3 2e 2c 3c d8  |........IT...,<.|
00001c60  86 d6 a2 9a 94 4f 34 32  d2 63 09 67 3d 5c 0e e0  |.....O42.c.g=\..|
00001c70  b2 26 97 d8 89 3f cd 95  f0 59 a2 7e 4a 1b 98 24  |.&...?...Y.~J..$|
00001c80  86 af 69 6d ae 38 70 e0  5e 2c 3c f4 b3 79 08 01  |..im.8p.^,<..y..|
00001c90  7e f2 45 01 c0 2b dd 9e  68 d2 10 e9 a3 05 e7 de  |~.E..+..h.......|
00001ca0  80 a9 16 5a 48 40 d1 33  03 d7 8f 9f 71 61 e8 8b  |...ZH@.3....qa..|
00001cb0  6e 43 2a 39 3f 60 36 6e  bb c6 12 cf ca b8 1d c1  |nC*9?`6n........|
00001cc0  64 ba 87 40 7f ff ed 55  f0 59 a2 7e 60 72 47 06  |d..@...U.Y.~`rG.|
00001cd0  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc da f6 61  |..J.l..J.:&....a|
00001ce0  c9 52 d8 6c ba 86 fb be  8a 39 7f dd 6d 5e 89 f4  |.R.l.....9..m^..|
00001cf0  db 64 4e b7 3a 48 41 30  92 95 8e 91 bc 37 c9 9c  |.dN.:HA0.....7..|
00001d00  fd 08 c8 ec 50 1e 1b 37  5a 54 cd 67 3d 5c 0e e0  |....P..7ZT.g=\..|
00001d10  b2 26 83 b8 7f ff d1 95  f0 59 a2 7e 60 71 82 24  |.&.......Y.~`q.$|
00001d20  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 73  |.<..E. ..E{..e.s|
00001d30  dd a5 16 bf 95 2d 7a 33  fe c0 4c de 78 ec 0c d4  |.....-z3..L.x...|
00001d40  40 d8 cd c9 89 17 81 d2  9d 67 e7 5d 38 b1 04 ae  |@........g.]8...|
00001d50  dc 66 76 4d 20 1e 6a 85  a4 c6 12 cf ca b8 1d c1  |.fvM .j.........|
00001d60  64 aa af 70 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |d..p{..U.Y.....l|
00001d70  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 18 08 a0 b6  |)........M......|
00001d80  92 8f a0 52 16 63 92 b1  3b b4 72 6f 2f 4b de 3d  |...R.c..;.ro/K.=|
00001d90  90 1f c5 39 23 b4 76 71  25 24 b7 25 3e 2c 3d a0  |...9#.vq%$.%>,=.|
00001da0  c2 a5 7c e1 90 0f 35 42  d6 e3 09 67 3d 5c 0e e0  |..|...5B...g=\..|
00001db0  b2 26 47 78 a9 3f ed 95  f0 59 a2 7e 60 71 82 24  |.&Gx.?...Y.~`q.$|
00001dc0  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 73  |.<..E. ..E{..e.s|
00001dd0  dd c6 b9 3f d1 f3 eb eb  dc b3 d7 fe c3 60 6e 7d  |...?.........`n}|
00001de0  9f fc 35 a6 24 5d 07 4a  77 92 dd d7 78 b1 01 c7  |..5.$].Jw...x...|
00001df0  fb 00 fe 54 a8 1e 68 65  9a c6 12 cf ca b8 1d c1  |...T..he........|
00001e00  64 ba af e8 7f ff cd 55  f0 59 a2 7e 60 72 47 06  |d......U.Y.~`rG.|
00001e10  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc eb f4 cc  |..J.l..J.:&.....|
00001e20  69 57 fa b0 5d 96 0c 1a  7b 28 35 08 25 97 95 ee  |iW..]...{(5.%...|
00001e30  aa 38 89 ed b1 1f c8 93  25 80 44 f7 3c 37 c8 9a  |.8......%.D.<7..|
00001e40  dd 4c ae e6 a8 1e 6a 97  5a 63 09 67 3d 5c 0e e0  |.L....j.Zc.g=\..|
00001e50  b2 1e 87 30 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |...0.....Y.~`q.$|
00001e60  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 36 96 ed  |.<..E. ..E{..6..|
00001e70  c8 dd 15 ba 06 e0 c3 00  df b8 ef b4 3b a0 25 fb  |............;.%.|
00001e80  d4 05 6e 76 24 67 a7 4a  77 19 8e ad 78 b0 f1 f2  |..nv$g.Jw...x...|
00001e90  5a ad 85 89 e8 1e 1b 25  a6 a6 f2 cf ca b8 1d c1  |Z......%........|
00001ea0  64 bc ab 28 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |d..({..U.Y.....l|
00001eb0  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 17 fb b4 69  |)........M.....i|
00001ec0  b2 02 23 79 47 3a 12 85  8c 1c 97 6a 76 aa 65 dd  |..#yG:.....jv.e.|
00001ed0  55 e8 41 b2 62 45 e0 26  4a c2 b4 e2 8e 2c 61 9e  |U.A.bE.&J....,a.|
00001ee0  ff 2a c6 1e fc 0f 35 42  d6 d3 79 67 3d 5c 0e e0  |.*....5B..yg=\..|
00001ef0  b2 1d 87 78 a9 3f d1 95  f0 59 a2 7e 60 71 82 24  |...x.?...Y.~`q.$|
00001f00  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 36 94 d4  |.<..E. ..E{..6..|
00001f10  49 31 1e 92 39 c9 18 63  1e 8a 34 c6 2d b5 4f e9  |I1..9..c..4.-.O.|
00001f20  e3 51 4e 49 8d d3 11 d2  9d c5 d1 55 f8 b0 f4 97  |.QNI.......U....|
00001f30  9f 54 7a 4e c8 1f f8 c5  a6 a6 f2 cf ca b8 1d c1  |.TzN............|
00001f40  64 cc a7 28 7f ff cd 55  f0 59 a2 7e 4a 1f 97 42  |d..(...U.Y.~J..B|
00001f50  c8 24 04 66 e7 23 de 40  3d 21 ad d2 2a 86 42 45  |.$.f.#.@=!..*.BE|
00001f60  eb 39 db b6 09 b4 c6 db  46 09 9b 0f 1e e2 7f 12  |.9......F.......|
00001f70  07 25 7a 6b 4c 6a 49 07  4a c2 ed 90 8b fa 7d 7a  |.%zkLjI.J.....}z|
00001f80  16 89 14 00 58 1e 16 a5  9a a6 f2 ce 7a b8 1d c1  |....X.......z...|
00001f90  64 3a 8f c0 7f ff ed 95  f0 59 a1 ec a9 41 69 43  |d:.......Y...AiC|
00001fa0  6d 10 47 9a d5 f7 d0 83  11 30 df 10 d7 fa 62 c6  |m.G......0....b.|
00001fb0  d6 70 97 77 76 80 5d aa  40 81 c4 b6 4e ff e0 6c  |.p.wv.].@...N..l|
00001fc0  a9 54 a6 4d 89 17 81 d2  9d 67 13 36 5e 2c 3d 39  |.T.M.....g.6^,=9|
00001fd0  5e 61 56 b6 17 9c 0d 92  d2 53 79 67 3d 5c 0e e0  |^aV......Syg=\..|
00001fe0  e1 5e 82 b0 85 3f d1 55  f0 59 a2 7e 60 78 ab 43  |.^...?.U.Y.~`x.C|
00001ff0  ce 07 37 88 00 ae a4 5b  7d 80 dc 39 df 3a b5 a2  |..7....[}..9.:..|
00002000  d7 4f d3 1b ea d1 e4 0b  c6 3b 6a d0 2e b5 f9 fe  |.O.......;j.....|
00002010  71 69 56 7f d8 dd 40 dd  29 d7 9c 27 8e 1b e5 57  |qiV...@.)..'...W|
00002020  55 bd 6a 21 97 9c 07 a2  d2 63 09 67 3d 5c 0e e0  |U.j!.....c.g=\..|
00002030  e1 25 3f b8 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |.%?......Y.~`q.$|
00002040  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 70  |.<..E. ..E{..e.p|
00002050  d7 2e 5d 7c 8b 2c d6 f6  cb 79 7c ce 1e fa c7 b3  |..]|.,...y|.....|
00002060  92 e5 36 c6 37 4b 37 4a  77 37 5c 4e 38 b0 f5 ba  |..6.7K7Jw7\N8...|
00002070  25 87 00 a1 67 b6 32 45  9a c6 12 cf ca b8 1d c1  |%...g.2E........|
00002080  64 cc 83 50 7f ff ed 55  f0 59 a2 7e 60 72 47 06  |d..P...U.Y.~`rG.|
00002090  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc eb f4 cc  |..J.l..J.:&.....|
000020a0  69 2c 15 85 71 61 19 80  69 ab cc 82 ed 7a 89 5f  |i,..qa..i....z._|
000020b0  67 1a e4 9b 93 11 fc 60  92 a9 72 5a dc 58 7b d3  |g......`..rZ.X{.|
000020c0  85 82 a3 b6 20 1e 6a 97  59 63 09 67 3d 5c 0e e0  |.... .j.Yc.g=\..|
000020d0  b2 16 57 b8 7f ff cd 95  f0 59 a2 7e 60 71 82 24  |..W......Y.~`q.$|
000020e0  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1c 6c  |.<..E. ..E{..e.l|
000020f0  33 9a aa 6f 41 0f 78 86  b2 9f cc 93 0a e3 a8 e5  |3..oA.x.........|
00002100  86 2c 4d c9 8d d1 f5 d2  9d cd 39 c3 f8 b0 f1 6b  |.,M.......9....k|
00002110  fc 24 41 55 70 1f 4c 65  a4 c6 12 cf ca b8 1d c1  |.$AUp.Le........|
00002120  64 cb 07 f0 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |d...{..U.Y.....l|
00002130  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 18 08 a0 bb  |)........M......|
00002140  88 31 b9 8e 50 4f 47 c0  d3 9b e6 5d 80 8b e6 a0  |.1..POG....]....|
00002150  61 38 d5 db 9d 8d dc 68  49 4c a4 ed ee 2c 3d c5  |a8.....hIL...,=.|
00002160  f7 a9 53 49 54 0f ad 52  d2 63 09 67 3d 5c 0e e0  |..SIT..R.c.g=\..|
00002170  b2 26 53 b0 a9 3f d1 95  f0 59 a2 7e 60 71 82 24  |.&S..?...Y.~`q.$|
00002180  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 85  |.<..E. ..E{..e..|
00002190  c3 49 43 e7 d0 e2 7b 38  d4 17 57 4f c7 87 74 f4  |.IC...{8..WO..t.|
000021a0  79 e0 9b 9d 24 1e 1c e9  58 5c dd 96 38 b1 01 32  |y...$...X\..8..2|
000021b0  5b ca 09 7f 18 1e 1b 25  9a c6 12 cf ca b8 1d c1  |[......%........|
000021c0  64 bb 0f 60 7f ff ed 55  f0 59 a2 7e 60 72 47 06  |d..`...U.Y.~`rG.|
000021d0  91 e6 4a d9 6c 9e 0c 4a  f6 3a 26 ad bc eb f4 d3  |..J.l..J.:&.....|
000021e0  d0 06 ad 23 26 7e 51 e9  ee 8a ff 38 40 83 6a 5f  |...#&~Q....8@.j_|
000021f0  98 12 9c 93 12 2f 03 a5  3a c9 4c 3e 5c 58 81 88  |...../..:.L>\X..|
00002200  1f 23 21 31 88 1e 16 b7  59 63 09 67 3d 5c 0e e0  |.#!1....Yc.g=\..|
00002210  b2 16 4b 60 7f ff d1 95  f0 59 a2 7e 60 71 82 24  |..K`.....Y.~`q.$|
00002220  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 65 1e 73  |.<..E. ..E{..e.s|
00002230  d8 13 99 9f 37 3b 83 e2  58 af 10 45 a9 16 56 f6  |....7;..X..E..V.|
00002240  33 aa 4f 6d 8d d2 b5 d2  9d dc 65 75 b8 6f 97 d3  |3.Om......eu.o..|
00002250  ea 19 65 3d 18 1f 92 05  a4 c6 12 cf ca b8 1d c1  |..e=............|
00002260  64 db 27 b0 7b ff cd 55  f0 59 a1 ec a9 14 06 6c  |d.'.{..U.Y.....l|
00002270  29 04 d1 c3 19 02 0a 05  07 4d 9b a0 17 fb b3 d9  |)........M......|
00002280  5f 7b 7a 2a 32 8e 60 ab  dd 9d 40 6e ca 76 fc 7f  |_{z*2.`...@n.v..|
00002290  47 fa 0d 39 26 37 7a e1  25 53 cc 63 7e 2c 40 25  |G..9&7z.%S.c~,@%|
000022a0  76 07 ee 32 78 0f a6 62  d3 53 79 67 3d 5c 0e e0  |v..2x..b.Syg=\..|
000022b0  b2 1e 43 70 a9 3f cd 95  f0 59 a2 7e 60 71 82 24  |..Cp.?...Y.~`q.$|
000022c0  ab 3c 1c e4 45 9d 20 a2  a9 45 7b 0f fc 36 96 ed  |.<..E. ..E{..6..|
000022d0  c8 66 5d 18 39 e3 76 43  da 43 80 24 bc 37 03 f7  |.f].9.vC.C.$.7..|
000022e0  2b 5a 2c b6 23 f9 03 a5  61 71 c2 39 b8 6f 90 4b  |+Z,.#...aq.9.o.K|
000022f0  fd 32 a6 4f f8 1e 16 a5  a6 a6 f2 cf ca b8 1d c1  |.2.O............|
00002300  64 bc b3 28 7f ff cd 55  4f 67 67 53 00 04 b8 bc  |d..(...UOggS....|
00002310  00 00 00 00 00 00 2d 4b  46 66 03 00 00 00 25 6a  |......-KFf....%j|
00002320  9e dd 01 50 f0 33 43 89  6b 67 d8 ef da af ec c4  |...P.3C.kg......|
00002330  81 b6 d3 22 ce bd da 50  ed 52 35 a6 48 66 14 a1  |..."...P.R5.Hf..|
00002340  58 67 a6 a0 8e 76 fe fa  a5 bc 26 07 96 09 4f aa  |Xg...v....&...O.|
00002350  6e 58 1d 5e 00 a5 e0 ed  e8 0c 3a 22 16 8b 89 a7  |nX.^......:"....|
00002360  e6 2d ba 96 3d 82 28 6f  bb 7d e5 35 59 c2 2c 33  |.-..=.(o.}.5Y.,3|
00002370  4b 59 d5 17                                       |KY..|
