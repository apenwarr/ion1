/*
 * ion/splitwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_SPLITWIN_H
#define INCLUDED_SPLITWIN_H

#include "common.h"
#include "split.h"
#include "screen.h"

typedef WWindow *WSplitCreate(WScreen *scr, WRectangle geom);
extern WWindow *split_window(WWindow *wwin, int dir, int minsize,
							 WSplitCreate *fn);
extern WWindow *split_toplevel(WWorkspace *ws, int dir, int primn,
							   int minsize, WSplitCreate *fn);

#endif /* INCLUDED_SPLITWIN_H */
