/*
 * ion/readconfig.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <unistd.h>

#include <libtu/parser.h>
#include <libtu/tokenizer.h>

#include "readconfig.h"
#include "function.h"
#include "binding.h"
#include "winprops.h"
#include "frameid.h"
#include "global.h"
#include "font.h"
#include "draw.h"
#include "modules.h"


static WWinProp *tmp_winprop=NULL;
static WBindmap *tmp_bindmap;
static int tmp_funtab;


#define FOR_ALL_SELECTED_SCREENS(SCR) \
	FOR_ALL_SCREENS(SCR) if(scr->confsel)


/*{{{ Helpers */


static bool get_func(Tokenizer *tokz, Token *toks, int n,
					 WBinding *binding, int funtab)
{
	const char *name=TOK_STRING_VAL(&(toks[0]));
	WFunction *func;
	int i;
	
	func=lookup_func(name, funtab);
	
	if(func==NULL){
		tokz_warn(tokz, toks[0].line, "\"%s\" undefined", name);
		return FALSE;
	}
	
	if(!check_args(tokz, toks, n, func->argtypes))
		return FALSE;
	
	binding->func=func;
	
	for(i=1; i<n; i++){
		binding->args[i-1]=toks[i];
		toks[i].type=TOK_INVALID;
	}
	
	return TRUE;
}


static int find_ndx(const char *names[], const char *p)
{
	int i;
	for(i=0; names[i]!=NULL; i++){
		if(strcmp(p, names[i])==0)
			return i;
	}
	return -1;
}


#define BUTTON1_NDX 9


static const char *state_names[]={
	"Shift", "Lock", "Control",
	"Mod1", "Mod2", "Mod3", "Mod4", "Mod5", "AnyModifier",
	"Button1", "Button2", "Button3", "Button4", "Button5", "AnyButton",
	NULL
};


static int state_values[]={
	ShiftMask, LockMask, ControlMask,
	Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask, AnyModifier,
	Button1, Button2, Button3, Button4, Button5, AnyButton
};


static bool parse_keybut(Tokenizer *tokz, Token *tok,
						 uint *mod_ret, uint *kcb_ret, bool button)
{
	
	char *p=TOK_STRING_VAL(tok);
	char *p2;
	int keysym=NoSymbol, i;
	
	while(*p!='\0'){
		p2=strchr(p, '+');
		
		if(p2!=NULL)
			*p2='\0';
		
		if(!button)
			keysym=XStringToKeysym(p);
		
		if(!button && keysym!=NoSymbol){
			if(*kcb_ret!=NoSymbol){
				tokz_warn(tokz, tok->line, "Insane key combination");
				return FALSE;
			}
			*kcb_ret=keysym;
		}else{
			i=find_ndx(state_names, p);

			if(i<0){
				tokz_warn(tokz, tok->line, "\"%s\" unknown", p);
				return FALSE;
			}
			
			if(i>=BUTTON1_NDX){
				if(!button || *kcb_ret!=NoSymbol){
					tokz_warn(tokz, tok->line, "Insane button combination");
					return FALSE;
				}
				*kcb_ret=state_values[i];
			}else{
				*mod_ret|=state_values[i];
			}
		}

		if(p2==NULL)
			break;
		
		p=p2+1;
	}
	
	return TRUE;
}

#undef BUTTON1_NDX

/*}}}*/


/*{{{ Bindings */


/* action "mods+key/button", "func" [, args] */
static bool do_bind(Tokenizer *tokz, int n, Token *toks, int act, bool wr)
{
	WBinding binding=BINDING_INIT;
	uint kcb=NoSymbol, mod=tmp_bindmap->confdefmod;
	
	if(!parse_keybut(tokz, &(toks[1]), &mod, &kcb, act!=ACT_KEYPRESS))
		return TRUE;

	if(act==ACT_KEYPRESS){
		kcb=XKeysymToKeycode(wglobal.dpy, kcb);
		if(kcb==0)
			return FALSE;
	}
	
	if(wr && mod==0){
		tokz_warn(tokz, toks[0].line, "Cannot waitrel when no modifiers set. "
				  "Sorry.");
	}

	binding.waitrel=wr;
	binding.act=act;
	binding.state=mod;
	binding.kcb=kcb;
	
	if(!get_func(tokz, &(toks[2]), n-2, &binding, tmp_funtab))
		return TRUE; /* just ignore the error */ 
	
	if(add_binding(tmp_bindmap, &binding))
		return TRUE;

	destroy_binding(&binding);
	
	tokz_warn(tokz, toks[0].line, "Unable to bind \"%s\" to \"%s\"", 
			  TOK_STRING_VAL(&(toks[1])), TOK_STRING_VAL(&(toks[2])));
	
	return TRUE;
}


