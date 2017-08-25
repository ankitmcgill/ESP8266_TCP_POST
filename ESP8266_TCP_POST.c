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

#include "ESP8266_TCP_POST.h"

//LOCAL LIBRARY VARIABLES/////////////////////////////////////
//DEBUG RELATRED
static uint8_t _esp8266_tcp_post_debug;

//TCP RELATED
static struct espconn _esp8266_tcp_post_espconn;
static struct _esp_tcp _esp8266_tcp_post_user_tcp;

//SSL RELATED
static uint8_t _esp8266_tcp_post_ssl_on;

//IP / HOSTNAME RELATED
static const char* _esp8266_tcp_post_host_name;
static const char* _esp8266_tcp_post_host_ip;
static ip_addr_t _esp8266_tcp_post_resolved_host_ip;
static const char* _esp8266_tcp_post_host_path;
static uint16_t _esp8266_tcp_post_host_port;

//TIMER RELATED
static volatile os_timer_t _esp8266_tcp_post_reply_timeout_timer;
static volatile os_timer_t _esp8266_tcp_post_dns_timer;

//COUNTERS
static uint16_t _esp8266_tcp_post_dns_retry_count;

//TCP OBJECT STATE
static char* _esp8266_tcp_post_post_request_buffer;
static ESP8266_TCP_POST_STATE _esp8266_tcp_post_state;

//USER DATA RELATED
ESP8266_TCP_POST_REPLY _esp8266_tcp_post_reply;

//CALLBACK FUNCTION VARIABLES
static void (*_esp8266_tcp_post_dns_cb_function)(ip_addr_t*);
static void (*_esp8266_tcp_post_tcp_conn_cb)(void*);
static void (*_esp8266_tcp_post_tcp_discon_cb)(void*);
static void (*_esp8266_tcp_post_tcp_send_cb)(void*);
static void (*_esp8266_tcp_post_tcp_recv_cb)(void*, char*, unsigned short);
static void (*_esp8266_tcp_post_tcp_user_data_ready_cb)(ESP8266_TCP_POST_REPLY*);

