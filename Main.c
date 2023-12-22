/*
 Programa prueba AN2 = Temperatura y UART.
 */


#pragma config CPD = OFF, BOREN = OFF, IESO = OFF, DEBUG = OFF, FOSC = HS
#pragma config FCMEN = OFF, MCLRE = ON, WDTE = OFF, CP = OFF, LVP = OFF
#pragma config PWRTE = ON, BOR4V = BOR21V, WRT = OFF

#pragma intrinsic(_delay)

#define _XTAL_FREQ 20000000 // necessary for __delay_us

#include <xc.h>
#include <stdio.h>
#include "spi-master-v1.h"

#define NIVEL_RUIDO 0b0000 //AN0/RA0
#define HUMEDAD 0b0001 //AN1/RA1
#define TEMPERATURA 0b0010 //AN2/RA2

int cont = 0;
float salida;
int grow = 1;


unsigned int humidity, NR, NR_aux, NR_mayor, temp = 0;
int cont_v, cont_NR, cont_sec = 0;
int x;
char leer;


void SPI_Init_Master();
void SK9822_SendColor(uint8_t brigthness, uint8_t red, uint8_t green, uint8_t blue, int n);

void init_CAD(void) {
    ADCON0bits.ADCS = 0b10; //TAD minimo = 1.6us & 20MHz ->  Fos/32 = 10
    ADCON0bits.ADON = 1; //ADC enable bit
    ADCON1bits.ADFM = 1; //Right justified
    ADCON1bits.VCFG0 = 0; //Vss
    ADCON1bits.VCFG1 = 0; //Vdd	
}

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

unsigned int Read_ADC(char channel) {
    unsigned int salida;
    ADCON0bits.ADON = 1; //ADC enable bit
    ADCON0bits.CHS = channel;
    ADCON0bits.GO = 1;
    while (ADCON0bits.nDONE);
    salida = (uint16_t) ((ADRESH << 8) | ADRESL);
    return salida;
}

void init_interrupcionesADC(void) {
    INTCONbits.GIE = 1; //Enable all unmasked interrupts
    INTCONbits.T0IE = 1; //Enable TMR0 interrupt
    ADIE = 1; //Enable ADC interrupt
}

void init_timer2(void) {
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

void init_PWM(void) {
    CCP1CONbits.P1M = 0b00; //Single output, P1A modulated
    CCP1CONbits.DC1B = 0; //PWM mode
    CCP1CONbits.CCP1M = 0b1100; //PWM mode, all active-high
    CCPR1L = 0;

}

void init_portC(void) {
    TRISC = 0; //Port C output
    ANSEL = 0; //Config digital
    ANSELH = 0;

}

void init_interrupciones(void) {
    INTCONbits.GIE = 1; //Enable all unmasked interrupts
    INTCONbits.T0IE = 1; //Enable TMR0 interrupt
    ADIE = 1; //Enable ADC interrupt
}

void __interrupt() int_pasive(void) {
    TMR0 = 157;
    if (cont_v == 1000) {

        humidity = Read_ADC(0b0001);
        temp = Read_ADC(0b0010);

        cont_v = 0;
    }

    if (cont_NR == 200) {

        NR = Read_ADC(0b0000);
        if (NR_mayor < NR) {
            NR_mayor = NR;
        }
        cont_sec++;

        if (cont_sec == 100) {
            NR_aux = NR_mayor;
            NR_mayor = 0;
            cont_sec = 0;
        }

        cont_NR = 0;
    }

    PIR1bits.TMR2IF = 0;
    cont_v++;
    cont_NR++;
    INTCONbits.T0IF = 0;
}

/* It is needed for printf */
void putch(char c) {
    while (!TRMT) {
        continue;
    }
    TXREG = c;
}

void main(void) {
    OSCCON = 0b00001000; // External cristal

    float temp_V, tem_mV;
    int degreesC;
    int hum_percentage;
    char NR_lvl;
    int p_vent;
    uint8_t brillo, R, G, B;

    init_CAD();
    init_uart();
    init_timer0();
    init_portB();
    SPI_Init_Master();
    init_timer2();
    init_PWM();
    init_portC();
    init_interrupciones();

    p_vent = 25;
    x = (p_vent * 166) / 100;
    SK9822_SendColor(31, 150, 0, 0, 10);

    degreesC = eeprom_read(0);
    hum_percentage = eeprom_read(1);
    
    while (1) {

        printf("----------------------------------------\r\n");
        degreesC = (((temp / 1024.0)*5.0) / 0.01) - 33;
        hum_percentage = (((((humidity) / 1024.0)*5.0) - 0.826) / 0.0315);
        CCPR1L = x;

        if (degreesC > 0) {
            printf("Temperatura: %d\r\n", degreesC);
        } else {
            printf("Temperatura: SENSOR APAGADO\r\n");
        }

        if (humidity > 0) {
            printf("Humedad: %d%%\r\n", hum_percentage);
        } else {
            printf("Humedad: SENSOR APAGADO\r\n");
        }

        if (NR_mayor > 0 && NR_mayor <= 400) {
            printf("NR: RUIDO BAJO\r\n");
        } else if (NR_mayor > 400 && NR_mayor <= 900) {
            printf("NR: RUIDO MEDIO\r\n");
        } else if (NR_mayor > 900) {
            printf("NR: RUIDO ALTO\r\n");
        } else {
            printf("NR: SENSOR APAGADO\r\n");
        }
        printf("NR: %d\r\n", NR_mayor);

        printf("----------------------------------------\r\n");

        eeprom_write(0, degreesC);
        eeprom_write(1, hum_percentage);

        __delay_ms(1000);

    }
}

void SPI_Init_Master() {
    TRISC0 = 0; // SCK como salida (reloj)
    TRISC5 = 0; // SDO como salida (Maestro manda esclavo)
}

void SK9822_SendColor(uint8_t brigthness, uint8_t red, uint8_t green, uint8_t blue, int n) {

    uint8_t init = 0b11100000;
    uint8_t global = init | brigthness;
    // inicio de trama
    for (int i = 0; i < 4; i++) {
        spi_write_read((char) 0x00);
    }

    for (int i = 0; i < n; i++) {
        spi_write_read((char) global);
        spi_write_read((char) blue); // Blue
        spi_write_read((char) green); // Green
        spi_write_read((char) red); // Red
    }

    // fin de trama
    for (int i = 0; i < 4; i++) {
        spi_write_read((char) 0xFF);
    }

}
