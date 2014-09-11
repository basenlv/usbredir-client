/* 
 * File: 
 *     usbredir_monitor.h
 *
 * Destription:
 *     hold on usbredir device monitor thread header file
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#ifndef _USBREDIR_MONITOR_H
#define _USBREDIR_MONITOR_H


BOOL_T usbredir_monitor_create_thread();
BOOL_T usbredir_monitor_destroy_thread();

#ifdef ENABLE_ENUMERATE_DEVICE
void usbredir_monitor_enumerate_device();
#endif

#endif
