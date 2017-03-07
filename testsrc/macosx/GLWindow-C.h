//
//  GLWindow-C.h
//  Photoframe
//
//  Created by Martijn Vernooij on 05/02/2017.
//
//

#ifndef GLWindow_C_h
#define GLWindow_C_h

void startCocoa(int argc, const char *argv[], void (*initFunc)());
GLuint gl_driver_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc );

#endif /* GLWindow_C_h */
