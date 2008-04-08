/*
 * ion/global.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_GLOBAL_H
#define INCLUDED_GLOBAL_H

#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "common.h"
#include "screen.h"
#include "client.h"
#include "window.h"
#include "binding.h"


enum{
	INPUT_NORMAL,
	INPUT_GRAB,
	INPUT_SUBMAPGRAB,
	INPUT_WAITRELEASE
};

enum{
	OPMODE_INIT,
	OPMODE_NORMAL,
	OPMODE_DEINIT
};

typedef void xInputEventHandler(XEvent *ev);
typedef struct input_event_handler{
	xInputEventHandler *keyboard;
	xInputEventHandler *pointer;
}InputHandler;

typedef struct input_handler_context{
	struct input_handler_context *prev, *next;
	InputHandler *oldhandler;
}InputHandlerContext;

INTRSTRUCT(WGlobal)

DECLSTRUCT(WGlobal){
	int argc;
	char **argv;
	
	Display *dpy;
	const char *display;
	int conn;
	
	char *client_id;
	char *state_file;
	
	XContext win_context;
	Atom atom_wm_state;
	Atom atom_wm_change_state;
	Atom atom_wm_protocols;
	Atom atom_wm_delete;
	Atom atom_wm_take_focus;
	Atom atom_wm_colormaps;
	Atom atom_frame_id;
	Atom atom_workspace;
	Atom atom_selection;
#ifndef CF_NO_MWM_HINTS	
	Atom atom_mwm_hints;
#endif

	WScreen *screens;

	WScreen *current_screen;
	WWindow *current_wswindow;
	WThing *previous;
	WThing *grab_holder;
	WThing *focus_next;
	int input_mode;
	int opmode;
	int previous_protect;

	Time resize_delay;
	Time dblclick_delay;
	bool opaque_resize;
	
	WThing *shortcuts[127]; /* WFrame's and WClients apply for a shortcut */
	WClient *client_list;
	
	WBindmap main_bindmap, tab_bindmap, moveres_bindmap, input_bindmap;

	InputHandler *input_handler;
};

extern WGlobal wglobal;

extern void deinit();


#endif /* INCLUDED_GLOBAL_H */
