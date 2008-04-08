/*
 * ion/function.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include "common.h"
#include "functionp.h"
#include "exec.h"
#include "frame.h"
#include "clientwin.h"
#include "client.h"
#include "focus.h"
#include "workspace.h"
#include "pointer.h"
#include "wedln.h"
#include "edln.h"
#include "query.h"
#include "resize.h"
#include "modules.h"
#include "complete.h"
#include "key.h"
#include "shortcut.h"


/*{{{ Definitions and stuff */


extern WDragHandler p_tabdrag_handler;
extern WDragHandler p_resize_handler;

extern void commandmode_enter();
extern void commandmode_leave();


/*}}}*/


/*{{{ Wrappers/helpers */


static void wrap_switch_client(WClient *client)
{
	frame_switch_client(CLIENT_FRAME(client), client);
}


/*}}}*/


/*{{{ Function table */

static WFunction funcs[]={
	/* frame */
	FN(l,	generic, WFrame,	"switch_nth",		frame_switch_nth),
	FN_VOID(generic, WFrame, 	"switch_next",		frame_switch_next),
	FN_VOID(generic, WFrame, 	"switch_prev",		frame_switch_prev),
	FN_VOID(generic, WFrame,	"split_vert",		split_vert),
	FN_VOID(generic, WFrame,	"split_horiz",		split_horiz),
	FN_VOID(generic, WFrame,	"destroy_frame",	destroy_frame),
	FN_VOID(generic, WFrame,	"closedestroy",		closedestroy),
	FN_VOID(generic, WFrame,	"attach_tagged",	frame_attach_tagged),
	FN_VOID(generic, WFrame,	"set_shortcut",		set_shortcut),
	FN_VOID(generic, WFrame,	"goto_shortcut",	goto_shortcut),
	
	FN(ss, 	generic, WFrame,	"query_runfile",	query_runfile),
	FN(ss, 	generic, WFrame,	"query_runwith",	query_runwith),
	FN(ss,	generic, WFrame,	"query_yesno",		query_yesno),
	FN_VOID(generic, WFrame,	"query_function",	query_function),
	FN_VOID(generic, WFrame,	"query_exec",		query_exec),
	FN_VOID(generic, WFrame,	"query_attachclient", query_attachclient),
	FN_VOID(generic, WFrame,	"query_gotoclient", query_gotoclient),
	FN_VOID(generic, WFrame,	"query_workspace",	query_workspace),
	FN_VOID(generic, WFrame,	"query_workspace_with",	query_workspace_with),
	FN_VOID(generic, WFrame,	"query_renameworkspace", query_renameworkspace),
	
	FN_VOID(generic, WWindow,	"goto_above",		goto_above),
	FN_VOID(generic, WWindow,	"goto_below",		goto_below),
	FN_VOID(generic, WWindow,	"goto_right",		goto_right),
	FN_VOID(generic, WWindow,	"goto_left",		goto_left),
	FN_VOID(generic, WWindow,	"resize_vert",		resize_vert),
	FN_VOID(generic, WWindow,	"resize_horiz",		resize_horiz),
	FN_VOID(generic, WWindow,	"maximize_vert", 	maximize_vert),
	FN_VOID(generic, WWindow,	"maximize_horiz", 	maximize_horiz),

	FN(l, 	generic, WFrame,	"set_width",		set_width),
	FN(l, 	generic, WFrame,	"set_height",		set_height),
	FN(d, 	generic, WFrame,	"set_widthq",		set_widthq),
	FN(d, 	generic, WFrame,	"set_heightq",		set_heightq),

	FN(s,	generic, WWorkspace,"split_top",   		split_top),
	
	/* client */
	FN_VOID(cclient, WClient,	"close",			close_client),
	FN_VOID(cclient, WClient,	"close_main",		close_client_main),
	FN_VOID(cclient, WClient,	"kill",				kill_client),
	FN_VOID(cclient, WClient,	"toggle_tagged",	client_toggle_tagged),
	FN_VOID(cclient, WClient,	"quote_next",		quote_next),
	FN_VOID(generic, WClient,	"switch_tab",		wrap_switch_client),
	
	/* mouse move/resize and tab drag */
	FN_VOID(drag, WFrame,		"p_resize",			&p_resize_handler),
	FN_VOID(drag, WClient,		"p_tabdrag", 		&p_tabdrag_handler),
	
	/* screen */
	FN_SCREEN(l,				"switch_ws_nth",	switch_workspace_nth),
	FN_SCREEN_VOID(				"switch_ws_next",	switch_workspace_next),     
	FN_SCREEN_VOID(				"switch_ws_prev",	switch_workspace_prev),     
 	FN_SCREEN(l,				"switch_ws_next_n",	switch_workspace_next_n),
 	FN_SCREEN(l,				"switch_ws_prev_n",	switch_workspace_prev_n),
	FN_SCREEN(s,				"exec",				wm_exec),
	
	/* global */
	FN_GLOBAL_VOID(				"goto_previous",	goto_previous),
	FN_GLOBAL_VOID(				"restart",			wm_restart),
	FN_GLOBAL_VOID(				"exit",				wm_exit),
	FN_GLOBAL(s,				"restart_other",	wm_restart_other),
	FN_GLOBAL(s,				"goto_client_name",  goto_client_name),
	FN_GLOBAL(s,				"switch_ws_name",	switch_workspace_name),
	FN_GLOBAL(ll,				"switch_ws_nth2",	switch_workspace_nth2),
	FN_GLOBAL_VOID(				"clear_tags",		clear_tags),

	FN(s, generic, WThing,		"command_sequence", command_sequence),
	
	FN_VOID(generic, WFrame,	"enter_command_mode", commandmode_enter),
	FN_VOID(generic, WFrame,	"leave_command_mode", commandmode_leave),

	{NULL, NULL, NULL, NULL, NULL}
};


