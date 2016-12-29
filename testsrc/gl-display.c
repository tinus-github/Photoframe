//
//  gl-display.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/03/16.
//
// This is actually a GL/bcm (for the Raspberry Pi) hybrid

#include "gl-display.h"

#include <bcm_host.h>

#include <assert.h>
#include <sys/time.h>

#include "../lib/linmath/linmath.h"

#include "gl-texture.h"
#include "gl-shape.h"
#include "gl-tile.h"
#include "gl-stage.h"
#include "gl-container.h"
#include "gl-container-2d.h"
#include "gl-tiled-image.h"
#include "gl-renderloop-member.h"
#include "gl-renderloop.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

static GL_STATE_T *global_gl_state = 0;
static void set_global_gl_state(GL_STATE_T *state);

void gl_display_setup();


