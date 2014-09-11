
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <libusb.h>

#include "usbredirfilter.h"
#include "usbredir_type.h"
#include "usbredir_init.h"
#include "usbredir_mgr.h"

static BOOL_T running_status;
#define KEYWORD "filter"
#define FILTER_FILE "/etc/usbredir_rules.conf"

#define PORT(n) port##n

static const struct option longopts[]={
    {"server", required_argument, NULL, 's'},
    {"portlist", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

static void usage(int exit_code, char *argv0)
{
    fprintf(exit_code?stderr:stdout,
        "Usage: %s [-s --server <ip>] [-p|--portlist <port1:port2:port3:port4>]\n",
        argv0);
    exit(exit_code);
}

static void quit_handler()
{
    printf("\ncalling quit handler...\n");

    usbredir_close();

    running_status = FALSE;
}

char *clearup_string1(char *buf)
{
    int len;
    char ch;
    char *ptr;

    if (NULL == buf)
        return NULL;

    /* skip space and tab */
    while(*buf) 
    {
        if (*buf == ' ' || *buf == '\t')
            buf++;
        else
            break;        
    } 

    if (buf) 
        ptr = buf;
    else
        return NULL;

    /* skip space and tab, return and new line character  */
    len = strlen(buf);
    while(len){
        ch = buf[len-1];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'){
            buf[len-1] = '\0';
            len--;
        }
        else{
            break;
        }
    }

    return ptr;
}

char *clearup_string2(char *buf)
{
    int len;
    char ch;
    char *ptr;

    if (NULL == buf)
        return NULL;

    /* skip space and tab */
    while(*buf) 
    {
        if (*buf == ' ' || *buf == '\t' || *buf == '=' || *buf == '\"')
            buf++;
        else
            break;        
    } 

    if (buf) 
        ptr = buf;
    else
        return NULL;

    /* skip space and tab, return and new line character  */
    len = strlen(buf);
    while(len){
        ch = buf[len-1];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '=' || ch == '\"'){
            buf[len-1] = '\0';
            len--;
        }
        else{
            break;
        }
    }

    return ptr;
}

int main(int argc, char *argv[])
{
    int i;
    int r, o;
    char *endptr;
    char *delim;
    char *server;
    UINT32_T serverip;
    char *portlist;
    UINT16_T port1, port2, port3, port4;
    struct sigaction act;
    char *ptr;
    char buf[1024];
    FILE *fp = NULL;
    struct usbredirfilter_rule *rules = NULL;
    int rule_count = 0;

    struct in_addr inp;

    server = NULL;
    serverip = 0;
    port1 = port2 = port3 = port4 = 0;
    //while((o = getopt_long(argc, argv, "hp:q:m:n:", longopts, NULL)) != -1){
    while((o = getopt_long(argc, argv, "hs:p:", longopts, NULL)) != -1){
        switch(o){
            case 's':
                server = optarg;
		if (1 != inet_pton(AF_INET, server, &inp)){
		    fprintf(stderr, "Invalid value for server:%s\n", server);
		    exit(1);
		}
		serverip = inp.s_addr;
                break;
            case 'p':
                portlist = optarg;
                delim = strchr(portlist, ':'); 
                if (!delim){
		    port1 = strtol(optarg, &endptr, 10);
		    if (*endptr != '\0'){
			fprintf(stderr, "Invalid value for --portlist:%s\n", optarg);
			usage(1, argv[0]);
		    }
                }else{
                    i = 0;
                    while(1)
                    {
                        switch(++i)
                        {
                            case 1:
                                *delim = '\0'; 
				PORT(1) = strtol(portlist, &endptr, 10);
				if (*endptr != '\0'){
				    fprintf(stderr, "Invalid value for --portlist:%s\n", optarg);
				    usage(1, argv[0]);
				}
                                portlist = delim+1;
                                break;
                            case 2:
                                *delim = '\0'; 
				PORT(2) = strtol(portlist, &endptr, 10);
				if (*endptr != '\0'){
				    fprintf(stderr, "Invalid value for --portlist:%s\n", optarg);
				    usage(1, argv[0]);
				}
                                portlist = delim+1;
                                break;
                            case 3:
                                *delim = '\0'; 
				PORT(3) = strtol(portlist, &endptr, 10);
				if (*endptr != '\0'){
				    fprintf(stderr, "Invalid value for --portlist:%s\n", optarg);
				    usage(1, argv[0]);
				}
                                portlist = delim+1;
                                break;
                            case 4:
                                *delim = '\0'; 
				PORT(4) = strtol(portlist, &endptr, 10);
				if (*endptr != '\0'){
				    fprintf(stderr, "Invalid value for --portlist:%s\n", optarg);
				    usage(1, argv[0]);
				}
                                portlist = delim+1;
                                break;
                            default:
				fprintf(stderr, "Invalid value for --portlist:%s\n", optarg);
				usage(1, argv[0]);
                                break;
                        }

                        delim = strchr(portlist, ':'); 
                        if (!delim)
                        {
			    PORT(4) = strtol(portlist, &endptr, 10);
                            break;
                        } 

                    }
                  
                }
                break;
            case '?':
            case 'h':
                usage(o == '?', argv[0]); 
                break;
        }
    }

    if (optind != argc){
        fprintf(stderr, "Invalid argument\n");
        usage(1, argv[0]);
    }

    if ( serverip == 0 || NULL == server){
        fprintf(stderr, "Invalid argument\n");
        usage(1, argv[0]);
    }

    if (port1 == 0 && port2 == 0 && port3 == 0 && port4 == 0){
        fprintf(stderr, "Invalid argument\n");
        usage(1, argv[0]);
    }

    memset(&act, 0, sizeof(act));
    act.sa_handler = quit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);

    running_status = FALSE;
    usbredir_open();

    if (port1)   
        usbredir_mgr_set_serverport(port1);
    if (port2)   
        usbredir_mgr_set_serverport(port2);
    if (port3)   
        usbredir_mgr_set_serverport(port3);
    if (port4)   
        usbredir_mgr_set_serverport(port4);
    if (serverip)   
        usbredir_mgr_set_serverip(serverip);

    fp = fopen(FILTER_FILE, "r");
    if (fp){
        while(fgets(buf, sizeof(buf),fp))
        {
            if ( NULL != (ptr = clearup_string1(buf))){
                if (*ptr == '#')   /* skip comment line */
                    continue;
                else if (0 == strncmp(ptr, KEYWORD, strlen(KEYWORD)) ) 
                {
                    ptr = clearup_string2(ptr+strlen(KEYWORD));
                    if (ptr){
                        r = usbredirfilter_string_to_rules(ptr, ",", "|", &rules, &rule_count);
		        if (r){
		            printf("filter string to rules fail, %d\n", r);
			    return -1;
		        }
		        else{
			    usbredir_mgr_set_filtrules(rules, rule_count);
			    free(rules);
			    rules = NULL;
		        }
                    }
                    else{
                        printf("%s\n", ptr);
                    }
                }
                else{
                    printf("%s\n", ptr);
                }
            }

        }
    }
    else{
        printf("fopen fail.\n");
        return -1;
    }

    usbredir_mgr_set_service_available(TRUE);
    usbredir_mgr_set_enabled(TRUE);
    running_status = TRUE;

    while(running_status)
        sleep(1);

    if (rules){
        free(rules);
    }

    if (fp){
        fclose(fp);
        fp = NULL;
    }

    printf("usbredir test terminate.\n");

    return 0;
}

