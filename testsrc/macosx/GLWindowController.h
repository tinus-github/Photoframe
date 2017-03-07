//
//  GLWindow.h
//  Photoframe
//
//  Created by Martijn Vernooij on 05/02/2017.
//
// From: https://github.com/beelsebob/Cocoa-GL-Tutorial

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>
#import <CoreVideo/CVDisplayLink.h>

#define kFailedToInitialiseGLException @"Failed to initialise OpenGL"

typedef struct
{
	GLfloat x,y;
} Vector2;

typedef struct
{
	GLfloat x,y,z,w;
} Vector4;

typedef struct
{
	GLfloat r,g,b,a;
} Colour;

@interface GLWindowController : NSObject

@property (nonatomic, readwrite, retain) IBOutlet NSWindow *window;
@property (nonatomic, readwrite, retain) IBOutlet NSOpenGLView *view;

@end
