.PHONY: clean

OPTFLAGS = -O3 -I$(CURDIR)

OBJECTS = image.o loadimage.o loadexif.o infrastructure/gl-object.o gl-texture.o gl-shape.o gl-tile.o gl-stage.o \
	  gl-container.o gl-container-2d.o gl-tiled-image.o gl-renderloop.o gl-renderloop-member.o egl-driver.o \
	  gl-value-animation.o gl-value-animation-easing.o \
          labels/gl-label.o labels/gl-label-renderer.o labels/gl-label-scroller-segment.o \
	  labels/gl-label-scroller.o gl-rectangle.o gl-bitmap.o \
	  infrastructure/gl-notice.o infrastructure/gl-notice-subscription.o \
	  infrastructure/gl-workqueue.o infrastructure/gl-workqueue-job.o \
	  gl-image.o gl-framed-shape.o

CFLAGS_ALL = -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	  -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	  -ftree-vectorize -pipe -Wno-psabi -Wno-deprecated-declarations

LIBEXIF_A = ../lib/libexif/libexif/.libs/libexif.a
LIBEXIF_INCLUDEDIR = -I../lib/libexif
LIBFREETYPE_CFLAGS = `pkg-config freetype2 --cflags`
LIBFREETYPE_LIBS = `pkg-config freetype2 --libs`
LIBHARFBUZZ_CFLAGS = `pkg-config harfbuzz --cflags`
LIBHARFBUZZ_LIBS = `pkg-config harfbuzz --libs`
PTHREAD_LIBS = -pthread
PTHREAD_CFLAGS = $(PTHREAD_LIBS)

image: $(OBJECTS) $(LIBEXIF_A)
	cc -o image -Wl,--whole-archive $(OBJECTS) \
	-L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt \
	-L/opt/vc/src/hello_pi/libs/vgfont -ldl -lm -ljpeg \
	-Wl,--no-whole-archive $(LIBEXIF_A) -rdynamic $(OPTFLAGS) \
	$(LIBFREETYPE_LIBS) $(LIBHARFBUZZ_LIBS) $(PTHREAD_LIBS)

image.o: image.c loadimage.h gl-texture.h gl-shape.h gl-container.h
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
	 -Wno-deprecated-declarations $(OPTFLAGS)

egl-driver.o: egl-driver.c gl-texture.h gl-tile.h gl-stage.h gl-shape.h gl-renderloop-member.h \
		gl-renderloop.h gl-container.h gl-container-2d.h gl-tiled-image.h egl-driver.h gl-value-animation.h labels/gl-label.h \
		labels/gl-label-scroller-segment.h gl-rectangle.h gl-bitmap.h \
		infrastructure/gl-notice-subscription.h infrastructure/gl-notice.h \
		infrastructure/gl-workqueue.h infrastructure/gl-workqueue-job.h \
		gl-image.h gl-framed-shape.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c egl-driver.c -o egl-driver.o \
	$(OPTFLAGS)

gl-texture.o: gl-texture.c gl-texture.h infrastructure/gl-object.h gl-bitmap.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-texture.c -o gl-texture.o $(OPTFLAGS)

gl-shape.o: gl-shape.c gl-shape.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-shape.c -o gl-shape.o $(OPTFLAGS)

infrastructure/gl-object.o: infrastructure/gl-object.c infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-c infrastructure/gl-object.c -o infrastructure/gl-object.o $(OPTFLAGS)

gl-bitmap.o: gl-bitmap.c gl-bitmap.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-c gl-bitmap.c -o gl-bitmap.o $(OPTFLAGS)

