/*********************************************************************
    FileName:     	IO_Board.c
    Dependencies:	See #includes
    Processor:		PIC32MZ
    Hardware:		MainBrain32 rev 0.20
    Complier:  	    XC32 4.40
    Author:         Larry Knight 2025

    Software License Agreement:
        This software is licensed under the Apache License Agreement

    Description:

    File Description:
        Code for the I/O Board

    Change History:
/***********************************************************************/

#include <xc.h>
#include <stdbool.h>
#include "MainBrain.h"

void IO_Board_Init(void)
{
    //Board Select Address
    //A0
    //PORTD.15
    TRISDbits.TRISD15 = 0;   
    PORTDbits.RD15 = 0;
    
    //A1
    //PORTF.13
    TRISFbits.TRISF13 = 0;   
    PORTFbits.RF13 = 0;  
    
    //A2
    //PORTD.14
    TRISDbits.TRISD14 = 0;   
    PORTDbits.RD14 = 0;     
}





