OPUSDIR=../../tools/opus

INCLUDE:=
INCLUDE+= -I../
INCLUDE+= -I../../tools
INCLUDE+= -I$(OPUSDIR)
INCLUDE+= -I$(OPUSDIR)/celt
INCLUDE+= -I$(OPUSDIR)/include

CFLAGS:=
CFLAGS+= -Wall --std=gnu99 -g

LAPLACE_CSRC:=
LAPLACE_CSRC+= test_laplace_decode.c
LAPLACE_CSRC+= ../symbol_context.c
LAPLACE_CSRC+= $(OPUSDIR)/celt/entdec.c
LAPLACE_CSRC+= $(OPUSDIR)/celt/entenc.c
LAPLACE_CSRC+= $(OPUSDIR)/celt/laplace.c

laplace: $(LAPLACE_CSRC)
	gcc $(CFLAGS) $(INCLUDE) $(LAPLACE_CSRC) -o test_laplace_decode
