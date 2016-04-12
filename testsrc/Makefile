.PHONY: clean

image: image.o loadimage.o loadexif.o gl-display.o gl-image.o
	cc -o image -Wl,--whole-archive image.o loadimage.o \
	loadexif.o gl-display.o gl-image.o \
	-L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt \
	-L/opt/vc/src/hello_pi/libs/vgfont -ldl -lm -ljpeg -lexif \
	-Wl,--no-whole-archive -rdynamic

image.o: image.c loadimage.h gl-display.h
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
	 -Wno-deprecated-declarations

gl-display.o: gl-display.c gl-display.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-display.c -o gl-display.o \
	-Wno-deprecated-declarations

gl-image.o: gl-image.c gl-image.h gl-display.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	-ftree-vectorize -pipe -Wno-psabi \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
	-I/opt/vc/include/interface/vmcs_host/linux \
	-c gl-image.c -o gl-image.o \
	-Wno-deprecated-declarations

loadimage.o: loadimage.c loadimage.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	 -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	 -ftree-vectorize -pipe -Wno-psabi \
	 -I/opt/vc/include/ \
	 -c loadimage.c -o loadimage.o \
	 -Wno-deprecated-declarations

loadexif.o: loadexif.c loadexif.h loadimage.h
	cc -g -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
	 -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall \
	 -ftree-vectorize -pipe -Wno-psabi \
	 -I/opt/vc/include/ \
	 -c loadexif.c -o loadexif.o \
	 -Wno-deprecated-declarations

decodejpeg2rgb: decodejpeg2rgb.o
	cc -o decodejpeg2rgb -Wl,--whole-archive decodejpeg2rgb.o -L/opt/vc/lib/ \
	-lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt \
	-L/opt/vc/src/hello_pi/libs/vgfont -ldl -lm -ljpeg -lilclient \
	-L/opt/vc/src/hello_pi/libs/ilclient -lopenmaxil \
	-Wl,--no-whole-archive -rdynamic

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
	 -Wno-deprecated-declarations

clean:
	rm -f image image.o gl-display.o loadimage.o loadexif.o decodejpeg2rgb decodejpeg2rgb.o