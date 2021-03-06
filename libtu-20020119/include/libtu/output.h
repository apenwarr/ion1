/*
 * libtu/output.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef LIBTU_OUTPUT_H
#define LIBTU_OUTPUT_H

#include <stdarg.h>

#include "types.h"

typedef void WarnHandler(const char *);
extern WarnHandler *set_warn_handler(WarnHandler *handler);

extern void verbose(const char *p, ...);
extern void verbose_v(const char *p, va_list args);
extern void verbose_enable(bool enable);
extern int verbose_indent(int depth);

extern void warn_progname_enable(bool enable);

extern void die(const char *p, ...);
extern void die_v(const char *p, va_list args);

extern void die_obj(const char *obj, const char *p, ...);
extern void die_obj_v(const char *obj, const char *p, va_list args);
extern void die_obj_line(const char *obj, int line, const char *p, ...);
extern void die_obj_line_v(const char *obj, int line, const char *p, va_list args);

extern void die_err();
extern void die_err_obj(const char *obj);
extern void die_err_obj_line(const char *obj, int line);


extern void warn(const char *p, ...);
extern void warn_v(const char *p, va_list args);

extern void warn_obj(const char *obj, const char *p, ...);
extern void warn_obj_v(const char *obj, const char *p, va_list args);
extern void warn_obj_line(const char *obj, int line, const char *p, ...);
extern void warn_obj_line_v(const char *obj, int line, const char *p, va_list args);

extern void warn_err();
extern void warn_err_obj(const char *obj);
extern void warn_err_obj_line(const char *obj, int line);


extern char *errmsg(const char *p, ...);
extern char *errmsg_v(const char *p, va_list args);

extern char *errmsg_obj(const char *obj, const char *p, ...);
extern char *errmsg_obj_v(const char *obj, const char *p, va_list args);
extern char *errmsg_obj_line(const char *obj, int line, const char *p, ...);
extern char *errmsg_obj_line_v(const char *obj, int line, const char *p, va_list args);

extern char *errmsg_err();
extern char *errmsg_err_obj(const char *obj);
extern char *errmsg_err_obj_line(const char *obj, int line);

extern void libtu_asprintf(char **ret, const char *fmt, ...);
extern void libtu_vasprintf(char **ret, const char *fmt, va_list args);

#endif /* LIBTU_OUTPUT_H */
