/*
 * ion/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <X11/Xlib.h>
#include <string.h>

#include "thing.h"
#include "frame.h"
#include "client.h"
#include "global.h"
#include "draw.h"
#include "font.h"
#include "drawp.h"


/*{{{ Primitives */


/* Draw a border at x, y with outer width w x h. Top and left 'tl' pixels
 * wide with color 'tlc' and bottom and right 'br' pixels with colors 'brc'.
 */
void do_draw_border(Window win, GC gc, int x, int y, int w, int h,
					int tl, int br, Pixel tlc, Pixel brc)
{
	XPoint points[3];
	int i=0, a=0;
	
	w--;
	h--;

	XSetForeground(wglobal.dpy, gc, tlc);

	for(i=a=0; i<tl; i++){
		points[0].x=x+i;		points[0].y=y+h-a+1;
		points[1].x=x+i;		points[1].y=y+i;
		points[2].x=x+w-a+1;	points[2].y=y+i;

		if(i<br)
			a++;
	
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}

	
	XSetForeground(wglobal.dpy, gc, brc);

	for(i=a=0; i<br; i++){	
		points[0].x=x+w-i;		points[0].y=y+a;
		points[1].x=x+w-i;		points[1].y=y+h-i;
		points[2].x=x+a+1;		points[2].y=y+h-i;
	
		if(i<tl)
			a++;
		
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}
}


void draw_border(DrawInfo *dinfo)
{
	do_draw_border(WIN, XGC, X, Y, W, H, BORDER->tl, BORDER->br, 
				   COLORS->hl, COLORS->sh);
}


void draw_box(DrawInfo *dinfo, bool fill)
{
	if(fill){
		XSetForeground(wglobal.dpy, XGC, COLORS->bg);
		XFillRectangle(wglobal.dpy, WIN, XGC, C_X, C_Y, C_W, C_H);
	}

	draw_border(dinfo);
}


void draw_textbox(DrawInfo *dinfo,
				  const char *str, int align, bool fill)
{
	int len;
	int tw, tx, ty;
	
	draw_box(dinfo, fill);

	if(str==NULL)
		return;

	len=strlen(str);
	
	if(align!=ALIGN_LEFT){
		tw=XTextWidth(FONT, str, len);
		
		if(align==ALIGN_CENTER)
			tx=I_X+I_W/2-tw/2;
		else
			tx=I_X+I_W-tw;
	}else{
		tx=I_X;
	}
	
	ty=I_Y+I_H/2-FONT_HEIGHT(FONT)/2+FONT_BASELINE(FONT);
	
	XSetForeground(wglobal.dpy, XGC, COLORS->fg);
	XDrawString(wglobal.dpy, WIN, XGC, tx, ty, str, len);
}


/* */


static void copy_masked(const WGRData *grdata, Drawable src, Drawable dst,
						int src_x, int src_y, int w, int h,
						int dst_x, int dst_y)
{

	XSetClipMask(wglobal.dpy, grdata->copy_gc, src);
	XSetClipOrigin(wglobal.dpy, grdata->copy_gc, dst_x, dst_y);
	XCopyPlane(wglobal.dpy, src, dst, grdata->copy_gc, src_x, src_y,
			   w, h, dst_x, dst_y, 1);
}


/*}}}*/


/*{{{ Frame */


void draw_frame(const WFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=GRDATA_OF(frame);
	
	dinfo->win=FRAME_WIN(frame);
	dinfo->grdata=grdata;
	dinfo->gc=grdata->gc;
	dinfo->geom=grdata->border_off;
	dinfo->geom.w+=FRAME_W(frame);
	dinfo->geom.h+=FRAME_H(frame);
	dinfo->border=&(grdata->frame_border);
	
	if(IS_ACTIVE_FRAME(frame))
		dinfo->colors=&(grdata->act_frame_colors);
	else
		dinfo->colors=&(grdata->frame_colors);
	
	if(complete)
		XClearWindow(wglobal.dpy, FRAME_WIN(frame));

/*#if 1*/
	draw_box(dinfo, FALSE);
/*#else
	draw_box(dinfo, TRUE);
#endif*/
	
	draw_frame_bar(frame, !grdata->bar_inside_frame);
}

