/*********************************************************************
    FileName:     	Delay.c
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

#include <xc.h>
#include "MainBrain.h"

int RTC_delay_counter;

void Delay32(uint8_t prescale, uint32_t _delay32)
{
    if (_delay32 == 0)
    {
        return;
    }
    
    // Stop timer
    T4CONbits.ON = 0;

    // 32-bit mode MUST be enabled before loading PR registers
    T4CONbits.T32 = 1;

    // Prescaler
    T4CONbits.TCKPS = prescale;

    // Load 32-bit period
    PR4 = (uint16_t)(_delay32 & 0xFFFF);
    PR5 = (uint16_t)(_delay32 >> 16);

    // Clear counters
    TMR4 = 0;
    TMR5 = 0;

    // Clear & enable IRQ
    IFS0bits.T5IF = 0;
    IEC0bits.T5IE = 1;    // NECESSARY on PIC32MZ!

    // Start timer
    T4CONbits.ON = 1;

    // Poll until interrupt flag set
    while (!IFS0bits.T5IF)
    {
        LED_Port(TMR4);
    }

    T4CONbits.ON = 0;
}

//Seconds delay
void LongDelay(int delay_s)
{
    RTC_delay_counter = 0;
    
    while(RTC_delay_counter < delay_s)
    {
        
    }
}