/* */


static bool opt_kpress(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n, toks, ACT_KEYPRESS, FALSE);
}


static bool opt_kpress_waitrel(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n, toks, ACT_KEYPRESS, TRUE);
}


static bool opt_mpress(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n, toks, ACT_BUTTONPRESS, FALSE);
}


static bool opt_mdrag(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n, toks, ACT_BUTTONMOTION, FALSE);
}


static bool opt_mclick(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n, toks, ACT_BUTTONCLICK, FALSE);
}


static bool opt_mdblclick(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n, toks, ACT_BUTTONDBLCLICK, FALSE);
}


static bool opt_submap(Tokenizer *tokz, int n, Token *toks)
{
	WBinding binding=BINDING_INIT;
	uint kcb=NoSymbol, mod=tmp_bindmap->confdefmod;
	
	if(!parse_keybut(tokz, &(toks[1]), &mod, &kcb, 0))
		return TRUE;

	kcb=XKeysymToKeycode(wglobal.dpy, kcb);
	if(kcb==0)
		return FALSE;

	binding.act=ACT_KEYPRESS;
	binding.state=mod;
	binding.kcb=kcb;
	
	binding.submap=create_bindmap();
	
	if(binding.submap==NULL)
		return FALSE;

	if(add_binding(tmp_bindmap, &binding)){
		binding.submap->parent=tmp_bindmap;
		tmp_bindmap=binding.submap;
		return TRUE;
	}

	destroy_binding(&binding);
	
	tokz_warn(tokz, toks[0].line, "Unable to bind \"%s\" to \"%s\"", 
			  TOK_STRING_VAL(&(toks[1])), TOK_STRING_VAL(&(toks[2])));
	
	return TRUE;
}


/* */


static bool opt_set_mod(Tokenizer *tokz, int n, Token *toks)
{
	uint mod=0, kcb=NoSymbol;
	
	if(!parse_keybut(tokz, &(toks[1]), &mod, &kcb, FALSE))
		return FALSE; 
	
	tmp_bindmap->confdefmod=mod;
	
	return TRUE;
}


static bool opt_bindings(Tokenizer *tokz, int n, Token *toks)
{
	tmp_bindmap=&(wglobal.main_bindmap);
	tmp_funtab=FUNTAB_MAIN;
	return TRUE;
}


static bool opt_tab_bindings(Tokenizer *tokz, int n, Token *toks)
{
	tmp_bindmap=&(wglobal.tab_bindmap);
	tmp_funtab=FUNTAB_MAIN;
	return TRUE;
}


static bool opt_input_bindings(Tokenizer *tokz, int n, Token *toks)
{
	tmp_bindmap=&(wglobal.input_bindmap);
	tmp_funtab=FUNTAB_INPUT;
	return TRUE;
}


static bool opt_moveres_bindings(Tokenizer *tokz, int n, Token *toks)
{
	tmp_bindmap=&(wglobal.moveres_bindmap);
	tmp_funtab=FUNTAB_MOVERES;
	return TRUE;
}

extern WBindmap commandmode_bindmap;
static bool opt_commandmode_bindings(Tokenizer *tokz, int n, Token *toks)
{
	tmp_bindmap=&commandmode_bindmap;
	tmp_funtab=FUNTAB_MAIN;
	return TRUE;
}


static bool end_bindings(Tokenizer *tokz, int n, Token *toks)
{
	tmp_bindmap=tmp_bindmap->parent;
	return TRUE;
}


