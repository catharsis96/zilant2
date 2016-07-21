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


#define UDP_PORT	4000
#define	UIP_DS6_DEFAULT_PREFIX	0xaaaa
// Etimer settings
#define SEND_INTERVAL	(CLOCK_SECOND * 5)

static struct etimer periodic_timer;


// UDP settings and payload structure


static struct simple_udp_connection unicast_connection;
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



// Global variables
static int value;



/*---------------------------------------------------------------------------*/

// Global variables



static int counter = 0;

// Application specific event value user for inter-process communication
static process_event_t event_data_ready;


/*---------------------------------------------------------------------------*/


static void
print_mpu_reading(int reading)
{
  if(reading < 0) {
    printf("-");
    reading = -reading;
  }

  printf("%d.%02d", reading / 100, reading % 100);
}





/*---------------------------------------------------------------------------*/

// Floor function
float my_floor(float num){
	if(num>=0.0f) return (float) ((int)num);
	else return(float)((int)num-1);
}

/*---------------------------------------------------------------------------*/

static void udp_received(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen) {
	if(datalen == sizeof(struct udp_payload)){
		struct udp_payload mesg;
		memcpy(&mesg, data, sizeof(struct udp_payload));
		printf("Data received on port %d from port %d with length %d\n", receiver_port, sender_port, datalen);
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

PROCESS(wireless_communication_process, "Wireless Communication process");
PROCESS(measure_process, "Measuring process");
AUTOSTART_PROCESSES(&measure_process, &wireless_communication_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(measure_process, ev, data)
{
	PROCESS_EXITHANDLER()  // Cleanup code (if any)
	PROCESS_BEGIN();

	static struct etimer et;
	int value;
	message.uid = 1;
	strcpy(message.name, "Yan");
	while(1) {

		//etimer_set(&et, CLOCK_SECOND);
		etimer_set(&et, SEND_INTERVAL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));



		// Read Temperature Sensor BMP280
		SENSORS_ACTIVATE(bmp_280_sensor);
		PROCESS_YIELD();
		value = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_TEMP);
		if(value != CC26XX_SENSOR_READING_ERROR) {
			message.temperature = (float)value / 100;
			//printf("001{\"uid\": %s, \"Temperature\": %ld.%02d}002\n", /*(int) message.uid*/message.name, (long) message.temperature, (unsigned) ((message.temperature-my_floor(message.temperature))*100));
		} else {
			//printf("Temperature Read Error\n");
			message.temperature = 0;
		}



		// Read Ambient Light Sensor OPT3001
		SENSORS_ACTIVATE(opt_3001_sensor);
		PROCESS_YIELD();
		value = opt_3001_sensor.value(0);
		if(value != CC26XX_SENSOR_READING_ERROR) {
			message.light = (float)value;
			//printf("001{\"uid\": %s, \"Light\": %d.%02d}002\n", /*(int) message.uid*/message.name, (long) message.light / 100, (unsigned) message.light % 100);
		} else {
			//printf("OPT: Light Read Error\n");
			message.light = 0;
		}

		// Read Humidity Sensor HDC1000
		SENSORS_ACTIVATE(hdc_1000_sensor);
		PROCESS_YIELD();
		value = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_HUMIDITY);
		if(value != CC26XX_SENSOR_READING_ERROR) {
			message.humidity = (float)value;
			//printf("001{\"uid\": %s, \"Humidity\": %d.%02d}002\n",  /*(int) message.uid*/message.name, (long) message.humidity / 100, (unsigned) message.humidity % 100);
		} else {
			//printf("HDC: Humidity Read Error\n");
			message.humidity = 0;
		}

		// Read Barometric Pressure Sensor BMP280
		SENSORS_ACTIVATE(bmp_280_sensor);
		PROCESS_YIELD();
		value = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_PRESS);
		if(value != CC26XX_SENSOR_READING_ERROR) {
			message.pressure = (float)value;
			//printf("001{\"uid\": %s, \"Pressure\": %d.%02d}002\n", /*(int) message.uid*/message.name, (long) message.pressure / 100, (unsigned) message.pressure % 100);
		} else {
			//printf("BAR: Pressure Read Error\n");
			message.pressure = 0;
		}

		printf("001{\"uid\": \"%s\", \"Temperature\": %ld.%02d, \"Light\": %d.%02d, \"Humidity\": %d.%02d, \"Pressure\": %d.%02d}002\n", /*(int) message.uid*/message.name, (long) message.temperature, (unsigned) ((message.temperature-my_floor(message.temperature))*100), (long) message.light / 100, (unsigned) message.light % 100, (long) message.humidity / 100, (unsigned) message.humidity % 100, (long) message.pressure / 100, (unsigned) message.pressure % 100);



		//printf("BAR (avg.): Pressure=%d.%02d hPa\n", bmp_pressure / 100, bmp_pressure % 100);
		//printf("HDC (avg.): Humidity=%d.%02d %%RH\n", hdc_humidity / 100, hdc_humidity % 100);
		//printf("OPT (avg.): Light=%d.%02d lux\n", opt_light / 100, opt_light % 100);


	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/



PROCESS_THREAD(wireless_communication_process, ev, data)

{
	PROCESS_EXITHANDLER()
	PROCESS_BEGIN();

	// Setting node address
	uip_ipaddr_t *my_ip_address;
	my_ip_address = set_global_address();

	// UDP register
	simple_udp_register(&unicast_connection, UDP_PORT, NULL, UDP_PORT, udp_received);

	while(1) {

		// Wait for a fixed amount of time
		etimer_set(&periodic_timer, SEND_INTERVAL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));



		// Create the message to be sent
		message.sequence_number++;


	    // Deriving the address of the node we are talking to
	    uip_ipaddr_t recipient_address;
	    uiplib_ipaddrconv("fe80::212:4b00:afe:c200", &recipient_address);

	    // Sending the message
	    if(uip_ipaddr_cmp(my_ip_address, &recipient_address)) {
	    	printf("ERROR: Sending to myself: stopping...\n");
	    	uip_debug_ipaddr_print(&recipient_address);
	    	printf(" = ");
	    	uip_debug_ipaddr_print(my_ip_address);
	    	printf("\n");
	     }
	    else {
	    	simple_udp_sendto(&unicast_connection, &message, sizeof(struct udp_payload), &recipient_address);
	        printf("Sending message number %lu to ", message.sequence_number);
	    	uip_debug_ipaddr_print(&recipient_address);
	    	printf("\n\n");
	    }

	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