static WFunction moveres_funcs[]={
	FN_VOID(generic, WWindow,	"end_resize",		end_resize),
	FN_VOID(generic, WWindow,	"cancel_resize",	cancel_resize),
	FN_VOID(generic, WWindow,	"grow",				grow),
	FN_VOID(generic, WWindow,	"shrink",			shrink),
	{NULL, NULL, NULL, NULL, NULL}
};


static WFunction input_funcs[]={
	FN_VOID(edln, WEdln,		"back",				edln_back),
	FN_VOID(edln, WEdln,		"forward",			edln_forward),
	FN_VOID(edln, WEdln,		"bol",				edln_bol),
	FN_VOID(edln, WEdln,		"eol",				edln_eol),
	FN_VOID(edln, WEdln,		"skip_word",		edln_skip_word),
	FN_VOID(edln, WEdln,		"bskip_word",		edln_bskip_word),
	FN_VOID(edln, WEdln,		"delete",			edln_delete),
	FN_VOID(edln, WEdln,		"backspace",		edln_backspace),
	FN_VOID(edln, WEdln,		"kill_to_eol",		edln_kill_to_eol),
	FN_VOID(edln, WEdln,		"kill_to_bol",		edln_kill_to_bol),
	FN_VOID(edln, WEdln,		"kill_line",		edln_kill_line),
	FN_VOID(edln, WEdln,		"kill_word",		edln_kill_word),
	FN_VOID(edln, WEdln,		"bkill_word",		edln_bkill_word),
	FN_VOID(edln, WEdln,		"set_mark",			edln_set_mark),
	FN_VOID(edln, WEdln,		"cut",				edln_cut),
	FN_VOID(edln, WEdln,		"copy",				edln_copy),
	FN_VOID(edln, WEdln,		"complete",			edln_complete),
	FN_VOID(edln, WEdln,		"history_next",		edln_history_next),
	FN_VOID(edln, WEdln,		"history_prev",		edln_history_prev),
	FN_VOID(generic, WEdln,		"paste",			wedln_paste),
	FN_VOID(generic, WEdln,		"finish",			wedln_finish),	
	FN_VOID(generic, WInput,	"cancel",			input_cancel),	
	FN_VOID(generic, WInput,	"scrollup",			input_scrollup),
	FN_VOID(generic, WInput,	"scrolldown",		input_scrolldown),
	{NULL, NULL, NULL, NULL, NULL}
};


static WFunction *funtabs[]={
	funcs,
	moveres_funcs,
	input_funcs
};


/*}}}*/


/*{{{ lookup_func */


