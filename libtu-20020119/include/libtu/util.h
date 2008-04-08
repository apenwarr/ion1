/*
 * libtu/util.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef LIBTU_UTIL_H
#define LIBTU_UTIL_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "optparser.h"

extern void libtu_init(const char *argv0);
extern void libtu_init_copt(int argc, char *const argv[],
							const OptParserCommonInfo *cinfo);
					   

extern const char *prog_execname();

#endif /* LIBTU_UTIL_H */
