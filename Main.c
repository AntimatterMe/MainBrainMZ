/*********************************************************************
    FileName:     	Main.c
    Dependencies:	See #includes
    Processor:		PIC32MZ
    Hardware:		MainBrain MZ
    Complier:		XC32 4.40
    Author:		Larry Knight 2023
/*********************************************************************
 
    Software License Agreement:
 
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 
    Description:
        System Clock = 200 - 250 MHz

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
#pragma config FSOSCEN =    ON
#pragma config IESO =       OFF
#pragma config POSCMOD =    EC
#pragma config OSCIOFNC =   OFF
#pragma config FCKSM =      CSECMD
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
#pragma config FPLLMULT =   MUL_50
#pragma config FPLLODIV =   DIV_2
#pragma config UPLLFSEL =   FREQ_24MHZ

/*** DEVCFG3 ***/
#pragma config USERID =     0xffff
#pragma config FMIIEN =     OFF
#pragma config FETHIO =     OFF
#pragma config PGL1WAY =    OFF
#pragma config PMDL1WAY =   OFF
#pragma config IOL1WAY =    OFF
#pragma config FUSBIDIO =   OFF

#include <xc.h>
#include <proc/p32mz2048efh100.h>
#include <sys/attribs.h>
#include <sys/kmem.h>
#include <stdint.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "MainBrain.h"

volatile uint32_t Last_Memory[10];

volatile bool inMotion = false;
bool test_result;
bool full_access;
bool screen_available = true;
uint32_t lastError;
char myStr[20] = "null";
char ConnectedStr[10] = {"Connected"};
char ReadyStr[10] = {"Ready... "};
char OfflineStr[11] = {"Offline..."};
char FullSpeed[7] = {"12Mb/s"};
char HiSpeed[8] = {"480Mb/s"};
char DACtitleStr[14] = {"DAC Utilities"};
char cmdStr[4] = {"cmd"};
char memStr[4] = {"mem"};
char brdStr[4] = {"brd"};
char scopeTitleStr[6] = {"Scope"};
int8_t dev_data = 0;
uint8_t screen;
bool NeedsRefresh = true;
uint8_t default_Display_Brightness = 1;
bool Level2MenuActive = false;
uint8_t CurrentLevel = HOMELEVEL;
uint8_t FlashConfigData[100];
uint8_t BuildPList = 0;
uint8_t requestDirective = 0;
uint8_t PeripheralList[7] = {0};
uint8_t current_board_address;
uint16_t myArray[480];
int test1 = 0;
bool updated = false;
uint16_t adc_data = 0;
int myi = 0;
float ADC_Volts;
uint8_t buf[10];
bool screenTouched = false;
bool Message = false;
uint8_t old_screen = 0;

