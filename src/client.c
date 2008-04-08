/*
 * ion/client.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "client.h"
#include "sizehint.h"
#include "frame.h"
#include "clientwin.h"
#include "global.h"
#include "font.h"
#include "thingp.h"
#include "property.h"
#include "focus.h"
#include "draw.h"
#include "complete.h"


static void client_remove_child(WClient *client, WThing *thing);

static WThingFuntab client_funtab={
	deinit_client,
	client_remove_child
};

IMPLOBJ(WClient, WThing, &client_funtab)


/*{{{ Create/destroy */


static bool init_client(WClient *client)
{
	LINK_ITEM(wglobal.client_list, client, g_client_next, g_client_prev);
	client->label=NULL;
	client->thing.flags|=WTHING_DESTREMPTY;
	client->label_next=client;
	client->label_prev=client;
	client->label_inst=0;
	return TRUE;
}


WClient *create_client(WScreen *scr)
{
	CREATETHING_IMPL(WClient, client, scr, (p));
}


void deinit_client(WClient *client)
{
	client_unuse_label(client);
	UNLINK_ITEM(wglobal.client_list, client, g_client_next, g_client_prev);
	if(client->label!=NULL)
		free(client->label);
}


/*}}}*/


/*{{{ Add/remove clientwin */


void client_add_clientwin(WClient *client, WClientWin *cwin)
{
	link_thing((WThing*)client, (WThing*)cwin);
	
	if(FIRST_THING(client, WClientWin)==cwin)
		client_use_label(client);
	
	if(CLIENT_HAS_FRAME(client))
		frame_add_clientwin(CLIENT_FRAME(client), client, cwin);
}


static void client_remove_child(WClient *client, WThing *thing)
{
	WFrame *frame;
	WClientWin *n;
	
	if(!WTHING_IS(thing, WClientWin))
		return;
	
	frame=FIND_PARENT(client, WFrame);
	
	if(frame==NULL || !IS_ACTIVE_FRAME(frame))
		return;
	
	if(frame->current_client!=client)
		return;
	
	unlink_thing(thing);
	n=LAST_THING(client, WClientWin);
	if(n!=NULL)
		set_focus((WThing*)n);
}


/*}}}*/


/*{{{ Clientwin wrappers */


void hide_client(WClient *client)
{
	WClientWin *cwin;

	FOR_ALL_TYPED(client, cwin, WClientWin){
		hide_clientwin(cwin);
	}
}


void show_client(WClient *client)
{
	WClientWin *cwin, *first;
	
	FOR_ALL_TYPED(client, cwin, WClientWin){
		show_clientwin(cwin);
	}
}


void focus_client(WClient *client)
{
	WClientWin *cwin=LAST_THING(client, WClientWin);
	
	if(cwin!=NULL)
		focus_clientwin(cwin);
}


void close_client(WClient *client)
{
	WClientWin *cwin=LAST_THING(client, WClientWin);
	
	if(cwin!=NULL)
		close_clientwin(cwin);
}


void close_client_main(WClient *client)
{
	WClientWin *cwin=FIRST_THING(client, WClientWin);
	
	if(cwin!=NULL)
		close_clientwin(cwin);
}


void kill_client(WClient *client)
{
	WClientWin *cwin=FIRST_THING(client, WClientWin);
	
	if(cwin!=NULL)
		kill_clientwin(cwin);
}


/*}}}*/


/*{{{ Reparent/fit/reconf */


void reparent_client(WClient *client, Window win, int x, int y)
{
	WClientWin *cwin;
	
	FOR_ALL_TYPED(client, cwin, WClientWin){
		reparent_clientwin(cwin, win, x, y);
	}
}


void fit_client_frame(WClient *client, WFrame *frame)
{
	WClientWin *cwin;
	
	FOR_ALL_TYPED(client, cwin, WClientWin){
		fit_clientwin_frame(cwin, frame);
	}
}


void reparent_fit_client_frame(WClient *client, WFrame *frame)
{
	WClientWin *cwin;
	
	FOR_ALL_TYPED(client, cwin, WClientWin){
		reparent_fit_clientwin_frame(cwin, frame);
	}
}


void reconf_client_frame(WClient *client, WFrame *frame)
{
	WClientWin *cwin;
	
	FOR_ALL_TYPED(client, cwin, WClientWin){
		reconf_clientwin_frame(cwin, frame);
	}
}


/*}}}*/


/*{{{ Labels */


static char *untitled_label="<untitled>";
static char *empty_label="<empty>";


static const char *client_beg_label(WClient *client)
{
	WClientWin *cwin=FIRST_THING(client, WClientWin);

	if(cwin==NULL){
		return empty_label;
	}else{
		if(cwin->name!=NULL)
			return cwin->name;
		/*else if(cwin->icon_name!=NULL)
			return cwin->icon_name;*/
	}
	return untitled_label;
}


#define CLIENTNUM_TMPL "<%d>"

void client_make_label(WClient *client, int maxw)
{
	const char *str=client_beg_label(client);
	char tmp[16];
	WGRData *grdata;
	
	if(client->label_inst!=0)
		sprintf(tmp, CLIENTNUM_TMPL, client->label_inst);
	else
		*tmp='\0';
		
	if(client->label!=NULL)
		free(client->label);
	
	grdata=GRDATA_OF(client);
	
	client->label=make_label(grdata->tab_font, str, tmp, maxw, NULL);
}


