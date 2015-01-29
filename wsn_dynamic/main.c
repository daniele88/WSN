#include "msp430g2553.h"
#include <stdint.h>
#include <stdlib.h>
#include "lib/define.h"
#include "lib/utils.h"
#include "lib/queue.h"
#include "lib/g2553.h"
#include "lib/cc2500.h"
#include "lib/spi.h"

//#define MAC_SINK 0 //comment or uncomment to be the sink
#define DEBUG_0 //comment or uncomment to read from the battery meter or generate random numbers
#define DEBUG_1 //comment or uncomment to blink the led when a packet is received

/*
 * Global variables
 */

uint8_t my_beacon[BEACON_HEADER+N_SENSORS];
packet_t* txBuffer_data;
int16_t rssi;
uint8_t power=0xFF;
int beacon_loss=0, packet_send=0, next_hop=1, soc_next_hop=0, n_hop=255,k=0,n_wakeup=-1,wait_for_mac=0, my_mac, mac_not_set=1;
int window[N_SENSORS];
volatile int n_beacon_received=0,data_received,panic=1;
volatile int ok=0;

#ifdef MAC_SINK
int mac_array[N_SENSORS];
#endif
volatile short soc=100;

uint8_t cc2500_rx_callback(uint8_t*, uint8_t );
void Read_data();
void Read_i2c_dummy();
void Panic();

/********************************************************************************************************************************
 ************************************************** Main program ****************************************************************
 ********************************************************************************************************************************/

void main(void)
{
	volatile int i;
	int min, max;
#ifdef MAC_SINK
	volatile long int j=0;
#endif

	WDTCTL = WDTPW + WDTHOLD; // Stop WDT
	BCSCTL3 |= XCAP_3; //12.5pF cap- setting for 32768Hz crystal

	// Setup oscillator for 16MHz operation for cc2500
	BCSCTL1 = 0xD9; //D9 -> 11011001;

	// Wait for changes to take effect
	__delay_cycles(4000);
	LED_PxOUT &= ~(LED1); //Outputs
	LED_PxDIR = LED1; //Outputs
	BCSCTL2 = DIVS_2; // SMCLK source its freq from MCLK/8
	P2DIR |= BIT2;
	P1SEL |= BIT6 + BIT7; // Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 + BIT7; // Assign I2C pins to USCI_B0

	CCTL0 = CCIE; // CCR0 interrupt enabled
	CCR0 = 8192; // 512 -> 1 sec, 30720 -> 1 min
	TACTL = TASSEL_1 + ID_3 + MC_1; // ACLK, /8, upmode

	setup_cc2500(cc2500_rx_callback); //setup in rx mode

	cc2500_disable_addressing();

	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC; // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST; // Use SMCLK, keep SW reset
	UCB0BR0 = 12; // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x055; // Slave Address is 055h
	UCB0CTL1 &= ~UCSWRST; // Clear SW reset, resume operation
	IE2 |= UCB0RXIE + UCB0TXIE; // Enable RX, TX interrupt

	// Build packet
	/*txBuffer[0] = 6;         // Packet length n.b. for dummies -> last index!!!
	txBuffer[1] = 0x01;      // Packet source address
	txBuffer[2] = 0xFF;		// Packet destination address
	txBuffer[3] = 0xFF;		// Packet next hop address
	txBuffer[4] = 0x00;		// Packet type
	txBuffer[5] = 0x02;		// data
	txBuffer[6] = 0x06;*/

#ifdef MAC_SINK
	my_beacon[1] = MAC_SINK;
	my_beacon[4] = 0;
	for (i=0;i<N_SENSORS;i++)
		mac_array[i]=0;
#else
	my_beacon[4] = N_SENSORS+1; // number of sensors
#ifdef DEBUG_0
	srand(TAR);
	soc=rand()%30+70;
#else
	Read_i2c();
#endif
#endif
	my_beacon[3] = 	soc;
	my_beacon[2] = 0;
	my_beacon[0] = 4;

	while(1)
	{
		// I am the sink
#ifdef MAC_SINK
		__bis_SR_register(LPM3_bits + GIE); // Enter LPM3 w/interrupt
		cc2500_turn_on();
		cc2500_tx(my_beacon, my_beacon[0]+1); //change setup to tx mode
		cc2500_set_rx();
		//blink(my_beacon[3]);
#ifdef DEBUG_1
		blink(1, 30000);
#endif
		while(TAR<(WINDOW_TIME+OFFSET));
		cc2500_turn_off();
#else
		// i am a normal sensor
		if (panic) // i don't know any other sensors
			Panic();

		__bis_SR_register( LPM3_bits + GIE );   // Enable interrupts and sleep

#ifdef DEBUG_0
		Read_i2c_dummy();
#else
		Read_i2c();
#endif

		// if i am the sensor with lower battery, update beacon
		my_beacon[3] = (soc < soc_next_hop) ? soc : soc_next_hop;

		if((++n_wakeup%N_DISCOVERY)==0)
			inizialize_window(window);

		//while (TAR<OFFSET);

		get_min_max(window, &min,&max);
		if(min>1) // min!=0 && min!=1
			while(TAR<(WINDOW_TIME/N_SENSORS*(min-1)));
		cc2500_turn_on();
		/*if(min==0 || min==1)
			while (TAR<OFFSET);
		else
			while (TAR<ONE_SECOND/N_SENSORS*(min-1));*/

		for (k=min;k<=max;k++)
		{
			data_received=0;
			if (mac_not_set)
			{
				if (!wait_for_mac){
					if (min>1)
						while(TAR<(WINDOW_TIME/N_SENSORS*(min-1))+OFFSET); // + rand()%WINDOW_TIME/N_SENSORS+
					else
						while(TAR<OFFSET);
					uint8_t* dhcp_request=make_dhcp_request(next_hop);
					cc2500_tx(dhcp_request, dhcp_request[0]+1);
					cc2500_set_rx();
				}
				while(TAR<(WINDOW_TIME+OFFSET)); //wait for all the window!!
				wait_for_mac = (++wait_for_mac==3) ? 0 : wait_for_mac;
				break;
			}
			else {
				if (k==my_mac) // it's my turn to send data
				{
					//do
					//{
					cc2500_set_power(0xFF); 	// set maximum power to transmit my beacon

					cc2500_tx(my_beacon, my_beacon[0]+1); //change setup to tx mode and send my beacon
					inc_window(window, k);
					// send all data i have in my queue
					cc2500_set_power(power);  //set power as desired by other nodes
					packet_t* pointer =  txBuffer_data -> next; //First node is dummy node.
					// Iterate through the entire list
					while(pointer!=NULL)
					{
						cc2500_tx(pointer->data, pointer->data[0]+1); //change setup to tx mode and send data
						pointer = pointer -> next; //Go to the next node.
					}
					//}
					//while(TAR<(ONE_SECOND/N_SENSORS*k+OFFSET));
					cc2500_set_rx();
				}
				while(TAR<(WINDOW_TIME/N_SENSORS*k+OFFSET)); // 2048 -> one second
				if (data_received)
					inc_window(window, k);
				else if (k!=my_mac)
					dec_window(window, k);
			}
		}
		cc2500_turn_off();

		if(n_beacon_received)
		{
			n_beacon_received = 0;
			beacon_loss=0;
#ifdef DEBUG_1
			blink(1, 30000);
#endif
		}
		else
			panic = (++beacon_loss==N_BEACON_PANIC) ? 1 : 0;

		// while(TAR<(2048+10)); // wait for all the other time windows
#endif
	}
}