int main(void)
{       
    while(CLKSTATbits.POSCRDY == 0);
        
    SystemSetup();
    
    //Indicates System Setup completed
    LED_Port(0x1);
    
    PMP_init();
    
    //Indicates PMP Setup completed
    LED_Port(0x2);
    
    //SRAM initialize
    REN70V05_Init();
    
    //Indicates SRAM Setup completed
    LED_Port(0x3);
    
    //Test SRAM
    //test_result = memtest_70V05(40);
    
    //Indicates SRAM Test completed
    LED_Port(0x4);
    
    //Release the SRAM
    //ReleaseSRAM();

    Flash_Init();

    //Indicates Flash Setup completed
    LED_Port(0x5);
        
    //32 bit timer
    TMR4_init();
    
    //Indicates Timer 5 Setup completed
    LED_Port(0x7);    
    
    //I2C Initialize
    I2C_init();

    //Indicates Touch (I2C) Setup completed
    LED_Port(0x8);
    
    TMR1_init();
      
    //Indicates Timer 1 Setup completed
    LED_Port(0x9);
    
    ADC_init();
    
    //Indicates ADC Setup completed
    LED_Port(0xa);
    
    Display_init(); 
    
    //Indicates Display Setup completed
    LED_Port(0xb);
   
    SetDisplayBrightness();
    
    //Clear the Screen
    Display_CLRSCN(0xffff);   

    //Turn on the Display
    Display_DISPON();  
    
    //Timer 6 Init
    TMR6_init();
    
    //Real time clock init
    RTCC_init();
    
    //Indicates Real Time Clock completed
    LED_Port(0xc);  
    
    //Increase POSC freq
    //58 seems to be the highest stable value
    SetFreqPOSC(56);
    
    //Beep the buzzer to indicate full config
    Beep();
    
    //enable the interrupts
    __builtin_enable_interrupts();    
  
    //Clear the LED Port 
    //System Setup is Complete
    LED_Port(0x0);
    
    //Load the Splash Screen
    ShowSplashScreen(0);
    
    //Specify the screen to load
    screen = HOME_SCREEN;  
    
    //Build peripherals list
    PeripheralList[1] = 1;
    PeripheralList[2] = 2;
    PeripheralList[3] = 3;
    PeripheralList[4] = 4;
    PeripheralList[5] = 5;
    PeripheralList[6] = 6;
    PeripheralList[7] = 7;
    
    uint8_t a = 5;
    
    //Main program Loop
    while(1)
    {    	
        //Heart beat
        //Signal out on pin 50
        PORTCbits.RC15 = !PORTCbits.RC15;  
	
	//check for an active message
	if(Message == 1)
	{
	    //save current screen
	    old_screen = screen;
	    
	    //Act on the message
	    MessageBox(myStr, 5);
	    CurrentLevel = HOMELEVEL;
	    NeedsRefresh = true;
	    
	    //enable the screen
	    NeedsRefresh = true;
	    
	    //Clear the Message flag
	    Message = 0;	    
	}
		
	//check for a active directive
	if(requestDirective == 1)
	{
	    //This initiates an I/O cycle
	    Directive(current_board_address);
	    
	    //clear the Directive flag
	    requestDirective = 0;	    
	}
	
        //Load the current screen
        switch(screen)
        {
            case HOME_SCREEN:
                if(NeedsRefresh == true)
                {
                    ShowSplashScreen(0);
                    NeedsRefresh = false;
                    CurrentLevel = HOMELEVEL;
                }
		break;
            case INFO_SCREEN:
                if(NeedsRefresh == true)
                {
                    DrawScreen(INFO_SCREEN, HeaderString);
                    CurrentLevel = HOMELEVEL;
		    NeedsRefresh = false;
                }                
		break;
	}      
	
        //Read single point touch position
	if((scn_pos_x > 0) && (scn_pos_x < 480) && (scn_pos_y > 0) && (scn_pos_y < 320))
	{
	    hchar = 10;
	    vchar = 60;
	    
	    if(screenTouched == false)
	    {
		screen = INFO_SCREEN;
		NeedsRefresh = true;
		screenTouched = true;
	    }
	    
	    vchar = 15;
	    hchar = 350;
	    Binary2ASCIIBCD(scn_pos_x);
	    WriteChar(hchar, vchar, d2, black, 0x04D3);
	    WriteChar(hchar, vchar, d1, black, 0x04D3);
	    WriteChar(hchar, vchar, d0, black, 0x04D3);
	    
	    WriteChar(hchar, vchar, ',', black, 0x04D3);
	    
	    Binary2ASCIIBCD(scn_pos_y);
	    WriteChar(hchar, vchar, d2, black, 0x04D3);
	    WriteChar(hchar, vchar, d1, black, 0x04D3);
	    WriteChar(hchar, vchar, d0, black, 0x04D3);
	}
    }
}

