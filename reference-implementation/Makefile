C_SOURCES=
C_SOURCES+= celt_decode_frame.c
C_SOURCES+= range_coder.c
C_SOURCES+= frame_context.c
C_SOURCES+= symbol_context.c
C_SOURCES+= celt_util.c
C_SOURCES+= bit_allocation.c

all: $(C_SOURCES)
	gcc -Wall -std=gnu99 -g $(C_SOURCES) -lm -o celt-decode-frame

pvq-test: pvq.c pvq_test.c
	gcc -Wall -std=gnu99 -g $^ -o $@
