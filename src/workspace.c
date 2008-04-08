/*
 * ion/workspace.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <X11/Xmd.h>

#include "common.h"
#include "screen.h"
#include "workspace.h"
#include "property.h"
#include "frame.h"
#include "focus.h"
#include "global.h"
#include "split.h"
#include "window.h"
#include "thingp.h"
#include "modules.h"
#include "complete.h"


static WThingFuntab workspace_funtab={
	deinit_workspace,
	workspace_remove_child
};

IMPLOBJ(WWorkspace, WThing, &workspace_funtab)


/*{{{ Utility */


bool visible_workspace(WWorkspace *ws)
{
	WScreen *scr=SCREEN_OF(ws);
	return (scr!=NULL && ws==scr->current_workspace);
}


bool on_visible_workspace(WThing *thing)
{
	WWorkspace *ws;
	
	if(WTHING_IS(thing, WScreen))
		return TRUE;
	
	ws=FIND_PARENT(thing, WWorkspace);
	
	return (ws!=NULL && visible_workspace(ws));
}


bool active_workspace(WWorkspace *ws)
{
	WScreen *scr=wglobal.current_screen;
	return (scr!=NULL && ws==scr->current_workspace);
}


bool on_active_workspace(WThing *thing)
{
	WWorkspace *ws;
	
	if(WTHING_IS(thing, WScreen))
		return (((WScreen*)thing)==wglobal.current_screen);
	
	ws=FIND_PARENT(thing, WWorkspace);
	
	return (ws!=NULL && active_workspace(ws));
}


WWorkspace *lookup_workspace(const char *name)
{
	WWorkspace *ws;
	WScreen *scr;
	
	FOR_ALL_SCREENS(scr){
		FOR_ALL_TYPED(scr, ws, WWorkspace){
			if(ws->name!=NULL && strcmp(ws->name, name)==0)
				return ws;
		}
	}
	
	return NULL;
}


int complete_workspace(char *nam, char ***cp_ret, char **beg)
{
	WWorkspace *ws;
	WScreen *scr;
	char *name;
	int n=0, l=strlen(nam);
	
	*cp_ret=NULL;

	FOR_ALL_SCREENS(scr){
		FOR_ALL_TYPED(scr, ws, WWorkspace){
			
			if((name=ws->name)==NULL)
				continue;
			
			if(l && strncmp(name, nam, l))
				continue;
			
			add_to_complist_copy(cp_ret, &n, name);
		}
	}
	
	return n;
}


/*}}}*/


/*{{{ Switch */


bool do_activate_workspace(WScreen *scr, WWorkspace *ws)
{
	WWorkspace *old;
	WWindow *obj;
	
	old=scr->current_workspace;
	
	if(old==ws)
		return FALSE;

	scr->current_workspace=ws;

	FOR_ALL_TYPED(ws, obj, WWindow){
		obj->flags&=~WWINDOW_UNMAPPABLE;
		do_map_window(obj);
	}

	if(old!=NULL){
		FOR_ALL_TYPED(old, obj, WWindow){
			do_unmap_window(obj);
			obj->flags|=WWINDOW_UNMAPPABLE;
		}
	}

	set_string_property(scr->root.win, wglobal.atom_workspace, ws->name);

	CALL_HOOKS_P(workspace_switch_hook, scr);

	return TRUE;
}


static void goto_ws(WScreen *scr, WWorkspace *ws)
{
	WWindow *obj;
	
	obj=find_current(ws);

	set_previous((WThing*)obj);
	
#ifdef CF_WARP_WS
	warp(obj);
#else
	if(wglobal.current_screen!=scr)
		warp(obj);
	else
		do_set_focus((WThing*)obj);
#endif
	wglobal.current_wswindow=obj;
}


static void do_switch_workspace(WScreen *scr, WWorkspace *ws)
{
	if(scr==NULL)
		return;
	
	if(!do_activate_workspace(scr, ws) && 
	   wglobal.current_screen==scr)
		return;
	
	goto_ws(scr, ws);
}


/* */


void switch_workspace(WWorkspace *ws)
{
	WScreen *scr;
	
	assert(ws!=NULL);
	
	scr=SCREEN_OF(ws);

	do_switch_workspace(scr, ws);
}


bool switch_workspace_nth(WScreen *scr, int num)
{
	WWorkspace *ws=NTH_THING(scr, num, WWorkspace);
	
	if(ws==NULL)
		return FALSE;

	do_switch_workspace(scr, ws);
	
	return TRUE;
}


static WScreen *nth_screen(int num)
{
	WScreen *scr;
	
	FOR_ALL_SCREENS(scr){
		if(num==0)
			return scr;
		num--;
	}
	return NULL;
}


