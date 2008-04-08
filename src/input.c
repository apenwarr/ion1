/*
 * ion/input.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "inputp.h"
#include "window.h"
#include "global.h"


static WThingFuntab input_funtab={deinit_input, NULL};
IMPLOBJ(WInput, WWindow, &input_funtab)


/*{{{ Scroll */


void input_scrollup(WInput *input)
{
	CALL_FUNTAB(input, WInputFuntab, scrollup_fn)(input);
}


void input_scrolldown(WInput *input)
{
	CALL_FUNTAB(input, WInputFuntab, scrolldown_fn)(input);
}


/*}}}*/


/*{{{ Draw */


void setup_input_dinfo(WInput *input, DrawInfo *dinfo)
{
	WScreen *scr=SCREEN_OF(input);
	WGRData *grdata=&(scr->grdata);
	
	dinfo->win=input->win.win;
	dinfo->gc=grdata->gc;
	dinfo->grdata=grdata;
	dinfo->colors=&(grdata->input_colors);
	dinfo->border=&(grdata->input_border);
	dinfo->font=INPUT_FONT(grdata);
}


void input_draw(WInput *input, bool complete)
{
	CALL_FUNTAB(input, WInputFuntab, draw_fn)(input, complete);
}


/*}}}*/


/*{{{ Resize */


static void input_calc_size(WInput *input, WRectangle *geom)
{
	CALL_FUNTAB(input, WInputFuntab, calcsize_fn)(input, geom);
}


void input_refit(WInput *input)
{
	WRectangle geom=input->max_geom;
	input_calc_size(input, &geom);
	input->win.geom=geom;
	XMoveResizeWindow(wglobal.dpy, input->win.win,
					  geom.x, geom.y, geom.w, geom.h);
}


void input_resize(WInput *input, WRectangle geom)
{
	input->max_geom=geom;
	input_refit(input);
}


/*}}}*/


/*{{{ Init/deinit */


bool init_input(WInput *input, WWindow *parent, WRectangle geom)
{
	Window win;
	
	input->max_geom=geom;
	input_calc_size(input, &geom);

	win=create_simple_window_bg(parent, geom.x, geom.y, geom.w, geom.h,
								GRDATA_OF(input)->input_colors.bg);

	if(!init_window((WWindow*)input, win, geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}
	
	XSelectInput(wglobal.dpy, input->win.win, INPUT_MASK);

	input->win.bindmap=&(wglobal.input_bindmap);
	
	link_thing((WThing*)parent, (WThing*)input);
	map_window((WWindow*)input);

	return TRUE;
}


void deinit_input(WInput *input)
{
	deinit_window((WWindow*)input);
}


void input_cancel(WInput *input)
{
	destroy_thing((WThing*)input);
}


/*}}}*/