/*}}}*/


/*{{{ Screen (graphics)*/


/* Font */

static bool opt_screen_font(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;

	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.font=load_font(wglobal.dpy,
								   TOK_STRING_VAL(&(toks[1])));
	}
	
	return TRUE;
}


static bool opt_screen_tab_font(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.tab_font=load_font(wglobal.dpy,
									   TOK_STRING_VAL(&(toks[1])));
	}
	
	return TRUE;
}


static bool opt_screen_term_font(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	XFontStruct *term_font;
	int w, h;
	
	term_font=load_font(wglobal.dpy, TOK_STRING_VAL(&(toks[1])));
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->w_unit=term_font->max_bounds.width;
		scr->h_unit=term_font->max_bounds.ascent+term_font->max_bounds.descent;
	}

	XFreeFont(wglobal.dpy, term_font);
	
	return TRUE;
}


/* Border */

static int do_border(Tokenizer *tokz, int n, Token *toks, WBorder *bd)
{
	int tl, br, ipad;
	
	tl=TOK_LONG_VAL(&(toks[1]));
	br=TOK_LONG_VAL(&(toks[2]));
	ipad=TOK_LONG_VAL(&(toks[3]));
	
	if(tl<0 || br<0 || ipad<0){
		tokz_warn(tokz, toks[1].line, "Erroneous values");
		return FALSE;
	}

	bd->tl=tl;
	bd->br=br;
	bd->ipad=ipad;
	
	return TRUE;
}

#define BDHAND(NAME)                                               \
 static int opt_screen_##NAME(Tokenizer *tokz, int n, Token *toks) \
 {                                                                 \
	WScreen *scr;                                                  \
	FOR_ALL_SELECTED_SCREENS(scr){                                 \
		do_border(tokz, n, toks, &(scr->grdata.NAME));             \
	}                                                              \
	return TRUE;                                                   \
 }


BDHAND(frame_border)
BDHAND(tab_border)
BDHAND(input_border)

#undef BDHAND


/* Colors */

static bool do_colorgroup(Tokenizer *tokz, Token *toks,
						  WScreen *scr, WColorGroup *cg)
{
	int cnt=0;
	
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[1])), &(cg->hl));
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[2])), &(cg->sh));
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[3])), &(cg->bg));
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[4])), &(cg->fg));
	
	if(cnt!=4){
		tokz_warn(tokz, toks[1].line, "Unable to allocate one or more colors");
		return FALSE;
	}
	
	return TRUE;
}

#define CGHAND(CG)                                                \
 static bool opt_screen_##CG(Tokenizer *tokz, int n, Token *toks) \
 {                                                                \
	WScreen *scr;                                                 \
	FOR_ALL_SELECTED_SCREENS(scr){                                \
		do_colorgroup(tokz, toks, scr, &(scr->grdata.CG));        \
	}                                                             \
	return TRUE;                                                  \
 }

#define CGHANDSC(CG)                                              \
 static bool opt_screen_##CG(Tokenizer *tokz, int n, Token *toks) \
 {                                                                \
	WScreen *scr;                                                 \
	FOR_ALL_SELECTED_SCREENS(scr){                                \
		do_colorgroup(tokz, toks, scr, &(scr->grdata.CG));        \
		scr->grdata.shortcutc_set=TRUE;							  \
	}                                                             \
	return TRUE;                                                  \
 }

CGHAND(frame_colors)
CGHAND(tab_colors)
CGHAND(tab_sel_colors)
CGHAND(act_frame_colors)
CGHAND(act_tab_colors)
CGHAND(act_tab_sel_colors)
CGHAND(input_colors)
CGHANDSC(act_tab_shortcut_colors)
CGHANDSC(tab_shortcut_colors)

#undef CGHAND


static bool opt_screen_background_color(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		/* TODO: free */
		if(!alloc_color(scr, TOK_STRING_VAL(&(toks[1])),
						&(scr->grdata.bgcolor))){
			tokz_warn(tokz, toks[1].line,
					  "Unable to allocate one or more colors");
		}
	}
	
	return TRUE;
}


