//
//  GLWindow.m
//  Photoframe
//
//  Created by Martijn Vernooij on 05/02/2017.
//
//

#import "GLWindowController.h"

#import "error.h"

#include "gl-renderloop.h"
#include "gl-renderloop-member.h"
#include "gl-stage.h"

typedef struct
{
	Vector4 position;
	Colour colour;
} Vertex;

static void (*initFunction)();
static GLWindowController* mainWindowController;

static void gl_driver_clear();

@interface GLWindowController ()

- (void)createOpenGLView;

- (void)createDisplayLink;

- (void)createOpenGLResources;
- (void)loadShader;
- (GLuint)compileShaderOfType:(GLenum)type file:(NSString *)file;
- (void)linkProgram:(GLuint)program;
- (void)validateProgram:(GLuint)program;

- (void)loadBufferData;

- (void)renderForTime:(CVTimeStamp)time;

@end

CVReturn displayCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext);

CVReturn displayCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	GLWindowController *controller = (GLWindowController *)displayLinkContext;
	[controller renderForTime:*inOutputTime];
	return kCVReturnSuccess;
}

@implementation GLWindowController
{
	CVDisplayLinkRef displayLink;
	
	GLuint shaderProgram;
	GLuint vertexArrayObject;
	GLuint vertexBuffer;
	
	GLint positionUniform;
	GLint colourAttribute;
	GLint positionAttribute;
}

@synthesize view;
@synthesize window;

- (void)awakeFromNib
{
	mainWindowController = self;
	[self createOpenGLView];
	[self createOpenGLResources];
	initFunction();
	[self createDisplayLink];
}

- (void)createOpenGLView
{
	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
	{
		//NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy ,
		NSOpenGLPFAColorSize    , 24                           ,
		NSOpenGLPFAAlphaSize    , 8                            ,
		NSOpenGLPFADoubleBuffer ,
		NSOpenGLPFAAccelerated  ,
		NSOpenGLPFANoRecovery   ,
		0
	};
	NSOpenGLPixelFormat *pixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes] autorelease];
	[self setView:[[[NSOpenGLView alloc] initWithFrame:[[[self window] contentView] bounds] pixelFormat:pixelFormat] autorelease]];
	[[[self window] contentView] addSubview:[self view]];
	
	NSRect b = [[self view] bounds];
	gl_stage *stage = gl_stage_get_global_stage();
	stage->f->set_dimensions(stage, b.size.width, b.size.height);
}

- (void)createDisplayLink
{
	CGDirectDisplayID displayID = CGMainDisplayID();
	CVReturn error = CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink);
	
	if (kCVReturnSuccess == error)
	{
		CVDisplayLinkSetOutputCallback(displayLink, displayCallback, self);
		CVDisplayLinkStart(displayLink);
	}
	else
	{
		NSLog(@"Display Link created with error: %d", error);
		displayLink = NULL;
	}
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	
	gl_renderloop_member *renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.action = &gl_driver_clear;
	
	renderloop->f->append_child(renderloop, gl_renderloop_phase_clear, renderloop_member);
}

- (void)createOpenGLResources
{
	[[[self view] openGLContext] makeCurrentContext];
//	[self loadShader];
//	[self loadBufferData];
}