/********************************************************************************************************************************
 ************************************************** End main program ************************************************************
 ********************************************************************************************************************************/

// This function is called to process the received packet
uint8_t cc2500_rx_callback( uint8_t* buffer, uint8_t length )
{
	int i;
	/* Build data packet
	txBuffer[0] = 5;         // Packet length n.b. for dummies -> last index!!!
	txBuffer[1] = 0x01;      // Packet source address
	txBuffer[2] = 0xFF;		// Packet type
	txBuffer[3] = 0xFF;		// Packet next hop address
	txBuffer[4] = 0x00;		// Packet destination address
	txBuffer[5] = 0x02;		// data
	 */

	/* Build beacon packet
	txBuffer[0] = 4;         // Packet length n.b. for dummies -> last index!!!
	txBuffer[1] = 0x01;      // Packet source address
	txBuffer[2] = 0x00;		// Packet type
	txBuffer[3] = 0x10;		// soc
	txBuffer[4] = 0x10;		// num hop
	 */

#ifdef MAC_SINK
	if (buffer[1]==0x03)  // packet type dhcp_discover
	{
		//send back dhcp_response
		uint8_t dhcp_response[PACKET_SIZE];
		dhcp_response[0]=PACKET_SIZE+1;
		dhcp_response[2]=0x04; // type dhcp response
		dhcp_response[1]=buffer[0]; // ID
		int z=N_SENSORS;
		int has_mac=0;
		// has the node already have a MAC or not?
		do{
			if (mac_array[--z]==buffer[0]) {
				has_mac=1;
				break;
			}
		}
		while(z>0);

		if (has_mac)
			dhcp_response[3]=z;
		else
		{
			//find a free MAC (zero is reserved for sink)
			z=N_SENSORS;
			do{
				if (!mac_array[--z]) {
					mac_array[z]=buffer[0];
					dhcp_response[3]=z;
					break;
				}
			}
			while(z>0);
		}

		if (TAR<WINDOW_TIME/N_SENSORS)
			cc2500_tx(dhcp_response, dhcp_response[0]+1);
		else
			insert(txBuffer_data, dhcp_response, dhcp_response[0]+1); //TODO check array index!!
		cc2500_set_rx();
	}
	//__no_operation();
#endif

#ifndef MAC_SINK
	if (buffer[1]==0x00) // i received a beacon
	{
		if (buffer[0] == next_hop) // beacon is from my next hop
		{
			reset_tar(WINDOW_TIME/N_SENSORS*buffer[0]+OFFSET);  // let's synchronize
			soc_next_hop = buffer[2];
			//power=buffer[4]; // my next hop tells me how to setup my transmit power
			panic = 0;
			//panic = (soc_next_hop < MIN_SOC) ? 1 : 0;
			for(i=0;i<N_SENSORS;i++) // to scan all the ack received in my next hop's beacon
				my_delete(txBuffer_data, i, buffer[BEACON_HEADER+i]); //delete all packet from my tx queue that my next hop has already received
		}
		// the packet was from someone else
		else if (buffer[3] <= my_beacon[4] || panic)
			if ((buffer[3] == my_beacon[4] && buffer[2] > soc_next_hop) || panic)
			{
				//i have found a better next hop or i am in panic so i accept everything
				reset_tar(WINDOW_TIME/N_SENSORS*buffer[0]+OFFSET); // let's synchronize
				next_hop = buffer[0];
				soc_next_hop = buffer[2];
				panic = 0;
				my_beacon[4] = ++(buffer[3]);
			}
		n_beacon_received=1;
	}
	else if (buffer[1]==0x01)// i received data
	{
		rssi= rssi_to_dbm(buffer[length]);
		if (buffer[0] == my_mac) //check if it's for me
		{
			// the packet is for me so save data, process it or do whatever you want here
			// ....
		}
		else if (buffer[2] == my_mac) //check if i am the next hop for the sender -> n.b. data is travelling to the sink!!
		{
			//i have to check if i have enough free space in my queue to put the new packet
			//if not the packet is discarded (the sender will not receive the ack so it will try again later to send me the data)
			//it's done also to be sure i don't go into another window sending too many packets when it's my turn to send data
			if (packet_send<CC2500_BUFFER_LENGTH)
			{
				insert(txBuffer_data, buffer, next_hop);
				++packet_send;
			}
		}
	}
	else if(buffer[1]==0x04 && buffer[0]==ID)
	{
		//i received my dhcp response
		my_mac=buffer[2];
		if (mac_not_set)
		{
			mac_not_set=0;
			wait_for_mac=0;
			my_beacon[1] = my_mac;
		}
	}
	else if(buffer[1]==0x03 && buffer[2]==my_mac)
		//i received a dhcp request, if i am the sender's next hop i have to propagate it to the sink
		insert(txBuffer_data, buffer, next_hop);
	data_received=1;
#endif

	return 0;
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	//P1OUT ^= LED1;                      // Toggle P1.0 using exclusive-OR
	LPM3_EXIT;
}