static bool opt_screen_selection_colors(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		/* TODO: free */
		if(!alloc_color(scr, TOK_STRING_VAL(&(toks[1])),
						&(scr->grdata.selection_bgcolor)) ||
		   !alloc_color(scr, TOK_STRING_VAL(&(toks[2])),
						&(scr->grdata.selection_fgcolor))){
		
			tokz_warn(tokz, toks[1].line,
					  "Unable to allocate one or more colors");
		}
	}
	
	return TRUE;
}
		

/* Look/misc */

static bool opt_screen_spacing(Tokenizer *tokz, int n, Token *toks)
{
	long spacing=TOK_LONG_VAL(&(toks[1]));
	WScreen *scr;

	if(spacing<0){
		tokz_warn(tokz, toks[1].line, "Invalid value");
		return FALSE;
	}
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.spacing=spacing;
	}
	
	return TRUE;
}


static bool opt_screen_bar_inside_frame(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.bar_inside_frame=TOK_BOOL_VAL(&(toks[1]));
	}
	
	return TRUE;
}

static bool opt_screen_shortcut_corner(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	char *data=TOK_STRING_VAL(&(toks[1]));
	int val=0;

	if(!strncasecmp(data, "right", 5))
		val=1;
	else if(!strncasecmp(data, "left", 4))
		val=0;
	else{
		tokz_warn(tokz, toks[1].line,
				  "Invalid value `%s' given to shortcut_corner.", data);
	}

	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.shortcut_corner=val;
	}

	return TRUE;
}


/* Begin/end */

static bool begin_screen(Tokenizer *tokz, int n, Token *toks)
{	
	int i;
	int sn;
	WScreen *scr;
	
	for(i=1; i<n; i++){
		sn=TOK_LONG_VAL(&(toks[i]));
		FOR_ALL_SCREENS(scr){
			if(scr->xscr==sn)
				scr->confsel=TRUE;
		}
	}

	return TRUE;
}


static bool end_screen(Tokenizer *tokz, int n, Token *toks)
{	
	WScreen *scr;
	
	FOR_ALL_SCREENS(scr){
		scr->confsel=FALSE;
	}

	return TRUE;
}


/*}}}*/


/*{{{ Global/misc */


static bool opt_opaque_resize(Tokenizer *tokz, int n, Token *toks)
{
	wglobal.opaque_resize=TOK_BOOL_VAL(&(toks[1]));
	
	return TRUE;
}

static bool opt_dblclick_delay(Tokenizer *tokz, int n, Token *toks)
{
	int dd=TOK_LONG_VAL(&(toks[1]));

	wglobal.dblclick_delay=(dd<0 ? 0 : dd);
	
	return TRUE;
}


static bool opt_resize_delay(Tokenizer *tokz, int n, Token *toks)
{
	int rd=TOK_LONG_VAL(&(toks[1]));

	wglobal.resize_delay=(rd<0 ? 0 : rd);
	
	return TRUE;
}


static bool opt_module(Tokenizer *tokz, int n, Token *toks)
{
	return load_module(TOK_STRING_VAL(&(toks[1])));
}


/*}}}*/


/*{{{ Window props */


static bool opt_winprop_max_size(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->max_w=TOK_LONG_VAL(&(toks[1]));;
	tmp_winprop->max_h=TOK_LONG_VAL(&(toks[2]));;
	tmp_winprop->flags|=CWIN_PROP_MAXSIZE;
	return TRUE;
}


static bool opt_winprop_aspect(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->aspect_w=TOK_LONG_VAL(&(toks[1]));;
	tmp_winprop->aspect_h=TOK_LONG_VAL(&(toks[2]));;
	tmp_winprop->flags|=CWIN_PROP_ASPECT;
	return TRUE;
}


static bool opt_winprop_switchto(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->switchto=TOK_BOOL_VAL(&(toks[1]));
	return TRUE;
}


static bool opt_winprop_stubborn(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->stubborn=TOK_BOOL_VAL(&(toks[1]));
	return TRUE;
}

