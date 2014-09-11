/* 
 * File: 
 *     usbredir_om.h
 *
 * Destription:
 *     hold on usbredir ctrl block data structure header file
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#ifndef _USBREDIR_OM_H
#define _USBREDIR_OM_H

BOOL_T usbredir_om_init();
void usbredir_om_reset();

void usbredir_om_set_libusbctx(void *ctx); 
void usbredir_om_get_libusbctx(void **ctx); 

void usbredir_om_set_enabled(BOOL_T enabled);
void usbredir_om_get_enabled(BOOL_T *enabled);

void usbredir_om_set_service_available(BOOL_T available);
void usbredir_om_get_service_available(BOOL_T *available);

void usbredir_om_set_serverip(UINT32_T serverip);
void usbredir_om_get_serverip(UINT32_T *serverip);

BOOL_T usbredir_om_set_serverport(UINT16_T port);
BOOL_T usbredir_om_get_serverport(UINT16_T *port);

BOOL_T usbredir_om_set_filtrules(struct usbredirfilter_rule *rules, UINT32_T rule_count);
BOOL_T usbredir_om_get_filtrules(struct usbredirfilter_rule **rules, UINT32_T *rule_count);

#endif