- (void)loadShader
{
	GLuint vertexShader;
	GLuint fragmentShader;
	
	vertexShader   = [self compileShaderOfType:GL_VERTEX_SHADER   file:[[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"vsh"]];
	fragmentShader = [self compileShaderOfType:GL_FRAGMENT_SHADER file:[[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"]];
	
	if (0 != vertexShader && 0 != fragmentShader)
	{
		shaderProgram = glCreateProgram();
		GetError();
		
		glAttachShader(shaderProgram, vertexShader  );
		GetError();
		glAttachShader(shaderProgram, fragmentShader);
		GetError();
		
		glBindFragDataLocation(shaderProgram, 0, "fragColour");
		
		[self linkProgram:shaderProgram];
		
		positionUniform = glGetUniformLocation(shaderProgram, "p");
		GetError();
		if (positionUniform < 0)
		{
			[NSException raise:kFailedToInitialiseGLException format:@"Shader did not contain the 'p' uniform."];
		}
		colourAttribute = glGetAttribLocation(shaderProgram, "colour");
		GetError();
		if (colourAttribute < 0)
		{
			[NSException raise:kFailedToInitialiseGLException format:@"Shader did not contain the 'colour' attribute."];
		}
		positionAttribute = glGetAttribLocation(shaderProgram, "position");
		GetError();
		if (positionAttribute < 0)
		{
			[NSException raise:kFailedToInitialiseGLException format:@"Shader did not contain the 'position' attribute."];
		}
		
		glDeleteShader(vertexShader  );
		GetError();
		glDeleteShader(fragmentShader);
		GetError();
	}
	else
	{
		[NSException raise:kFailedToInitialiseGLException format:@"Shader compilation failed."];
	}
}

- (GLuint)compileShaderOfType:(GLenum)type file:(NSString *)file
{
	GLuint shader;
	const GLchar *source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSASCIIStringEncoding error:nil] cStringUsingEncoding:NSASCIIStringEncoding];
	
	if (nil == source)
	{
		[NSException raise:kFailedToInitialiseGLException format:@"Failed to read shader file %@", file];
	}
	
	shader = glCreateShader(type);
	GetError();
	glShaderSource(shader, 1, &source, NULL);
	GetError();
	glCompileShader(shader);
	GetError();
	
#if defined(DEBUG)
	GLint logLength;
	
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	GetError();
	if (logLength > 0)
	{
		GLchar *log = malloc((size_t)logLength);
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		GetError();
		NSLog(@"Shader compilation failed with error:\n%s", log);
		free(log);
	}
#endif
	
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	GetError();
	if (0 == status)
	{
		glDeleteShader(shader);
		GetError();
		[NSException raise:kFailedToInitialiseGLException format:@"Shader compilation failed for file %@", file];
	}
	
	return shader;
}

- (void)linkProgram:(GLuint)program
{
	glLinkProgram(program);
	GetError();
	
#if defined(DEBUG)
	GLint logLength;
	
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	GetError();
	if (logLength > 0)
	{
		GLchar *log = malloc((size_t)logLength);
		glGetProgramInfoLog(program, logLength, &logLength, log);
		GetError();
		NSLog(@"Shader program linking failed with error:\n%s", log);
		free(log);
	}
#endif
	
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	GetError();
	if (0 == status)
	{
		[NSException raise:kFailedToInitialiseGLException format:@"Failed to link shader program"];
	}
}

- (void)validateProgram:(GLuint)program
{
	GLint logLength;
	
	glValidateProgram(program);
	GetError();
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	GetError();
	if (logLength > 0)
	{
		GLchar *log = malloc((size_t)logLength);
		glGetProgramInfoLog(program, logLength, &logLength, log);
		GetError();
		NSLog(@"Program validation produced errors:\n%s", log);
		free(log);
	}
	
	GLint status;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
	GetError();
	if (0 == status)
	{
		[NSException raise:kFailedToInitialiseGLException format:@"Failed to link shader program"];
	}
}

- (void)loadBufferData
{
	Vertex vertexData[4] = {
		{ .position = { .x=-0.5, .y=-0.5, .z=0.0, .w=1.0 }, .colour = { .r=1.0, .g=0.0, .b=0.0, .a=1.0 } },
		{ .position = { .x=-0.5, .y= 0.5, .z=0.0, .w=1.0 }, .colour = { .r=0.0, .g=1.0, .b=0.0, .a=1.0 } },
		{ .position = { .x= 0.5, .y= 0.5, .z=0.0, .w=1.0 }, .colour = { .r=0.0, .g=0.0, .b=1.0, .a=1.0 } },
		{ .position = { .x= 0.5, .y=-0.5, .z=0.0, .w=1.0 }, .colour = { .r=1.0, .g=1.0, .b=1.0, .a=1.0 } }
	};
	
	glGenVertexArrays(1, &vertexArrayObject);
	GetError();
	glBindVertexArray(vertexArrayObject);
	GetError();
	
	glGenBuffers(1, &vertexBuffer);
	GetError();
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	GetError();
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), vertexData, GL_STATIC_DRAW);
	GetError();
	
	glEnableVertexAttribArray((GLuint)positionAttribute);
	GetError();
	glEnableVertexAttribArray((GLuint)colourAttribute  );
	GetError();
	glVertexAttribPointer((GLuint)positionAttribute, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, position));
	GetError();
	glVertexAttribPointer((GLuint)colourAttribute  , 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, colour  ));
	GetError();
}

