/*
 * ion/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_SCREEN_H
#define INCLUDED_SCREEN_H

#include "common.h"

INTROBJ(WScreen)

#include "workspace.h"
#include "grdata.h"
#include "window.h"

#define SCREEN_OF(X) screen_of((WThing*)X)
#define ROOT_OF(X) root_of((WThing*)X)
#define GRDATA_OF(X) grdata_of((WThing*)X)
#define FOR_ALL_SCREENS(SCR)                     \
	for(SCR=wglobal.screens;                     \
		SCR!=NULL;                               \
		SCR=(WScreen*)(((WThing*)(SCR))->t_next))


DECLOBJ(WScreen){
	WWindow root;
	
	int xscr;
	
	Colormap default_cmap;

	int workspace_count;
	WWorkspace *current_workspace;
	bool confsel;
	
	int w_unit, h_unit;
	
	WGRData grdata;
};


extern Window create_simple_window_bg(const WWindow *p,
									  int x, int y, int w, int h,
									  Pixel bg);
extern Window create_simple_window(const WWindow *p,
								   int x, int y, int w, int h);

extern WScreen *preinit_screen(int xscr);
extern void postinit_screen(WScreen *scr);
extern void deinit_screen(WScreen *scr);

extern WScreen *screen_of(const WThing *thing);
extern WGRData *grdata_of(const WThing *thing);
extern Window root_of(const WThing *thing);
extern bool same_screen(const WThing *t1, const WThing *t2);

#endif /* INCLUDED_SCREEN_H */
