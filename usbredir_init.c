/* 
 * File: 
 *     usbredir_init.c
 *
 * Destription:
 *     hold on usbredir open&close API 
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>

#include "usbredirfilter.h"
#include "usbredir_type.h"
#include "usbredir_log.h"
#include "usbredir_mgr.h"
#include "usbredir_monitor.h"
#include "usbredir_event.h"

static BOOL_T usbredir_initted = FALSE;

void usbredir_open()
{
    int log_level = LOG_INFO;
    char *log_level_string = NULL;
    const struct libusb_version *libusb_rev;

    if (TRUE == usbredir_initted){
        fprintf(stderr, "usbredir has been opened\n");
        return;
    }

    log_level_string = getenv("USBREDIR_LOG_LEVEL");
    if (log_level_string){
        log_level = atoi(log_level_string);
    }

    usbredir_log_init(log_level); 

    libusb_rev = libusb_get_version();
    USBREDIR_LOG_INFO("====================USBREDIR Init=========================");
    USBREDIR_LOG_INFO("Using libusb v%d.%d.%d.%d", libusb_rev->major, libusb_rev->minor,
                                        libusb_rev->micro, libusb_rev->nano);

    if (FALSE == usbredir_mgr_init())
        return;

    usbredir_event_init();

    usbredir_initted = TRUE;

    USBREDIR_LOG_INFO("*** usbredir opened ***");
}

void usbredir_close()
{
    if (FALSE == usbredir_initted)
        return;

    usbredir_event_destroy();

    usbredir_monitor_destroy_thread();

    usbredir_mgr_destroy();

    USBREDIR_LOG_INFO("*** usbredir closed ***");

    usbredir_log_destroy(); 

    usbredir_initted = FALSE;
}

