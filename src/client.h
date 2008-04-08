/*
 * ion/client.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_CLIENT_H
#define INCLUDED_CLIENT_H

#include "common.h"

INTROBJ(WClient)

#include "thing.h"
#include "clientwin.h"
#include "screen.h"


#define CLIENT_DRAG 			0x0001
#define CLIENT_TAGGED			0x0002
#define CLIENT_URGENT			0x0004
#define CLIENT_WILD				0x0008

#define CLIENT_HAS_FRAME(CLIENT) 	WTHING_HAS_PARENT(CLIENT, WFrame)
#define CLIENT_FRAME(CLIENT) 		WTHING_PARENT(CLIENT, WFrame)


DECLOBJ(WClient){
	WThing thing;
	int flags;
	
	char *label;
	int shortcut; /* valid values: '0'-'9','A'-'z' */
	int label_inst;
	WClient *label_next, *label_prev;
	
	WClient *g_client_next, *g_client_prev;
};


extern WClient *create_client(WScreen *scr);
extern void deinit_client(WClient *client);

extern void client_add_clientwin(WClient *client, WClientWin *cwin);

extern void hide_client(WClient *client);
extern void show_client(WClient *client);
extern void focus_client(WClient *client);
extern void close_client(WClient *client);
extern void close_client_main(WClient *client);
extern void kill_client(WClient *client);

extern void reparent_client(WClient *client, Window win, int x, int y);
extern void fit_client_frame(WClient *client, WFrame *frame);
extern void reparent_fit_client_frame(WClient *client, WFrame *frame);
extern void reconf_client_frame(WClient *client, WFrame *frame);
extern void set_client_frame_id(WClient *client, int id);

extern void client_make_label(WClient *client, int maxw);
extern char* client_full_label(WClient *client);
extern void client_use_label(WClient *client);
extern void client_unuse_label(WClient *client);

extern void client_toggle_tagged(WClient *client);
extern void clear_tags();

extern bool goto_window(WWindow *window);
extern void goto_client(WClient *client);
extern void goto_previous();
extern bool goto_client_name(const char *cname);

extern WClient* lookup_client(const char *name);
extern int complete_client(char *nam, char ***cp_ret, char **beg);

#endif /* INCLUDED_CLIENT_H */
