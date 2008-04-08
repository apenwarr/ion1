/*
 * ion/common.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_COMMON_H
#define INCLUDED_COMMON_H

#include <libtu/types.h>
#include <libtu/output.h>
#include <libtu/misc.h>
#include <libtu/dlist.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct WRectangle_struct{
	int x, y;
	int w, h;
} WRectangle;

typedef ulong Pixel;

#include "obj.h"
#include "config.h"

#endif /* INCLUDED_COMMON_H */
