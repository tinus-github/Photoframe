.PHONY: clean

DEPDIR := .d

$(shell mkdir -p $(DEPDIR) >/dev/null)
$(shell mkdir -p $(DEPDIR)/qrcode >/dev/null)
$(shell mkdir -p $(DEPDIR)/config >/dev/null)
$(shell mkdir -p $(DEPDIR)/fs >/dev/null)
$(shell mkdir -p $(DEPDIR)/images >/dev/null)
$(shell mkdir -p $(DEPDIR)/slideshow >/dev/null)
$(shell mkdir -p $(DEPDIR)/infrastructure >/dev/null)
$(shell mkdir -p $(DEPDIR)/labels >/dev/null)

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

CC = cc

OPTFLAGS = -O3 -I$(CURDIR)

OBJECTS = image.o infrastructure/gl-object.o gl-texture.o gl-shape.o gl-tile.o gl-stage.o \
	  gl-container.o gl-container-2d.o gl-tiled-image.o gl-renderloop.o gl-renderloop-member.o egl-driver.o \
	  gl-value-animation.o gl-value-animation-easing.o \
          labels/gl-label.o labels/gl-label-renderer.o labels/gl-label-scroller-segment.o \
	  labels/gl-label-scroller.o gl-rectangle.o gl-bitmap.o \
	  infrastructure/gl-notice.o infrastructure/gl-notice-subscription.o \
	  infrastructure/gl-workqueue.o infrastructure/gl-workqueue-job.o \
	  gl-image.o gl-framed-shape.o slideshow/gl-slide.o slideshow/gl-slide-image.o \
	  slideshow/gl-slideshow.o slideshow/gl-slideshow-images.o slideshow/gl-sequence.o slideshow/gl-sequence-ordered.o \
	  slideshow/gl-sequence-random.o slideshow/gl-sequence-selection.o common-driver.o \
	  images/loadimage.o images/loadexif.o images/gl-bitmap-scaler.o \
	  images/loadimage-jpg.o images/loadimage-png.o images/loadimage-bmp.o \
	  fs/gl-url.o fs/gl-stream.o fs/gl-stream-file.o fs/gl-stream-rewindable.o \
	  fs/gl-directory.o fs/gl-directory-file.o fs/gl-tree-cache-directory.o fs/gl-tree-cache-directory-ordered.o \
	  fs/gl-source-manager.o config/gl-configuration.o config/gl-config-section.o config/gl-config-value.o \
	  qrcode/gl-qrcode-data.o qrcode/gl-qrcode-image.o

CFLAGS_ALL = -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	  -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	  -ftree-vectorize -pipe -Wno-psabi -Wno-deprecated-declarations

CFLAGS_VC = 	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	  -I/opt/vc/include/interface/vmcs_host/linux

LIBEXIF_A = ../lib/libexif/libexif/.libs/libexif.a
LIBEXIF_INCLUDEDIR = -I../lib/libexif
LIBFREETYPE_CFLAGS = `pkg-config freetype2 --cflags`
LIBFREETYPE_LIBS = `pkg-config freetype2 --libs`
LIBHARFBUZZ_CFLAGS = `pkg-config harfbuzz --cflags`
LIBHARFBUZZ_LIBS = `pkg-config harfbuzz --libs`
LIBPNG_CFLAGS = `pkg-config libpng --cflags`
LIBPNG_LIBS = `pkg-config libpng --libs`
PTHREAD_LIBS = -pthread
PTHREAD_CFLAGS = $(PTHREAD_LIBS)
MININI_LIBS = ../lib/minIni/dev/minIni.o
MININI_CFLAGS = -I../lib/minIni/dev
QDBMP_CFLAGS = -I../lib/qdbmp
QDBMP_LIBS = ../lib/qdbmp/qdbmp.o
ARC4RANDOM_CFLAGS = -I../lib/mt-arc4random
ARC4RANDOM_LIBS = ../lib/mt-arc4random/arc4random.a
QRENCODE_LIBS = ../lib/libqrencode/install/lib/libqrencode.a
QRENCODE_CFLAGS = `PKG_CONFIG_PATH=../lib/libqrencode/install/lib/pkgconfig pkg-config --cflags libqrencode`

CFLAGS = $(CFLAGS_ALL) \
	$(OPTFLAGS) \
	$(CFLAGS_VC) \
	$(LIBEXIF_INCLUDEDIR) \
	$(LIBFREETYPE_CFLAGS) \
	$(LIBHARFBUZZ_CFLAGS) \
	$(LIBPNG_CFLAGS) \
	$(PTHREAD_FLAGS) \
	$(MININI_CFLAGS) \
	$(QDBMP_CFLAGS) \
	$(ARC4RANDOM_CFLAGS) \
	$(QRENCODE_CFLAGS)

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) -c

image: $(OBJECTS) $(LIBEXIF_A)
	cc -o image -Wl,--whole-archive $(OBJECTS) \
	-L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt \
	-L/opt/vc/src/hello_pi/libs/vgfont -ldl -lm -ljpeg \
	-Wl,--no-whole-archive $(LIBEXIF_A) -rdynamic $(OPTFLAGS) \
	$(LIBFREETYPE_LIBS) $(LIBHARFBUZZ_LIBS) $(PTHREAD_LIBS) \
	$(LIBPNG_LIBS) $(QDBMP_LIBS) $(MININI_LIBS) $(ARC4RANDOM_LIBS) $(QRENCODE_LIBS)

image.o: image.c images/loadimage.h gl-texture.h gl-shape.h gl-container.h
	cc -g  -DUSE_OPENGL -DUSE_EGL -DIS_RPI -DSTANDALONE \
	 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX \
	 -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	 -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g \
	 -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize \
	 -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST \
	 -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi \
	 -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	 -I/opt/vc/include/interface/vmcs_host/linux -I./ \
	 -I/opt/vc/src/hello_pi/libs/ilclient \
	 -I/opt/vc/src/hello_pi/libs/vgfont -g -c image.c -o image.o \
	 -Wno-deprecated-declarations $(OPTFLAGS) $(ARC4RANDOM_CFLAGS)

%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

clean:
	rm -f image decodejpeg2rgb decodejpeg2rgb.o $(OBJECTS)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