static char *get_shortcut_label(char shortcut)
{
	static char buffer[10];

	snprintf(buffer, 10, "%c", shortcut);
	return buffer;
}

static void draw_shortcut_tab(const WFrame *frame, DrawInfo *dinfo, WGRData *grdata)
{
	if(frame->shortcut!=0){
		if(IS_ACTIVE_FRAME(frame))
			COLORS=&(grdata->act_tab_shortcut_colors);
		else
			COLORS=&(grdata->tab_shortcut_colors);
		dinfo->geom.w=FRAME_SHORTCUT_W;
		draw_textbox(dinfo, get_shortcut_label(frame->shortcut), CF_TAB_TEXT_ALIGN, TRUE);
		X+=FRAME_SHORTCUT_W+grdata->spacing;
		dinfo->geom.w=frame->tab_w;
	}
}

void draw_frame_bar(const WFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WClient *client, *next;
	WScreen *scr=SCREEN_OF(frame);
	WGRData *grdata=&(scr->grdata);
	WRectangle bg;
	
	frame_bar_geom(frame, &bg);
	
	dinfo->win=FRAME_WIN(frame);
	dinfo->grdata=grdata;
	dinfo->gc=grdata->tab_gc;
	dinfo->geom=bg;
	dinfo->border=&(grdata->tab_border);
	dinfo->font=grdata->tab_font;

	if(complete){
		if(!grdata->bar_inside_frame){
#if 1
			XSetForeground(wglobal.dpy, XGC, grdata->bgcolor);
			XFillRectangle(wglobal.dpy, WIN, XGC, X, Y,
						   bg.w, H+grdata->spacing);
#else
			XClearArea(wglobal.dpy, WIN, X, Y,
					   bg.w, H+grdata->spacing, False);
#endif
		}else{
			XClearArea(wglobal.dpy, WIN, X, Y, bg.w, H, False);
		}
	}

	/* draw shortcut tab in top-left corner? */
	if(!grdata->shortcut_corner)
		draw_shortcut_tab(frame, dinfo, grdata);

	client=FIRST_THING(frame, WClient);
	
	if(client==NULL){
		if(frame->shortcut)
			W=bg.w-FRAME_SHORTCUT_W;

		if(IS_ACTIVE_FRAME(frame))
			COLORS=&(grdata->act_tab_sel_colors);
		else
			COLORS=&(grdata->tab_sel_colors);
		draw_textbox(dinfo, "<empty frame>", CF_TAB_TEXT_ALIGN, TRUE);

		/* draw shortcut tab in top-right corner? */
		X+=frame->tab_w+grdata->spacing;
		if(grdata->shortcut_corner)
			draw_shortcut_tab(frame, dinfo, grdata);
		return;
	}

	dinfo->geom.w=frame->tab_w;
	
	for(; client!=NULL; client=next){
		next=NEXT_THING(client, WClient);
		
		if(next==NULL){
			int d=0;
			if(frame->shortcut && grdata->shortcut_corner)
				d=FRAME_SHORTCUT_W+grdata->spacing;
			W=bg.w-(X-bg.x)-d;
		}

		if(IS_ACTIVE_FRAME(frame)){
			if(client==frame->current_client)
				COLORS=&(grdata->act_tab_sel_colors);
			else
				COLORS=&(grdata->act_tab_colors);
		}else{
			if(client==frame->current_client)
				COLORS=&(grdata->tab_sel_colors);
			else
				COLORS=&(grdata->tab_colors);
		}
		
		draw_textbox(dinfo, client->label, CF_TAB_TEXT_ALIGN, TRUE);
		
		if(client->flags&CLIENT_TAGGED){
			XSetForeground(wglobal.dpy, grdata->copy_gc, COLORS->fg);
			copy_masked(grdata, grdata->stick_pixmap, WIN, 0, 0,
						grdata->stick_pixmap_w, grdata->stick_pixmap_h,
						I_X+I_W-grdata->stick_pixmap_w, I_Y);
		}

		if(client->flags&CLIENT_DRAG){
			if(grdata->bar_inside_frame){
				if(IS_ACTIVE_FRAME(frame)){
					XSetForeground(wglobal.dpy, grdata->stipple_gc,
								   grdata->act_frame_colors.bg);
				}else{
					XSetForeground(wglobal.dpy, grdata->stipple_gc,
								   grdata->frame_colors.bg);
				}
			}
			XFillRectangle(wglobal.dpy, WIN, grdata->stipple_gc, X, Y, W, H);
		}
		
		X+=W+grdata->spacing;/*frame->tab_w+grdata->spacing;*/
	}

	/* draw shortcut tab in top-right corner? */
	/*X-=grdata->spacing;*/
	if(grdata->shortcut_corner){
		/*X=bg.w-FRAME_SHORTCUT_W;*/
		draw_shortcut_tab(frame, dinfo, grdata);
	}
}


