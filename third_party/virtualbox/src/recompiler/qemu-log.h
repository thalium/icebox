#ifndef QEMU_LOG_H
#define QEMU_LOG_H

/* The deprecated global variables: */
extern FILE *logfile;
extern int loglevel;


/*
 * The new API:
 *
 */

/* Log settings checking macros: */

/* Returns true if qemu_log() will really write somewhere
 */
#ifndef VBOX
#define qemu_log_enabled() (logfile != NULL)
#else
# define qemu_log_enabled() LogIsEnabled()
#endif

/* Returns true if a bit is set in the current loglevel mask
 */
#define qemu_loglevel_mask(b) ((loglevel & (b)) != 0)


/* Logging functions: */

/* main logging function
 */
#ifndef VBOX
#define qemu_log(...) do {                 \
        if (logfile)                       \
            fprintf(logfile, ## __VA_ARGS__); \
    } while (0)
#else
# define qemu_log(...) Log((__VA_ARGS__))
#endif

/* vfprintf-like logging function
 */
#ifndef VBOX
#define qemu_log_vprintf(fmt, va) do {     \
        if (logfile)                       \
            vfprintf(logfile, fmt, va);    \
    } while (0)
#else
# define qemu_log_vprintf(fmt, va) do { \
        if (LogIsEnabled()) \
            RTLogLoggerExV(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, fmt, va); \
    } while (0)
#endif

/* log only if a bit is set on the current loglevel mask
 */
#ifndef VBOX
#define qemu_log_mask(b, ...) do {         \
        if (loglevel & (b))                \
            fprintf(logfile, ## __VA_ARGS__); \
    } while (0)
#else
# define qemu_log_mask(b, ...) do { \
        if (loglevel & (b)) \
            Log((__VA_ARGS__)); \
    } while (0)
#endif




/* Special cases: */

/* cpu_dump_state() logging functions: */
#ifndef VBOX
#define log_cpu_state(env, f) cpu_dump_state((env), logfile, fprintf, (f));
#else
#define log_cpu_state(env, f) cpu_dump_state((env), NULL, NULL, (f));
#endif
#define log_cpu_state_mask(b, env, f) do {           \
      if (loglevel & (b)) log_cpu_state((env), (f)); \
  } while (0)

/* disas() and target_disas() to logfile: */
#define log_target_disas(start, len, flags) \
        target_disas(logfile, (start), (len), (flags))
#define log_disas(start, len) \
        disas(logfile, (start), (len))

/* page_dump() output to the log file: */
#define log_page_dump() page_dump(logfile)



/* Maintenance: */

/* fflush() the log file */
#ifndef VBOX
#define qemu_log_flush() fflush(logfile)
#else
# define qemu_log_flush()           RTLogFlush(NULL)
#endif

/* Close the log file */
#ifndef VBOX
#define qemu_log_close() do { \
        fclose(logfile);      \
        logfile = NULL;       \
    } while (0)
#else
# define qemu_log_close()           do { } while (0)
#endif

/* Set up a new log file */
#ifndef VBOX
#define qemu_log_set_file(f) do { \
        logfile = (f);            \
    } while (0)
#else
# define qemu_log_set_file(f)       do { } while (0)
#endif

/* Set up a new log file, only if none is set */
#ifndef VBOX
#define qemu_log_try_set_file(f) do { \
        if (!logfile)                 \
            logfile = (f);            \
    } while (0)
#else
#define qemu_log_try_set_file(f)    do { } while (0)
#endif


#endif
