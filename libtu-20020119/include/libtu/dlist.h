/*
 * libtu/common.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef LIBTU_DLIST_H
#define LIBTU_DLIST_H

#define LINK_ITEM(LIST, ITEM, NEXT, PREV) \
	(ITEM)->NEXT=NULL;                    \
	if((LIST)==NULL){                     \
		(LIST)=(ITEM);                    \
		(ITEM)->PREV=(ITEM);              \
	}else{                                \
		(ITEM)->PREV=(LIST)->PREV;        \
		(ITEM)->PREV->NEXT=(ITEM);        \
		(LIST)->PREV=(ITEM);              \
	}


#define LINK_ITEM_FIRST(LIST, ITEM, NEXT, PREV) \
	(ITEM)->NEXT=(LIST);                        \
	if((LIST)==NULL){                           \
		(ITEM)->PREV=(ITEM);                    \
	}else{                                      \
		(ITEM)->PREV=(LIST)->PREV;              \
		(LIST)->PREV=(ITEM);                    \
	}                                           \
	(LIST)=(ITEM);


#define LINK_ITEM_LIST LINK_ITEM


#define LINK_ITEM_BEFORE(LIST, BEFORE, ITEM, NEXT, PREV) \
	(ITEM)->NEXT=(BEFORE);                               \
	(ITEM)->PREV=(BEFORE)->PREV;                         \
	(BEFORE)->PREV=(ITEM);                               \
	if((BEFORE)==(LIST))                                 \
		(LIST)=(ITEM);                                   \
	else                                                 \
		(ITEM)->PREV->NEXT=(ITEM)


#define LINK_ITEM_AFTER(LIST, AFTER, ITEM, NEXT, PREV) \
	(ITEM)->NEXT=(AFTER)->NEXT;                        \
	(ITEM)->PREV=(AFTER);                              \
	(AFTER)->NEXT=(ITEM);                              \
	if((ITEM)->NEXT==NULL)                             \
		(LIST)->PREV=(ITEM);                           \
	else                                               \
		(ITEM)->NEXT->PREV=ITEM;


#define UNLINK_ITEM(LIST, ITEM, NEXT, PREV) \
	if((ITEM)==(LIST)){                     \
		(LIST)=(ITEM)->NEXT;                \
		if((LIST)!=NULL)                    \
			(LIST)->PREV=(ITEM)->PREV;      \
	}else if((ITEM)->NEXT==NULL){           \
		(ITEM)->PREV->NEXT=NULL;            \
		(LIST)->PREV=(ITEM)->PREV;          \
	}else{                                  \
		(ITEM)->PREV->NEXT=(ITEM)->NEXT;    \
		(ITEM)->NEXT->PREV=(ITEM)->PREV;    \
	}

#endif /* LIBTU_DLIST_H */
