/*
 Programa prueba AN0 = Nivel de Ruido y UART.
 */


#pragma config CPD = OFF, BOREN = OFF, IESO = OFF, DEBUG = OFF, FOSC = HS
#pragma config FCMEN = OFF, MCLRE = ON, WDTE = OFF, CP = OFF, LVP = OFF
#pragma config PWRTE = ON, BOR4V = BOR21V, WRT = OFF

#pragma intrinsic(_delay)

#define _XTAL_FREQ 20000000 // necessary for __delay_us

#include <xc.h>
#include <stdio.h>
 
int cont = 0;
int salida;

void init_CAD_NR(void)
{
    ADCON0bits.ADCS =  0b10; //TAD minimo = 1.6us & 20MHz ->  Fos/32 = 10
	
	ADCON0bits.CHS = 0b0000; //Analog Channel AN0/RA0
		
	ADCON0bits.ADON = 1; //ADC enable bit
	
	ADCON1bits.ADFM = 1; //Right justified
	
	ADCON1bits.VCFG0 = 0; //Vss
	ADCON1bits.VCFG1 = 0; //Vdd	
} 
 
void init_uart(void)
{  
  TXSTAbits.BRGH = 0;
  BAUDCTLbits.BRG16 = 0;

  // SPBRGH:SPBRG = 
  SPBRGH = 0;
  SPBRG = 32;  // 9600 baud rate with 20MHz Clock
  
  TXSTAbits.SYNC = 0; /* Asynchronous */
  TXSTAbits.TX9 = 0; /* TX 8 data bit */
  RCSTAbits.RX9 = 0; /* RX 8 data bit */

  PIE1bits.TXIE = 0; /* Disable TX interrupt */
  PIE1bits.RCIE = 0; /* Disable RX interrupt */

  RCSTAbits.SPEN = 1; /* Serial port enable */

  TXSTAbits.TXEN = 0; /* Reset transmitter */
  TXSTAbits.TXEN = 1; /* Enable transmitter */
  
}
 
void init_timer0(void)
{
	OPTION_REGbits.T0CS = 0; //Internal instruction cycle clock (F OSC/4)
	OPTION_REGbits.PSA = 0; //Prescaler is assigned to the Timer0 module
	OPTION_REGbits.PS = 0b111; //Preescalar a 1:256
	TMR0 = 157; // T = (4/Fosc) * Prescaler * (255-TMR0); 
}

 void init_portB(void)
{
    TRISA = 0xFF; //Puerto A input 
	TRISB = 0; //Puerto B output
	ANSEL = 1; //Config analog

}

 
void init_interrupcionesADC (void)
{
    INTCONbits.GIE = 1; //Enable all unmasked interrupts
    INTCONbits.T0IE = 1; //Enable TMR0 interrupt
    ADIE = 1; //Enable ADC interrupt
}
 
void __interrupt() int_pasive(void)
{	    
	TMR0 = 157;   
	if(cont == 100)
	{   
        ADCON0bits.GO = 1;
        salida = (ADRESH << 8) + ADRESL;
		PIR1bits.ADIF = 0;
        PORTB = ADRESL;
        cont = 0;
	}    
    cont ++;
    INTCONbits.T0IF=0;
}
 
/* It is needed for printf */
void putch(char c)
{ 
	while(!TRMT){
        continue;
    }
    TXREG = c;
 } 

void main(void)
{ 
  OSCCON = 0b00001000; // External cristal
  
  init_CAD_NR();
  init_uart();
  init_timer0();
  init_portB();
  init_interrupcionesADC(); 

  while(1){
    printf("%d\r\n",salida);
  
  }
 }
