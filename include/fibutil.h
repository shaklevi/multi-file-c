#ifndef FIBTOOL_FIBUTIL_H
#define FIBTOOL_FIBUTIL_H

/* Public API of the shared library libfibutil.so */

typedef enum {
    FLOG_ERROR = 0,
    FLOG_WARN  = 1,
    FLOG_INFO  = 2,
    FLOG_DEBUG = 3
} flog_level_t;

void flog_set_level(flog_level_t level);

/** reads the FIBTOOL_LOG environment variable (error|warn|info|debug) */
void flog_init_from_env(void);

void flog_msg(flog_level_t level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/** version string baked into the shared library at build time */
const char *fibutil_version(void);

#endif /* FIBTOOL_FIBUTIL_H */