- (void)renderForTime:(CVTimeStamp)time
{
	[[[self view] openGLContext] makeCurrentContext];
	
	glClearColor(0.0, 0.0, 0.0, 1.0);
	GetError();
	glClear(GL_COLOR_BUFFER_BIT);
	GetError();
	
	gl_stage *stage = gl_stage_get_global_stage();
	glViewport(0, 0, stage->data.width, stage->data.height);

	GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebufferStatus != GL_FRAMEBUFFER_UNDEFINED) {
		gl_renderloop_loop_once();
	}
	
#if 0
	glUseProgram(shaderProgram);
	GetError();
	
	GLfloat timeValue = (GLfloat)(time.videoTime) / (GLfloat)(time.videoTimeScale);
	Vector2 p = { .x = 0.5f * sinf(timeValue), .y = 0.5f * cosf(timeValue) };
	glUniform2fv(positionUniform, 1, (const GLfloat *)&p);
	GetError();
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	GetError();
#endif
	[[[self view] openGLContext] flushBuffer];
}

- (void)dealloc
{
	glDeleteProgram(shaderProgram);
	GetError();
	glDeleteBuffers(1, &vertexBuffer);
	GetError();
	
	CVDisplayLinkStop(displayLink);
	CVDisplayLinkRelease(displayLink);
	[view release];
	
	[super dealloc];
}

- (GLuint)compileShaderOfType:(GLenum)type string:(const GLchar *)source
{
	GLuint shader;
	
	const GLchar *sources[2];
	sources[0] =
		"#version 120 \n"
		"#define precision \n"
		"#define mediump \n";
	sources[1] = source;
	
	shader = glCreateShader(type);
	GetError();
	glShaderSource(shader, 2, sources, NULL);
	GetError();
	glCompileShader(shader);
	GetError();
	
#if 1//defined(DEBUG)
	GLint logLength;
	
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	GetError();
	if (logLength > 0)
	{
		GLchar *log = malloc((size_t)logLength);
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		GetError();
		NSLog(@"Shader compilation failed with error:\n%s", log);
		free(log);
	}
#endif
	
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	GetError();
	if (0 == status)
	{
		glDeleteShader(shader);
		GetError();
		[NSException raise:kFailedToInitialiseGLException format:@"Shader compilation failed for source %s", source];
	}
	
	return shader;
}

- (GLuint)loadProgram:(const GLchar *)vertexStr fragment:(const GLchar *)fragmentStr
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint ret;
	
	vertexShader   = [self compileShaderOfType:GL_VERTEX_SHADER   string:vertexStr];
	fragmentShader = [self compileShaderOfType:GL_FRAGMENT_SHADER string:fragmentStr];
	
	ret = glCreateProgram();
	GetError();
	
	glAttachShader(ret, vertexShader  );
	GetError();
	glAttachShader(ret, fragmentShader);
	GetError();
	
	glLinkProgram ( ret );
	GetError();
	
	//[self validateProgram:ret];
	
	return ret;
}

@end

static void gl_driver_clear()
{
	gl_stage *stage = gl_stage_get_global_stage();
	
	// Set the viewport
	glViewport(0, 0, stage->data.width, stage->data.height);
	
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
}

GLuint gl_driver_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc )
{
	return [mainWindowController loadProgram:vertShaderSrc fragment:fragShaderSrc];
}

void startCocoa(int argc, const char *argv[], void (*initFunc)())
{
	initFunction = initFunc;
	NSApplicationMain(argc, argv);
}
