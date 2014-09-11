/* 
 * File: 
 *     usbredir_mgr.h
 *
 * Destription:
 *     hold on usbredir management interface API header file
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#ifndef _USBREDIR_MGR_H
#define _USBREDIR_MGR_H

BOOL_T usbredir_mgr_init();
void usbredir_mgr_destroy();

void usbredir_mgr_set_enabled(BOOL_T enabled);
void usbredir_mgr_get_enabled(BOOL_T *enabled);

void usbredir_mgr_set_service_available(BOOL_T available);
void usbredir_mgr_get_service_available(BOOL_T *available);

void usbredir_mgr_set_serverip(UINT32_T serverip);
void usbredir_mgr_get_serverip(UINT32_T *serverip);

BOOL_T usbredir_mgr_set_serverport(UINT16_T port);
BOOL_T usbredir_mgr_get_serverport(UINT16_T *port);

BOOL_T usbredir_mgr_set_filtrules(struct usbredirfilter_rule *rules, UINT32_T rule_count);
BOOL_T usbredir_mgr_get_filtrules(struct usbredirfilter_rule **rules, UINT32_T *rule_count);

#endif
