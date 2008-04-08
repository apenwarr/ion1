/*
 * ion/complete.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef EDLN_COMPLETE_H
#define EDLN_COMPLETE_H

#include "edln.h"

void edln_complete(Edln *edln);
bool add_to_complist_copy(char ***cp_ret, int *n, const char *nam);
bool add_to_complist(char ***cp_ret, int *n, char *nam);

#endif /* EDLN_COMPLETE_H */