char *client_full_label(WClient *client)
{
	const char *str=client_beg_label(client);
	char tmp[16];
	
	if(client->label_inst!=0){
		sprintf(tmp, CLIENTNUM_TMPL, client->label_inst);
		return scat(str, tmp);
	}else{
		return scopy(str);
	}
}


static void use_label(WClient *client, const char *label)
{
	WClient *p, *np, *minp=NULL;
	int mininst=INT_MAX;
	const char *str;
	
	for(p=wglobal.client_list; p!=NULL; p=p->g_client_next){
		if(p==client)
			continue;
		str=client_beg_label(p);
		if(strcmp(str, label)==0)
			break;
	}
	
	if(p==NULL)
		return;
	
	for(; ; p=np){
		assert(p!=client);
		np=p->label_next;
		if(p->label_inst+1==np->label_inst){
			continue;
		}else if(p->label_inst>=np->label_inst && np->label_inst!=0){
			mininst=0;
			minp=p;
		}else if(p->label_inst+1<mininst){
			mininst=p->label_inst+1;
			minp=p;
			continue;
		}
		break;
	}
	
	assert(minp!=NULL);
	
	np=minp->label_next;
	client->label_next=np;
	np->label_prev=client;
	minp->label_next=client;
	client->label_prev=minp;
	client->label_inst=mininst;
}


void client_use_label(WClient *client)
{
	WFrame *frame;
	const char *p=client_beg_label(client);
	
	use_label(client, p);
	
	frame=FIND_PARENT(client, WFrame);
	
	if(frame!=NULL)
		frame_recalc_bar(frame);
}


void client_unuse_label(WClient *client)
{
	client->label_next->label_prev=client->label_prev;
	client->label_prev->label_next=client->label_next;
	client->label_next=client;
	client->label_prev=client;
	client->label_inst=0;
}
	
	
/*}}}*/


/*{{{ Misc. */


void client_toggle_tagged(WClient *client)
{
	/* ad hoc */
	client->flags^=CLIENT_TAGGED;
	
	if(CLIENT_HAS_FRAME(client))
		draw_frame_bar(CLIENT_FRAME(client), FALSE);
}


void clear_tags()
{
	WClient *client;
	
	for(client=wglobal.client_list; client!=NULL; client=client->g_client_next){
		if(!(client->flags&CLIENT_TAGGED))
			continue;
		client->flags&=~CLIENT_TAGGED;
		if(CLIENT_HAS_FRAME(client))
			draw_frame_bar(CLIENT_FRAME(client), FALSE);
	}
}


void set_client_frame_id(WClient *client, int id)
{
	WClientWin *cwin=FIRST_THING(client, WClientWin);
	if(cwin!=NULL)
		set_integer_property(cwin->win, wglobal.atom_frame_id, id);
}


bool goto_window(WWindow *wwin)
{
	WWorkspace *ws;
	
	if(wglobal.current_wswindow==wwin)
		return FALSE;
	
	ws=FIND_PARENT(wwin, WWorkspace);

	if(ws==NULL)
		return FALSE;

	set_previous((WThing*)wwin);
	protect_previous();

	if(!active_workspace(ws))
		switch_workspace(ws);
	
	warp(wwin);
	
	unprotect_previous();
	
	return TRUE;
}


void goto_client(WClient *client)
{
	WFrame *frame=FIND_PARENT(client, WFrame);

	if(frame==NULL)
		return;

	set_previous((WThing*)client);
	protect_previous();

	goto_window((WWindow*)frame);
	
	if(frame->current_client!=client)
		frame_switch_client(frame, client);
	
	unprotect_previous();
}


void goto_previous()
{
	WThing *t=wglobal.previous;
	
	if(t==NULL)
		return;

	if(WTHING_IS(t, WClient))
		goto_client((WClient*)t);
	else if(WTHING_IS(t, WWindow))
		goto_window((WWindow*)t);
}

bool goto_client_name(const char *cname)
{
	WClient *client=lookup_client(cname);

	if(client==NULL)
		return FALSE;

	goto_client(client);

	return TRUE;
}

/*}}}*/


/*{{{ Lookup */


WClient *lookup_client(const char *cname)
{
	WClient *client;
	char *name;
	
	for(client=wglobal.client_list;
		client!=NULL;
		client=client->g_client_next){
		
		name=client_full_label(client);
		if(name==NULL)
			continue;
		
		if(strcmp(name, cname)){
			free(name);
			continue;
		}
		
		free(name);
		
		return client;
	}
	
	return NULL;
}


int complete_client(char *nam, char ***cp_ret, char **beg)
{
	WClient *client;
	char *name;
	char **cp;
	int n=0, l=strlen(nam);
	int lnum=0;
	
	*cp_ret=NULL;
	
again:
	
	for(client=wglobal.client_list;
		client!=NULL;
		client=client->g_client_next){
		
		name=(char*)client_full_label(client);
		if(name==NULL)
			continue;
		
		if((lnum==0 && l && strncmp(name, nam, l)) ||
		   (strstr(name, nam)==NULL)){
			   free(name);
			   continue;
		}
		
		if(!add_to_complist(cp_ret, &n, name))
			free(name);
	}
	
	if(n==0 && lnum==0 && l>1){
		lnum=1;
		goto again;
	}
	
	return n;
}


/*}}}*/
