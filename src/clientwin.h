/*
 * ion/clientwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_CLIENTWIN_H
#define INCLUDED_CLIENTWIN_H

#include "common.h"

INTROBJ(WClientWin)

#include "thing.h"
#include "clientwin.h"
#include "frame.h"
#include "screen.h"


#define CWIN_P_WM_DELETE 		0x0001
#define CWIN_P_WM_TAKE_FOCUS 	0x0002
#define CWIN_KLUDGE_ACROBATIC	0x0004
#define CWIN_PROP_MAXSIZE 		0x0008
#define CWIN_PROP_ASPECT 		0x0010

#define MANAGE_RESPECT_POS		0x0001
#define MANAGE_INITIAL			0x0002

#define CLIENTWIN_HAS_CLIENT(CWIN) 	WTHING_HAS_PARENT(CWIN, WClient)
#define CLIENTWIN_CLIENT(CWIN) 		WTHING_PARENT(CWIN, WClient)


DECLOBJ(WClientWin){
	WThing thing;
	
	int flags;
	int state;
	Window win;
	WRectangle geom;

	long event_mask;
	int orig_bw;

	Window transient_for;
	Colormap cmap;

	XSizeHints size_hints;
	char *name;
};
	

extern WClientWin *manage_clientwin(Window win, int mflags);
extern void deinit_clientwin(WClientWin *cwin);
extern void unmap_clientwin(WClientWin *cwin);
extern void destroy_clientwin(WClientWin *cwin);
							  
extern void kill_clientwin(WClientWin *cwin);
extern void close_clientwin(WClientWin *cwin);
							  
extern void hide_clientwin(WClientWin *cwin);
extern void show_clientwin(WClientWin *cwin);
extern void focus_clientwin(WClientWin *cwin);
extern void iconify_clientwin(WClientWin *cwin);
							  
extern void set_clientwin_size(WClientWin *cwin, int w, int h);
extern void clientwin_reconf_at(WClientWin *cwin, int rootx, int rooty);

extern void set_clientwin_name(WClientWin *cwin, char *p);

extern WClientWin *find_clientwin(Window win);

extern void reconf_clientwin(WClientWin *cwin, int rootx, int rooty);
extern void sendconfig_clientwin(WClientWin *cwin);
extern void reparent_clientwin(WClientWin *cwin, Window win, int x, int y);
extern void fit_clientwin_frame(WClientWin *cwin, WFrame *frame);
extern void reparent_fit_clientwin_frame(WClientWin *cwin, WFrame *frame);
extern void reconf_clientwin_frame(WClientWin *cwin, WFrame *frame);
extern void reparent_fit_clientwin(WClientWin *cwin, Window win,
								   WRectangle geom);

extern void get_protocols(WClientWin *cwin);

extern void refit(WClientWin *cwin, int w, int h);

#endif /* INCLUDED_CLIENTWIN_H */
