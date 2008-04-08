/*
 * ion/draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_DRAW_H
#define INCLUDED_DRAW_H

#include "common.h"
#include "frame.h"
#include "screen.h"
#include "grdata.h"

extern void draw_frame(const WFrame *frame, bool complete);
extern void draw_frame_bar(const WFrame *frame, bool complete);

extern void draw_rubberband(WScreen *scr, WRectangle rect,
							bool vertical);
extern void set_moveres_pos(WScreen *scr, int x, int y);
extern void set_moveres_size(WScreen *scr, int w, int h);

extern void draw_tabdrag(const WClient *client);

extern bool alloc_color(WScreen *scr, const char *name, Pixel *cret);
extern void setup_color_group(WScreen *scr, WColorGroup *cg,
							  Pixel hl, Pixel sh, Pixel bg, Pixel fg);

extern void preinit_graphics(WScreen *scr);
extern void postinit_graphics(WScreen *scr);

#endif /* INCLUDED_DRAW_H */