static WFunction *look_in_funtab(WFunction *func, const char *name)
{
	while(func->callhnd!=NULL){
		if(strcmp(func->name, name)==0)
			return func;
		func++;
	}
	return NULL;
}


WFunction *lookup_func(const char *name, int funtab)
{
	WFunction *func=funtabs[funtab];
	WFunction *ft;
	
	func=look_in_funtab(func, name);
	
	if(func!=NULL || funtab!=FUNTAB_MAIN)
		return func;
	
	for(ft=(WFunction*)miter_begin("funtab");
		ft!=NULL;
		ft=(WFunction*)miter_next("funtab")){
		
		func=look_in_funtab(ft, name);
		
		if(func!=NULL)
			break;
	}
	
	miter_end();
	
	return func;
}


int complete_mainfunc(char *nam, char ***cp_ret, char **beg)
{
	char *name;
	char **cp;
	int n=0, l=strlen(nam);
	WFunction *func=funtabs[FUNTAB_MAIN];
	bool m=FALSE;
	
	*cp_ret=NULL;
	
again:
	
	for(; func->callhnd!=NULL; func++){
		
		if((name=func->name)==NULL)
			continue;
		
		if(l && strncmp(name, nam, l))
			continue;
		
		add_to_complist_copy(cp_ret, &n, name);
	}
	
	if(!m){
		func=(WFunction*)miter_begin("funtab");
		m=TRUE;
	}else{
		func=(WFunction*)miter_next("funtab");
	}
	
	if(func!=NULL)
		goto again;
	
	miter_end();
	
	return n;
}

/*}}}*/


/*{{{ Call handlers */


void callhnd_direct(WThing *thing, WFunction *func,
					int n, const Token *args)
{
	typedef void Func(WThing*, int, const Token*);
	((Func*)func->fn)(thing, n, args);
}


void callhnd_generic_void(WThing *thing, WFunction *func,
						  int n, const Token *args)
{
	typedef void Func(WThing*);
	thing=find_parent(thing, func->objdescr);

	if(thing!=NULL)
		((Func*)func->fn)(thing);
}


void callhnd_generic_l(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, int);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_LONG_VAL(args));
}


void callhnd_generic_d(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, double);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_DOUBLE_VAL(args));
}


void callhnd_generic_s(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(WThing*, char*);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_STRING_VAL(args));
}


void callhnd_generic_ss(WThing *thing, WFunction *func,
						int n, const Token *args)
{
	typedef void Func(WThing*, char*, char*);
	thing=find_parent(thing, func->objdescr);
	if(thing!=NULL)
		((Func*)func->fn)(thing, TOK_STRING_VAL(args), TOK_STRING_VAL(args+1));
}


void callhnd_global_void(WThing *thing, WFunction *func,
						 int n, const Token *args)
{
	typedef void Func();
	((Func*)func->fn)();
}


void callhnd_global_l(WThing *thing, WFunction *func,
					  int n, const Token *args)
{
	typedef void Func(int);
	((Func*)func->fn)(TOK_LONG_VAL(args));
}


void callhnd_global_ll(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	typedef void Func(int, int);
	((Func*)func->fn)(TOK_LONG_VAL(args), TOK_LONG_VAL(args+1));
}


void callhnd_global_s(WThing *thing, WFunction *func,
					  int n, const Token *args)
{
	typedef void Func(char*);
	((Func*)func->fn)(TOK_STRING_VAL(args));
}


void callhnd_edln_void(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
	WEdln *wedln;
	typedef void Func(Edln*);
	
	if(!WTHING_IS(thing, WEdln))
		return;
	
	wedln=(WEdln*)thing;
	
	((Func*)func->fn)(&(wedln->edln));
}


static WClient *get_cclient(WThing *thing)
{
	WFrame *frame;
	
	if(WTHING_IS(thing, WClient))
		return (WClient*)thing;
	
	frame=FIND_PARENT(thing, WFrame);
	
	if(frame==NULL)
		return NULL;
	
	return frame->current_client;
	
}


void callhnd_cclient_void(WThing *thing, WFunction *func,
						  int n, const Token *args)
{
	typedef void Func(WClient*);
	WClient *client=get_cclient(thing);
	
	if(client!=NULL)
		((Func*)func->fn)(client);
}


/*}}}*/


