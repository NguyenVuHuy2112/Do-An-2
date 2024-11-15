#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Set to enable TSCH security */
#ifndef WITH_SECURITY
#define WITH_SECURITY 0
#endif /* WITH_SECURITY */

/*******************************************************/
/********************* Enable TSCH *********************/
/*******************************************************/

/* Needed for CC2538 platforms only */
/* For TSCH we have to use the more accurate crystal oscillator
 * by default the RC oscillator is activated */
#define SYS_CTRL_CONF_OSC32K_USE_XTAL 1
#define WITH_RPL 1
#define RPL_BORDER_ROUTER 0

#define RPL_CONF_OF rpl_mrhof
#define RPL_CONF_INIT_LINK_METRIC RPL_INIT_LINK_METRIC_ETX



/* Needed for cc2420 platforms only */
/* Disable DCO calibration (uses timerB) */
#define DCOSYNCH_CONF_ENABLED 0
/* Enable SFD timestamps (uses timerB) */
#define CC2420_CONF_SFD_TIMESTAMPS 1

/* Enable Sixtop Implementation */
#define TSCH_CONF_WITH_SIXTOP 1

/*******************************************************/
/******************* Configure TSCH ********************/
/*******************************************************/

/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0xabcd

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

/* 6TiSCH schedule length */


#define TSCH_CONF_EB_PERIOD (4 * CLOCK_SECOND)
#define TSCH_CONF_DEFAULT_TIMESLOT_LENGTH 20000
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 7
#define TSCH_CONF_MAX_FRAME_RETRIES 5
#define QUEUEBUF_CONF_NUM 16
#define UIP_CONF_BUFFER_SIZE 256


#if WITH_SECURITY

/* Enable security */
#define LLSEC802154_CONF_ENABLED 1

#endif /* WITH_SECURITY */

#endif /* PROJECT_CONF_H_ */
