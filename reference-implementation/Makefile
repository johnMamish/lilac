C_SOURCES=
C_SOURCES+= celt_decode_frame.c
C_SOURCES+= range_coder.c
C_SOURCES+= frame_context.c
C_SOURCES+= symbol_context.c
C_SOURCES+= celt_util.c

all: $(C_SOURCES)
	gcc -Wall -std=gnu99 -g $(C_SOURCES) -o celt-decode-frame