/*}}}*/


/*{{{ Moveres */


/*void draw_rubberband(WRectangle rect)
{
	XPoint fpts[5];
	WGRData *grdata=?;
	
	fpts[0].x=rect.x;
	fpts[0].y=rect.y;
	fpts[1].x=rect.x+rect.w;
	fpts[1].y=rect.y;
	fpts[2].x=rect.x+rect.w;
	fpts[2].y=rect.y+rect.h;
	fpts[3].x=rect.x;
	fpts[3].y=rect.y+rect.h;
	fpts[4].x=rect.x;
	fpts[4].y=rect.y;
	
	XDrawLines(wglobal.dpy, SCREEN->root, grdata->xor_gc, fpts, 5,
			   CoordModeOrigin);
}*/

void draw_rubberband(WScreen *scr, WRectangle rect, bool vertical)
{
	Window win=scr->root.win;
	GC gc=scr->grdata.xor_gc;
	
	if(vertical){
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x, rect.y, rect.x+rect.w, rect.y);
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x, rect.y+rect.h, rect.x+rect.w, rect.y+rect.h);
	}else{
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x, rect.y, rect.x, rect.y+rect.h);
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x+rect.w, rect.y, rect.x+rect.w, rect.y+rect.h);
	}
}


static char moveres_tmpstr[CF_MAX_MOVERES_STR_SIZE];


static void do_draw_moveres(WScreen *scr, const char *str)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=&(scr->grdata);
	
	dinfo->win=grdata->moveres_win;
	dinfo->grdata=grdata;
	dinfo->gc=grdata->tab_gc;
	dinfo->geom.x=0;
	dinfo->geom.y=0;
	dinfo->geom.w=grdata->moveres_geom.w;
	dinfo->geom.h=grdata->moveres_geom.h;
	dinfo->border=&(grdata->tab_border);
	dinfo->font=grdata->tab_font;
	dinfo->colors=&(grdata->tab_colors);

	draw_textbox(dinfo, str, ALIGN_CENTER, TRUE);
}


void set_moveres_pos(WScreen *scr, int x, int y)
{
	sprintf(moveres_tmpstr, "%+d %+d", x, y);
	do_draw_moveres(scr, moveres_tmpstr);
}


void set_moveres_size(WScreen *scr, int w, int h)
{
	sprintf(moveres_tmpstr, "%dx%d", w, h);
	do_draw_moveres(scr, moveres_tmpstr);
}


/*}}}*/


/*{{{ Tagdrag */