bool switch_workspace_nth2(int scrnum, int num)
{
	WScreen *scr=nth_screen(scrnum);
	
	if(scr==NULL)
		return FALSE;
	
	if(num<0){
		if(scr->current_workspace!=NULL)
			goto_ws(scr, scr->current_workspace);
		return TRUE;
	}
	
	return switch_workspace_nth(scr, num);
}
	
	   
extern void switch_workspace_next(WScreen *scr)
{
	WWorkspace *ws=NEXT_THING(scr->current_workspace, WWorkspace);
	if(ws==NULL)
		ws=FIRST_THING(scr, WWorkspace);
	if(ws!=NULL)
		do_switch_workspace(scr, ws);
}


extern void switch_workspace_prev(WScreen *scr)
{
	WWorkspace *ws=PREV_THING(scr->current_workspace, WWorkspace);
	if(ws==NULL)
		ws=LAST_THING(scr, WWorkspace);
	if(ws!=NULL)
		do_switch_workspace(scr, ws);
}


extern void switch_workspace_next_n(WScreen *scr, int num)
{
	WWorkspace *ws=scr->current_workspace;
	while(num-->0 && ws!=NULL)
		ws=NEXT_THING(ws, WWorkspace);
	if(ws!=NULL)
		do_switch_workspace(scr, ws);
}


extern void switch_workspace_prev_n(WScreen *scr, int num)
{
	WWorkspace *ws=scr->current_workspace;
	while(num-->0 && ws!=NULL)
		ws=PREV_THING(ws, WWorkspace);
	if(ws!=NULL)
		do_switch_workspace(scr, ws);
}


bool switch_workspace_name(const char *str)
{
	WWorkspace *ws=lookup_workspace(str);
	
	if(ws==NULL)
		return FALSE;

	switch_workspace(ws);
	
	return TRUE;
}


/*}}}*/


/*{{{ Create/destroy */


static WFrame *create_initial_frame(WScreen *scr, WWorkspace *ws)
{
	WFrame *frame;
	
	frame=create_frame(scr, scr->root.geom, 0, 0);
	
	if(frame==NULL)
		return NULL;
	
	ws->splitree=(WObj*)frame;
	add_workspace_window(ws, (WWindow*)frame);

	return frame;
}


static char *unique_ws(const char *nameo)
{
	char *tmp=NULL, *name=NULL;
	int i=1;
	
	if(nameo==NULL)
		return NULL;
	
	name=scopy(nameo);
	
	if(name==NULL)
		return NULL;
	
	stripws(name);
	
	if(lookup_workspace(name)==NULL)
		return name;
	
	while(1){
		libtu_asprintf(&tmp, "%s<%d>", name, i);
		if(tmp==NULL)
			break;
		if(lookup_workspace(tmp)==NULL)
			break;
		i++;
		free(tmp);
	}
	free(name);
	return tmp;
}


static bool init_workspace(WWorkspace *ws,
						   WScreen *scr, const char *name, bool ci)
{
	ws->name=unique_ws(name);
	
	if(ws->name==NULL){
		warn_err();
		return FALSE;
	}

	ws->thing.flags|=WTHING_DESTREMPTY;
	ws->splitree=NULL;
	
	if(ci){
		if(!create_initial_frame(scr, ws)){
			free(ws->name);
			return FALSE;
		}
	}
	
	link_thing((WThing*)scr, (WThing*)ws);
	scr->workspace_count++;
	
	return TRUE;
}


WWorkspace *create_workspace(WScreen *scr, const char *name, bool ci)
{
	CREATETHING_IMPL(WWorkspace, workspace, scr, (p, scr, name, ci));
}


void rename_workspace(WWorkspace *ws, const char *name)
{
	char *newname;
	
	if(ws->name!=NULL) {
		free(ws->name);
		ws->name=NULL;
	}
	
	newname=unique_ws(name);
	if(newname!=NULL)
		ws->name=newname;
}


void deinit_workspace(WWorkspace *ws)
{
	if(ws->name!=NULL)
		free(ws->name);
}


/*}}}*/


/*{{{ Init */


void init_workspaces(WScreen *scr)
{
	WWorkspace *ws=NULL;
	char *wsname=NULL;

	if(scr->workspace_count==0){
		ws=create_workspace(scr, "main", TRUE);
	}else{
		wsname=get_string_property(scr->root.win, wglobal.atom_workspace, NULL);
		if(wsname!=NULL){
			ws=lookup_workspace(wsname);
			free(wsname);
		}
		if(ws==NULL)
			ws=FIRST_THING(scr, WWorkspace);
	}
	
	assert(ws!=NULL);

	switch_workspace(ws);
}


/*}}}*/


/*{{{ Add/remove */


void workspace_remove_child(WWorkspace *ws, WThing *thing)
{
	if(!WTHING_IS(thing, WWindow))
		return;
	
	workspace_remove_window(ws, (WWindow*)thing);
}



void add_workspace_window(WWorkspace *ws, WWindow *nw)
{
	link_thing((WThing*)ws, (WThing*)nw);
	
	if(visible_workspace(ws))
		map_window(nw);
	else
		nw->flags|=WWINDOW_UNMAPPABLE;
}


/*}}}*/
