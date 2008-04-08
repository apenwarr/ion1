/*
 * ion/confws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libtu/parser.h>
#include <libtu/tokenizer.h>

#include "workspace.h"
#include "frameid.h"
#include "global.h"
#include "split.h"


extern char *includepaths[];

static WWsSplit *current_split=NULL;
static WWorkspace *current_ws=NULL;
static WScreen *current_screen=NULL;


/*{{{ Conf. functions */


static WRectangle get_geom()
{
	WRectangle geom;
	int s, pos;
	
	if(current_split==NULL){
		geom=current_screen->root.geom;
	}else{
		geom=current_split->geom;

		if(current_split->dir==VERTICAL){
			pos=geom.y;
			s=geom.h;
		}else{
			pos=geom.x;
			s=geom.w;
		}
		
		if(current_split->tl==NULL){
			s=current_split->tmpsize;
		}else{
			s-=current_split->tmpsize;
			pos+=current_split->tmpsize;
		}
			
		if(current_split->dir==VERTICAL){
			geom.y=pos;
			geom.h=s;
		}else{
			geom.x=pos;
			geom.w=s;
		}
	}
	
	return geom;
}


static bool check_splits(const Tokenizer *tokz, int l)
{
	if(current_split!=NULL){
		if(current_split->br!=NULL){
			tokz_warn(tokz, l,
					  "A split can only contain two subsplits or frames");
			return FALSE;
		}
	}else{
		if(current_ws->splitree!=NULL){
			tokz_warn(tokz, l, "There can only be one frame or split at "
							   "workspace level");
			return FALSE;
		}
	}
	
	return TRUE;
}


static bool opt_workspace_split(int dir, Tokenizer *tokz, int n, Token *toks)
{
	WRectangle geom;
	WWsSplit *split;
	int brs, tls;
	int w, h;
	
	if(!check_splits(tokz, toks[1].line))
		return FALSE;
	   
	tls=TOK_LONG_VAL(&(toks[1]));
	brs=TOK_LONG_VAL(&(toks[2]));
	
	geom=get_geom();
	
	if(dir==HORIZONTAL)
		tls=geom.w*tls/(tls+brs);
	else
		tls=geom.h*tls/(tls+brs);
	
	split=create_split(dir, NULL, NULL, geom);
	
	if(split==NULL)
		return FALSE;
		
	split->tmpsize=tls;
	
	if(current_split==NULL)
		current_ws->splitree=(WObj*)split;
	else if(current_split->tl==NULL)
		current_split->tl=(WObj*)split;
	else
		current_split->br=(WObj*)split;
	
	split->parent=current_split;
	
	current_split=split;
	
	return TRUE;
}


static bool opt_workspace_vsplit(Tokenizer *tokz, int n, Token *toks)
{
	return opt_workspace_split(VERTICAL, tokz, n, toks);
}


static bool opt_workspace_hsplit(Tokenizer *tokz, int n, Token *toks)
{
	return opt_workspace_split(HORIZONTAL, tokz, n, toks);
}


static bool opt_split_end(Tokenizer *tokz, int n, Token *toks)
{
	WWsSplit *split=current_split;

	current_split=split->parent;
	
	if(split->br!=NULL)
		return TRUE;

	tokz_warn(tokz, tokz->line, "Split not full");
	
	remove_split(current_ws, split);
	
	return TRUE;
}


/*static bool opt_split_cancel(Tokenizer *tokz, int n, Token *toks)
{
	current_split=current_split->parent;
	return FALSE;
}*/


static bool opt_workspace_frame(Tokenizer *tokz, int n, Token *toks)
{
	WRectangle geom;
	int id;
	WFrame *frame;
	
	if(!check_splits(tokz, toks[1].line))
		return FALSE;
	
	id=TOK_LONG_VAL(&(toks[1]));
	geom=get_geom();
	
	frame=create_frame(current_screen, geom, id, 0);
	
	if(frame==NULL)
		return FALSE;
	
	if(current_split==NULL)
		current_ws->splitree=(WObj*)frame;
	else if(current_split->tl==NULL)
		current_split->tl=(WObj*)frame;
	else
		current_split->br=(WObj*)frame;
	
	frame->win.split=current_split;

	add_workspace_window(current_ws, (WWindow*)frame);
	
	return TRUE;
}


static bool opt_workspace(Tokenizer *tokz, int n, Token *toks)
{
	char *name=TOK_STRING_VAL(&(toks[1]));
	
	if(*name=='\0'){
		tokz_warn(tokz, toks[1].line, "Empty name");
		return FALSE;
	}
	
	current_ws=create_workspace(current_screen, name, FALSE);
	
	if(current_ws==NULL)
		return FALSE;
	
	return TRUE;
}


static bool opt_workspace_end(Tokenizer *tokz, int n, Token *toks)
{
	if(current_ws->splitree==NULL){
		tokz_warn(tokz, tokz->line, "Workspace empty");
		destroy_thing((WThing*)current_ws);
	}
	current_ws=NULL;
	return TRUE;
}


/*static bool opt_workspace_cancel(Tokenizer *tokz, int n, Token *toks)
{
	destroy_thing((WThing*)current_ws);
	current_ws=NULL;
	return FALSE;
}*/



/*}}}*/


/*{{{ Save functions */


static void indent(FILE *file, int lvl)
{
	while(lvl--)
		putc('\t', file);
}


