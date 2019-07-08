#include "LPC17xx.h"                    // Device header
#include "RTE_Components.h"
#include CMSIS_device_header
#include "cmsis_os2.h"

#include <stdio.h>
#include <stdint.h>
#include "GPIO_LPC17xx.h"               // Keil::Device:GPIO
#include "PIN_LPC17xx.h"                // Keil::Device:PIN


#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/priv/tcp_priv.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "lwip/apps/httpd.h"


#define NET_TICK_INTERVAL_MS  10


static void net_init (void);
static void net_periodic (uint32_t localtime);
static void net_timer_tick (void const *arg);

struct netif netif;
uint32_t LocalTime = 0;
uint8_t  LEDs = 0;


static void net_timer_tick (void const *arg) {
  /* System tick timer callback function */
  LocalTime += NET_TICK_INTERVAL_MS;
}

static void net_create_timer (void) {
  osTimerId_t id;
  id = osTimerNew ((osTimerFunc_t)&net_timer_tick, osTimerPeriodic, NULL, NULL);
  osTimerStart (id, NET_TICK_INTERVAL_MS);
}

static void net_init (void) {
 struct ip4_addr ipaddr;
  struct ip4_addr netmask;
  struct ip4_addr gw;
	
	lwip_init ();
	IP4_ADDR(&gw, 192,168,1,1);
	IP4_ADDR(&ipaddr, 192,168,1,137);
	IP4_ADDR(&netmask, 255,255,255,0);

	
			netif_add(&netif, &ipaddr, &netmask,  &gw, NULL, &ethernetif_init, &ethernet_input);
			netif_set_default(&netif);
			netif_set_up(&netif);
			net_create_timer ();
}

static void net_periodic (uint32_t localtime) {
  static uint32_t TCPTimer = 0;
  static uint32_t ARPTimer = 0;
  static uint32_t LEDTimer = 0;
#if LWIP_DHCP
  static uint32_t DHCPfineTimer   = 0;
  static uint32_t DHCPcoarseTimer = 0;
#endif
  static uint32_t IPaddress = 1;
  static char buff[20];
  uint8_t ip[4];

#if LWIP_TCP
  /* TCP periodic process every 250 ms */
  if (localtime - TCPTimer >= TCP_TMR_INTERVAL) {
    TCPTimer = localtime;
    tcp_tmr();
  }
#endif
  
  /* ARP periodic process every 5s */
  if ((localtime - ARPTimer) >= ARP_TMR_INTERVAL) {
    ARPTimer = localtime;
    etharp_tmr();
  }

#if LWIP_DHCP
  /* Fine DHCP periodic process every 500ms */
  if (localtime - DHCPfineTimer >= DHCP_FINE_TIMER_MSECS) {
    DHCPfineTimer = localtime;
    dhcp_fine_tmr();
  }

  /* DHCP Coarse periodic process every 60s */
  if (localtime - DHCPcoarseTimer >= DHCP_COARSE_TIMER_MSECS) {
    DHCPcoarseTimer = localtime;
    dhcp_coarse_tmr();
  }
#endif

  /* Link check and LED blink process every 500ms */
  if ((localtime - LEDTimer) >= 500) {
    LEDTimer = localtime;
    ethernetif_check_link (&netif);
    if (netif_is_link_up(&netif)) {
//      GPIO_PinWrite(4,28,1);
      /* Print IP address on LCD display */
      if (IPaddress != netif.ip_addr.addr) {
        IPaddress = netif.ip_addr.addr;
        ip[0] = (uint8_t)(IPaddress);
        ip[1] = (uint8_t)(IPaddress >> 8);
        ip[2] = (uint8_t)(IPaddress >> 16);
        ip[3] = (uint8_t)(IPaddress >> 24);

//        GLCD_DrawString (0, 4*24, "                    ");
        sprintf (buff, "IP: %d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
//        GLCD_DrawString (0, 4*24, buff);
      }
    }
    else {
//     GPIO_PinWrite(4,28,0);
      if (IPaddress) {
        IPaddress = 0;
//        GLCD_DrawString (0, 4*24, "Link Down...        ");
      }
    }
		GPIO_PinWrite(4,28,!GPIO_PinRead(4,28));
  }
}

void app_main (void *argument) {
//  LED_Initialize ();

//  GLCD_Initialize         ();
//  GLCD_SetBackgroundColor (GLCD_COLOR_BLUE);
//  GLCD_SetForegroundColor (GLCD_COLOR_WHITE);
//  GLCD_ClearScreen        ();
//  GLCD_SetFont            (&GLCD_Font_16x24);
//  GLCD_DrawString         (0, 0*24, "RTE and LwIP");
//  GLCD_DrawString         (0, 1*24, "HTTP demo example");

  net_init ();
//  httpd_init ();

  /* Infinite loop */
  while (1) {
    /* check if any packet received */
    ethernetif_poll (&netif);
    /* handle periodic timers for LwIP */
    net_periodic (LocalTime);
  }
}


int main(void){
	
	 SystemCoreClockUpdate ();                 /* Update system core clock       */  

	GPIO_SetDir(0,7,1);
	GPIO_SetDir(4,28,1);
	
	GPIO_PinWrite(0,7,1);
	GPIO_PinWrite(4,28,0);
	
	osKernelInitialize ();
	osDelay(500);
	

	osThreadNew(app_main, NULL, NULL);    // Create application main thread
  osKernelStart();                      // Start thread execution
  for (;;) {}
	
}

