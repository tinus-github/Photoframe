.PHONY: clean

OPTFLAGS = -O3

OBJECTS = image.o loadimage.o loadexif.o gl-object.o gl-texture.o gl-shape.o gl-tile.o gl-stage.o \
	  gl-container.o gl-container-2d.o gl-tiled-image.o gl-renderloop.o gl-renderloop-member.o egl-driver.o
LIBEXIF_A = ../lib/libexif/libexif/.libs/libexif.a
LIBEXIF_INCLUDEDIR = -I../lib/libexif

image: $(OBJECTS) $(LIBEXIF_A)
	cc -o image -Wl,--whole-archive $(OBJECTS) \
	-L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt \
	-L/opt/vc/src/hello_pi/libs/vgfont -ldl -lm -ljpeg \
	-Wl,--no-whole-archive $(LIBEXIF_A) -rdynamic $(OPTFLAGS)

image.o: image.c loadimage.h gl-display.h gl-texture.h gl-shape.h gl-container.h
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

gl-display.o: gl-display.c gl-display.h gl-texture.h gl-shape.h gl-renderloop-member.h gl-renderloop.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-display.c -o gl-display.o \
	-Wno-deprecated-declarations $(OPTFLAGS)

egl-driver.o: egl-driver.c gl-display.h gl-texture.h gl-tile.h gl-stage.h gl-shape.h gl-renderloop-member.h gl-renderloop.h gl-container.h gl-container-2d.h gl-tiled-image.h egl-driver.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c egl-driver.c -o egl-driver.o \
	-Wno-deprecated-declarations $(OPTFLAGS)

gl-image.o: gl-image.c gl-image.h gl-display.h gl-texture.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-image.c -o gl-image.o \
	-Wno-deprecated-declarations $(OPTFLAGS)

gl-rect.o: gl-rect.c gl-rect.h gl-display.h gl-texture.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-rect.c -o gl-rect.o \
	-Wno-deprecated-declarations $(OPTFLAGS)

gl-texture.o: gl-texture.c gl-texture.h gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-Wno-deprecated-declarations \
	-c gl-texture.c -o gl-texture.o $(OPTFLAGS)

gl-shape.o: gl-shape.c gl-shape.h gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-Wno-deprecated-declarations \
	-c gl-shape.c -o gl-shape.o $(OPTFLAGS)

gl-object.o: gl-object.c gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-Wno-deprecated-declarations \
	-c gl-object.c -o gl-object.o $(OPTFLAGS)

gl-tile.o: gl-tile.c gl-tile.h gl-shape.h gl-texture.h gl-display.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-Wno-deprecated-declarations \
	-c gl-tile.c -o gl-tile.o $(OPTFLAGS)

gl-stage.o: gl-stage.c gl-stage.h gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-Wno-deprecated-declarations \
	-c gl-stage.c -o gl-stage.o $(OPTFLAGS)

gl-container.o: gl-container.c gl-shape.h gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-Wno-deprecated-declarations \
	-c gl-container.c -o gl-container.o $(OPTFLAGS)

gl-container-2d.o: gl-container.h gl-shape.h gl-object.h gl-container-2d.h gl-container-2d.c
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-Wno-deprecated-declarations \
	-c gl-container-2d.c -o gl-container-2d.o $(OPTFLAGS)

gl-tiled-image.o: gl-tiled-image.c gl-tiled-image.h gl-container-2d.h gl-container.h gl-shape.h gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-Wno-deprecated-declarations \
	-c gl-tiled-image.c -o gl-tiled-image.o $(OPTFLAGS)

gl-renderloop.o: gl-renderloop.c gl-renderloop.h gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-Wno-deprecated-declarations \
	-c gl-renderloop.c -o gl-renderloop.o $(OPTFLAGS)

gl-renderloop-member.o: gl-renderloop-member.c gl-renderloop.h gl-renderloop-member.h gl-object.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-Wno-deprecated-declarations \
	-c gl-renderloop-member.c -o gl-renderloop-member.o $(OPTFLAGS)

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