void SystemSetup(void)
{   
    //disable the interrupts
    __builtin_disable_interrupts();    
    
    // Unlock Sequence
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;  
    
    //Init USB module
    USB_init();
    
    // Set multi vector interrupt mode
    INTCONbits.MVEC = 1; 
    
    unsigned int cp0;
    
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    ANSELF = 0;
    ANSELG = 0;
    
    //Buzzer control pin
    TRISAbits.TRISA1 = 0;

    //This pin is used by the peripherals to indicate they need serviced
    TRISAbits.TRISA7 = 1;
    
    //LED Port
    TRISFbits.TRISF3    = 0;
    TRISFbits.TRISF2    = 0;
    TRISFbits.TRISF8    = 0;
    TRISAbits.TRISA6    = 0;
    TRISAbits.TRISA14   = 0;
    TRISAbits.TRISA15   = 0;
    TRISDbits.TRISD10   = 0;
    TRISDbits.TRISD11   = 0;
    
    //RC13 - Secondary OSC in
    TRISCbits.TRISC13 = 1;

    //RC15
    TRISCbits.TRISC15 = 0;
    
    //RG6
    TRISGbits.TRISG6 = 0;
    
    //RA7
    //Input from peripheral
    TRISAbits.TRISA7 = 1;
    
    //Pin Mapping
    //Maps OC5 to pin
    RPD4R = 0x0b;
    
    //USB
    //This allows the usage of RF3 as I/O while still using the USB module 
    //Device mode only
    USBCRCONbits.USBIDOVEN = 1;
    USBCRCONbits.USBIDVAL = 1;
    
    PRISS = 0x76543210;
    
    //PBCLK1 - System Clock
    // Peripheral Bus 1 cannot be turned off, so there's no need to turn it on
    PB1DIVbits.PBDIV = 1; // Peripheral Bus 1 Clock Divisor Control (PBCLK1 is SYSCLK divided by 1)

    //PBCLK2 - PMP
    PB2DIVbits.PBDIV = 1; // Peripheral Bus 2 Clock Divisor Control (PBCLK2 is SYSCLK divided by 1)
    PB2DIVbits.ON = 1; // Peripheral Bus 2 Output Clock Enable (Output clock is enabled)
    while (!PB2DIVbits.PBDIVRDY);

    //PBCLK3 - Feeds TMR1
    PB3DIVbits.PBDIV = 0; 
    PB3DIVbits.ON = 1; 
    while (!PB3DIVbits.PBDIVRDY);
    
    //PB4DIV
    PB4DIVbits.PBDIV = 1; // Peripheral Bus 4 Clock Divisor Control (PBCLK4 is SYSCLK divided by 1)
    PB4DIVbits.ON = 1; // Peripheral Bus 4 Output Clock Enable (Output clock is enabled)
    while (!PB4DIVbits.PBDIVRDY); // Wait until it is ready to write to

    //PB5DIV
    PB5DIVbits.PBDIV = 1; // Peripheral Bus 5 Clock Divisor Control (PBCLK5 is SYSCLK divided by 1)
    PB5DIVbits.ON = 1; // Peripheral Bus 5 Output Clock Enable (Output clock is enabled)
    while (!PB5DIVbits.PBDIVRDY); // Wait until it is ready to write to

    // PB7DIV
    PB7DIVbits.PBDIV = 0; // Peripheral Bus 7 Clock Divisor Control (PBCLK7 is SYSCLK divided by 1)
    PB7DIVbits.ON = 1; // Peripheral Bus 7 Output Clock Enable (Output clock is enabled)
    while (!PB7DIVbits.PBDIVRDY); // Wait until it is ready to write to

    //PB8DIV
    PB8DIVbits.PBDIV = 1; // Peripheral Bus 8 Clock Divisor Control (PBCLK8 is SYSCLK divided by 1)
    PB8DIVbits.ON = 1; // Peripheral Bus 8 Output Clock Enable (Output clock is enabled)
    while (!PB8DIVbits.PBDIVRDY); // Wait until it is ready to write to

    //PRECON - Set up prefetch
    PRECONbits.PFMSECEN = 0;  // Flash SEC Interrupt Enable (Do not generate an interrupt when the PFMSEC bit is set)
    PRECONbits.PREFEN = 3; // Predictive Prefetch Enable (Enable predictive prefetch for any address)
    PRECONbits.PFMWS = 2; // PFM Access Time Defined in Terms of SYSCLK Wait States (Two wait states)
    
    CFGCONbits.USBSSEN = 1;

    // Set up caching
    cp0 = _mfc0(16, 0);
    cp0 &= ~0x07;
    cp0 |= 0b011; // K0 = Cacheable, non-coherent, write-back, write allocate
    _mtc0(16, 0, cp0);  
    
    while(CLKSTATbits.DIVSPLLRDY == 0);
}

