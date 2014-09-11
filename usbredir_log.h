/* 
 * File: 
 *     usbredir_log.h
 *
 * Destription:
 *     hold on usbredir logging API header file
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */


#ifndef _USBREDIR_LOG_H
#define _USBREDIR_LOG_H

#include <stdarg.h>

typedef enum
{
    LOG_NONE, 
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG, 
}USBREDIR_LOG_LEVEL_T;


void usbredir_log_init(USBREDIR_LOG_LEVEL_T log_level);
void usbredir_log_destroy();
void usbredir_log(USBREDIR_LOG_LEVEL_T log_level, const char *format, ...);
void usbredir_log_getlevel(int *log_level);

#define USBREDIR_LOG_DEBUG(format, ...) do{                               \
                     usbredir_log(LOG_DEBUG, format, ##__VA_ARGS__);      \
                             }while(0)

#define USBREDIR_LOG_INFO(format, ...) do{                               \
                     usbredir_log(LOG_INFO, format, ##__VA_ARGS__);      \
                             }while(0)

#define USBREDIR_LOG_WARN(format, ...) do{                               \
                     usbredir_log(LOG_WARN, format, ##__VA_ARGS__);      \
                             }while(0)

#define USBREDIR_LOG_ERROR(format, ...) do{                              \
                     usbredir_log(LOG_ERROR, format, ##__VA_ARGS__);     \
                             }while(0)

#define USBREDIR_LOG_FATAL(format, ...) do{                              \
                     usbredir_log(LOG_FATAL, format, ##__VA_ARGS__);     \
                             }while(0)

#endif /* end of _USBREDIR_LOG_H */
