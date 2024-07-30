/*********************************************************************
    FileName:     	Main.c
    Dependencies:	See #includes
    Processor:		PIC32MZ
    Hardware:		MainBrain32 rev 0.20
    Complier:  	    XC32 4.40
    Author:         Larry Knight 2023

    Software License Agreement:
        This software is licensed under the Apache License Agreement

    Description:
        System Clock = 250 MHz

    File Description:

    Change History:
/***********************************************************************/

#pragma config DEBUG =      OFF
#pragma config JTAGEN =     OFF
#pragma config ICESEL =     ICS_PGx2
#pragma config TRCEN =      OFF
#pragma config BOOTISA =    MIPS32
#pragma config FECCCON =    OFF_UNLOCKED
#pragma config FSLEEP =     OFF
#pragma config DBGPER =     PG_ALL
#pragma config SMCLR =      MCLR_NORM
#pragma config SOSCGAIN =   GAIN_2X
#pragma config SOSCBOOST =  ON
#pragma config POSCGAIN =   GAIN_2X
#pragma config POSCBOOST =  OFF
#pragma config EJTAGBEN =   NORMAL
#pragma config CP =         OFF

/*** DEVCFG1 ***/
#pragma config FNOSC =      SPLL
#pragma config DMTINTV =    WIN_127_128
#pragma config FSOSCEN =    OFF
#pragma config IESO =       OFF
#pragma config POSCMOD =    EC
#pragma config OSCIOFNC =   ON
#pragma config FCKSM =      CSECME
#pragma config WDTPS =      PS1048576
#pragma config WDTSPGM =    STOP
#pragma config FWDTEN =     OFF
#pragma config WINDIS =     NORMAL
#pragma config FWDTWINSZ =  WINSZ_25
#pragma config DMTCNT =     DMT31
#pragma config FDMTEN =     OFF

/*** DEVCFG2 ***/
//250 MHz Core
#pragma config FPLLIDIV =   DIV_3
#pragma config FPLLRNG =    RANGE_5_10_MHZ
#pragma config FPLLICLK =   PLL_POSC
#pragma config FPLLMULT =   MUL_60
#pragma config FPLLODIV =   DIV_2
#pragma config UPLLFSEL =   FREQ_24MHZ

/*** DEVCFG3 ***/
#pragma config USERID =     0xffff
#pragma config FMIIEN =     ON
#pragma config FETHIO =     ON
#pragma config PGL1WAY =    OFF
#pragma config PMDL1WAY =   OFF
#pragma config IOL1WAY =    OFF
#pragma config FUSBIDIO =   ON

#include <xc.h>
#include <p32xxxx.h>
#include <proc/p32mz0512efk100.h>
#include <sys/attribs.h>
#include <sys/kmem.h>
#include <stdint.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void USB_init(void);
void SystemSetup(void);
void LED_Port(int8_t led_port_data);
void I2C_init(void);

int main(void)
{       
    while(CLKSTATbits.POSCRDY == 0);
        
    SystemSetup();
    
    LED_Port(0x0);
    
    USB_init();
    
    I2C_init();
    
    while(1)
    {
        //Heart beat
        PORTAbits.RA5 = !PORTAbits.RA5;
        //LED_Port(I2C2STAT);
    }
}

//Timer 2
void __attribute__((vector(_TIMER_2_VECTOR), interrupt(ipl3srs), nomips16)) timer2_handler()
{
    // Reset interrupt flag
    IFS0bits.T2IF = 0;
}

