/*
 * myproject_rx.c
 *
 *  Created on: 20 èşë. 2016 ã.
 *      Author: Alex
 */
#include "contiki.h"  // Contiki core
#include "contiki-net.h"
#include <stdio.h>
#include "button-sensor.h"
#include "dev/leds.h"
#include "board-peripherals.h"

#include "simple-udp.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/ip/uiplib.h"

#include "net/rpl/rpl.h"


#define UDP_PORT 4000
#define UIP_DS6_DEFAULT_PREFIX 0xaaaa

// Etimer settings
#define SEND_INTERVAL		(10 * CLOCK_SECOND)

static struct simple_udp_connection unicast_connection;

// UDP payload structure
struct udp_payload{
	uint32_t	sequence_number;
	uint8_t		uid;
	float		temperature;
	float		light;
	float		humidity;
	float		pressure;
	char		name[15];
};
static struct udp_payload message;







/*---------------------------------------------------------------------------*/
PROCESS(unicast_receiver_process, "Unicast receiver example process");
AUTOSTART_PROCESSES(&unicast_receiver_process);
/*---------------------------------------------------------------------------*/

// Floor function
float my_floor(float num){
	if(num>=0.0f) return (float) ((int)num);
	else return(float)((int)num-1);
}

static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
	if(datalen == sizeof(struct udp_payload)){
		struct udp_payload mesg;
		memcpy(&mesg, data, sizeof(struct udp_payload));

		//printf("001{\"uid\": %s, \"Temperature\": %ld.%02d}002\n", /*(int) message.uid*/message.name, (long) message.temperature, (unsigned) ((message.temperature-my_floor(message.temperature))*100));
		//printf("001{\"uid\": %s, \"Light\": %d.%02d}002\n", /*(int) message.uid*/message.name, (long) message.light / 100, (unsigned) message.light % 100);
		//printf("001{\"uid\": %s, \"Humidity\": %d.%02d}002\n",  /*(int) message.uid*/message.name, (long) message.humidity / 100, (unsigned) message.humidity % 100);
		//printf("001{\"uid\": %s, \"Pressure\": %d.%02d}002\n", /*(int) message.uid*/message.name, (long) message.pressure / 100, (unsigned) message.pressure % 100);
		//printf("\\n{'uid': %s, 'Temperature': %ld.%02d}\\n", /*(int) mesg.uid*/ mesg.name, (long) mesg.temperature, (unsigned) ((mesg.temperature-my_floor(mesg.temperature))*100));
		printf("001{\"uid\": \"%s\", \"Temperature\": %ld.%02d, \"Light\": %d.%02d, \"Humidity\": %d.%02d, \"Pressure\": %d.%02d}002\n", /*(int) message.uid*/mesg.name, (long) mesg.temperature, (unsigned) ((mesg.temperature-my_floor(mesg.temperature))*100), (long) mesg.light / 100, (unsigned) mesg.light % 100, (long) mesg.humidity / 100, (unsigned) mesg.humidity % 100, (long) mesg.pressure / 100, (unsigned) mesg.pressure % 100);
		//printf("001{\"uid\": %s, \"Light\": %d.%02d}002\n", /*(int) message.uid*/message.name, (long) message.light / 100, (unsigned) message.light % 100);
		//printf("001{\"uid\": %s, \"Humidity\": %d.%02d}002\n",  /*(int) message.uid*/message.name, (long) message.humidity / 100, (unsigned) message.humidity % 100);
		//printf("001{\"uid\": %s, \"Pressure\": %d.%02d}002\n", /*(int) message.uid*/message.name, (long) message.pressure / 100, (unsigned) message.pressure % 100);


		//printf("%s\n", mesg.name);
	}
	else{
		printf("Error: data received on port %d from port %d with length %d (unknown length != %d)\n", receiver_port, sender_port, datalen, sizeof(struct udp_payload));
	}
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t *
set_global_address(void)
{

	static uip_ipaddr_t ipaddr;
	uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	printf("IPv6 addresses: ");
	uint16_t i = 0;
	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
		uint8_t state = uip_ds6_if.addr_list[i].state;
		if(uip_ds6_if.addr_list[i].isused &&
				(state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
			uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
			printf("\n");
		}
	}

	return &ipaddr;
}
/*---------------------------------------------------------------------------*/
static void
create_rpl_dag(uip_ipaddr_t *ipaddr)
{
	struct uip_ds6_addr *root_if;

	root_if = uip_ds6_addr_lookup(ipaddr);
	if(root_if != NULL) {
		rpl_dag_t *dag;
		uip_ipaddr_t prefix;

		rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
		dag = rpl_get_any_dag();
		uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
		rpl_set_prefix(dag, &prefix, 64);
		PRINTF("created a new RPL dag\n");
	} else {
		PRINTF("failed to create a new RPL DAG\n");
	}
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_receiver_process, ev, data)
{
	PROCESS_EXITHANDLER()
		  PROCESS_BEGIN();

	uip_ipaddr_t *ipaddr;

	ipaddr = set_global_address();

	create_rpl_dag(ipaddr);

	// Register simple UDP
	simple_udp_register(&unicast_connection, UDP_PORT, NULL, UDP_PORT, receiver);

	while(1) {
		PROCESS_WAIT_EVENT();
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

