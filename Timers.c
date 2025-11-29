/*********************************************************************
    FileName:     	Timers.c
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

int ADC0_result;
float ADC6_result;

//2KHz
const uint16_t TMR1_FREQ_SET = 0xe500; 

//System Clock = 1.8KHz
void TMR1_init(void)
{
    IPC1bits.T1IP = 6;
    IPC1bits.T1IS = 3;
    
    IEC0bits.T1IE = 1;

    T1CON = 0;
    T1CONbits.TCKPS = 0;
    IFS0bits.T1IF = 0;
    T1CONbits.ON = 1;
}

//Used for the Display back light PWM
void TMR2_init(void)
{
    IPC2bits.T2IP = 6;
    IPC2bits.T2IS = 2;
    
    IEC0bits.T2IE = 0;
    
    T2CONbits.ON = 1;
}

void TMR3_init(void)
{
    IPC3bits.T3IP = 7;
    IPC3bits.T3IS = 1;
    
    IEC0bits.T3IE = 0;
    
    T3CONbits.ON = 0;
}

void TMR4_init(void)
{
    T4CONbits.ON = 0;
    IPC4bits.T4IP = 5;
    IPC4bits.T4IS = 3;
    IEC0bits.T4IE = 1;
    T4CONbits.T32 = 1;
    IFS0bits.T4IF = 0;   
}


void TMR5_init(void)
{
    T5CONbits.ON = 0;
    IPC6bits.T5IP = 5;
    IPC6bits.T5IS = 2;
    IEC0bits.T5IE = 1;
    IFS0bits.T5IF = 0;
}

//This timer is used for the heartbeat LED ON time
void TMR6_init(void)
{
    T6CONbits.ON = 0;
    T6CONbits.TCKPS = 7;
    IPC7bits.T6IP = 5;
    IPC7bits.T6IS = 1;
    IEC0bits.T6IE = 1;
    IFS0bits.T6IF = 0;
}

//Timer 1
void __attribute__((vector(_TIMER_1_VECTOR), interrupt(ipl6srs), nomips16)) TMR1_handler()
{
    //presets the TMR1 period
    PR1 = TMR1_FREQ_SET;
    
    //Outputs proof of interrupt on pin 10
    PORTGbits.RG6 = !PORTGbits.RG6;
    
    //Trigger a conversion
    ADCCON3bits.GSWTRG = 1;
    
    //Wait the conversions to complete
    while (ADCDSTAT1bits.ARDY0 == 0);
    ADC0_result = ADCDATA0;
    
    while (ADCDSTAT1bits.ARDY6 == 0);
    ADC6_result = ADCDATA6;

    //Clear interrupt flag
    IFS0bits.T1IF = 0;
}

//Timer 2
void __attribute__((vector(_TIMER_2_VECTOR), interrupt(ipl6srs), nomips16)) TMR2_handler()
{
    //Clear interrupt flag
    IFS0bits.T2IF = 0;
}

//Timer 3
void __attribute__((vector(_TIMER_3_VECTOR), interrupt(ipl6srs), nomips16)) TMR3_handler()
{
    //Clear interrupt flag
    IFS0bits.T3IF = 0;
}

//Timer 4
void __attribute__((vector(_TIMER_4_VECTOR), interrupt(ipl5srs), nomips16)) TMR4_handler()
{
    PORTDbits.RD11 = !PORTDbits.RD11;

    //Clear interrupt flag
    IFS0bits.T4IF = 0;
}

//Timer 5
void __attribute__((vector(_TIMER_5_VECTOR), interrupt(ipl5srs), nomips16)) TMR5_handler()
{
    PORTDbits.RD10 = !PORTDbits.RD10;

    //Clear interrupt flag
    IFS0bits.T5IF = 0;
}

//Timer 6
void __attribute__((vector(_TIMER_6_VECTOR), interrupt(ipl5srs), nomips16)) TMR6_handler()
{
    //Controls LED0 ON time
    // LED OFF
    PORTFbits.RF3 = 0;   
    
    // Stop timer
    T6CONbits.ON = 0;       
    
    //Clear interrupt flag
    IFS0bits.T6IF = 0;
}

