/*********************************************************************
    FileName:     	RTCC.c
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

#include "xc.h"
#include "MainBrain.h"

void RTCC_init(void)
{
    RTCCONbits.ON = 0;
    
    IPC41bits.RTCCIP = 4;
    IPC41bits.RTCCIS = 3;
    
    IFS5bits.RTCCIF = 0;
    IEC5bits.RTCCIE = 0;
    
    //Clock Select
    RTCCONbits.RTCCLKSEL = 1;
    
    OSCCONbits.SOSCEN = 1;
    
    //RTC write enable
    RTCCONbits.RTCWREN = 1;
    
    //Setup alarm for interrupt
    RTCALRMbits.ARPT = 0xff;
    RTCALRMbits.CHIME = 1;
    RTCALRMbits.AMASK = 1;      //4 = every 10 minutes
    RTCALRMbits.ALRMEN = 1;
    
    //calibration - clocks per minute +/-
    RTCCONbits.CAL = 10;
    
    //Set the time
    //Default is zeroed
    RTCDATEbits.YEAR10 = 2;
    RTCDATEbits.YEAR01 = 4;
    RTCDATEbits.MONTH10 = 0;
    RTCDATEbits.MONTH01 = 0; 
    RTCDATEbits.DAY10 = 0;
    RTCDATEbits.DAY01 = 0;
    RTCTIMEbits.HR10 = 0;
    RTCTIMEbits.HR01 = 0;
    RTCTIMEbits.MIN10 = 0;
    RTCTIMEbits.MIN01 = 0;
    RTCTIMEbits.SEC10 = 0;
    RTCTIMEbits.SEC01 = 0; 
    
    //Set actual time here
//    RTCDATEbits.YEAR10 = 2;
//    RTCDATEbits.YEAR01 = 4;
//    RTCDATEbits.MONTH10 = 1;
//    RTCDATEbits.MONTH01 = 2; 
//    RTCDATEbits.DAY10 = 2;
//    RTCDATEbits.DAY01 = 4;
//    RTCTIMEbits.HR10 = 0;
//    RTCTIMEbits.HR01 = 5;
//    RTCTIMEbits.MIN10 = 1;
//    RTCTIMEbits.MIN01 = 9;
//    RTCTIMEbits.SEC10 = 0;
//    RTCTIMEbits.SEC01 = 0;    

    //set alarm 
    ALRMTIMEbits.HR10 = 0;
    ALRMTIMEbits.HR01 = 0;
    ALRMTIMEbits.MIN10 = 0;
    ALRMTIMEbits.MIN01 = 0;
    ALRMTIMEbits.SEC10 = 0;
    ALRMTIMEbits.SEC01 = 0;
    
    RTCCONbits.ON = 1;
    
    //lock the registers
    RTCCONbits.RTCWREN = 0;    
    
    IEC5bits.RTCCIE = 1;
}

//Real Time Clock
void __attribute__((vector(_RTCC_VECTOR), interrupt(ipl4srs), nomips16)) RTCC_Handler()
{
    RTC_delay_counter++;
    
    //Turns ON LED0
    PORTFbits.RF3 = 1;
    
    //Starts timer 6 which controls the time LED0 is ON
    TMR6= 0;
    T6CONbits.ON = 1; 

    IFS5bits.RTCCIF = 0;
}