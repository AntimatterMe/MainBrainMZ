/*********************************************************************
    FileName:     	Display.c
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

#include <string.h>
#include <xc.h>
#include "MainBrain.h"

uint8_t back_level;
int CanvasColor;
int TextColor;
unsigned ascii_char;
int color1;
int color2;
uint16_t hchar;
uint16_t vchar;
uint8_t Display_Read;
uint8_t disData[10];
unsigned row_start;
unsigned row_end;
unsigned col_start;
unsigned col_end;
int t;

void Backlight_Control(uint8_t back_level)
{   
    //PWM Setup for backlight control
    //sets frequency
    PR2 = 300;

    OC5CONbits.OCTSEL = 0;  //select TMR2
    OC5CONbits.OCM = 6;     //PWM mode, no fault pin
    OC5CONbits.ON = 1;

    //specifies duty cycle in percent
    if((back_level <= 10) | (back_level >= 0))
    {
        if(back_level == 0)
        {
            OC5RS = 300;
        }
        if(back_level == 1)
        {
            OC5RS = 270;
        }
        if(back_level == 2)
        {
            OC5RS = 243;
        }
        if(back_level == 3)
        {
            OC5RS = 216;
        }
        if(back_level == 4)
        {
            OC5RS = 189;
        }
        if(back_level == 5)
        {
            OC5RS = 162;
        }
        if(back_level == 6)
        {
            OC5RS = 135;
        }
        if(back_level == 7)
        {
            OC5RS = 108;
        }
        if(back_level == 8)
        {
            OC5RS = 81;
        }
        if(back_level == 9)
        {
            OC5RS = 54;
        }
        if(back_level == 10)
        {
            OC5RS = 27;
        }

    }

    T2CONbits.ON = 1;

    return;
}

void Speaker_Control(unsigned freq)
{   
    //PWM Setup for speaker control
    //sets frequency
    PR3 = freq;

    OC2CONbits.OCTSEL = 1;  //select TMR3
    OC2CONbits.OCM = 6;     //PWM mode, no fault pin
    OC2CONbits.ON = 1;

    OC2RS = PR3 / 2;


    if(freq != 0)
    {
        T3CONbits.ON = 1;
    }

    return;
}

void Display_init(void)
{
    //RG12 and RG14 = data bits D17 and D16
    TRISGbits.TRISG12 = 0;
    TRISGbits.TRISG14 = 0;
    PORTGbits.RG12 = 0;
    PORTGbits.RG14 = 0;

    //Configure Pin for D/C
    TRISBbits.TRISB1 = 0;
    PORTBbits.RB1 = 1;        

    //Select Display (/CS) 
    TRISAbits.TRISA9 = 0;
    PORTAbits.RA9 = 0;

    //Software RESET
    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;
    PMDOUT = 0x01;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    //>120mS Delay
    //to allow display to complete reset
    //set Timer 5 prescaler /256
    Delay32(7, 65535);

     //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //SLPOUT
    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;
    PMDOUT = 0x011;
    while(PMMODEbits.BUSY == 1);

    //Set pixel format = 16-bit
    //05h = 16-bit
    //06h = 18-bit
    //07h = 24-bit
    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;
    PMDOUT = 0x03a;
    while(PMMODEbits.BUSY == 1);
    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;
    PMDOUT = 0x05;
    while(PMMODEbits.BUSY == 1);

    //Set Screen Rotation
    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //MADCTL - Memory Access Control
    PMDOUT = 0x36;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;
    //D7 (MY)
    //D6 (MX)
    //D5 (MV)
    //D4 (ML)
    //D3 (RGB) 1=BGR. 0=RGB
    //D2 (reserved)
    //D1 (SS)
    //D0 (GS)
    //0x28 = wide screen, connector on righthand side
    //0xe8 = wide screen, connector on Lefthand side
    PMDOUT = 0x28;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//00h - no operation
void Display_NOP(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 00h = NOP
    PMDOUT = 0x00;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    return;
}

//01 - software reset
void Display_SWRESET(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 01h = SWRESET
    PMDOUT = 0x01;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    return;
}

//04h - Read Display Identification Information
uint8_t Display_RDDIDIF(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 04h = RDDPM
    PMDOUT = 0x04;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read Display Data
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Read Display Data
    disData[3] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    disData[4] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//05 - Read Number of the Errors on DSI
void Display_RDNUMED(void)
{
    //Select Display (/CS)  
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 05h = RDDPM
    PMDOUT = 0x05;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read Display Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//09h - Read Display Status
void Display_RDDST(void)
{
    //Select Display (/CS)  
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 09h = RDDPM
    PMDOUT = 0x09;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for Display
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Read Display Data
    disData[3] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    disData[4] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    disData[5] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//0Ah - read power mode
void Display_RDDPM(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 0ah = RDDPM
    PMDOUT = 0x0a;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read the Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//0Bh - read display MADCTL
uint8_t Display_RDDMADCTL(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 0bh = RDDPM
    PMDOUT = 0x0b;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read the Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//0Ch - get pixel format
void Display_RDDCOLMOD(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 0ch = RDDCOLMOD
    PMDOUT = 0x0c;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read the Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//0Dh - get display mode
void Display_RDDIM(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 0dh = RDDIM
    PMDOUT = 0x0d;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read the Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//0Eh - get signal mode
void Display_RDDSM(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 0eh = RDDSM
    PMDOUT = 0x0e;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read the Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//0Fh - get diagnostic result
void Display_RDDSDR(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 0fh = RDDSDR
    PMDOUT = 0x0f;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read the Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//10h - enter sleep mode
void Display_SLPIN(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 10h = SLPIN
    PMDOUT = 0x10;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//11h - exit sleep mode
void Display_SLPOUT(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 11h = SLPOUT
    PMDOUT = 0x11;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//12h - enter partial mode
void Display_PTLON(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 12h = PTLON
    PMDOUT = 0x12;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//13h - enter normal mode
void Display_NORON(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 13h = NORON
    PMDOUT = 0x13;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//20h - exit inversion mode
void Display_INVOFF(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 20h = INVOFF
    PMDOUT = 0x20;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//21h - enter inversion mode
void Display_INVON(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 21h = INVON
    PMDOUT = 0x21;
    while(PMMODEbits.BUSY == 1);

     //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//22h - All Pixels OFF
void Display_ALLPOFF(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 22h = INVON
    PMDOUT = 0x22;
    while(PMMODEbits.BUSY == 1);

     //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//23h - All Pixels ON
void Display_ALLPON(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 23h = INVON
    PMDOUT = 0x23;
    while(PMMODEbits.BUSY == 1);

     //Select Display (/CS)    
    PORTAbits.RA9 = 1;

   return;
}

//28h - display off
void Display_DISPOFF(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 28h = DSPOFF
    PMDOUT = 0x28;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    return;
}

//29h - display on
void Display_DISPON(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 29h = DISPON
    PMDOUT = 0x29;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    return;
}

//2a - set column address
void Display_CASET(uint16_t col_start, uint16_t col_end)
{
    if(col_end == 0)
    {
        col_end = col_start + 15;
    }

    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    PMDOUT = 0x02a;
    while(PMMODEbits.BUSY == 1);
    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;
    //break in to bytes HI and LO
    //Column Start
    //HI
    if(col_start > 0xff)
    {
        PMDOUT = 1;       
    }
    else
    {
        PMDOUT = 0;               
    }
    while(PMMODEbits.BUSY == 1);
    //LO
    PMDOUT = col_start;
    while(PMMODEbits.BUSY == 1);

    //Column End
    //HI
    if(col_end > 0xff)
    {
        PMDOUT = 1;       
    }
    else
    {
        PMDOUT = 0;               
    }
    while(PMMODEbits.BUSY == 1);
    //LO
    PMDOUT = col_end;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    return;
}

//2Bh - set row address
void Display_RASET(unsigned row_start, unsigned row_end)
{
    if(row_end == 0)
    {
        row_end = row_start + 21;
    }

    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    PMDOUT = 0x02b;
    while(PMMODEbits.BUSY == 1);
    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;
    //break in to bytes HI and LO
    //Row Start
    //HI
    if(row_start > 0xff)
    {
        PMDOUT = 1;       
    }
    else
    {
        PMDOUT = 0;               
    }
    while(PMMODEbits.BUSY == 1);
    //LO
    PMDOUT = row_start;
    while(PMMODEbits.BUSY == 1);

    //Row End
    //HI
    if(row_end > 0xff)
    {
        PMDOUT = 1;       
    }
    else
    {
        PMDOUT = 0;               
    }
    while(PMMODEbits.BUSY == 1);
    //LO
    PMDOUT = row_end;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    return;
}

//2Ch - memory write
void Display_RAMWR(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 2Ch = RAMWR
    PMDOUT = 0x2C;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //pixel data is then written here
    //until another command is given

    //Select Display (/CS)    
    //PORTAbits.RA9 = 1;

    return;
}

//2Eh - memory read
void Display_RAMRD(void)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 2eh = RDDSM
    PMDOUT = 0x2e;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Dummy Read for PMP
    disData[0] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Dummy read for display
    disData[1] = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    //Read the Data
    disData[2] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Read the Data
    disData[3] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Read the Data
    disData[4] = PMRDIN;
    while(PMMODEbits.BUSY == 1);
    
    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    return;
}

//30h - Set_partial_area
void Display_PLTAR(unsigned SR_HI, unsigned SR_LO, unsigned ER_HI, unsigned ER_LO)
{
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 0;

    //Command 30h
    PMDOUT = 0x30;
    while(PMMODEbits.BUSY == 1);

    //RB1 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;

    //Upper byte start row
    PMDOUT = SR_HI;
    while(PMMODEbits.BUSY == 1);

    //Lower byte start row
    PMDOUT = SR_LO;
    while(PMMODEbits.BUSY == 1);

    //Upper byte end row
    PMDOUT = ER_HI;
    while(PMMODEbits.BUSY == 1);

    //Upper byte end row
    PMDOUT = ER_LO;
    while(PMMODEbits.BUSY == 1);

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;
}

//draws a rectangular block of colored pixels
void Display_Rect(unsigned col_start, unsigned col_end, unsigned row_start, unsigned row_end, unsigned rect_color)
{
    unsigned a = col_start;
    unsigned b = row_start;
    unsigned c = col_end;
    unsigned d = row_end;

    Display_CASET(col_start, col_end - 1);    
    Display_RASET(row_start, row_end - 1);

    Display_RAMWR();

    
    //Select Display (/CS)    
    PORTAbits.RA9 = 0;

    for(int i=0;i<(col_end - col_start) * (row_end - row_start); i++)
    {
        PMDOUT = rect_color;
        while(PMMODEbits.BUSY == 1);
	for(int i=0;i<5;i++);	
    }    

    //Select Display (/CS)    
    PORTAbits.RA9 = 1;

    col_start = a;
    row_start = b;
    col_end = c;
    row_end = d;
}

void Display_CLRSCN(int CanvasColor)
{
    Display_CASET(0, 479);
    Display_RASET(0, 319);
    Display_RAMWR();

    for(int i=0;i<=153600;i++)
    {
        PMDOUT = CanvasColor;
        while(PMMODEbits.BUSY == 1);    	
    }
}

//Writes a character to the screen
void WriteChar(unsigned col_start, unsigned row_start, unsigned ascii_char, int TextColor, int CanvasColor)
{
    //Makes sure no out of bounds
    if(ascii_char > 0x7F || ascii_char < 0)
    {
        ascii_char = 0x2a;
    }

    int x;
    int text_char = ascii_char - 32;
    int off_set = lut[text_char];
    int read = courier_new_16pt_bold[off_set];
    int a = 0b10000000;
    int pixel = 0;
    int s = 0;
    int i;

    col_end = col_start + 15;
    row_end = row_start + 21;
    Display_CASET(col_start, col_end);
    Display_RASET(row_start, row_end);

    Display_RAMWR();

    for(s=0;s<=45;s++)
    {
        for(i=0;i<=7;i++)
        {
            x = read & a;
            if(x > 0)
            {
                pixel = TextColor;
            }
            else
            {
                pixel = CanvasColor;
            }

            //Select Display (/CS)    
            PORTAbits.RA9 = 0;

            PMDOUT = pixel;
            while(PMMODEbits.BUSY == 1);

            //Select Display (/CS)    
            PORTAbits.RA9 = 1;

            a = a / 2;
        }
        off_set++;
        read = courier_new_16pt_bold[off_set];
        a = 0b10000000;
    }
    //this keeps track of the horizontal position.
    hchar = hchar + 15;    return;
}

//Writes a null terminated string from a previously defined array
void WriteString(unsigned col_start, unsigned row_start, char array_name[], int TextColor, int CanvasColor)
{
    int offset = 0;

    //must initialize test_char to something other than \0
    //so we might as well use the array data
    char test_char = array_name[offset];

    while(test_char != '\0')
    {
        test_char = array_name[offset];

        Display_CASET(col_start, col_start + 15);

        WriteChar(col_start, row_start, test_char, TextColor, CanvasColor);    
        offset++;       
        col_start = col_start + 15;
    }
    //cleans up the extra white space
    hchar = hchar - 15;
}

//Writes a character with 2 background colors for button text
void WriteButtonChar(unsigned col_start, unsigned row_start, unsigned ascii_char, int TextColor, int color1, int color2)
{
    //Makes sure no out of bounds
    if(ascii_char > 0x7F || ascii_char < 0)
    {
        ascii_char = 0x2a;
    }

    int x;
    int text_char = ascii_char - 32;
    int off_set = lut[text_char];
    int read = courier_new_16pt_bold[off_set];
    int a = 0b10000000;
    int pixel = 0;
    int s = 0;
    int i;

    col_end = col_start + 15;
    row_end = row_start + 21;
    Display_CASET(col_start, col_end);
    Display_RASET(row_start, row_end);

    Display_RAMWR();

    for(s=0;s<44;s++)
    {
        for(i=0;i<=7;i++)
        {
            x = read & a;
            if(x > 0)
            {
                pixel = TextColor;
            }
            else
            {
                if(s < 24)
                {
                    pixel = color1;
                }
                else
                {
                    pixel = color2;
                }
            }

            //Select Display (/CS)    
            PORTAbits.RA9 = 0;

            PMDOUT = pixel;
            while(PMMODEbits.BUSY == 1);

            //Select Display (/CS)    
            PORTAbits.RA9 = 1;

            a = a / 2;
        }
        off_set++;
        read = courier_new_16pt_bold[off_set];
        a = 0b10000000;
    }
    //this keeps track of the horizontal position.
    hchar = hchar + 15;
    return;
}

//Writes a null terminated string from a previously defined array with button colors
void WriteButtonString(unsigned col_start, unsigned row_start, char array_name[], int TextColor, int color1, int color2)
{
    int offset = 0;

    //must initialize test_char to something other than \0
    //so we might as well use the array data
    char test_char = array_name[offset];

    while(test_char != '\0')
    {
        test_char = array_name[offset];

        Display_CASET(col_start, col_start + 15);

        WriteButtonChar(col_start, row_start, test_char, TextColor, color1, color2);   
        offset++;       
        col_start = col_start + 15;
    }
    //cleans up the extra white space
    hchar = hchar - 15;
}

void DrawButton(unsigned col_start, unsigned row_start, uint8_t length, uint8_t height, int color1, int color2, int border_color, char array_name[])
{
    //Draw border
    //Display_Rect(20, 170, 50, 52, 0xad55);
    Display_Rect(col_start, col_start + length, row_start, row_start + 2, border_color);
    //Display_Rect(168, 170, 50, 100, 0xad55);
    Display_Rect((col_start + length) - 2, col_start + length, row_start, row_start + height, border_color);
    //Display_Rect(20, 170, 98, 100, 0xad55);
    Display_Rect(col_start, col_start + length, (row_start + height) - 2, row_start + height, border_color);
    //Display_Rect(20, 22, 50, 100, 0xad55);
    Display_Rect(col_start, col_start + 2, row_start, row_start + height, border_color);

    //Draw upper half
    //Display_Rect(colStart + 2, 168, 52, 75, 0xef7d);
    Display_Rect(col_start + 2, (col_start + length) - 2, row_start + 2, row_start + (height / 2), color1);
    //Draw lower half
    //Display_Rect(22, 168, 75, 98, 0xd6ba);
    Display_Rect(col_start + 2, (col_start + length) - 2, row_start + (height / 2), (row_start + height) - 2, color2);

    //Draw Button Text
    WriteButtonString(col_start + ((length - ((strlen(array_name) * 15))) / 2), row_start + 13, array_name, TextColor, color1, color2);//((length - strlen(array_name)) / 2)
}
