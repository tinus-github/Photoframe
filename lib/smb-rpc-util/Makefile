.PHONY: clean all

DEPDIR := .d

$(shell mkdir -p $(DEPDIR) >/dev/null)
# $(shell mkdir -p $(DEPDIR)/qrcode >/dev/null)

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$(@D)/$*.Td
POSTCOMPILE = mv -f $(DEPDIR)/$(@D)/$*.Td $(DEPDIR)/$(@D)/$*.d

CC = cc

OPTFLAGS = -O3 -I$(CURDIR)

TARGETS = smb-rpc-util smb-rpc-util-test

OBJECTS_UTIL = main.o smb-rpc-encode.o smb-rpc-auth.o smb-rpc-smb.o smb-rpc-command.o smb-rpc-buffer.o
OBJECTS_TEST = smb-rpc-util-test.o smb-rpc-encode.o

OBJECTS = $(OBJECTS_UTIL) $(OBJECTS_TEST)

SRCS = $(OBJECTS:.o=.c)

CFLAGS_ALL = -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
-ftree-vectorize -pipe -Wno-psabi -Wno-deprecated-declarations

SMBCLIENT_CFLAGS = `pkg-config smbclient --cflags`
SMBCLIENT_LIBS = `pkg-config smbclient --libs`
MININI_LIBS = ../../lib/minIni/dev/minIni.o
MININI_CFLAGS = -I../../lib/minIni/dev

CFLAGS = $(CFLAGS_ALL) \
$(OPTFLAGS) \
$(SMBCLIENT_CFLAGS) \
$(MININI_CFLAGS)

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) -c

all: $(TARGETS)

smb-rpc-util: $(OBJECTS_UTIL)
	cc -o smb-rpc-util -Wl,--whole-archive $(OBJECTS_UTIL) \
	-Wl,--no-whole-archive -rdynamic $(OPTFLAGS) \
	$(SMBCLIENT_LIBS)

smb-rpc-util-test: $(OBJECTS_TEST)
	cc -o smb-rpc-util-test -Wl,--whole-archive $(OBJECTS_TEST) \
	-Wl,--no-whole-archive -rdynamic $(OPTFLAGS) \
	$(MININI_CFLAGS)

%.o : %.c

%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

clean:
	rm -f $(TARGETS) $(OBJECTS)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
