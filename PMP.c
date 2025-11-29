/*********************************************************************
    FileName:     	PMP.c
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

#include <proc/p32mz2048efh100.h>

void PMP_init(void)
{
    // /ACK
    TRISGbits.TRISG13 = 1;
    
    //RESET the Module
    PMCONbits.ON = 0;
    
    //PMCON: Parallel Port Control Register
    PMCONbits.ADRMUX = 0;               //Address and data are separate
    PMCONbits.PMPTTL = 0;               //Schmitt Trigger input buffer
    PMCONbits.PTWREN = 1;               //WR enable strobe port
    PMCONbits.PTRDEN = 1;               //RD enable strobe port
    PMCONbits.CSF = 0;                  //PMCS1 and PMCS2 not used
    PMCONbits.CS1P = 0;                 //PMCS1 is active low
    PMCONbits.WRSP = 0;                 //WR strobe is active low
    PMCONbits.RDSP = 0;                 //RD strobe is active low
    PMCONbits.DUALBUF = 1;              //Double Buffer on/off
    
    //PMMODE: Parallel Port Mode Register
    PMMODEbits.IRQM = 0;                //No interrupts
    PMMODEbits.INCM = 0;                //No auto address inc/dec
    PMMODEbits.MODE16 = 1;              //16-bit mode
    PMMODEbits.MODE = 2;                //Master mode 2
    PMMODEbits.WAITB = 3;               //RD/WR data setup wait states
    PMMODEbits.WAITM = 7;               //RD/WR data wait states
    PMMODEbits.WAITE = 3;               //Data hold wait states
                                                                        
    //PMADDR: Parallel Port Address Register
    PMADDRbits.CS2 = 0;                 //PMCS2 is inactive
    PMADDRbits.CS1 = 0;                 //PMCS1 is inactive
    
    //PMAEN: PARALLEL PORT PIN ENABLE REGISTER
    PMAENbits.PTEN = 0xffff;
    
    //RA9 = /CS0 (Display)
    TRISAbits.TRISA9 = 0;
    PORTAbits.RA9 = 1;
    
    //RA0 = /CS1 (Dual Port RAM)
    TRISAbits.TRISA0 = 0;
    PORTAbits.RA0 = 1;    
    
    //RA10 = /CS2 (External Flash)
    TRISAbits.TRISA10 = 0;
    PORTAbits.RA10 = 1;
    
    //Enable the Module
    PMCONbits.ON = 1;
}