static void write_obj(FILE *file, WObj *obj, int lvl)
{
	WWsSplit *split;
	int tls, brs;
	
	if(WOBJ_IS(obj, WFrame)){
		indent(file, lvl);
		fprintf(file, "frame %d\n", ((WFrame*)obj)->frame_id);
		return;
	}
	
	if(!WOBJ_IS(obj, WWsSplit))
		return;
	
	split=(WWsSplit*)obj;
	
	tls=tree_size(split->tl, split->dir);
	brs=tree_size(split->br, split->dir);
	
	indent(file, lvl);
	if(split->dir==HORIZONTAL)
		fprintf(file, "hsplit %d, %d {\n", tls, brs);
	else
		fprintf(file, "vsplit %d, %d {\n", tls, brs);
	
	write_obj(file, split->tl, lvl+1);
	write_obj(file, split->br, lvl+1);
	
	indent(file, lvl);
	fprintf(file, "}\n");
}

	
static void dodo_write_workspaces(FILE *file)
{
	WWorkspace *ws;
	int i=0;
	
	FOR_ALL_TYPED(current_screen, ws, WWorkspace){
		i++;
		if(ws->name==NULL){
			warn("Not saving workspace %d -- no name", i);
			continue;
		}
		
		if(ws->splitree==NULL){
			warn("Empty workspace -- this cannot happen");
			continue;
		}
		
		fprintf(file, "workspace \"%s\" {\n", ws->name);
		write_obj(file, ws->splitree, 1);
		fprintf(file, "}\n");
	}
}


/*}}}*/


/*{{{ ConfOpts */


static ConfOpt split_opts[]={
	{"vsplit", "ll",  opt_workspace_vsplit, split_opts},
	{"hsplit", "ll",  opt_workspace_hsplit, split_opts},
	{"frame", "l",  opt_workspace_frame, NULL},
	
	{"#end", NULL, opt_split_end, NULL},
	/*{"#cancel", NULL, opt_split_cancel, NULL},*/
	{NULL, NULL ,NULL, NULL}
};


ConfOpt workspace_opts[]={
	{"vsplit", "ll",  opt_workspace_vsplit, split_opts},
	{"hsplit", "ll",  opt_workspace_hsplit, split_opts},
	{"frame", "l",  opt_workspace_frame, NULL},
	
	{"#end", NULL, opt_workspace_end, NULL},
	/*{"#cancel", NULL, opt_workspace_cancel, NULL},*/
	{NULL, NULL ,NULL, NULL}
};


ConfOpt wsconf_opts[]={
	{"workspace", "s", opt_workspace, workspace_opts},
	{NULL, NULL ,NULL, NULL}
};


/*}}}*/


/*{{{ read_workspace, write_workspaces */


static char *get_wsconfname()
{
	char *dir;
	char *tmp;
	char *display;
	
	display=XDisplayName(wglobal.display);
	
	tmp=strchr(display, ':');
	if(tmp!=NULL){
		tmp=strchr(tmp, '.');
		if(tmp!=NULL)
			*tmp='\0';
	}

	dir=getenv("HOME");
	
	if(dir==NULL){
		warn("Could not get $HOME");
		return NULL;
	}
	
	tmp=ALLOC_N(char, strlen(dir)+strlen(display)+32);
	
	if(tmp==NULL){
		warn_err();
		return NULL;
	}
	
	sprintf(tmp, "%s/.ion/workspaces-%s.%d.conf", dir, display,
			current_screen->xscr);
	
	return tmp;
}


static bool do_read_workspaces()
{
	char *wsconf;
	bool retval=FALSE;
	Tokenizer *tokz;
	
	wsconf=get_wsconfname();
	
	if(wsconf==NULL)
		return FALSE;
	
	if(access(wsconf, F_OK)==0){
		tokz=tokz_open(wsconf);
		
		if(tokz!=NULL){
			tokz->flags=TOKZ_ERROR_TOLERANT;
			tokz_set_includepaths(tokz, includepaths);
			retval=parse_config_tokz(tokz, wsconf_opts);
			tokz_close(tokz);
		}
	}
	
	free(wsconf);
	return retval;
}


bool read_workspaces(WScreen *scr)
{
	bool successp;
	
	current_screen=scr;
	successp=do_read_workspaces();
	current_screen=NULL;
	
	return successp;
}


static bool ensuredir(char *f)
{
	char *p=strrchr(f, '/');

	if(p==NULL)
		return TRUE;
	
	*p='\0';
	
	if(access(f, F_OK)){
		if(mkdir(f, 0700)){
			warn_err_obj(f);
			*p='/';
			return FALSE;
		}
	}

	*p='/';
	return TRUE;
}


static bool do_write_workspaces()
{
	char *wsconf;
	FILE *file;
	
	wsconf=get_wsconfname();
	
	if(wsconf==NULL)
		return FALSE;

	if(!ensuredir(wsconf))
		return FALSE;
	
	file=fopen(wsconf, "w");
	
	if(file==NULL){
		warn_err_obj(wsconf);
		free(wsconf);
		return FALSE;
	}
	
	fprintf(file, "# This file was created by and is modified by Ion.\n");
	
	dodo_write_workspaces(file);
	
	fclose(file);
	free(wsconf);
	
	return TRUE;
}


bool write_workspaces(WScreen *scr)
{
	bool successp;
	
	current_screen=scr;
	successp=do_write_workspaces();
	current_screen=NULL;
	
	return successp;
}

/*}}}*/