void LED_Port(int8_t led_port_data)
{
    //LED_Port:
    //LED0              RF3
    //LED1              RF2 
    //LED2              RF8
    //LED3              RA6
    //LED4              RA14
    //LED5              RA15
    //LED6              RD10
    //LED7              RD11

   uint32_t temp = led_port_data;
    
    //LED0
    temp = led_port_data & 0x01;  
    PORTFbits.RF3 = temp;
    
    //LED_1
    temp = led_port_data & 0x02;
    temp = temp >> 1;
    PORTFbits.RF2 = temp;
    
    //LED_2
    temp = led_port_data & 0x04;
    temp = temp >> 2;
    PORTFbits.RF8 = temp;
    
    //LED_3
    temp = led_port_data & 0x08;
    temp = temp >> 3;
    PORTAbits.RA6 = temp;
    
    //LED_4
    temp = led_port_data & 0x10;
    temp = temp >> 4;
    PORTAbits.RA14 = temp;
    
    //LED_5
    temp = led_port_data & 0x20;
    temp = temp >> 5;
    PORTAbits.RA15 = temp;
    
    //LED_6
    temp = led_port_data & 0x40;
    temp = temp >> 6;
    PORTDbits.RD10 = temp;
    
    //LED_7
    temp = led_port_data & 0x80;
    temp = temp >> 7;
    PORTDbits.RD11 = temp;   
}

void GetTime(void)
{
//    int a = hchar;
//    int b = vchar;
//    
//    hchar = 350;
//    vchar = 8;

    Binary2ASCIIBCD(RTCTIMEbits.HR10);    
    WriteChar(hchar, vchar, d0, black, 0x04D3);
    
    Binary2ASCIIBCD(RTCTIMEbits.HR01);    
    WriteChar(hchar, vchar, d0, black, 0x04D3);
    
    //colon
    WriteChar(hchar, vchar, ':', black, 0x04D3);
    
    Binary2ASCIIBCD(RTCTIMEbits.MIN10);    
    WriteChar(hchar, vchar, d0, black, 0x04D3);
    
    Binary2ASCIIBCD(RTCTIMEbits.MIN01);    
    WriteChar(hchar, vchar, d0, black, 0x04D3);
    
    //colon
    WriteChar(hchar, vchar, ':', black, 0x04D3);
    
    Binary2ASCIIBCD(RTCTIMEbits.SEC10);    
    WriteChar(hchar, vchar, d0, black, 0x04D3);
    
    Binary2ASCIIBCD(RTCTIMEbits.SEC01);    
    WriteChar(hchar, vchar, d0, black, 0x04D3);
    
//    hchar = a;
//    vchar = b;
}

void SystemReset(void)
{
    //unlock the system
    SYSKEY = 0x00000000;
    //write invalid key to force lock 
    SYSKEY = 0xAA996655;
    //write key1 to SYSKEY 
    SYSKEY = 0x556699AA;
    //write key2 to SYSKEY
    // OSCCON is now unlocked
    /* set SWRST bit to arm reset */ 
    RSWRSTSET = 1;
    /* read RSWRST register to trigger reset */ 
    int dummy = RSWRST;
    /* prevent any unwanted code execution until reset occurs*/ 
    while(1);
}

/*************************************************************
 Sends an I/O request to the peripheral.
 
 SRAM Data must be written prior to calling this function.
 
 Once the board address is selected the peripheral reads
 SRAM location 0 of it's predefined memory region and returns
 an /ACK when finished.
 
 This function responds to the /ACK by setting the current address
 to zero.
*************************************************************/
void Directive(uint8_t badd)
{
    //Zero Timer 3 and set period
    //timeout is about 1 uS
    //non-timeout is about 475 nS
    PR3 = 0xffff;
    TMR3 = 0;
    
    //Set the pre-scaler
    T3CONbits.TCKPS = 7;
    
    //Write the command at board address of the I/O card
    //REN70V05_WR((((boardAddress) - 1) * 0x400), EP[1].rx_buffer[2]);

    //Set the board select address
    SetPeripheralAddress(badd);

    //Start Timer 3
    T3CONSET = 1 << 15; 
    
    //Wait for either the /ACK or a timeout to occur
    while((!IFS0bits.T3IF) && (PORTGbits.RG13));
    
    //Check to see what happened
    //If /ACK went low add the board to the list
    if(!PORTGbits.RG13)
    {
	//PeripheralList[EP[1].rx_buffer[1]] = 1;
	LED_Port(0x2);
    }
    else   
    {
	// we timed out
	//PeripheralList[EP[1].rx_buffer[1]] = 0;
	LED_Port(0x4);
    }

    
    //Reset the board select address
    SetPeripheralAddress(0);

    //Stop Timer 3
    T3CONCLR = 1 << 15;     
    
    //Reset the flag
    IFS0bits.T3IF = 0;
}