void draw_tabdrag(const WClient *client)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WFrame *frame=CLIENT_FRAME(client);
	WGRData *grdata=GRDATA_OF(client);
	
	if(frame==NULL)
		return;
	
	dinfo->win=grdata->tabdrag_win;
	dinfo->grdata=grdata;
	dinfo->gc=grdata->tab_gc;
	dinfo->geom.x=0;
	dinfo->geom.y=0;
	dinfo->geom.w=grdata->tabdrag_geom.w;
	dinfo->geom.h=grdata->tabdrag_geom.h;
	dinfo->border=&(grdata->tab_border);
	dinfo->font=grdata->tab_font;

	if(IS_ACTIVE_FRAME(frame)){
		if(client==frame->current_client)
			dinfo->colors=&(grdata->act_tab_sel_colors);
		else
			dinfo->colors=&(grdata->act_tab_colors);
	}else{
		if(client==frame->current_client)
			dinfo->colors=&(grdata->tab_sel_colors);
		else
			dinfo->colors=&(grdata->tab_colors);
	}

	draw_textbox(dinfo, client->label, ALIGN_CENTER, TRUE);
}


/*}}}*/


/*{{{ Precalc */


static void calc_frame_offs(WScreen *scr)
{
	WGRData *grdata=&(scr->grdata);
	const WBorder *border=&(grdata->frame_border);
	const WBorder *tab_border=&(grdata->tab_border);
	int sp=grdata->spacing;
	
	grdata->bar_h=(FONT_HEIGHT(grdata->tab_font)+
				   BORDER_TL_TOTAL(tab_border)+BORDER_BR_TOTAL(tab_border));
	
#if 1
	grdata->border_off.x=0;
	grdata->border_off.y=0;
#else
	grdata->border_off.x=sp;
	grdata->border_off.y=sp;
#endif
	grdata->border_off.w=-sp*2;
	grdata->border_off.h=-sp*2;

	grdata->bar_off=grdata->border_off; /* except that h isn't used */
	
	if(grdata->bar_inside_frame){
		grdata->bar_off.y+=BORDER_TL_TOTAL(border);
		/*B grdata->bar_off.y+=grdata->bar_h+sp*2; grdata->bar_off.y*=-1; */
		grdata->bar_off.x+=BORDER_TL_TOTAL(border);
		grdata->bar_off.w-=BORDER_TL_TOTAL(border)*2;
		/*grdata->border_off.h-=sp;*/
	}else{
		/*B*/ grdata->border_off.y+=grdata->bar_h+sp;
		grdata->border_off.h-=grdata->bar_h+sp;
	}
	
	grdata->client_off=grdata->border_off;
	grdata->client_off.x+=BORDER_TL_TOTAL(border);
	grdata->client_off.y+=BORDER_TL_TOTAL(border);
	grdata->client_off.w-=BORDER_TL_TOTAL(border)*2;
	grdata->client_off.h-=BORDER_TL_TOTAL(border)*2;
	
	if(grdata->bar_inside_frame){
		/*B*/ grdata->client_off.y+=grdata->bar_h+sp*2;
		grdata->client_off.h-=grdata->bar_h+sp*2;
	}	
}


/*}}}*/


/*{{{ Color alloc */


bool alloc_color(WScreen *scr, const char *name, Pixel *cret)
{
	XColor c;
	bool ret=FALSE;

	if(XParseColor(wglobal.dpy, scr->default_cmap, name, &c)){
		ret=XAllocColor(wglobal.dpy, scr->default_cmap, &c);
		*cret=c.pixel;
	}
	return ret;
}
	


static void free_cg(WScreen *scr, WColorGroup *cg)
{
	Pixel pixels[4];
	
	pixels[0]=cg->bg;
	pixels[1]=cg->fg;
	pixels[2]=cg->hl;
	pixels[3]=cg->sh;
	
	XFreeColors(wglobal.dpy, scr->default_cmap, pixels, 4, 0);
}

	
void setup_color_group(WScreen *scr, WColorGroup *cg,
					   Pixel hl, Pixel sh, Pixel bg, Pixel fg)
{
	free_cg(scr, cg);
					
	cg->bg=bg;
	cg->hl=hl;
	cg->sh=sh;
	cg->fg=fg;
}


/*}}}*/