static bool opt_winprop_transient_mode(Tokenizer *tokz, int n, Token *toks)
{
	char *mod=TOK_IDENT_VAL(&(toks[1]));
	int tm;

	if(strcmp(mod, "normal")==0){
		tm=TRANSIENT_MODE_NORMAL;
	}else if(strcmp(mod, "current")==0){
		tm=TRANSIENT_MODE_CURRENT;
	}else if(strcmp(mod, "off")==0){
		tm=TRANSIENT_MODE_OFF;
	}else{
		tokz_warn(tokz, toks[1].line,
				  "Invalid transient mode `%s', must be 'normal',"
				  " 'current' or 'off'", mod);
		return FALSE;
	}

	tmp_winprop->transient_mode=tm;
	
	return TRUE;
}


static bool opt_winprop_acrobatic(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->flags|=CWIN_KLUDGE_ACROBATIC;
	return TRUE;
}


static bool begin_winprop(Tokenizer *tokz, int n, Token *toks)
{
	WWinProp *wrop;
	char *wclass, *winstance;
	
	tmp_winprop=ALLOC(WWinProp);
	
	if(tmp_winprop==NULL){
		warn_err();
		return FALSE;
	}
	
	tmp_winprop->flags=0;
	tmp_winprop->data=wclass=TOK_TAKE_STRING_VAL(&(toks[1]));
	tmp_winprop->switchto=-1;
	tmp_winprop->stubborn=0;
	tmp_winprop->transient_mode=TRANSIENT_MODE_NORMAL;

	winstance=strchr(wclass, '.');

	if(winstance!=NULL){
		*winstance++='\0';
		if(strcmp(winstance, "*")==0)
			winstance=NULL;
	}
	
	if(strcmp(wclass, "*")==0)
		wclass=NULL;
	
	tmp_winprop->wclass=wclass;
	tmp_winprop->winstance=winstance;
	
	return TRUE;
}
	

static bool end_winprop(Tokenizer *tokz, int n, Token *toks)
{
	register_winprop(tmp_winprop);
	tmp_winprop=NULL;
	
	return TRUE;
}


#define cancel_winprop end_winprop


/*}}}*/


/*{{{ The ConfOpts */


static ConfOpt screen_opts[]={
	{"term_font", "s", opt_screen_term_font, NULL},

	{"font", "s", opt_screen_font, NULL},
	{"tab_font", "s", opt_screen_tab_font, NULL},

	{"frame_border", "lll", opt_screen_frame_border, NULL},
	{"tab_border", "lll", opt_screen_tab_border, NULL},
	{"input_border", "lll", opt_screen_input_border, NULL},
	
	{"act_tab_colors", "ssss", opt_screen_act_tab_colors, NULL},
	{"act_tab_sel_colors", "ssss", opt_screen_act_tab_sel_colors, NULL},
	{"act_frame_colors", "ssss", opt_screen_act_frame_colors, NULL},
	{"act_tab_shortcut_colors", "ssss", opt_screen_act_tab_shortcut_colors, NULL},
	{"tab_colors", "ssss", opt_screen_tab_colors, NULL},
	{"tab_sel_colors", "ssss", opt_screen_tab_sel_colors, NULL},
	{"tab_shortcut_colors", "ssss", opt_screen_tab_shortcut_colors, NULL},
	{"frame_colors", "ssss", opt_screen_frame_colors, NULL},
	{"input_colors", "ssss", opt_screen_input_colors, NULL},
	{"background_color", "s", opt_screen_background_color, NULL},
	{"selection_colors", "ss", opt_screen_selection_colors, NULL},
	
	{"spacing", "l", opt_screen_spacing, NULL},
	{"bar_inside_frame", "b", opt_screen_bar_inside_frame, NULL},
	{"shortcut_corner", "s", opt_screen_shortcut_corner, NULL},
	 
	{"#end", NULL, end_screen, NULL},
	{"#cancel", NULL, end_screen, NULL},
	
	{NULL, NULL, NULL, NULL}
};


