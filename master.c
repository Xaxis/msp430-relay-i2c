//******************************************************************************
// MSP430G2x31 - I2C Relay Controller, Master
//
// Description: This is the test firmware for the I2C master controller. It
// currently advances, by action of s2 on the launchpad (falling edge of P1.3),
// the control word from 0x00 through 0x1111 were each bit is the relay to be
// (0) opened or (1) closed.
//
//******************************************************************************

#include <msp430g2231.h>

// Variable for transmitted data
char MST_Data = 0;

// Address is 0x48 << 1 bit + 0 for Write
char SLV_Addr = 0x90;

// State variable
int I2C_State = 0;						

void main(void) {

  // Use volatile to prevent removal
  volatile unsigned int i;

  // Stop watchdog
  WDTCTL = WDTPW + WDTHOLD;

  // If calibration constants erased do not load, trap CPU!!
  if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF) {
    while(1);                          
  }

  // Set DCO
  BCSCTL1 = CALBC1_1MHZ;               
  DCOCTL = CALDCO_1MHZ;

  // P1.6 & P1.7 Pullups, others to 0
  P1OUT = 0xC0;                        
  P1OUT |= BIT3;

  // P1.3, P1.6 & P1.7 Pullups
  P1REN |= 0xC0;                       
  P1REN |= BIT3;
  P1DIR = 0xFF;

  // Unused pins as outputs. s2 as input.
  P1DIR &= ~BIT3;

  // Enable P1 interrupt for P1.3
  P1IE |= BIT3; 	   				
  P2OUT = 0;
  P2DIR = 0xFF;

  // Port & USI mode setup
  USICTL0 = USIPE6+USIPE7+USIMST+USISWRST;

  // Enable I2C mode & USI interrupt
  USICTL1 = USII2C+USIIE;

  // Setup USI clocks: SCL = SMCLK/8 (~125kHz)
  USICKCTL = USIDIV_3+USISSEL_2+USICKPL;

  // Disable automatic clear control
  USICNT |= USIIFGCC;

  // Enable USI
  USICTL0 &= ~USISWRST;

  // Clear pending flag
  USICTL1 &= ~USIIFG;

  // Clear p1.3 interrupt flag.
  P1IFG &= ~BIT3;						
  _EINT();

  while(1) {

    // Set flag and start communication
    USICTL1 |= USIIFG;

    // CPU off, await USI interrupt
    LPM0;

    // Used for IAR
    NOP();

    // Dummy delay between communication cycles
    for (i = 0; i < 5000; i++);        
  }
}

/******************************************************
 // Port1 isr
 ******************************************************/

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {

  // Disable I2C interrupt until we finish.
  USICTL1 &= ~USIIE;

  // Clear own interrupt until we finish
  P1IE &= ~BIT3;				

  if(MST_Data < 15) {

    // Increment Master data
    MST_Data++;            
  } else {
    MST_Data = 0;
  }
  P1OUT &= ~BIT0;
  __delay_cycles(1000000);
  P1OUT |= BIT0;

  // Re-enable own interrupt and clear interrupt flag.
  P1IE |= BIT3;				
  P1IFG &= ~BIT3;

  // Re-enable I2C interrupt and clear pending flag.
  USICTL1 |= USIIE;			
  USICTL1 &= ~USIIFG;
}

/******************************************************
 // USI isr
 ******************************************************/
#pragma vector = USI_VECTOR
__interrupt void USI_TXRX (void) {
  switch(I2C_State) {

    // Generate Start Condition & send address to slave
    case 0:

      // LED on: sequence start
      //P1OUT |= 0x01;          

      // Generate Start Condition...
      USISRL = 0x00;           
      USICTL0 |= USIGE+USIOE;
      USICTL0 &= ~USIGE;

      // ... and transmit address, R/W = 0
      USISRL = SLV_Addr;

      // Bit counter = 8, TX Address
      USICNT = (USICNT & 0xE0) + 0x08;

      // Go to next state: receive address (N)Ack
      I2C_State = 2;           
      break;

    // Receive Address Ack/Nack bit
    case 2:

      // SDA = input
      USICTL0 &= ~USIOE;

      // Bit counter = 1, receive (N)Ack bit
      USICNT |= 0x01;

      // Go to next state: check (N)Ack
      I2C_State = 4;           
      break;

    // Process Address Ack/Nack & handle data TX
    case 4:

      // SDA = output
      USICTL0 |= USIOE;

      // If Nack received, send stop
      if (USISRL & 0x01) {       
        USISRL = 0x00;

        // Bit counter = 1, SCL high, SDA low
        USICNT |=  0x01;

        // Go to next state: generate Stop
        I2C_State = 10;

        // Turn on LED: error
        //P1OUT |= 0x01;         

      // Ack received, TX data to slave...
      } else {

        // Load data byte
        USISRL = MST_Data;

        // Bit counter = 8, start TX
        USICNT |=  0x08;

        // Go to next state: receive data (N)Ack
        I2C_State = 6;

        // Turn off LED
        //P1OUT &= ~0x01;        
      } 
      break;

    // Receive Data Ack/Nack bit
    case 6:

      // SDA = input
      USICTL0 &= ~USIOE;

      // Bit counter = 1, receive (N)Ack bit
      USICNT |= 0x01;

      // Go to next state: check (N)Ack
      I2C_State = 8;           
      break;

    // Process Data Ack/Nack & send Stop
    case 8: 
      USICTL0 |= USIOE;

      // If Nack received...
      if (USISRL & 0x01) {

        // Turn on LED: error
        //P1OUT |= 0x01;         

      // Ack received
      } else {

        // Turn off LED
        //P1OUT &= ~0x01;        
      }
      // Send stop...
      USISRL = 0x00;

      // Bit counter = 1, SCL high, SDA low
      USICNT |=  0x01;

      // Go to next state: generate Stop
      I2C_State = 10;          
      break;

    // Generate Stop Condition
    case 10:

      // USISRL = 1 to release SDA
      USISRL = 0x0FF;

      // Transparent latch enabled
      USICTL0 |= USIGE;

      // Latch/SDA output disabled
      USICTL0 &= ~(USIGE+USIOE);

      // Reset state machine for next transmission
      I2C_State = 0;

      // Exit active for next transfer
      LPM0_EXIT;               
      break;
  }

  // Clear pending flag
  USICTL1 &= ~USIIFG;                  
}

