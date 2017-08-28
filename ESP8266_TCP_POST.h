/****************************************************************
* ESP8266 TCP POST LIBRARY
* (SUPPORTS SSL)
*
* AUGUST 25 2017
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
*
* NOTE : TO USE SECURE(SSL) FEATURE, NEED TO ENABLE ssl UNDER LIBS
*        IN ESP8266 MAKEFILE
*
* REFERENCES
* ------------
*		(1) Espressif Sample Codes
*				http://espressif.com/en/support/explore/sample-codes
*
****************************************************************/

#ifndef _ESP8266_TCP_POST_H_
#define _ESP8266_TCP_POST_H_

#include "osapi.h"
#include "mem.h"
#include "ets_sys.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"

#define ESP8266_TCP_POST_DNS_MAX_TRIES      5
#define ESP8266_TCP_POST_REPLY_TIMEOUT_MS	  5000
#define ESP8266_TCP_POST_POST_STRING        "POST %s HTTP/1.1\r\nHost: %s\r\n%s"

#define ESP8266_TCP_POST_PORT_HTTP          80
#define ESP8266_TCP_POST_PORT_HTTPS         443

#define ESP8266_TCP_POST_SSL_BUFFER_SIZE    5120

//CUSTOM VARIABLE STRUCTURES/////////////////////////////
typedef enum
{
	ESP8266_TCP_POST_STATE_DNS_RESOLVED,
	ESP8266_TCP_POST_STATE_ERROR,
	ESP8266_TCP_POST_STATE_OK
} ESP8266_TCP_POST_STATE;

typedef struct
{
	char tcp_reply_packet_terminating_chars[10]; //SHOULD BE NULL TERMINATED
	uint8_t tcp_reply_http_code;
	char* reply_packet_pointer;
}ESP8266_TCP_POST_REPLY;
//END CUSTOM VARIABLE STRUCTURES/////////////////////////

//FUNCTION PROTOTYPES/////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_SetDebug(uint8_t debug_on);
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_Initialize(const char* hostname,
													                          const char* host_ip,
                          													uint16_t host_port,
                          													const char* host_path,
                                                    uint8_t ssl_on);
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_Intialize_Request_Buffer(uint32_t buffer_size, char* custom_header);
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_SetDnsServer(uint8_t num_dns, ip_addr_t* dns);
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_SetCallbackFunctions(void (*tcp_con_cb)(void*),
                    															void (*tcp_discon_cb)(void*),
                    															void (tcp_send_cb)(void*),
                    															void (tcp_recv_cb)(void*, char*, unsigned short),
                                                  void (user_data_ready_cb)(ESP8266_TCP_POST_REPLY*));

//GET PARAMETERS FUNCTIONS
const char* ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetSourceHost(void);
const char* ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetSourcePath(void);
uint16_t ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetSourcePort(void);
ESP8266_TCP_POST_STATE ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetState(void);

//CONTROL FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_ResolveHostName(void (*user_dns_cb_fn)(ip_addr_t*));
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_DoPost(char* user_post_data);

//INTERNAL CALLBACK FUNCTIONS
void ICACHE_FLASH_ATTR _esp8266_tcp_post_dns_timer_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_post_dns_found_cb(const char* name, ip_addr_t* ipAddr, void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_post_connect_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_post_disconnect_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_post_send_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_post_receive_cb(void* arg, char* pusrdata, unsigned short length);
void ICACHE_FLASH_ATTR _esp8266_tcp_post_receive_timeout_cb(void);
//END FUNCTION PROTOTYPES/////////////////////////////////
#endif
