/*
 * ion/signal.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_SIGNAL_H
#define INCLUDED_SIGNAL_H

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

INTRSTRUCT(WTimer)

DECLSTRUCT(WTimer){
	struct timeval when;
	void (*handler)(WTimer *timer);
	WTimer *next;
};

#define INIT_TIMER(FUN) {{0, 0}, FUN, NULL}

extern void check_signals();
extern void trap_signals();
extern void set_timer(WTimer *timer, uint msecs);
extern void reset_timer(WTimer *timer);

#endif /* INCLUDED_SIGNAL_H */
