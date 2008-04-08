/*
 * ion/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <limits.h>
#include <unistd.h>
#include <string.h>

#include <libtu/parser.h>
#include <libtu/tokenizer.h>
#include <libtu/output.h>

#include "common.h"
#include "global.h"
#include "frame.h"
#include "client.h"
#include "queryp.h"
#include "exec.h"
#include "focus.h"
#include "workspace.h"
#include "wedln.h"
#include "complete_file.h"
#include "function.h"
#include "wmessage.h"


/*{{{ Generic */


static void frame_set_input(WFrame *frame, WInput *input)
{
	frame->current_input=input;
	if(IS_ACTIVE_FRAME(frame))
		set_focus((WThing*)input);
}
	

WEdln *do_query_edln(WFrame *frame, WEdlnHandler *handler,
					 const char *prompt, const char *dflt,
					 EdlnCompletionHandler *chnd)
{
	WRectangle geom;
	WEdln *wedln;

	if(frame->current_input!=NULL)
		return NULL;
	
	frame_client_geom(frame, &geom);
	
	wedln=create_wedln((WWindow*)frame, geom, handler, prompt, dflt);
	
	if(wedln!=NULL){
		wedln->edln.completion_handler=chnd;
		frame_set_input(frame, (WInput*)wedln);
	}

	return wedln;
}


void fwarn(WFrame *frame, char *p)
{
	WRectangle geom;
	WEdln *wedln;
	WMessage *wmsg;
	char *p2;
	
	if(p==NULL || frame->current_input!=NULL)
		return;
	
	frame_client_geom(frame, &geom);
	
	p2=scat("-Error- ", p);
	if(p2!=NULL){
		wmsg=create_wmsg((WWindow*)frame, geom, p2);
		free(p2);
	}else{
		wmsg=create_wmsg((WWindow*)frame, geom, p);
	}
	free(p);
	
	if(wmsg!=NULL)
		frame_set_input(frame, (WInput*)wmsg);
}

#define FWARN(ARGS) fwarn((WFrame*)thing, errmsg ARGS)

/*}}}*/


/*{{{ Files */


static char wdbuf[PATH_MAX+10]="/";
static int wdstatus=0;

static const char *my_getwd()
{
	
	if(wdstatus==0){
		if(getcwd(wdbuf, PATH_MAX)==NULL){
			wdstatus=-1;
			strcpy(wdbuf, "/");
		}else{
			wdstatus=1;
			strcat(wdbuf, "/");
		}
	}
	
	return wdbuf;
}

	
void handler_runfile(WThing *thing, char *str, char *userdata)
{
	char *p;
	
	if(*str=='\0')
		return;
	
	if(userdata!=NULL)
		do_open_with(SCREEN_OF(thing), userdata, str);
	
	p=strrchr(str, '/');
	if(p==NULL){
		wdstatus=0;
	}else{
		*(p+1)='\0';
		strncpy(wdbuf, str, PATH_MAX);
	}
}


void handler_runwith(WThing *thing, char *str, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	
	if(userdata==NULL)
		return;
	
	if(*str!='\0')
		do_open_with(scr, userdata, str);
	else
		wm_exec(scr, userdata);
}


void handler_exec(WThing *thing, char *str, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	
	if(*str==':')
		do_open_with(scr, "ion-runinxterm", str+1);
	else
		wm_exec(scr, str);
}


void query_exec(WFrame *frame)
{
	do_query_edln(frame, handler_exec, "Run:", NULL, complete_file_with_path);
}


void query_runfile(WFrame *frame, char *prompt, char *cmd)
{
	WEdln *wedln=do_query_edln(frame, handler_runfile,
							   prompt, my_getwd(), complete_file);
	if(wedln!=NULL)
		wedln->userdata=scopy(cmd);
}


void query_runwith(WFrame *frame, char *prompt, char *cmd)
{
	WEdln *wedln=do_query_edln(frame, handler_runwith,
							   prompt, NULL, NULL);
	if(wedln!=NULL)
		wedln->userdata=scopy(cmd);
}


/*}}}*/


/*{{{ Navigation */


static bool attach_test(WFrame *dst, WClient *client, WFrame *thing)
{
	if(!same_screen((WThing*)dst, (WThing*)client)){
		/* complaint should go in 'thing' -frame */
		FWARN(("Cannot attach: not on same screen."));
		return FALSE;
	}
	return frame_attach_client(dst, client, TRUE);
}


void handler_attachclient(WThing *thing, char *str, char *userdata)
{
	WClient *client=lookup_client(str);
	
	if(client==NULL){
		FWARN(("No client named '%s'", str));
		return;
	}
	
	attach_test((WFrame*)thing, client, (WFrame*)thing);
}


void handler_gotoclient(WThing *thing, char *str, char *userdata)
{
	WClient *client=lookup_client(str);
	
	if(client==NULL){
		FWARN(("No client named '%s'", str));
		return;
	}
	
	goto_client(client);
}


void query_attachclient(WFrame *frame)
{
	do_query_edln(frame, handler_attachclient,
				  "Attach client:", "", complete_client);
}


void query_gotoclient(WFrame *frame)
{
	do_query_edln(frame, handler_gotoclient,
				  "Goto client:", "", complete_client);
}


bool empty_name(const char *p)
{
	return (strspn(p, " \t")==strlen(p));
}


void handler_workspace(WThing *thing, char *name, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	WWorkspace *ws;
	
	if(empty_name(name))
		return;
	
	ws=lookup_workspace(name);
	
	if(ws==NULL){
		ws=create_workspace(scr, name, TRUE);
		if(ws==NULL){
			FWARN(("Unable to create workspace."));
			return;
		}
	}
	
	switch_workspace(ws);
}
		
		
void query_workspace(WFrame *frame)
{
	do_query_edln(frame, handler_workspace,
				  "Goto/create workspace:", "", complete_workspace);
}


