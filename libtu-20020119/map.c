/*
 * libtu/map.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <libtu/map.h>


int stringintmap_ndx(const StringIntMap *map, const char *str)
{
    int i;
	
	for(i=0; map[i].string!=NULL; i++){
		if(strcmp(str, map[i].string)==0)
			return i;
	}
	
	return -1;
}

