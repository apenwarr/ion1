/*
 * ion/winprops.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "property.h"
#include "winprops.h"


static WWinProp *winprop_list=NULL;


WWinProp *find_winprop(const char *wclass, const char *winstance)
{
	WWinProp *prop=winprop_list, *loosematch=NULL;
	int match, bestmatch=0;
	
	/* I assume there will not be that many winprops, so a naive algorithm
	 * and data structure should do. (linear search, linked list)
	 */
	
	for(; prop!=NULL; prop=prop->next){
		match=0;
		
		/* *.* -> 2
		 * *.bar -> 3
		 * foo.* -> 4
		 * foo.bar -> 5
		 */
		
		if(prop->wclass==NULL)
			match+=1;
		else if(wclass!=NULL && strcmp(prop->wclass, wclass)==0)
			match+=3;
		else
			continue;

		if(prop->winstance==NULL)
			match+=1;
		else if(winstance!=NULL && strcmp(prop->winstance, winstance)==0)
			match+=2;
		else
			continue;

		/* exact match? */
		if(match==5)
			return prop;
		
		if(match>bestmatch){
			bestmatch=match;
			loosematch=prop;
		}
	}
	
	return loosematch;
}


WWinProp *find_winprop_win(Window win)
{
	char *winstance, *wclass=NULL;
	int n, tmp;
	
	winstance=get_string_property(win, XA_WM_CLASS, &n);
	
	if(winstance==NULL)
		return NULL;
	
	tmp=strlen(winstance);
	if(tmp+1<n)
		wclass=winstance+tmp+1;

	return find_winprop(wclass, winstance);
}


void free_winprop(WWinProp *winprop)
{	
	if(winprop->prev!=NULL){
		UNLINK_ITEM(winprop_list, winprop, next, prev);
	}
	
	if(winprop->data!=NULL)
		free(winprop->data);
}


void register_winprop(WWinProp *winprop)
{
	LINK_ITEM(winprop_list, winprop, next, prev);
}