void handler_workspace_with(WThing *thing, char *name, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	WWorkspace *ws;
	WClient *client;
	WFrame *frame;
	
	if(empty_name(name))
		return;
	
	ws=lookup_workspace(name);
	client=lookup_client(userdata);
	
	if(ws!=NULL){
		frame=(WFrame*)find_current(ws);
		if(frame==NULL || !WTHING_IS(frame, WFrame)){
			FWARN(("Workspace %s has no current frame", name));
			return;
		}
	}else{
		if(client==NULL){
			FWARN(("Client disappeared"));
			return;
		}
	
		ws=create_workspace(scr, name, TRUE);
		if(ws==NULL){
			FWARN(("Unable to create workspace."));
			return;
		}
	
		frame=FIRST_THING(ws, WFrame);
	
		assert(frame!=NULL);
	}
	
	if(attach_test((WFrame*)frame, client, (WFrame*)thing))
		goto_client(client);
}


void query_workspace_with(WFrame *frame)
{
	WEdln *wedln;
	WClient *client=frame->current_client;
	char *p;
	
	if(client==NULL){
		query_workspace(frame);
		return;
	}
	
	p=client_full_label(client);
	
	wedln=do_query_edln(frame, handler_workspace_with,
						"Create workspace/attach:", p, complete_workspace);
	if(wedln==NULL)
		free(p);
	else
		wedln->userdata=p;
}


void handler_renameworkspace(WThing *thing, char *name, char *userdata)
{
	WWorkspace *ws=SCREEN_OF(thing)->current_workspace;

	if(empty_name(name))
		return;

	rename_workspace(ws, name);
}


void query_renameworkspace(WFrame *frame)
{
	WWorkspace *ws = wglobal.current_screen->current_workspace;
	
	do_query_edln(frame, handler_renameworkspace,
				  "Rename workspace to:", ws->name, complete_workspace);
}


/*}}}*/


/*{{{ Misc. */

/* ugly hack to know what we operate on in opt_default() */
static WThing *function_thing=NULL;
static WThing *function_frame=NULL;
static char *last_error_message=NULL;


static void function_warn_handler(const char *message)
{
	if(last_error_message!=NULL)
		free(last_error_message);
	last_error_message=scopy(message);
}


/* We don't want to refer to destroyed things. */
void query_check_function_thing(WThing *t)
{
	if(function_thing==t)
		function_thing=t->t_parent;
	if(function_frame==t)
		function_frame=NULL;
}


void query_set_function_thing(WThing *t)
{
	if(t!=NULL)
		function_thing=t;
}


static bool opt_default(Tokenizer *tokz, int n, Token *toks)
{
	WThing *thing=function_thing;
	WFunction *func;
	char *name=TOK_IDENT_VAL(&(toks[0]));
	
	if(thing==NULL)
		return FALSE;
	
	func=lookup_func(name, FUNTAB_MAIN);
	
	if(func==NULL){
		warn("Unknown function '%s' or not in FUNTAB_MAIN.", name);
		return FALSE;
	}
	
	if(!check_args(tokz, toks, n, func->argtypes)){
		warn("Argument check for function '%s' failed. Prototype is '%s'.",
			 name, func->argtypes);
		return FALSE;
	}
	
	func->callhnd(thing, func, n-1, &(toks[1]));
	
	/*if(wglobal.focus_next!=NULL)
		function_thing=wglobal.focus_next;*/
	
	return TRUE;
}


static ConfOpt command_opts[]={
	{"#default", NULL, opt_default, NULL},
	{NULL, NULL, NULL, NULL}
};


bool command_sequence(WThing *thing, char *fn)
{
	static bool command_sq=FALSE;
	bool retval;
	
	Tokenizer *tokz;

	if(command_sq){
		warn("Nested command sequence.");
		return FALSE;
	}
	
	command_sq=TRUE;
	function_thing=thing;

	tokz=tokz_prepare_buffer(fn, -1);
	tokz->flags|=TOKZ_DEFAULT_OPTION;
	retval=parse_config_tokz(tokz, command_opts);
	tokz_close(tokz);
	
	function_thing=NULL;
	command_sq=FALSE;
	
	return retval;
}


void handler_function(WThing *thing, char *fn, char *userdata)
{
	WarnHandler *old_warn_handler;
	Tokenizer *tokz;
	bool error;
	
	function_frame=thing;
	
	old_warn_handler=set_warn_handler(function_warn_handler);
	error=!command_sequence(thing, fn);
	set_warn_handler(old_warn_handler);
	
	if(function_frame!=NULL){
		if(last_error_message!=NULL){
			FWARN(("%s", last_error_message));
		}else if(error){
			FWARN(("An unknown error occurred while trying to "
				   "parse your request"));
		}
	}

	function_frame=NULL;

	if(last_error_message!=NULL){
		free(last_error_message);
		last_error_message=NULL;
	}
}


void handler_yesno(WThing *thing, char *yesno, char *fn)
{
	if(strcasecmp(yesno, "y") && strcasecmp(yesno, "yes"))
		return;
	
	handler_function(thing, fn, NULL);
}


void query_yesno(WFrame *frame, char *prompt, char *fn)
{
	WEdln *wedln=do_query_edln(frame, handler_yesno,
							   prompt, NULL, NULL);
	if(wedln!=NULL)
		wedln->userdata=scopy(fn);
}


void query_function(WFrame *frame)
{
	do_query_edln(frame, handler_function,
				  "Function name:", NULL, complete_mainfunc);
}


/*}}}*/

