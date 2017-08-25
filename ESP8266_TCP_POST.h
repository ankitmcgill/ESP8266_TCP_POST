/****************************************************************
* ESP8266 TCP POST LIBRARY
* (SUPPORTS SSL)
*
* AUGUST 25 2017
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
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

//CUSTOM VARIABLE STRUCTURES/////////////////////////////
//END CUSTOM VARIABLE STRUCTURES/////////////////////////

//FUNCTION PROTOTYPES/////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_TCP_POST_SetDebug(uint8_t debug_on);

//GET PARAMETERS FUNCTIONS

//CONTROL FUNCTIONS

//INTERNAL CALLBACK FUNCTIONS

//INTERNAL DATA PROCESSING FUNCTION
//END FUNCTION PROTOTYPES/////////////////////////////////
#endif
