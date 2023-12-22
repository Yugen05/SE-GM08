
#pragma config CPD = OFF, BOREN = OFF, IESO = OFF, DEBUG = OFF, FOSC = HS
#pragma config FCMEN = OFF, MCLRE = ON, WDTE = OFF, CP = OFF, LVP = OFF
#pragma config PWRTE = ON, BOR4V = BOR21V, WRT = OFF

#pragma intrinsic(_delay)

#define _XTAL_FREQ 20000000 // necessary for __delay_us


#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "i2c-v2.h"

#define CO2_ADDR 0xB5
#define LUX_ADDR 0x21

int cont = 0;
int dato, final;

void init_uart(void) {
    TXSTAbits.BRGH = 0;
    BAUDCTLbits.BRG16 = 0;

    // SPBRGH:SPBRG =
    SPBRGH = 0;
    SPBRG = 32; // 9600 baud rate with 20MHz Clock

    TXSTAbits.SYNC = 0; /* Asynchronous */
    TXSTAbits.TX9 = 0; /* TX 8 data bit */
    RCSTAbits.RX9 = 0; /* RX 8 data bit */

    PIE1bits.TXIE = 0; /* Disable TX interrupt */
    PIE1bits.RCIE = 0; /* Disable RX interrupt */

    RCSTAbits.SPEN = 1; /* Serial port enable */

    TXSTAbits.TXEN = 0; /* Reset transmitter */
    TXSTAbits.TXEN = 1; /* Enable transmitter */

}

void init_timer0(void) {
    OPTION_REGbits.T0CS = 0; //Internal instruction cycle clock (F OSC/4)
    OPTION_REGbits.PSA = 0; //Prescaler is assigned to the Timer0 module
    OPTION_REGbits.PS = 0b111; //Preescalar a 1:256
    TMR0 = 157; // T = (4/Fosc) * Prescaler * (255-TMR0);
}

void init_portB(void) {
    TRISA = 0xFF; //Puerto A input
    TRISB = 0; //Puerto B output
    ANSEL = 1; //Config analog

}

/* It is needed for printf */
void putch(char c) {
    while (!TRMT) {
        continue;
    }
    TXREG = c;
}

void i2cInit() {
    SSPCONbits.SSPM = 0b1000; //I2C Master mode, clock = Fosc/(4*(SSPAD + 1))
    SSPCONbits.SSPEN = 1;
    SSPSTATbits.CKE = 0;
    SSPSTATbits.SMP = 1;
    SSPADD = 49; //SPPAD = (Fos/(CK*4)) - 1 = (20MHz / (100KHz * 4)) - 1 = 49
    TRISC3 = 1;
    TRISC4 = 1;
}

void main(void) {

    OSCCON = 0b00001000; // External cristal

    i2cInit();
    init_uart();
    init_timer0();
    init_portB();
    int aux;

    while (1) {
        char ack;
        i2c_start();
        ack = i2c_write(0x20); //Lux write addr
        ack = i2c_write(0x04);//ALS High Resolution addr
        i2c_rstart();
        dato = i2c_write(0x21);//Lux read addr
        ack = 1;
        dato = i2c_read(ack);
        ack = 0;
        aux = i2c_read(ack);
        final = (aux << 8) + dato;
        printf("%d\r\n",dato);
        i2c_stop();
    }
}

