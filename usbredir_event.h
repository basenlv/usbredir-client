/* 
 * File: 
 *     usbredir_event.h
 *
 * Destription:
 *     hold on usbredir event thread API header file 
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#ifndef _USBREDIR_EVENT_H
#define _USBREDIR_EVENT_H

void usbredir_event_destroy();
void usbredir_event_init();
int usbredir_event_create_thread(libusb_context *ctx, libusb_device *dev, UINT32_T serverip, UINT16_T port);
int usbredir_event_destroy_thread(libusb_context *ctx, libusb_device *dev);


#endif /* end of _USBREDIR_EVENT_H */