/*************************************************************
 Builds a list of all the present peripheral devices
 This array is used for all future I/O requests 
*************************************************************/
void BuildPeripheralList(void)
{
    //Zero Timer 3
    PR3 = 0;
    
    //Set the pre-scaler
    T3CONbits.TCKPS = 7;
    
    //Start Timer 3
    T3CONSET = 1 << 15; 
    
    //Write the command at address 0 of the I/O card
    REN70V05_WR(0, EP[1].rx_buffer[2]);
    int i = 0;
//    for(int i=1;i<=7;i++)
//    {
	//Set the board select address
	SetPeripheralAddress(EP[1].rx_buffer[1]);

	//Wait for either the /ACK or a timeout to occur
	while((!IFS0bits.T3IF) && (PORTGbits.RG13));

	//Check to see what happened
	//If /ACK went low add the board to the list
	if(!PORTGbits.RG13)
	{
	    PeripheralList[i] = 1;
	}
	else
	{
	    // we timed out
	    PeripheralList[i] = 0;
	}

	//Reset the board select address
	SetPeripheralAddress(0);

	//Stop Timer 3
	T3CONCLR = 1 << 15; 
//    }
}

void SetPotentiometer(void)
{
    
}

void SetPeripheralAddress(uint8_t padd)
{
    if (padd >= 7) 
    {
        padd = 7;
    }

    // A0 - PORTD.15
    if ((padd & 0x01) != 0)  
    {
        PORTDbits.RD15 = 1;
    }
    else    
    {
        PORTDbits.RD15 = 0;
    }

    // A1 - PORTF.13
    if ((padd & 0x02) != 0)  
    {
        PORTFbits.RF13 = 1;
    }
    else    
    {
        PORTFbits.RF13 = 0;
    }
    
    // A2 - PORTD.14
    if ((padd & 0x04) != 0)  
    {
        PORTDbits.RD14 = 1;
    }
    else    
    {
        PORTDbits.RD14 = 0;
    }
}

void Clock(uint8_t ClockData)
{       
    //TextColor = black;
    
//    //Colon 1
//    Display_Rect(151, 163, 120, 130, TextColor);  
//    Display_Rect(151, 163, 160, 170, TextColor);  
//
//    //Colon 2
//    Display_Rect(317, 329, 120, 130, TextColor);  
//    Display_Rect(317, 329, 160, 170, TextColor);  
    
    if(ClockData == 46)
    {
	Display_Rect(hchar, hchar + 10, vchar, vchar + 10, black);    
    }
    
    if(ClockData == 48)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, black);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, black);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, black);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, black);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, white);    

    }

    if(ClockData == 49)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, white);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, white);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, white);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, white);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, white);    

    }

    if(ClockData == 50)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, black);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, white);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, black);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, black);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, white);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, black);    

    }

    if(ClockData == 51)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, black);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, black);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, white);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, white);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, black);    

    }

    if(ClockData == 52)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, white);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, white);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, white);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, black);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, black);    

    }

    if(ClockData == 53)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, black);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, white);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, black);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, white);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, black);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, black);    

    }

    if(ClockData == 54)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, white);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, white);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, black);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, black);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, black);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, black);    

    }

    if(ClockData == 55)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, black);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, white);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, white);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, white);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, white);    

    }

    if(ClockData == 56)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, black);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, black);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, black);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, black);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, black);    

    }

    if(ClockData == 57)
    {
            //segment A
            Display_Rect(hchar + 10, hchar + 50, vchar, vchar + 10, black);    
            //segment B
            Display_Rect(hchar + 50, hchar + 60, vchar, vchar + 50, black);
            //Segment C
            Display_Rect(hchar + 50, hchar + 60, vchar + 60, vchar + 110, black);
            //Segment D
            Display_Rect(hchar + 10, hchar + 50, vchar + 100, vchar + 110, white);    
            //Segment E
            Display_Rect(hchar, hchar + 10, vchar + 60, vchar + 110, white);
            //Segment F
            Display_Rect(hchar, hchar + 10, vchar, vchar + 50, black);
            //Segment G
            Display_Rect(hchar + 10, hchar + 50, vchar + 50, vchar + 60, black);    
    }
}