/*{{{ Init */


static int max_width(XFontStruct *font, const char *str)
{
	int maxw=0, w;
	
	while(*str!='\0'){
		w=XTextWidth(font, str, 1);
		if(w>maxw)
		maxw=w;
		str++;
	}
	
	return maxw;
}


static int chars_for_num(int d)
{
	int n=0;
	
	do{
		n++;
		d/=10;
	}while(d);
	
	return n;
}


static void create_wm_windows(WScreen *scr)
{
	WGRData *grdata=&(scr->grdata);
	XSetWindowAttributes attr;
	int w, h;
	
	/* Create move/resize position/size display window */
	w=3;
	w+=chars_for_num(scr->root.geom.w);
	w+=chars_for_num(scr->root.geom.h);
	w*=max_width(grdata->font, "0123456789x+"); 	
	w+=(BORDER_TL_TOTAL(&(grdata->tab_border))+
		BORDER_BR_TOTAL(&(grdata->tab_border)));
	
	h=(FONT_HEIGHT(grdata->tab_font)+
	   BORDER_TL_TOTAL(&(grdata->tab_border))+
	   BORDER_BR_TOTAL(&(grdata->tab_border)));
	
	grdata->moveres_geom.x=CF_MOVERES_WIN_X;
	grdata->moveres_geom.y=CF_MOVERES_WIN_Y;
	grdata->moveres_geom.w=w;
	grdata->moveres_geom.h=h;
	
	grdata->moveres_win=
		create_simple_window_bg((WWindow*)scr,
								CF_MOVERES_WIN_X, CF_MOVERES_WIN_Y, w, h,
								grdata->tab_sel_colors.bg);

	/* Create tab drag window */
	grdata->tabdrag_geom.x=0;
	grdata->tabdrag_geom.y=0;
	grdata->tabdrag_geom.w=16;
	grdata->tabdrag_geom.h=16;
	
	/*
 	grdata->tabdrag_win=create_simple_window(scr->root, 0, 0, 16, 16);
	*/
	attr.save_under=True;
	attr.background_pixel=grdata->frame_colors.bg;
	grdata->tabdrag_win=
		XCreateWindow(wglobal.dpy, scr->root.win,
					  0, 0, 16, 16, 0, CopyFromParent, InputOutput,
					  CopyFromParent, CWSaveUnder|CWBackPixel, &attr);
	
	/*XSelectInput(wglobal.dpy, grdata->tabdrag_win, ExposureMask);*/
}


/* This should be called before reading the configuration file */
void preinit_graphics(WScreen *scr)
{
	WGRData *grdata=&(scr->grdata);
	Pixel black, white;
	
	black=BlackPixel(wglobal.dpy, scr->xscr);
	white=WhitePixel(wglobal.dpy, scr->xscr);

#define INIT_CG(CG, HL, SH, BG, FG) CG.bg=BG; CG.hl=HL; CG.sh=SH; CG.fg=FG;
										/* hl, sh,    bg,    fg */
	INIT_CG(grdata->tab_sel_colors, 	white, white, white, black);
	INIT_CG(grdata->tab_colors, 		white, white, black, white);
	INIT_CG(grdata->frame_colors,		white, white, black, white);

	INIT_CG(grdata->act_tab_sel_colors, black, black, white, black);
	INIT_CG(grdata->act_tab_colors, 	black, black, black, white);
	INIT_CG(grdata->act_frame_colors,	white, white, black, white);

	INIT_CG(grdata->input_colors,		white, white, black, white);
	
#undef INIT_CG

	grdata->bgcolor=black;
	grdata->selection_bgcolor=white;
	grdata->selection_fgcolor=black;
	
	grdata->shortcutc_set=FALSE;
	
#define INIT_BD(BD, TL, BR, IPAD) BD.tl=TL; BD.br=BR; BD.ipad=IPAD;
	
	INIT_BD(grdata->frame_border, 1, 1, 0);
	INIT_BD(grdata->tab_border, 1, 1, 1);
	INIT_BD(grdata->input_border, 1, 1, 1);

#undef INIT_BD

	grdata->bar_inside_frame=FALSE;
	grdata->spacing=1;
	grdata->shortcut_corner=0;
	grdata->font=NULL;
	grdata->tab_font=NULL;
}


