/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#if defined ( __CC_ARM )  /* MDK ARM Compiler */
#include "lwip/sio.h"
#endif /* MDK ARM Compiler */
#include "ethernetif.h"

#include "FreeRTOS.h"
#include "task.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/* Private function prototypes -----------------------------------------------*/
static void ethernet_link_status_updated(struct netif *netif);
/* ETH Variables initialization ----------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Variables Initialization */
struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint8_t IP_ADDRESS[4];
uint8_t NETMASK_ADDRESS[4];
uint8_t GATEWAY_ADDRESS[4];

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
void MX_LWIP_Init(void)
{
	struct dhcp * pxDHCP;
	    TickType_t xTimeoutTick;
	    const ip_addr_t * pxIP;

  /* Initilialize the LwIP stack with RTOS */
  tcpip_init( NULL, NULL );

  /* add the network interface (IPv4/IPv6) with RTOS */
  netif_add(&gnetif, NULL, NULL, NULL, NULL, &ethernetif_init, &tcpip_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }

  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);

  /* Create the Ethernet link handler thread */
/* USER CODE BEGIN H7_OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */
  osThreadDef(EthLink, ethernet_link_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE *2);
  osThreadCreate (osThread(EthLink), &gnetif);
/* USER CODE END H7_OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */

/* USER CODE BEGIN 3 */
  	  //configPRINTF( ( "Getting IP address from DHCP ...\r\n" ) );
      netifapi_dhcp_start( &gnetif );
      pxDHCP = netif_dhcp_data( &gnetif );

      xTimeoutTick = xTaskGetTickCount() + 5000 * configTICK_RATE_HZ;

      while( pxDHCP->state != DHCP_STATE_BOUND &&
             xTimeoutTick > xTaskGetTickCount() )
      {
          vTaskDelay( 100 );
      }

      if( pxDHCP->state != DHCP_STATE_BOUND )
      {
          //configPRINTF( ( "DHCP failed \r\n" ) );
          //configASSERT( false );
      }
/*
      configPRINTF( ( "\r\n IPv4 Address : %u.%u.%u.%u\r\n", ( ( u8_t * ) &xNetif.ip_addr.addr )[ 0 ],
                      ( ( u8_t * ) &xNetif.ip_addr.addr )[ 1 ], ( ( u8_t * ) &xNetif.ip_addr.addr )[ 2 ], ( ( u8_t * ) &xNetif.ip_addr.addr )[ 3 ] ) );
      configPRINTF( ( "\r\n Gateway : %u.%u.%u.%u\r\n", ( ( u8_t * ) &xNetif.gw.addr )[ 0 ],
                      ( ( u8_t * ) &xNetif.gw.addr )[ 1 ], ( ( u8_t * ) &xNetif.gw.addr )[ 2 ], ( ( u8_t * ) &xNetif.gw.addr )[ 3 ] ) );

      if( ( pxIP = dns_getserver( 0 ) ) )
      {
          configPRINTF( ( "\r\n DNS : %u.%u.%u.%u\r\n", ( ( u8_t * ) &pxIP->addr )[ 0 ],
                          ( ( u8_t * ) &pxIP->addr )[ 1 ], ( ( u8_t * ) &pxIP->addr )[ 2 ], ( ( u8_t * ) &pxIP->addr )[ 3 ] ) );
      }

      configPRINTF( ( "\r\n" ) );
      */
/* USER CODE END 3 */
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

/**
  * @brief  Notify the User about the network interface config status 
  * @param  netif: the network interface
  * @retval None
  */
static void ethernet_link_status_updated(struct netif *netif) 
{
  if (netif_is_up(netif))
  {
/* USER CODE BEGIN 5 */
/* USER CODE END 5 */
  }
  else /* netif is down */
  {  
/* USER CODE BEGIN 6 */
/* USER CODE END 6 */
  } 
}

#if defined ( __CC_ARM )  /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd;

/* USER CODE BEGIN 7 */
  sd = 0; // dummy code
/* USER CODE END 7 */
	
  return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
/* USER CODE BEGIN 8 */
/* USER CODE END 8 */
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 9 */
  recved_bytes = 0; // dummy code
/* USER CODE END 9 */	
  return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 10 */
  recved_bytes = 0; // dummy code
/* USER CODE END 10 */	
  return recved_bytes;
}
#endif /* MDK ARM Compiler */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
