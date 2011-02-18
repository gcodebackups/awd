#ifndef _AWDUTIL_H
#define _AWDUTIL_H

#include "awd.h"
#include "awd_types.h"

#ifdef WIN32
#define TMPPATH_MAXLEN 256
#define TMPFILE_TEMPLATE "awd.XXXXXX"
#else
#define TMPFILE_TEMPLATE "/tmp/awd.XXXXXX"
#endif

// Utility functions
awd_float64 *   awdutil_id_mtx4(awd_float64 *);
int             awdutil_mktmp(char **path);

void            awdutil_write_mtx4(int, awd_float64 *);
void            awdutil_write_varstr(int, const char *);

awd_uint16      awdutil_swapui16(awd_uint16);
awd_uint32      awdutil_swapui32(awd_uint32);
awd_float32     awdutil_swapf32(awd_float32);
awd_float64     awdutil_swapf64(awd_float64);

#endif