C_SOURCES=
C_SOURCES+= anti_collapse.c
C_SOURCES+= celt_decode_frame.c
C_SOURCES+= range_coder.c
C_SOURCES+= frame_context.c
C_SOURCES+= symbol_context.c
C_SOURCES+= celt_util.c
C_SOURCES+= bit_allocation.c
C_SOURCES+= fine_energy.c
C_SOURCES+= band_decoder.c
C_SOURCES+= band_shapes.c
C_SOURCES+= pvq.c
C_SOURCES+= pvq_decoder.c

all: $(C_SOURCES)
	gcc -Wall -std=gnu99 -g $(C_SOURCES) -lm -o celt-decode-frame

pvq-test: pvq.c pvq_test.c
	gcc -Wall -std=gnu99 -g -O0 $^ -lm -o $@