void SystemSetup(void)
{   
	unsigned int cp0;
    
    ANSELA = 0;
    ANSELD = 0;
    ANSELF = 0;
    ANSELG = 0;
    
    TRISAbits.TRISA5 = 0;
    TRISGbits.TRISG12 = 0;
    TRISGbits.TRISG13 = 0;
    TRISGbits.TRISG14 = 0;
    TRISAbits.TRISA7 = 0;
    TRISAbits.TRISA6 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD4 = 0;
    TRISDbits.TRISD5 = 0;
    
    TRISF = 1;
    
    /* Enable pull-up resistor(s) */
    CNPUFbits.CNPUF3 = 1;       // Pull-up enables on RPF3 (To set USB as a B-Device )
        
    PRISS = 0x76543210; 
    
    __builtin_disable_interrupts(); // Disable all interrupt
    
    // Unlock Sequence
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;  

    // PB1DIV
    // Peripheral Bus 1 cannot be turned off, so there's no need to turn it on
    PB1DIVbits.PBDIV = 1; // Peripheral Bus 1 Clock Divisor Control (PBCLK1 is SYSCLK divided by 2)

    // PB2DIV
    PB2DIVbits.ON = 1; // Peripheral Bus 2 Output Clock Enable (Output clock is enabled)
    PB2DIVbits.PBDIV = 1; // Peripheral Bus 2 Clock Divisor Control (PBCLK2 is SYSCLK divided by 2)

    // PB3DIV
    PB3DIVbits.ON = 1; // Peripheral Bus 2 Output Clock Enable (Output clock is enabled)
    PB3DIVbits.PBDIV = 1; // Peripheral Bus 3 Clock Divisor Control (PBCLK3 is SYSCLK divided by 2)

    // PB4DIV
    PB4DIVbits.ON = 1; // Peripheral Bus 4 Output Clock Enable (Output clock is enabled)
    while (!PB4DIVbits.PBDIVRDY); // Wait until it is ready to write to
    PB4DIVbits.PBDIV = 0; // Peripheral Bus 4 Clock Divisor Control (PBCLK4 is SYSCLK divided by 1)

    // PB5DIV
    PB5DIVbits.ON = 1; // Peripheral Bus 5 Output Clock Enable (Output clock is enabled)
    PB5DIVbits.PBDIV = 1; // Peripheral Bus 5 Clock Divisor Control (PBCLK5 is SYSCLK divided by 2)

    // PB7DIV
    PB7DIVbits.ON = 1; // Peripheral Bus 7 Output Clock Enable (Output clock is enabled)
    PB7DIVbits.PBDIV = 0; // Peripheral Bus 7 Clock Divisor Control (PBCLK7 is SYSCLK divided by 1)

    // PB8DIV
    PB8DIVbits.ON = 1; // Peripheral Bus 8 Output Clock Enable (Output clock is enabled)
    PB8DIVbits.PBDIV = 1; // Peripheral Bus 8 Clock Divisor Control (PBCLK8 is SYSCLK divided by 2)

    // PRECON - Set up prefetch
    PRECONbits.PFMSECEN = 0;  // Flash SEC Interrupt Enable (Do not generate an interrupt when the PFMSEC bit is set)
    PRECONbits.PREFEN = 0b11; // Predictive Prefetch Enable (Enable predictive prefetch for any address)
    PRECONbits.PFMWS = 0b010; // PFM Access Time Defined in Terms of SYSCLK Wait States (Two wait states)
    
    CFGCONbits.USBSSEN = 1;

    // Set up caching
    cp0 = _mfc0(16, 0);
    cp0 &= ~0x07;
    cp0 |= 0b011; // K0 = Cacheable, non-coherent, write-back, write allocate
    _mtc0(16, 0, cp0);  

    INTCONbits.MVEC = 1; // Global interrupt enable

    // Lock 
    SYSKEY = 0x33333333;
    
    __builtin_enable_interrupts();    
}

void LED_Port(int8_t led_port_data)
{
    //LED0              PORTDbits.RD1
    //LED1              PORTDbits.RD4 
    //LED2              PORTDbits.RD5
    //LED3              PORTAbits.RA6
    //LED4              PORTAbits.RA7
    //LED5              PORTGbits.RG13
    //LED6              PORTGbits.RG14
    //LED7              PORTGbits.RG12

    uint32_t temp = led_port_data;
    
    //LED0
    temp = led_port_data & 0x01;  
    PORTDbits.RD1 = temp;
    
    //LED_1
    temp = led_port_data & 0x02;
    temp = temp >> 1;
    PORTDbits.RD4 = temp;
    
    //LED_2
    temp = led_port_data & 0x04;
    temp = temp >> 2;
    PORTDbits.RD5 = temp;
    
    //LED_3
    temp = led_port_data & 0x08;
    temp = temp >> 3;
    PORTAbits.RA6 = temp;
    
    //LED_4
    temp = led_port_data & 0x10;
    temp = temp >> 4;
    PORTAbits.RA7 = temp;
    
    //LED_5
    temp = led_port_data & 0x20;
    temp = temp >> 5;
    PORTGbits.RG13 = temp;
    
    //LED_6
    temp = led_port_data & 0x40;
    temp = temp >> 6;
    PORTGbits.RG14 = temp;
    
    //LED_7
    temp = led_port_data & 0x80;
    temp = temp >> 7;
    PORTGbits.RG12 = temp;   
}