void ICACHE_FLASH_ATTR ESP8266_TCP_POST_SetDebug(uint8_t debug_on)
{
    //SET DEBUG PRINTF ON(1) OR OFF(0)

    _esp8266_tcp_post_debug = debug_on;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_POST_Initialize(const char* hostname,
													                          const char* host_ip,
                          													uint16_t host_port,
                          													const char* host_path,
                                                    uint8_t ssl_on)
{
	//INITIALIZE TCP CONNECTION PARAMETERS
	//HOSTNAME (RESOLVED THROUGH DNS IF HOST IP = NULL)
	//HOST IP
	//HOST PORT
	//HOST PATH
	//TCP CONNECTION INTERVAL MS

	_esp8266_tcp_post_host_name = hostname;
	_esp8266_tcp_post_host_ip = host_ip;
	_esp8266_tcp_post_host_port = host_port;
	_esp8266_tcp_post_host_path = host_path;

	_esp8266_tcp_post_dns_retry_count = 0;

  _esp8266_tcp_post_ssl_on = ssl_on;

	_esp8266_tcp_post_state = ESP8266_TCP_POST_STATE_OK;

  if(_esp8266_tcp_post_debug)
	{
	    os_printf("ESP8266 : TCP POST : Initialized\n");
	}
}

void ICACHE_FLASH_ATTR ESP8266_TCP_POST_Intialize_Request_Buffer(uint32_t buffer_size)
{
	//ALLOCATE THE ESP8266 TCP POST REQUEST BUFFER

	_esp8266_tcp_post_post_request_buffer = (char*)os_zalloc(buffer_size);

	//GENERATE THE POST STRING USING HOST-NAME & HOST-PATH & USER SUPPLIED KEY-VALUE PAIRS
	os_sprintf(_esp8266_tcp_post_post_request_buffer, ESP8266_TCP_POST_POST_STRING,
			_esp8266_tcp_post_host_path, _esp8266_tcp_post_host_name);

  //ADD USER SUPPLIED KEY VALUE PAIRS TO POST STRING

	if(_esp8266_tcp_post_debug)
	{
	    os_printf("POST STRING : \n%s\n", _esp8266_tcp_post_post_request_buffer);
	}
}

void ICACHE_FLASH_ATTR ESP8266_TCP_POST_SetDnsServer(uint8_t num_dns, ip_addr_t* dns)
{
	//SET DNS SERVER RESOLVE HOSTNAME TO IP ADDRESS
	//MAX OF 2 DNS SERVER SUPPORTED (num_dns)

	if(num_dns == 1 || num_dns == 2)
	{
		espconn_dns_setserver(num_dns, dns);
	}
	return;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_POST_SetCallbackFunctions(void (*tcp_con_cb)(void*),
                    															void (*tcp_discon_cb)(void*),
                    															void (tcp_send_cb)(void*),
                    															void (tcp_recv_cb)(void*, char*, unsigned short),
                                                  void (user_data_ready_cb)(ESP8266_TCP_POST_REPLY*))
{
	//HOOK FOR THE USER TO PROVIDE CALLBACK FUNCTIONS FOR
	//VARIOUS INTERNAL TCP OPERATION
	//SET THE CALLBACK FUNCTIONS FOR THE EVENTS:
	//	(1) TCP CONNECT
	//	(2) TCP DISCONNECT
	//	(3) TCP RECONNECT
	//	(4) TCP SEND
	//	(5) TCP RECEIVE
	//IF A NULL FUNCTION POINTER IS PASSED FOR THE CB OF A PARTICULAR
	//EVENT, THE DEFAULT CALLBACK FUNCTION IS CALLED FOR THAT EVENT

  //TCP DATA READY USER CB
  _esp8266_tcp_post_tcp_user_data_ready_cb = user_data_ready_cb;

	//TCP CONNECT CB
	_esp8266_tcp_post_tcp_conn_cb = tcp_con_cb;

	//TCP DISCONNECT CB
	_esp8266_tcp_post_tcp_discon_cb = tcp_discon_cb;

	//TCP SEND CB
	_esp8266_tcp_post_tcp_send_cb = tcp_send_cb;

	//TCP RECEIVE CB
	_esp8266_tcp_post_tcp_recv_cb = tcp_recv_cb;
}

const char* ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetSourceHost(void)
{
	//RETURN HOST NAME STRING

	return _esp8266_tcp_post_host_name;
}

const char* ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetSourcePath(void)
{
	//RETURN HOST PATH STRING

	return _esp8266_tcp_post_host_path;
}

ESP8266_TCP_POST_STATE ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetState(void)
{
	//RETURN THE INTERNAL ESP8266 TCP STATE VARIABLE VALUE

	return _esp8266_tcp_post_state;
}

uint16_t ICACHE_FLASH_ATTR ESP8266_TCP_POST_GetSourcePort(void)
{
	//RETURN HOST REMOTE PORT NUMBER

	return _esp8266_tcp_post_host_port;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_POST_ResolveHostName(void (*user_dns_cb_fn)(ip_addr_t*))
{
	//RESOLVE PROVIDED HOSTNAME USING THE SUPPLIED DNS SERVER AND CALL THE USER PROVIDED
  //DNS DONE CB FUNCTION WHEN DONE

	//DONE ONLY IF THE HOSTNAME SUPPLIED IN INITIALIZATION FUNCTION IS NOT NULL. IF NULL,
  //USER SUPPLIED IP ADDRESS IS USED INSTEAD AND NO DNS REOSLUTION IS DONE

	//SET USER DNS RESOLVE CB FUNCTION
	_esp8266_tcp_post_dns_cb_function = user_dns_cb_fn;

	//SET DNS RETRY COUNTER TO ZERO
	_esp8266_tcp_post_dns_retry_count = 0;

	if(_esp8266_tcp_post_host_name != NULL)
	{
		//NEED TO DO DNS RESOLUTION

		//START THE DNS RESOLVING PROCESS AND TIMER
		struct espconn temp;
		_esp8266_tcp_post_resolved_host_ip.addr = 0;
		espconn_gethostbyname(&temp, _esp8266_tcp_post_host_name, &_esp8266_tcp_post_resolved_host_ip, _esp8266_tcp_post_dns_found_cb);
		os_timer_setfn(&_esp8266_tcp_post_dns_timer, (os_timer_func_t*)_esp8266_tcp_post_dns_timer_cb, &temp);
		os_timer_arm(&_esp8266_tcp_post_dns_timer, 1000, 0);
		return;
	}

	//NO NEED TO DO DNS RESOLUTION. USE USER SUPPLIED IP ADDRESS STRING
	_esp8266_tcp_post_resolved_host_ip.addr = ipaddr_addr(_esp8266_tcp_post_host_ip);

	_esp8266_tcp_post_state = ESP8266_TCP_POST_STATE_DNS_RESOLVED;

	//CALL USER SUPPLIED DNS RESOLVE CB FUNCTION
	(*_esp8266_tcp_post_dns_cb_function)(&_esp8266_tcp_post_resolved_host_ip);
}

void ICACHE_FLASH_ATTR ESP8266_TCP_POST_DoPost(char* user_post_data)
{
  //DO THE ACTUAL HTTP POST
  //USE THE DATA IN THE USER DATA CONTAINER TO BUILD THE ACTUAL POST STRING

  //CREATE THE FINAL POST STRING
  char* insert_location;
  if(user_post_data != NULL)
  {
    //APPEND USER POST DATA TO POST STRING
    char* insert_location = strstr(_esp8266_tcp_post_post_request_buffer, "\r\n\r\n");
    strncpy(insert_location + 4, user_post_data, strlen(user_post_data));
  }
  _esp8266_tcp_post_post_request_buffer[strlen(_esp8266_tcp_post_post_request_buffer)] = '\n';

  if(_esp8266_tcp_post_debug)
  {
      os_printf("ESP8266 : TCP POST : Post string ...\n");
      os_printf("%s\n---------\n", _esp8266_tcp_post_post_request_buffer);
  }

  _esp8266_tcp_post_espconn.proto.tcp = &_esp8266_tcp_post_user_tcp;
  _esp8266_tcp_post_espconn.type = ESPCONN_TCP;
  _esp8266_tcp_post_espconn.state = ESPCONN_NONE;

  os_memcpy(_esp8266_tcp_post_espconn.proto.tcp->remote_ip, (uint8_t*)(&_esp8266_tcp_post_resolved_host_ip.addr), 4);
	_esp8266_tcp_post_espconn.proto.tcp->remote_port = _esp8266_tcp_post_host_port;
	_esp8266_tcp_post_espconn.proto.tcp->local_port = espconn_port();

  espconn_regist_connectcb(&_esp8266_tcp_post_espconn, _esp8266_tcp_post_connect_cb);
	espconn_regist_disconcb(&_esp8266_tcp_post_espconn, _esp8266_tcp_post_disconnect_cb);

  if(_esp8266_tcp_post_ssl_on)
  {
    //SSL CONNECTION
    espconn_secure_set_size(1,ESP8266_TCP_POST_SSL_BUFFER_SIZE);
    espconn_secure_connect(&_esp8266_tcp_post_espconn);
  }
  else
  {
    //NON SSL CONNECTIONS
    espconn_connect(&_esp8266_tcp_post_espconn);
  }

  if(_esp8266_tcp_post_debug)
  {
      os_printf("ESP8266 : TCP POST : TCP connection initiated\n");
  }
}

void ICACHE_FLASH_ATTR _esp8266_tcp_post_dns_timer_cb(void* arg)
{
	//ESP8266 DNS CHECK TIMER CALLBACK FUNCTIONS
	//TIME PERIOD = 1 SEC

	//DNS TIMER CB CALLED IE. DNS RESOLUTION DID NOT WORK
	//DO ANOTHER DNS CALL AND RE-ARM THE TIMER

	_esp8266_tcp_post_dns_retry_count++;
	if(_esp8266_tcp_post_dns_retry_count == ESP8266_TCP_POST_DNS_MAX_TRIES)
	{
		//NO MORE DNS TRIES TO BE DONE
		//STOP THE DNS TIMER
		os_timer_disarm(&_esp8266_tcp_post_dns_timer);

		if(_esp8266_tcp_post_debug)
		{
		    os_printf("ESP8266 : TCP POST : DNS Max retry exceeded. DNS unsuccessfull\n");
		}

		_esp8266_tcp_post_state = ESP8266_TCP_POST_STATE_ERROR;

		//CALL USER DNS CB FUNCTION WILL NULL ARGUMENT)
		if(*_esp8266_tcp_post_dns_cb_function != NULL)
		{
			(*_esp8266_tcp_post_dns_cb_function)(NULL);
		}
		return;
	}

	if(_esp8266_tcp_post_debug)
	{
	    os_printf("ESP8266 : TCP POST : DNS resolve timer expired. Starting another timer of 1 second...\n");
	}

	struct espconn *pespconn = arg;
	espconn_gethostbyname(pespconn, _esp8266_tcp_post_host_name, &_esp8266_tcp_post_resolved_host_ip, _esp8266_tcp_post_dns_found_cb);
	os_timer_arm(&_esp8266_tcp_post_dns_timer, 1000, 0);
}

void ICACHE_FLASH_ATTR _esp8266_tcp_post_dns_found_cb(const char* name, ip_addr_t* ipAddr, void* arg)
{
	//ESP8266 TCP DNS RESOLVING DONE CALLBACK FUNCTION

	//DISABLE THE DNS TIMER
	os_timer_disarm(&_esp8266_tcp_post_dns_timer);

	if(ipAddr == NULL)
	{
		//HOST NAME COULD NOT BE RESOLVED
		if(_esp8266_tcp_post_debug)
		{
		    os_printf("ESP8266 : TCP POST : hostname : %s, could not be resolved\n", _esp8266_tcp_post_host_name);
		}

		_esp8266_tcp_post_state = ESP8266_TCP_POST_STATE_ERROR;

		//CALL USER PROVIDED DNS CB FUNCTION WITH NULL PARAMETER
		if(*_esp8266_tcp_post_dns_cb_function != NULL)
		{
			(*_esp8266_tcp_post_dns_cb_function)(NULL);
		}
		return;
	}

	//DNS GOT IP
	_esp8266_tcp_post_resolved_host_ip.addr = ipAddr->addr;
	if(_esp8266_tcp_post_debug)
	{
	    os_printf("ESP8266 : TCP POST : hostname : %s, resolved. IP = %d.%d.%d.%d\n", _esp8266_tcp_post_host_name,
																    *((uint8_t*)&_esp8266_tcp_post_resolved_host_ip.addr),
																    *((uint8_t*)&_esp8266_tcp_post_resolved_host_ip.addr + 1),
																    *((uint8_t*)&_esp8266_tcp_post_resolved_host_ip.addr + 2),
																    *((uint8_t*)&_esp8266_tcp_post_resolved_host_ip.addr + 3));
	}

	_esp8266_tcp_post_state = ESP8266_TCP_POST_STATE_DNS_RESOLVED;

	//CALL USER PROVIDED DNS CB FUNCTION
	if(*_esp8266_tcp_post_dns_cb_function != NULL)
	{
		(*_esp8266_tcp_post_dns_cb_function)(&_esp8266_tcp_post_resolved_host_ip);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_post_connect_cb(void* arg)
{
	//TCP CONNECT CALLBACK

	if(_esp8266_tcp_post_debug)
	{
	    os_printf("ESP8266 : TCP POST : TCP CONNECTED\n");
	}

	//GET THE NEW USER TCP CONNECTION
	struct espconn *pespconn = arg;

	//REGISTER SEND AND RECEIVE CALLBACKS
	espconn_regist_sentcb(pespconn, _esp8266_tcp_post_send_cb);
	espconn_regist_recvcb(pespconn, _esp8266_tcp_post_receive_cb);

	//SEND USER DATA (GET REQUEST)
	char* pbuf = (char*)os_zalloc(2*2048);
	os_sprintf(pbuf, _esp8266_tcp_post_post_request_buffer,
						pespconn->proto.tcp->remote_ip[0], pespconn->proto.tcp->remote_ip[1],
						pespconn->proto.tcp->remote_ip[2], pespconn->proto.tcp->remote_ip[3]);

	//SEND DATA AND DEALLOCATE BUFFER
  if(_esp8266_tcp_post_ssl_on)
  {
    espconn_secure_send(pespconn, pbuf, os_strlen(pbuf));
  }
  else
  {
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));
  }
	os_free(pbuf);

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_post_tcp_conn_cb != NULL)
	{
		(*_esp8266_tcp_post_tcp_conn_cb)(arg);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_post_disconnect_cb(void* arg)
{
	//TCP DISCONNECT CALLBACK

	if(_esp8266_tcp_post_debug)
	{
	    os_printf("ESP8266 : TCP POST : TCP DISCONNECTED\n");
	}

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_post_tcp_discon_cb != NULL)
	{
		(*_esp8266_tcp_post_tcp_discon_cb)(arg);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_post_send_cb(void* arg)
{
	//TCP SENT DATA SUCCESSFULLY CALLBACK

	if(_esp8266_tcp_post_debug)
	{
	    os_printf("ESP8266 : TCP POST : TCP DATA SENT\n");
	}

	//SET AND START THE TCP GET REPLY TIMEOUT TIMER
	os_timer_setfn(&_esp8266_tcp_post_reply_timeout_timer, (os_timer_func_t*)_esp8266_tcp_post_receive_timeout_cb, NULL);
	os_timer_arm(&_esp8266_tcp_post_reply_timeout_timer, ESP8266_TCP_POST_REPLY_TIMEOUT_MS, 0);
	if(_esp8266_tcp_post_debug)
	{
		os_printf("ESP8266 : TCP POST: Started 5 second reply timeout timer\n");
	}

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_post_tcp_send_cb != NULL)
	{
		(*_esp8266_tcp_post_tcp_send_cb)(arg);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_post_receive_cb(void* arg, char* pusrdata, unsigned short length)
{
	//TCP RECEIVED DATA CALLBACK

	if(_esp8266_tcp_post_debug)
	{
	    os_printf("ESP8266 : TCP POST : TCP DATA RECEIVED\n");
	}

	//PROCESS INCOMING TCP DATA

	//CHECK FOR USER DATA IN THE REPLY
	char* ptr;
	uint8_t counter = 0;

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_post_tcp_recv_cb != NULL)
	{
		(*_esp8266_tcp_post_tcp_recv_cb)(arg, pusrdata, length);
	}

	//CHECK FOR PACKET ENDING CONDITION
	if(strstr(pusrdata, _esp8266_tcp_post_reply.tcp_reply_packet_terminating_chars) != NULL)
	{
		//END OF PACKET
		//DISCONNECT TCP CONNECTION
		espconn_disconnect(&_esp8266_tcp_post_espconn);

		//STOP TCP GET REPLY TIMEOUT TIMER
		os_timer_disarm(&_esp8266_tcp_post_reply_timeout_timer);
		if(_esp8266_tcp_post_debug)
		{
			os_printf("ESP8266 : TCP POST : TCP post reply timeout timer stopped\n");
		}

		//CALL USER SPECIFIED DATA READY CALLBACK
		(*_esp8266_tcp_post_tcp_user_data_ready_cb)(&_esp8266_tcp_post_reply);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_post_receive_timeout_cb(void)
{
	//CALLBACK FOR TCP POST REPLY TIMEOUT TIMER
	//IF CALLED => TCP POST REPLY NOT RECEIVED IN SET TIME

	if(_esp8266_tcp_post_debug)
	{
		os_printf("ESP8266 : TCP POST : TCP get reply timeout !\n");
	}

	//DISCONNECT THE TCP CONNECTION TO END THE CURRENT TRANSACTION
	espconn_disconnect(&_esp8266_tcp_post_espconn);

	////CALL USER SPECIFIED DATA READY CALLBACK WITH NULL ARGUMENT
	(*_esp8266_tcp_post_tcp_user_data_ready_cb)(NULL);
}
