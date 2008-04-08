/*
 * ion/frameid.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <limits.h>

#include "common.h"
#include "frameid.h"
#include "frame.h"
#include "thing.h"
#include "property.h"
#include "global.h"
#include "client.h"


static int frame_id_cntr=FRAME_ID_START_CLIENT;
static int n_frames=0;

#define FOR_ALL_FRAMES(SCR, WS, FRAME) \
	FOR_ALL_SCREENS(SCR) \
	FOR_ALL_TYPED(SCR, WS, WWorkspace) \
	FOR_ALL_TYPED(WS, FRAME, WFrame)


/*{{{ Reorder */


static void reorder_frame_ids()
{
	WScreen *scr;
	WWorkspace *ws;
	WFrame *frame;
	WClient *client;
	WClientWin *cwin;
	
	frame_id_cntr=FRAME_ID_START_CLIENT;
	
	FOR_ALL_FRAMES(scr, ws, frame){
		if(frame->frame_id<FRAME_ID_START_CLIENT)
			continue;
	
		frame->frame_id=++frame_id_cntr;

		FOR_ALL_TYPED(frame, client, WClient){
			set_client_frame_id(client, frame->frame_id);
		}
	}
}


/*}}}*/


/*{{{ Alloc, free */


int new_frame_id()
{	
	if(((frame_id_cntr-FRAME_ID_START_CLIENT)&FRAME_ID_REORDER_INTERVAL)==
	   FRAME_ID_REORDER_INTERVAL-1 || frame_id_cntr==INT_MAX)
		reorder_frame_ids();

	if(frame_id_cntr==INT_MAX){
		die("You seem to have a _lot_ of frames on your screen and there"
			"are no free frame IDs (frame_id_cntr==INT_MAX)."
			"Refusing to continue. Thou can not always win."
			"Sorry.");
	}
	
	n_frames++;
	
	return ++frame_id_cntr;
}


int use_frame_id(int id)
{
	if(id>frame_id_cntr)
		frame_id_cntr=id;
	
	if(find_frame_by_id(id)!=NULL){
		warn("Frame ID %d is already in use --- allocating new.", id);
		return new_frame_id();
	}else{
		n_frames++;
		return id;
	}
}
	   
	   
void free_frame_id(int id)
{
	if(frame_id_cntr==INT_MAX)
		reorder_frame_ids();
	
	n_frames--;
}


/*}}}*/


/*{{{ Find */


WFrame *find_frame_by_id(int id)
{
	WScreen *scr;
	WWorkspace *ws;
	WFrame *frame;
	
	if(id==0)
		return NULL;
	
	FOR_ALL_FRAMES(scr, ws, frame){
		if(frame->frame_id==id)
			return frame;
	}

	return NULL;
}


/*}}}*/

