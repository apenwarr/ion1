/*
 * ion/frame.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_FRAME_H
#define INCLUDED_FRAME_H

#include "common.h"

INTROBJ(WFrame)

#include "window.h"
#include "client.h"
#include "input.h"
#include "screen.h"
#include "splitwin.h"

#define WFRAME_MAX_VERT 	0x0001
#define WFRAME_MAX_HORIZ	0x0002
#define WFRAME_SHADE		0x0004
#define WFRAME_NO_BAR		0x0008
#define WFRAME_MAX_BOTH		(WFRAME_MAX_VERT|WFRAME_MAX_HORIZ)

#define FRAME_SHORTCUT_W	23

#define FRAME_GEOM(FRAME) ((FRAME)->win.geom)
#define FRAME_X(FRAME) ((FRAME)->win.geom.x)
#define FRAME_Y(FRAME) ((FRAME)->win.geom.y)
#define FRAME_W(FRAME) ((FRAME)->win.geom.w)
#define FRAME_H(FRAME) ((FRAME)->win.geom.h)
#define FRAME_WIN(FRAME) ((FRAME)->win.win)
#define FRAME_CLIENT_WOFF(SCR) ((SCR)->grdata.client_off.w)
#define FRAME_CLIENT_HOFF(SCR) ((SCR)->grdata.client_off.h)

#define IS_ACTIVE_FRAME(FRAME) (wglobal.current_wswindow==(WWindow*)(FRAME))


DECLOBJ(WFrame){
	WWindow win;
	int flags;
	int frame_id;
	int tab_w;

	int client_count;
	int shortcut; /* valid values: '0'-'9','A'-'z' */
	WClient *current_client;
	WInput *current_input;
};


extern WFrame *create_frame(WScreen *scr,
							WRectangle area, int id, int flags);
extern void deinit_frame(WFrame *frame);
extern void destroy_frame(WFrame *frame);
extern void closedestroy(WFrame *frame);
extern void frame_remove_child(WFrame *frame, WThing *thing);

extern bool frame_attach_client(WFrame *frame, WClient *client, bool st);
extern void frame_detach_client(WFrame *frame, WClient *client);
extern void frame_attach_tagged(WFrame *frame);
extern void frame_add_clientwin(WFrame *frame, WClient *client,
								WClientWin *cwin);

extern void frame_switch_client(WFrame *frame, WClient *cwin);
extern void frame_switch_nth(WFrame *frame, int cwinnum);
extern void frame_switch_next(WFrame *frame);
extern void frame_switch_prev(WFrame *frame);

extern void focus_frame(WFrame *frame);
extern void activate_frame(WFrame *frame);
extern void deactivate_frame(WFrame *frame);


extern void set_frame_geom(WFrame *frame, WRectangle geom);
extern void set_frame_pos(WFrame *frame, int x, int y);
extern void set_frame_state(WFrame *frame, int stateflags);

extern void frame_toggle_sticky(WFrame *frame);
extern void frame_toggle_maximize(WFrame *frame, int mask);

extern void frame_bar_geom(const WFrame *frame, WRectangle *geom);
extern void frame_client_geom(const WFrame *frame, WRectangle *geom);

extern void frame_set_shortcut(WFrame *frame, int shortcut);

/* */


extern void frame_recalc_bar(WFrame *frame);

extern void split_vert(WFrame *frame);
extern void split_horiz(WFrame *frame);
extern void split_top(WWorkspace *ws, char *str);

extern WFrame *find_frame_of(Window win);

#endif /* INCLUDED_FRAME_H */
