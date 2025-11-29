/*********************************************************************
    FileName:     	REN70V05.c
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

/*     Flag Table        */
//Flag 000 = address 0000 - 03ff
//Flag 001 = address 0400 - 07ff
//Flag 010 = address 0800 - 0bff
//Flag 011 = address 0c00 - 0fff
//Flag 100 = address 1000 - 13ff
//Flag 101 = address 1400 - 17ff
//Flag 110 = address 1800 - 1bff
//Flag 110 = address 1c00 - 1fff
    

#include <xc.h>
#include "MainBrain.h"

uint8_t mdata_70V05;
uint32_t address_70V05;
volatile bool SRAM_BUSY = false;

void REN70V05_Init(void)
{    
   //RA0 = /CS1
    ANSELA = 0;
    TRISAbits.TRISA0 = 0;
    PORTAbits.RA0 = 1;
    
    //RG15 = /SEM
    TRISGbits.TRISG15 = 0;
    PORTGbits.RG15 = 1;
    
    //INT1 - INTR
    TRISEbits.TRISE8 = 1;
    
    //RB5 = M/S
    TRISBbits.TRISB5 = 0;    
    PORTBbits.RB5 = 1;
    
    //BUSY - BUSYR
    ANSELE = 0;
    TRISEbits.TRISE9 = 1;
    
    CNCONEbits.ON = 1;
    
    // Enable CN interrupt on RE9
    CNENEbits.CNIEE9 = 1;
    
    // Configure CN interrupt
    // Enable CN interrupt
    IEC3bits.CNEIE = 1;    
    
    // Set priority
    IPC30bits.CNEIP = 5; 
    IPC30bits.CNEIS = 1;
    
    // Clear the interrupt flag 
     IFS3bits.CNEIF = 0;       
     
     REN70V05_WR(0,1);
     Delay32(1,10000);
     mdata_70V05 = 0;
     REN70V05_RD(0);
     if(mdata_70V05 != 1)
     {
	 lastError = lastError | 4;
     }
     
     //clear the area for the peripherals list
     for(int i=0;i<8;i++)
     {
	 REN70V05_WR(i,0);
     }
}

int8_t REN70V05_RD(uint32_t address_70V05)
{        
    SRAM_BUSY = false;
    
    //get data
    PMRADDR = address_70V05;
    
    //CS1
    PORTAbits.RA0 = 0;    

    //dummy read
    while(PMMODEbits.BUSY == 1);
    mdata_70V05 = PMRDIN;
    
    while(PMMODEbits.BUSY == 1);
    mdata_70V05 = PMRDIN;

    //CS1
    PORTAbits.RA0 = 1;    
    
    return mdata_70V05;
}

void REN70V05_WR(uint32_t address_70V05, uint8_t mdata_70V05)
{        
    PMWADDR = address_70V05;
    
    //CS1
    PORTAbits.RA0 = 0; 
    
    while(PMMODEbits.BUSY == 1);
    PMDOUT = mdata_70V05;
    while(PMMODEbits.BUSY == 1);
    
    if(SRAM_BUSY == true)
    {
        while(PMMODEbits.BUSY == 1);
        PMDOUT = mdata_70V05;
        while(PMMODEbits.BUSY == 1);
    }
    
    SRAM_BUSY = false;
    
    //CS1
    PORTAbits.RA0 = 1;    
}

void __attribute__((vector(_CHANGE_NOTICE_E_VECTOR), interrupt(ipl5srs), nomips16)) CN_ISR()
{ 
    // Check if the interrupt is caused by RE9 and that it was the falling edge
    if (CNSTATEbits.CNSTATE9 && !PORTEbits.RE9)
    {    
        Delay32(0, 1);
        SRAM_BUSY = true;
    }
    
    // Clear the interrupt flag
    IFS3bits.CNEIF = 0;
}