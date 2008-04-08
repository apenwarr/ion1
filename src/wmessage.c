/*
 * ion/wmessage.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include "common.h"
#include "draw.h"
#include "drawp.h"
#include "font.h"
#include "global.h"
#include "event.h"
#include "thingp.h"
#include "exec.h"
#include "wmessage.h"
#include "inputp.h"

static void wmsg_calc_size(WMessage *wmsg, WRectangle *geom);
static void wmsg_draw(WMessage *wmsg, bool complete);
static void wmsg_scrollup(WMessage *edln);
static void wmsg_scrolldown(WMessage *edln);


static WInputFuntab wmsg_funtab={
	{deinit_wmsg, NULL},
	wmsg_calc_size,
	wmsg_scrollup,
	wmsg_scrolldown,
	wmsg_draw
};


IMPLOBJ(WMessage, WInput, &wmsg_funtab)


/*{{{ Sizecalc */


static void get_geom(WMessage *wmsg, DrawInfo *dinfo, bool max)
{
	if(max){
		dinfo->geom.w=wmsg->input.max_geom.w;
		dinfo->geom.h=wmsg->input.max_geom.h;
	}else{
		dinfo->geom.w=wmsg->input.win.geom.w;
		dinfo->geom.h=wmsg->input.win.geom.h;
	}
	dinfo->geom.x=0;
	dinfo->geom.y=0;
}


static void setup_wmsg_dinfo(WMessage *wmsg, DrawInfo *dinfo, bool max)
{
	setup_input_dinfo((WInput*)wmsg, dinfo);
	get_geom(wmsg, dinfo, max);
}


static void wmsg_calc_size(WMessage *wmsg, WRectangle *geom)
{
	WRectangle max_geom=*geom;
	DrawInfo dinfo_;
	int h;
	
	setup_wmsg_dinfo(wmsg, &dinfo_, TRUE);
	fit_listing(&dinfo_, &(wmsg->listing));
	
	h=INPUT_BORDER_SIZE(dinfo_.grdata)+wmsg->listing.toth;
	
	if(h>max_geom.h)
		h=max_geom.h;
	
	geom->h=h;
	geom->w=max_geom.w;
	geom->y=max_geom.y+max_geom.h-geom->h;
	geom->x=max_geom.x;
}


/*}}}*/


/*{{{ Draw */


void wmsg_draw(WMessage *wmsg, bool complete)
{
	DrawInfo dinfo_;
	setup_wmsg_dinfo(wmsg, &dinfo_, FALSE);
	draw_listing(&dinfo_, &(wmsg->listing), complete);
}


/*}}}*/


/*{{{ Scroll */


void wmsg_scrollup(WMessage *wmsg)
{
	if(scrollup_listing(&(wmsg->listing)))
		wmsg_draw(wmsg, TRUE);
}


void wmsg_scrolldown(WMessage *wmsg)
{
	if(scrolldown_listing(&(wmsg->listing)))
		wmsg_draw(wmsg, TRUE);
}

	
/*}}}*/


/*{{{ Init and deinit */


static bool init_wmsg(WMessage *wmsg,
					  WWindow *parent, WRectangle geom, char *msg)
{
	char **ptr;
	char *cmsg;
	
	cmsg=scopy(msg);
	
	if(cmsg==NULL){
		warn_err();
		return FALSE;
	}
	
	ptr=ALLOC_N(char*, 1);
	
	if(ptr==NULL){
		warn_err();
		free(cmsg);
		return FALSE;
	}
	
	*ptr=cmsg;
	
	init_listing(&(wmsg->listing));
	setup_listing(&(wmsg->listing), INPUT_FONT(GRDATA_OF(wmsg)), ptr, 1);

	if(!init_input((WInput*)wmsg, parent, geom)){
		free(cmsg);
		free(ptr);
		deinit_listing(&(wmsg->listing));
		return FALSE;
	}

	return TRUE;
}


WMessage *create_wmsg(WWindow *parent, WRectangle geom, char *msg)
{
	CREATETHING_IMPL(WMessage, wmsg, parent, (p, parent, geom, msg));
}


void deinit_wmsg(WMessage *wmsg)
{
	if(wmsg->listing.strs!=NULL)
		deinit_listing(&(wmsg->listing));

	deinit_window((WWindow*)wmsg);
}


/*}}}*/

