/* 
 * File: 
 *     usbredir_log.c
 *
 * Destription:
 *     hold on usbredir logging API
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "usbredir_type.h"
#include "usbredir_log.h"

#define USBREDIR_DEFAULT_LOG_LEVEL LOG_ERROR
#define USBREDIR_LOG_FILE "/var/log/usbredir.log"

static FILE *logging_fp = NULL;
static USBREDIR_LOG_LEVEL_T logging_level = USBREDIR_DEFAULT_LOG_LEVEL;

void usbredir_log_init(USBREDIR_LOG_LEVEL_T log_level)
{
    logging_fp = fopen(USBREDIR_LOG_FILE, "a");
    if (NULL == logging_fp)
    {
        fprintf(stderr, "Failed to open log file %s\n", USBREDIR_LOG_FILE);
    }

    if (log_level > LOG_NONE)
        logging_level = log_level;
}

void usbredir_log_destroy()
{
    if (logging_fp)
        fclose(logging_fp);

    logging_fp = NULL;
    logging_level = USBREDIR_DEFAULT_LOG_LEVEL;
}

void usbredir_log_getlevel(int *log_level)
{
    if (NULL == log_level)
        return;

    *log_level = logging_level;
}

void usbredir_vlog(USBREDIR_LOG_LEVEL_T log_level, const char *format, va_list args)
{
    char level[32];

    switch(log_level)
    {
        case LOG_DEBUG:
            strcpy(level, "Usbredir Debug: ");
            break;
        case LOG_INFO:
            strcpy(level, "Usbredir Info: ");
            break;
        case LOG_WARN:
            strcpy(level, "Usbredir Warn: ");
            break;
        case LOG_ERROR:
            strcpy(level, "Usbredir Error: ");
            break;
        case LOG_FATAL:
            strcpy(level, "Usbredir Fatal: ");
            break;
        default:
            strcpy(level, "Usbredir Unknow: ");
            break;
    }

    fprintf(logging_fp, "%s", level);
    vfprintf(logging_fp, format, args);
    fprintf(logging_fp,"\n");
    fflush(logging_fp);
}

void usbredir_log(USBREDIR_LOG_LEVEL_T log_level, const char *format, ...)
{
    va_list args;

    if (NULL == logging_fp)
        return;

    if (log_level > logging_level)
        return;

    va_start(args, format);

    usbredir_vlog(log_level, format, args);

    va_end(args);
}

