/*
 * ion/event.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_EVENT_H
#define INCLUDED_EVENT_H

#include "global.h" /* for InputHandler and InputHandlerContext */
#include "common.h"
#include "thing.h"


#define GRAB_POINTER_MASK (ButtonPressMask|ButtonReleaseMask|\
						   ButtonMotionMask)

#define ROOT_MASK	(SubstructureRedirectMask|          \
					 ColormapChangeMask|                \
					 ButtonPressMask|ButtonReleaseMask| \
					 PropertyChangeMask|KeyPressMask|   \
					 FocusChangeMask|EnterWindowMask)

#define FRAME_MASK	(FocusChangeMask|          \
					 ButtonPressMask|          \
					 ButtonReleaseMask|        \
					 KeyPressMask|             \
					 EnterWindowMask|          \
					 ExposureMask|             \
					 SubstructureRedirectMask)

#define CLIENT_MASK (ColormapChangeMask| \
					 PropertyChangeMask|FocusChangeMask| \
					 StructureNotifyMask)


extern void mainloop();
extern void get_event(XEvent *ev);
extern void get_event_mask(XEvent *ev, long mask);
extern void handle_event(XEvent *ev);

extern void do_grab_kb_ptr(Window win, WThing *thing, long eventmask);
extern void grab_kb_ptr(WThing *thing);
extern void ungrab_kb_ptr();

extern void default_pointer_handler(XEvent *ev);
extern void default_keyboard_handler(XEvent *ev);

extern void kill_focusenter_events();
extern void skip_focusenter();

extern void set_input_handler(InputHandler *handler, InputHandlerContext *context);
extern void restore_input_handler(InputHandlerContext *context);

#endif /* INCLUDED_EVENT_H */
