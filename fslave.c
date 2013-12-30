//******************************************************************************
//  MSP430G2x31 - I2C Relay Controller, Fraunchpad Slave
//
//   Description: This is a test version of the slave firmware for conveniently
// testing using the Fraunchpad and it's user LEDs 5-8 in place of relays.
//
//******************************************************************************


#include <msp430.h>

unsigned char RXData;

int main(void)

{

    WDTCTL = WDTPW + WDTHOLD;

    // Init SMCLK = MCLk = ACLK = 1MHz
    CSCTL0_H = 0xA5;
    CSCTL1 |= DCOFSEL0 + DCOFSEL1;          // Set max. DCO setting = 8MHz
    CSCTL2 = SELA_3 + SELS_3 + SELM_3;      // set ACLK = MCLK = DCO
    CSCTL3 = DIVA_3 + DIVS_3 + DIVM_3;      // set all dividers to 1MHz

		// Configure port 3 for LEDs
		P1DIR |= BIT4|BIT5|BIT6|BIT7;

    // Configure Pins for I2C
    P1SEL1 |= BIT6 + BIT7;                  // Pin init

    // Configure pins for LEDs 5-8
    P3DIR = BIT4+BIT5+BIT6+BIT7;

    // eUSCI configuration
    UCB0CTLW0 |= UCSWRST ;	            //Software reset enabled
    UCB0CTLW0 |= UCMODE_3  + UCSYNC;	    //I2C mode, sync mode
    UCB0I2COA0 = 0x48 + UCOAEN;   	    //own address is 0x48 + enable
    UCB0CTLW0 &=~UCSWRST;	            //clear reset register
    UCB0IE |=  UCRXIE0; 	            //receive interrupt enable

    __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
    __no_operation();
}




#pragma vector = USCI_B0_VECTOR
__interrupt void USCIB0_ISR(void)

{

   switch(__even_in_range(UCB0IV,0x1E))
    {
      case 0x00: break;                     // Vector 0: No interrupts break;
      case 0x02: break;                     // Vector 2: ALIFG break;
      case 0x04: break;                     // Vector 4: NACKIFG break;
      case 0x06: break;                     // Vector 6: STTIFG break;
      case 0x08: break;                     // Vector 8: STPIFG break;
      case 0x0a: break;                     // Vector 10: RXIFG3 break;
      case 0x0c: break;                     // Vector 14: TXIFG3 break;
      case 0x0e: break;                     // Vector 16: RXIFG2 break;
      case 0x10: break;                     // Vector 18: TXIFG2 break;
      case 0x12: break;                     // Vector 20: RXIFG1 break;
      case 0x14: break;                     // Vector 22: TXIFG1 break;
      case 0x16:
        RXData = UCB0RXBUF;                 // Get RX data
				if(RXData & BIT0) {
					P3OUT |= BIT4;
				} else P3OUT &= ~BIT4;
				if(RXData & BIT1) {
					P3OUT |= BIT5;
				} else P3OUT &= ~BIT5;
				if(RXData & BIT2) {
					P3OUT |= BIT6;
				} else P3OUT &= ~BIT6;
				if(RXData & BIT3) {
					P3OUT |= BIT7;
				} else P3OUT &= ~BIT7;
        break;                              // Vector 24: RXIFG0 break;
      case 0x18: break;                     // Vector 26: TXIFG0 break;
      case 0x1a: break;                     // Vector 28: BCNTIFG break;
      case 0x1c: break;                     // Vector 30: clock low timeout break;
      case 0x1e: break;                     // Vector 32: 9th bit break;
      default: break;
    }

}