static ConfOpt winprop_opts[]={
	{"max_size", "ll", opt_winprop_max_size, NULL},
	{"aspect", "ll", opt_winprop_aspect, NULL},
	{"acrobatic", NULL, opt_winprop_acrobatic, NULL},
	{"switchto", "b", opt_winprop_switchto, NULL},
	{"stubborn", "b", opt_winprop_stubborn, NULL},
	{"transient_mode", "i", opt_winprop_transient_mode, NULL},
	
	{"#end", NULL, end_winprop, NULL},
	/*{"#cancel", NULL, cancel_winprop, NULL},*/
	
	{NULL, NULL, NULL, NULL}
};


static ConfOpt bindings_opts[]={
	{"set_mod", "s", opt_set_mod, NULL},
	{"submap", "s", opt_submap, bindings_opts},
	
	{"kpress", "ss*", opt_kpress, NULL},
	{"kpress_waitrel", "ss*", opt_kpress_waitrel, NULL},
	{"mpress", "ss*", opt_mpress, NULL},
	{"mdrag", "ss*", opt_mdrag, NULL},
	{"mclick", "ss*", opt_mclick, NULL},
	{"mdblclick", "ss*", opt_mdblclick, NULL},

	{"#end", NULL, end_bindings, NULL},
	/*{"#cancel", NULL, end_bindings, NULL},*/
	
	{NULL, NULL, NULL, NULL}
};


static ConfOpt toplevel_opts[]={
	/* screens */
	{"screen", "l+", begin_screen, screen_opts},
	
	/* keybindings */
	{"bindings", NULL, opt_bindings, bindings_opts},
	{"tab_bindings", NULL, opt_tab_bindings, bindings_opts},
	{"input_bindings", NULL, opt_input_bindings, bindings_opts},
	{"moveres_bindings", NULL, opt_moveres_bindings, bindings_opts},
	{"commandmode_bindings", NULL, opt_commandmode_bindings, bindings_opts},
	
	/* window props */
	{"winprop" , "s", begin_winprop, winprop_opts},

	/* misc */
	{"dblclick_delay", "l", opt_dblclick_delay, NULL},
	{"resize_delay", "l", opt_resize_delay, NULL},
	{"opaque_resize", "b", opt_opaque_resize, NULL},
	
	{"module", "s", opt_module, NULL},
	
	{NULL, NULL, NULL, NULL}
};


/*}}}*/


char *includepaths[]={
	ETCDIR"/ion", NULL
};


static char *get_cfgfile(const char *appname, const char *confname)
{
	char *tmp, *home;
	
	if(confname==NULL)
		confname=appname;
	
	home=getenv("HOME");
	if(home!=NULL){
		libtu_asprintf(&tmp, "%s/.%s/%s.conf", home, appname, confname);
		if(tmp==NULL){
			warn_err();
			return NULL;
		}
	}
	
	if(tmp==NULL || access(tmp, F_OK)!=0){
		libtu_asprintf(&tmp, ETCDIR"/%s/%s.conf", appname, confname);
		if(tmp==NULL){
			warn_err();
			return NULL;
		}
	}
	return tmp;
}


static bool do_read_config_for(const char* cfgfile,
							   const char* modulename,
							   const ConfOpt *opts,
							   bool testfirst)
{
	char *tmp=NULL;
	bool retval=FALSE;
	Tokenizer *tokz;
	
	if(cfgfile==NULL){
		tmp=get_cfgfile("ion", modulename);
		if(tmp==NULL)
			return FALSE;
		cfgfile=tmp;
	}

	if(testfirst && access(tmp, F_OK)!=0)
		return TRUE;
	
	tokz=tokz_open(cfgfile);
	    
	if(tokz!=NULL){
		tokz->flags=TOKZ_ERROR_TOLERANT;
		tokz_set_includepaths(tokz, includepaths);
		retval=parse_config_tokz(tokz, opts);
		tokz_close(tokz);
	}
	
	if(tmp!=NULL)
		free(tmp);
	
	return retval;
}


bool read_config(const char* cfgfile)
{
	return do_read_config_for(cfgfile, "ion", toplevel_opts, FALSE);
}


bool read_config_for(const char *module, const ConfOpt *opts)
{
	return do_read_config_for(NULL, module, opts, TRUE);
}

