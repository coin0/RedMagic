#ifndef LOG_H
#define LOG_H

#include "debug.h"
#include "print.h"

typedef enum {
	LOG_ERR = 0,		//0
	LOG_WARN,		//1
	LOG_INFO,		//2
	LOG_DBG,		//3

	// always insert above
	LOG_LAST
} log_level_t;

#define LOG_LEVEL LOG_DBG	// build-macro: log level for kernel msg

// following KLOG_xxx definitions differ from log_level_t
// only used for conditions
#define KLOG_ERR   (LOG_LEVEL>=LOG_ERR)
#define KLOG_WARN  (LOG_LEVEL>=LOG_WARN)
#define KLOG_INFO  (LOG_LEVEL>=LOG_INFO)
#define KLOG_DBG   (LOG_LEVEL>=LOG_DBG)

//TODO: print for now
#define log_err 	if ( KLOG_ERR ) \
				printk
#define log_warn     if ( KLOG_WARN ) \
                                printk
#define log_info     if ( KLOG_INFO ) \
                                printk
#define log_dbg      if ( KLOG_DBG ) \
                                printk

#endif