/* This should be called after reading the configuration file */
void postinit_graphics(WScreen *scr)
{
	Display *dpy=wglobal.dpy;
	WGRData *grdata=&(scr->grdata);
	Window root=scr->root.win;

	Pixel black, white;
	XGCValues gcv;
	ulong gcvmask;
	Pixmap stipple_pixmap;
	GC tmp_gc;

	black=BlackPixel(wglobal.dpy, scr->xscr);
	white=WhitePixel(wglobal.dpy, scr->xscr);

	/* font */
	if(grdata->font==NULL)
		grdata->font=load_font(dpy, CF_FALLBACK_FONT_NAME);
	
	if(grdata->tab_font==NULL)
		grdata->tab_font=load_font(dpy, CF_FALLBACK_FONT_NAME);

	if(!grdata->shortcutc_set){
		grdata->act_tab_shortcut_colors=grdata->act_tab_colors;
		grdata->tab_shortcut_colors=grdata->tab_colors;
	}
	
	/* Create normal gc */
	gcv.line_style=LineSolid;
	gcv.line_width=1;
	gcv.join_style=JoinBevel;
	gcv.cap_style=CapButt;
	gcv.fill_style=FillSolid;
	gcv.font=grdata->font->fid;

	gcvmask=GCForeground|GCBackground|GCLineStyle|GCLineWidth|
			GCFillStyle|GCJoinStyle|GCCapStyle|GCFont;
	
	grdata->gc=XCreateGC(dpy, root, gcvmask, &gcv);

	/* Create tab gc (other font) */
	gcv.font=grdata->tab_font->fid;
	
	grdata->tab_gc=XCreateGC(dpy, root, gcvmask, &gcv);

	/* Create stipple pattern and stipple GC */
	stipple_pixmap=XCreatePixmap(dpy, root, 2, 2, 1);
	gcv.foreground=1;
	tmp_gc=XCreateGC(dpy, stipple_pixmap, GCForeground, &gcv);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 1);
	XSetForeground(dpy, tmp_gc, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 1);
	
	if(!grdata->bar_inside_frame)
		gcv.foreground=grdata->bgcolor;
	gcv.fill_style=FillStippled;
	gcv.stipple=stipple_pixmap;
	
	grdata->stipple_gc=XCreateGC(dpy, root, gcvmask|GCStipple, &gcv);
	
	XFreePixmap(dpy, stipple_pixmap);
	
	/* Create stick pixmap */
	grdata->stick_pixmap_w=7;
	grdata->stick_pixmap_h=7;
	grdata->stick_pixmap=XCreatePixmap(dpy, root, 7, 7, 1);
	
	XSetForeground(wglobal.dpy, tmp_gc, 0);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 0, 0, 7, 7);
	XSetForeground(wglobal.dpy, tmp_gc, 1);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 0, 2, 5, 2);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 3, 4, 2, 3);
	
	XFreeGC(dpy, tmp_gc);

	/* Create copy gc */
	gcv.foreground=black;
	gcv.background=white;
	gcv.line_width=2;
	grdata->copy_gc=XCreateGC(dpy, root, GCLineWidth|GCForeground|GCBackground,
							  &gcv);

	/* Create XOR gc (for resize) */
	gcv.subwindow_mode=IncludeInferiors;
	gcv.function=GXxor;
	gcv.foreground=~0L;
	gcv.fill_style=FillSolid;
	gcvmask|=GCFunction|GCSubwindowMode|GCForeground;

	grdata->xor_gc=XCreateGC(dpy, root, gcvmask, &gcv);

	/* the rest */
	calc_frame_offs(scr);
	create_wm_windows(scr);
}


/*}}}*/

