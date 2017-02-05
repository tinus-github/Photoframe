//
//  common-driver.h
//  Photoframe
//
//  Created by Martijn Vernooij on 05/02/2017.
//
//

#ifndef common_driver_h
#define common_driver_h

#include <stdio.h>
#include "gl-includes.h"

void gl_objects_setup();
GLuint driver_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc );

#endif /* common_driver_h */