void DMM(uint8_t data, uint16_t xchar, uint16_t ychar)
{
    switch(data)
    {
        case 46: // '.'
            Display_Rect(xchar + 50, xchar + 60, ychar + 100, ychar + 110, black);
            break;

        case 48: // '0'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  black); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, black); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, black); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  black); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  white); // G
            break;

        case 49: // '1'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  white); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, white); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, white); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  white); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  white); // G
            break;

        case 50: // '2'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  black); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, white); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, black); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, black); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  white); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  black); // G
            break;

        case 51: // '3'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  black); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, black); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, white); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  white); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  black); // G
            break;

        case 52: // '4'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  white); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, white); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, white); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  black); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  black); // G
            break;

        case 53: // '5'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  black); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  white); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, black); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, white); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  black); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  black); // G
            break;

        case 54: // '6'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  white); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  white); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, black); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, black); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  black); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  black); // G
            break;

        case 55: // '7'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  black); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, white); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, white); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  white); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  white); // G
            break;

        case 56: // '8'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  black); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, black); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, black); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  black); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  black); // G
            break;

        case 57: // '9'
            Display_Rect(xchar + 10, xchar + 50, ychar,        ychar + 10,  black); // A
            Display_Rect(xchar + 50, xchar + 60, ychar,        ychar + 50,  black); // B
            Display_Rect(xchar + 50, xchar + 60, ychar + 60,   ychar + 110, black); // C
            Display_Rect(xchar + 10, xchar + 50, ychar + 100,  ychar + 110, white); // D
            Display_Rect(xchar,      xchar + 10, ychar + 60,   ychar + 110, white); // E
            Display_Rect(xchar,      xchar + 10, ychar,        ychar + 50,  black); // F
            Display_Rect(xchar + 10, xchar + 50, ychar + 50,   ychar + 60,  black); // G
            break;

        default:
            // Draw nothing for unsupported characters
            break;
    }
}

void SetDisplayBrightness(void)
{
    Flash_RD(0);
    default_Display_Brightness = data_flash;
    
    //If the flash is not programmed then set the default
    if(data_flash == 0xff)
    {
        default_Display_Brightness = 5;
    }
    Backlight_Control(default_Display_Brightness);    
}

void Beep(void)
{
    //Beep speaker
    //Indicates System is ready
    //~2.7KHz
    
    for(int i=0;i<700;i++)
    {
        PORTAbits.RA1 = 1;
        Delay32(0, 0xb000);
        PORTAbits.RA1 = 0;
        Delay32(0, 0x7000);
    }    
}

void __attribute__((nomips16)) _general_exception_handler(void)
{
   unsigned int exccode = (_CP0_GET_CAUSE() & 0x7C) >> 2;

    LED_Port(exccode);  // show the raw exception code

    while(1)
    {
        Beep();
    }
}

bool SetFreqPOSC(uint8_t f)
{
    //increase system clock
    //This seems to be the max to have stable USB
    SPLLCONbits.PLLMULT = f;
    
    //The primary oscillator has started, stabilized, and is producing 
    //a valid clock signal.
    while(CLKSTATbits.POSCRDY == 0);
    
    //The final PLL clock?after multiplying and dividing?has settled and
    //can safely be used as the system clock.
    while(CLKSTATbits.DIVSPLLRDY == 0);
    
    return true;
}