gl-tile.o: gl-tile.c gl-tile.h gl-shape.h gl-texture.h gl-renderloop.h gl-renderloop-member.h
	cc -g $(CLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-tile.c -o gl-tile.o $(OPTFLAGS)

gl-rectangle.o: gl-rectangle.c gl-rectangle.h gl-shape.h
	cc -g $(CLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-rectangle.c -o gl-rectangle.o $(OPTFLAGS)

gl-stage.o: gl-stage.c gl-stage.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-stage.c -o gl-stage.o $(OPTFLAGS)

gl-container.o: gl-container.c gl-shape.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-container.c -o gl-container.o $(OPTFLAGS)

gl-container-2d.o: gl-container.h gl-shape.h infrastructure/gl-object.h gl-container-2d.h gl-container-2d.c
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-container-2d.c -o gl-container-2d.o $(OPTFLAGS)

gl-tiled-image.o: gl-tiled-image.c gl-tiled-image.h gl-container-2d.h \
		gl-container.h gl-shape.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-tiled-image.c -o gl-tiled-image.o $(OPTFLAGS)

gl-renderloop.o: gl-renderloop.c gl-renderloop.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-c gl-renderloop.c -o gl-renderloop.o $(OPTFLAGS)

gl-renderloop-member.o: gl-renderloop-member.c gl-renderloop.h \
		gl-renderloop-member.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-c gl-renderloop-member.c -o gl-renderloop-member.o $(OPTFLAGS)

gl-value-animation.o: gl-value-animation.c gl-value-animation.h \
		gl-renderloop.h gl-renderloop-member.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-value-animation.c -o gl-value-animation.o $(OPTFLAGS)

gl-value-animation-easing.o: gl-value-animation-easing.c gl-value-animation-easing.h gl-value-animation.h \
		 gl-renderloop.h gl-renderloop-member.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-value-animation-easing.c -o gl-value-animation-easing.o $(OPTFLAGS)

labels/gl-label.o: labels/gl-label.c labels/gl-label.h gl-container-2d.h \
		gl-container.h gl-shape.h infrastructure/gl-object.h gl-texture.h gl-tile.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	$(LIBFREETYPE_CFLAGS) $(LIBHARFBUZZ_CFLAGS) \
	-c labels/gl-label.c -o labels/gl-label.o $(OPTFLAGS)

labels/gl-label-scroller-segment.o: labels/gl-label-scroller-segment.c labels/gl-label-scroller-segment.h \
                labels/gl-label.h gl-container-2d.h gl-container.h gl-shape.h \
		infrastructure/gl-object.h gl-texture.h gl-tile.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	$(LIBFREETYPE_CFLAGS) $(LIBHARFBUZZ_CFLAGS) \
	-c labels/gl-label-scroller-segment.c -o labels/gl-label-scroller-segment.o $(OPTFLAGS)

labels/gl-label-scroller.o: labels/gl-label-scroller.c labels/gl-label-scroller.h \
		labels/gl-label.h gl-container-2d.h gl-container.h gl-shape.h \
		infrastructure/gl-object.h gl-texture.h gl-tile.h \
		labels/gl-label-scroller-segment.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	$(LIBFREETYPE_CFLAGS) $(LIBHARFBUZZ_CFLAGS) \
	-c labels/gl-label-scroller.c -o labels/gl-label-scroller.o $(OPTFLAGS)

labels/gl-label-renderer.o: labels/gl-label-renderer.c labels/gl-label-renderer.h \
		infrastructure/gl-object.h gl-texture.h gl-tile.h
	cc $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	$(LIBFREETYPE_CFLAGS) $(LIBHARFBUZZ_CFLAGS) \
	-c labels/gl-label-renderer.c -o labels/gl-label-renderer.o $(OPTFLAGS)

infrastructure/gl-notice-subscription.o: infrastructure/gl-notice-subscription.c infrastructure/gl-notice-subscription.h \
		infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-c infrastructure/gl-notice-subscription.c -o infrastructure/gl-notice-subscription.o $(OPTFLAGS)

infrastructure/gl-notice.o: infrastructure/gl-notice.c infrastructure/gl-notice.h \
		infrastructure/gl-object.h infrastructure/gl-notice-subscription.h
	cc -g $(CFLAGS_ALL) \
	-c infrastructure/gl-notice.c -o infrastructure/gl-notice.o $(OPTFLAGS)

infrastructure/gl-workqueue-job.o: infrastructure/gl-workqueue-job.c infrastructure/gl-workqueue-job.h \
		infrastructure/gl-object.h infrastructure/gl-notice.h
	cc -g $(CFLAGS_ALL) \
	-c infrastructure/gl-workqueue-job.c -o infrastructure/gl-workqueue-job.o $(OPTFLAGS)

infrastructure/gl-workqueue.o: infrastructure/gl-workqueue.c infrastructure/gl-workqueue.h \
		infrastructure/gl-object.h infrastructure/gl-workqueue-job.h
	cc -g $(CFLAGS_ALL) \
	-c infrastructure/gl-workqueue.c -o infrastructure/gl-workqueue.o $(OPTFLAGS) \
	$(PTHREAD_LIBS)

gl-image.o: gl-image.c gl-image.h \
		infrastructure/gl-workqueue.h infrastructure/gl-workqueue-job.h gl-stage.h \
		loadimage.h gl-tiled-image.h gl-container-2d.h \
		gl-container.h infrastructure/gl-object.h \
		infrastructure/gl-notice-subscription.h infrastructure/gl-notice.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-image.c -o gl-image.o $(OPTFLAGS)

gl-framed-shape.o: gl-framed-shape.c gl-framed-shape.h \
		gl-rectangle.h gl-container.h gl-container-2d.h gl-shape.h infrastructure/gl-object.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-framed-shape.c -o gl-framed-shape.o $(OPTFLAGS)

slideshow/gl-slide-image.o: slideshow/gl-slide-image.c slideshow/gl-slide-image.h \
		gl-container.h gl-container-2d.h gl-shape.h infrastructure/gl-object.h \
		infrastructure/gl-notice-subscription.h infrastructure/gl-notice.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c slideshow/gl-slide-image.c -o slideshow/gl-slide-image.o $(OPTFLAGS)

slideshow/gl-slide.o: slideshow/gl-slide.c slideshow/gl-slide.h \
		gl-container.h gl-container-2d.h gl-shape.h infrastructure/gl-object.h \
		infrastructure/gl-notice-subscription.h infrastructure/gl-notice.h
	cc -g $(CFLAGS_ALL) \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c slideshow/gl-slide.c -o slideshow/gl-slide.o $(OPTFLAGS)

loadimage.o: loadimage.c loadimage.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	 -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	 -ftree-vectorize -pipe -Wno-psabi \
	 -I/opt/vc/include/ \
	 -c loadimage.c -o loadimage.o \
	 -Wno-deprecated-declarations $(OPTFLAGS)

loadexif.o: loadexif.c loadexif.h loadimage.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	 -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	 -ftree-vectorize -pipe -Wno-psabi \
	 -I/opt/vc/include/ \
	 $(LIBEXIF_INCLUDEDIR) \
	 -c loadexif.c -o loadexif.o \
	 -Wno-deprecated-declarations $(OPTFLAGS)

decodejpeg2rgb: decodejpeg2rgb.o
	cc -o decodejpeg2rgb -Wl,--whole-archive decodejpeg2rgb.o -L/opt/vc/lib/ \
	-lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt \
	-L/opt/vc/src/hello_pi/libs/vgfont -ldl -lm -ljpeg -lilclient \
	-L/opt/vc/src/hello_pi/libs/ilclient -lopenmaxil \
	-Wl,--no-whole-archive -rdynamic $(OPTFLAGS)

decodejpeg2rgb.o: decodejpeg2rgb.c
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
	 -I/opt/vc/src/hello_pi/libs/vgfont -g -c decodejpeg2rgb.c -o decodejpeg2rgb.o \
	 -Wno-deprecated-declarations $(OPTFLAGS)

clean:
	rm -f image decodejpeg2rgb decodejpeg2rgb.o $(OBJECTS)
