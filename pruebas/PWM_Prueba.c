/*
 Prueba PWM del SMA.
 */

#pragma config CPD = OFF, BOREN = OFF, IESO = OFF, DEBUG = OFF, FOSC = HS
#pragma config FCMEN = OFF, MCLRE = ON, WDTE = OFF, CP = OFF, LVP = OFF
#pragma config PWRTE = ON, BOR4V = BOR21V, WRT = OFF

#pragma intrinsic(_delay)

#define _XTAL_FREQ 20000000 // necessary for __delay_us

#include <xc.h>
#include <stdio.h>

 
int cont = 0;
int grow = 1;
 
void init_timer2(void)
{
    T2CONbits.TOUTPS = 0b0000; //Preescalar 1:1   
	T2CONbits.TMR2ON = 1; //Enable TMR2     
    T2CONbits.T2CKPS = 0b00; //Postescalar 1:1
    
    PIE1bits.TMR2IE = 1; //Enable TMR2 to PR2 match interrupt 
    PIR1bits.TMR2IF = 0; //TMR2 to PR2 interrupt flag bit
    
    //PWM period = (PR2 + 1) * 4 * Tosc * TMR2 Prescale
    //PR2 = (PWM period * Fosc) / (4 * TMR2 Prescale) 
    //PR2 = ((1/30MHz) * 20MHz) / (4 * 1) = 166.67
    PR2 = 166; 
    
    
}

void init_PWM(void)
{
    CCP1CONbits.P1M = 0b00; //Single output, P1A modulated
    CCP1CONbits.DC1B = 0; //PWM mode
    CCP1CONbits.CCP1M = 0b1100; //PWM mode, all active-high
    CCPR1L = 0;
    
}

 void init_portC(void)
{
	TRISC = 0; //Port C output
	ANSEL = 0; //Config digital
    ANSELH = 0;

}
 
void init_interrupciones (void)
{
    INTCONbits.GIE = 1; //Enable all unmasked interrupts
    INTCONbits.PEIE = 1; //Enable Peripherical Interrupts
}

void __interrupt() PWM_pasive(void)
{
    if(PIR1bits.TMR2IF)
    {
        if(cont == 1500) //50ms => 50ms/(1/30KHz) = 1500
        {
            if(grow && CCPR1L == 166)
            {
                grow = 0;
            }
            else if(!grow && CCPR1L == 166)
            {
                grow = 1;
            }
            
            if(grow) CCPR1L++;
            else CCPR1L--;
            cont = 0;
        }
        
        cont ++;
    }
    
    PIR1bits.TMR2IF = 0;
}
 
void main(void)
{ 
  OSCCON = 0b00001000; // External cristal
  
  init_timer2();
  init_PWM();
  init_portC();
  init_interrupciones();

  while(1);
 }
