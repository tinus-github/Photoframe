//
//  driver.h
//  Photoframe
//
//  Created by Martijn Vernooij on 05/02/2017.
//
//

#ifndef driver_h
#define driver_h

#ifdef __APPLE__

#include "macosx/GLWindow-C.h"

#else

#include "egl-driver.h"

#endif

#include "common-driver.h"

#endif /* driver_h */
