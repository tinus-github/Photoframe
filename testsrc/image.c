
/*
 * code stolen from openGL-RPi-tutorial-master/encode_OGL/
 * and from OpenGL® ES 2.0 Programming Guide
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <bcm_host.h>

#include "loadimage.h"
#include "gl-display.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[])
{
	UserData user_data;
	int width, height;
	unsigned int orientation;
	
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	
	unsigned char *image;
	CUBE_STATE_T state, *p_state = &state;
	
	//image = esLoadTGA("jan.tga", &width, &height);
	gettimeofday ( &t1 , &tz );
	
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	image = loadJPEG(argv[1], 1920, 1080, &width, &height, &orientation);
	if (image == NULL) {
		fprintf(stderr, "No such image\n");
		exit(1);
	}
	fprintf(stderr, "Image is %d x %d\n", width, height);
	gettimeofday(&t2, &tz);
	deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
	
	printf("Image loaded in %1.4f seconds\n", deltatime);
	
	
	bcm_host_init();
	esInitContext(p_state);
	
	init_ogl(p_state, width, height)
	
	p_state->user_data = &user_data;
	p_state->width = width;
	p_state->height = height;
	
	if(!Init(p_state, orientation))
		return 0;
	
	esRegisterDrawFunc(p_state, Draw);
	
	eglSwapBuffers(p_state->display, p_state->surface);
	esMainLoop(p_state);
	
	return 0; // not reached
}
