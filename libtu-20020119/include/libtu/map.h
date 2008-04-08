/*
 * libtu/map.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef LIBTU_MAP_H
#define LIBTU_MAP_H

typedef struct _StringIntMap{
	const char *string;
	int value;
} StringIntMap;

#define END_STRINGINTMAP {NULL, 0}

/* Return the index of str in map or -1 if not found. */
extern int stringintmap_ndx(const StringIntMap *map, const char *str);
					 
#endif /* LIBTU_MAP_H */