#pragma vector=NMI_VECTOR
__interrupt void nmi_ (void)
{
	IFG1 &= ~OFIFG; // Clear OSCFault flag
	while (IFG1 & OFIFG); // OSCFault flag still set?
	IE1 |= OFIE; // Enable Osc Fault*/
}

// USCI_B0 Data ISR
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
	if(IFG2 & UCB0TXIFG)
	{
		IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag
		UCB0TXBUF = 0x2C; 						// Move RX data to TX
	}
	else if(IFG2 & UCB0RXIFG)
	{
		IFG2 &= ~UCB0RXIFG;                     // Clear USCI_B0 TX int flag
		soc = UCB0RXBUF;
	}
}

// USCI_B0 State ISR
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
{
	if (UCB0STAT & UCNACKIFG)
	{
		UCB0CTL1 |= UCTXSTP;
		UCB0STAT &= ~UCNACKIFG;
	}
	else if (UCB0STAT & UCSTTIFG)
		UCB0STAT &= ~UCSTTIFG;
}


void Read_data()
{
	//put data read from sensor into the queue to send it
	/*
	insert(...);
	++packet_send; //increment number of packet in the queue
	 */
}

void Read_i2c_dummy()
{
	soc-=(rand()%5);
	soc= (soc<0) ? 0 : soc;
}

void Panic()
{
	int i,j,n_panic=N_PANIC_LISTEN;
	__bis_SR_register(GIE);
	while (1)
	{
		P1OUT |= 0x01;
		cc2500_turn_on();
		for (i=0;panic && i<N_PANIC_LISTEN;i++) // i try N times to receive a beacon
		{
			while(TAR!=32768 && panic);
			for (j=0;j<100;j++);  // a little delay to let TAR increment from 32768
		}
		P1OUT &= 0xFE;
		cc2500_turn_off();
		if (!panic)
			break;
		__no_operation();
		// to save battery i sleep for a while (as the time goes far and i don't receive nothing i increment n_panic to sleep more and more
		for (i=0;i<n_panic;i++)
		{
			while(TAR!=32768);
			for (j=0;j<100;j++);  // a little delay to let TAR increment from 32768
		}
		++n_panic;
	}
}
