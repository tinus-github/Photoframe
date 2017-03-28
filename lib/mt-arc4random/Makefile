.PHONY: clean

OPTFLAGS = -O3 -I$(CURDIR)

CFLAGS_ALL = -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
		-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
		-ftree-vectorize -pipe -Wno-psabi -Wno-deprecated-declarations

OBJECTS = arc4random.o getentropy.o

arc4random.so: ${OBJECTS}
	cc ${OBJECTS} -shared \
	-o arc4random.so

arc4random.o: arc4random.c arc4random.h chacha_private.h
	cc ${CFLAGS_ALL} -c \
	arc4random.c -o arc4random.o

getentropy.o: getentropy.c
	cc ${CFLAGS_ALL} -c \
	getentropy.c -o getentropy.o

clean:
	rm -f arc4random.so ${OBJECTS}
