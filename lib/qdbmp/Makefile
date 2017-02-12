.PHONY: clean

OPTFLAGS = -O3 -I$(CURDIR)

CFLAGS_ALL = -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
		-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
		-ftree-vectorize -pipe -Wno-psabi -Wno-deprecated-declarations

OBJECTS = qdbmp.o

qdbmp.so: qdbmp.o
	cc ${OBJECTS} -shared \
	-o qdbmp.so

qdbmp.o: qdbmp.c qdbmp.h
	cc ${CFLAGS_ALL} -c \
	qdbmp.c -o qdbmp.o
