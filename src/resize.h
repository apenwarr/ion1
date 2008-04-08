/*
 * ion/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_RESIZE_H
#define INCLUDED_RESIZE_H

#include "common.h"
#include "window.h"
#include "frame.h"
#include "split.h"

extern void begin_resize(WWindow *wwin, int dir, int primn);
extern void resize(WWindow *wwin, int delta, int primn, bool settmp);
extern void resize_vert(WWindow *wwin);
extern void resize_horiz(WWindow *wwin);
extern void grow(WWindow *obj);
extern void shrink(WWindow *obj);
extern void end_resize(WWindow *wwin);
extern void cancel_resize(WWindow *wwin);
extern void maximize_vert(WWindow *wwin);
extern void maximize_horiz(WWindow *wwin);
extern void set_height(WWindow *wwin, uint h);
extern void set_width(WWindow *wwin, uint w);
extern void set_heightq(WWindow *wwin, double q);
extern void set_widthq(WWindow *wwin, double q);

#endif /* INCLUDED_RESIZE_H */
