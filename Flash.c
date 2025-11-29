/*********************************************************************
    FileName:     	Flash.c
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
#include <stdbool.h>
#include "MainBrain.h"

//Flash Sector Size
const uint16_t SECTOR_SIZE = 4095;

uint8_t data_flash;
unsigned long address_flash;
uint8_t Flash_MID;
uint8_t Flash_DID;
int flash_size;
unsigned long Bytes_used;
uint32_t dly = 10000;

void Flash_Init(void)
{
    //RB2 = A16
    TRISBbits.TRISB2 = 0;
    PORTBbits.RB2 = 0;
    
    //RB4 = A17
    TRISBbits.TRISB4 = 0;
    PORTBbits.RB4 = 0;
    
    //RB3 = A18
    TRISBbits.TRISB3 = 0;
    PORTBbits.RB3 = 0;
        
    //RA10 = /CS2
    TRISAbits.TRISA10 = 0;
    PORTAbits.RA10 = 1;
    
//******************************************************    
//          Get Manufacturer ID
//******************************************************    
    //CS2
    PORTAbits.RA10 = 0;
    
    //Enter sequence
    PMWADDR = 0x5555;
    PMDOUT = 0xaa;
    while(PMMODEbits.BUSY == 1);  

    PMWADDR = 0x2aaa;
    PMDOUT = 0x55;
    while(PMMODEbits.BUSY == 1);  
    Delay32(0, dly);
    PMWADDR = 0x5555;
    PMDOUT = 0x90;
    while(PMMODEbits.BUSY == 1);  
                
    //read device ID
    PMRADDR = 0x0;
    //dummy read for PMP
    Flash_MID = PMRDIN;
    while(PMMODEbits.BUSY == 1); 
    
    Flash_MID = PMRDIN;
    while(PMMODEbits.BUSY == 1);  
    
    //Exit sequence
    PMWADDR = 0;
    PMDOUT = 0xf0;
    while(PMMODEbits.BUSY == 1);  
            

    PORTAbits.RA10 = 1;
    
    //If the manufacturer ID is not 0xC2 then set error bit 0
    if(Flash_MID != 0xc2)
    {
	lastError = lastError | 1;
    }
    
//******************************************************    
//          Get Device ID
//******************************************************    
    //CS2
    PORTAbits.RA10 = 0;
        
    //Get Chip ID
    //Enter sequence
    PMWADDR = 0x5555;
    PMDOUT = 0xaa;
    while(PMMODEbits.BUSY == 1);  
   
    PMWADDR = 0x2aaa;
    PMDOUT = 0x55;
    while(PMMODEbits.BUSY == 1);  
    
    PMWADDR = 0x5555;
    PMDOUT = 0x90;
    while(PMMODEbits.BUSY == 1);  
            
    Delay32(0, dly);
    
    //read device ID
    PMRADDR = 0x1;
    
    //dummy read for PMP
    Flash_DID = PMRDIN;
    while(PMMODEbits.BUSY == 1); 
    
    Flash_DID = PMRDIN;
    while(PMMODEbits.BUSY == 1);  
    
    //Exit sequence
    PMWADDR = 1;
    PMDOUT = 0xf0;
    while(PMMODEbits.BUSY == 1);  
            
    Delay32(0, dly);
    
    //CS2
    PORTAbits.RA10 = 1;    
    
    //If device code is not 0x4F the set error bit 1
    if(Flash_DID != 0x4f)
    {
	lastError = lastError | 2;
    }

}

void Flash_High_Address(unsigned address_flash)
{
    //Handle the upper address pins (A16-18)
    //0000-ffff
    if(address_flash > 0xffff)
    {
        //RB2 = A16
        PORTBbits.RB2 = 1;

        //RB4 = A17
        PORTBbits.RB4 = 0;

        //RB3 = A18
        PORTBbits.RB3 = 0;
    }
    
    //10000-1ffff
    if(address_flash > 0x1ffff)
    {
        //RB2 = A16
        PORTBbits.RB2 = 0;

        //RB4 = A17
        PORTBbits.RB4 = 1;

        //RB3 = A18
        PORTBbits.RB3 = 0;
    }
    
    //20000-2ffff
    if(address_flash > 0x2ffff)
    {
        //RB2 = A16
        PORTBbits.RB2 = 1;

        //RB4 = A17
        PORTBbits.RB4 = 1;

        //RB3 = A18
        PORTBbits.RB3 = 0;
    }
    
    //30000-3ffff
    if(address_flash > 0x3ffff)
    {
        //RB2 = A16
        PORTBbits.RB2 = 0;

        //RB4 = A17
        PORTBbits.RB4 = 0;

        //RB3 = A18
        PORTBbits.RB3 = 1;
    }
    
     //40000-4ffff
    if(address_flash > 0x4ffff)
    {
        //RB2 = A16
        PORTBbits.RB2 = 1;

        //RB4 = A17
        PORTBbits.RB4 = 0;

        //RB3 = A18
        PORTBbits.RB3 = 1;
    }
    
    //50000-5ffff
    if(address_flash > 0x5ffff)
    {
        //RB2 = A16
        PORTBbits.RB2 = 0;

        //RB4 = A17
        PORTBbits.RB4 = 1;

        //RB3 = A18
        PORTBbits.RB3 = 1;
    }
    
    //60000-6ffff
    if(address_flash > 0x6ffff)
    {
        //RB2 = A16
        PORTBbits.RB2 = 1;

        //RB4 = A17
        PORTBbits.RB4 = 1;

        //RB3 = A18
        PORTBbits.RB3 = 1;
    }    
}

void Flash_RD(unsigned address_flash)
{  
    //Handle the upper address pins (A16-18)
    Flash_High_Address(address_flash);

    //Clear off upper bits and load in to PMP
    PMRADDR = address_flash & 0x0ffff;
    
    ///CS2
    PORTAbits.RA10 = 0;
        
    //dummy read
    data_flash = PMRDIN;
    while(PMMODEbits.BUSY == 1);
   
    data_flash = PMRDIN;
    while(PMMODEbits.BUSY == 1);

    ///CS2
    PORTAbits.RA10 = 1;
    
    //RB2 = A16
    PORTBbits.RB2 = 0;

    //RB4 = A17
    PORTBbits.RB4 = 0;

    //RB3 = A18
    PORTBbits.RB3 = 0;
}

void Flash_WR(unsigned address_flash, uint8_t data_flash)
{        
    LED_Port(2);
    //Handle the upper address pins (A16-18)
    Flash_High_Address(address_flash);
    
    ///CS2
    PORTAbits.RA10 = 0;
    
    //Enter sequence
    PMWADDR = 0x5555;
    PMDOUT = 0xaa;
    while(PMMODEbits.BUSY == 1);  
   
    PMWADDR = 0x2aaa;
    PMDOUT = 0x55;
    while(PMMODEbits.BUSY == 1);  
    
    PMWADDR = 0x5555;
    PMDOUT = 0xa0;
    while(PMMODEbits.BUSY == 1);  
            
    //write data
    //Clear off upper bits and load in to PMP
    PMWADDR = address_flash & 0x0ffff;
    PMRADDR = PMWADDR;
    
    PMDOUT = data_flash;
    while(PMMODEbits.BUSY == 1); 
    
    uint8_t j = 0;
    uint8_t i = PMRDIN;
    //Wait for end of program
    //using D7 - Data# polling
    
    while(data_flash != i)
    {
        i = PMRDIN;
        while(PMMODEbits.BUSY == 1);
        j++;
        LED_Port(j);
    }
    
    j = 0;
    
    //now we must verify at least 2 times
    //to prevent false rejection
    
    
    while(i != data_flash)
    {
        i = PMRDIN;
        while(PMMODEbits.BUSY == 1);
        j++;
        LED_Port(j);

    }
    LED_Port(0);
    ///CS2
    PORTAbits.RA10 = 1;
    
    //RB2 = A16
    PORTBbits.RB2 = 0;

    //RB4 = A17
    PORTBbits.RB4 = 0;

    //RB3 = A18
    PORTBbits.RB3 = 0; 
}

void Flash_Sector_Erase(int erase_sector)
{  
    data_flash = 0;
    
    erase_sector = erase_sector << 12;
    ///CS2
    PORTAbits.RA10 = 0;
    
    //Enter sequence
    PMWADDR = 0x5555;
    PMDOUT = 0xaa;
    while(PMMODEbits.BUSY == 1);  
   
    PMWADDR = 0x2aaa;
    PMDOUT = 0x55;
    while(PMMODEbits.BUSY == 1);  
    
    PMWADDR = 0x5555;
    PMDOUT = 0x80;
    while(PMMODEbits.BUSY == 1);  
            
    PMWADDR = 0x5555;
    PMDOUT = 0xaa;
    while(PMMODEbits.BUSY == 1);  
   
    PMWADDR = 0x2aaa;
    PMDOUT = 0x55;
    while(PMMODEbits.BUSY == 1);  
    
    //Sector address
    PMWADDR = erase_sector;
    PMDOUT = 0x30;
    while(PMMODEbits.BUSY == 1);  
    int j = 0;
    while(data_flash != 0xff)
    {
        data_flash = PMRDIN;
        while(PMMODEbits.BUSY == 1);
        
        j++;
        LED_Port(j);
    }
    //CS2
    PORTAbits.RA10 = 1;
}

void Flash_Chip_Erase(void)
{  
    int i;
    data_flash = 0;
    
    //CS2
    PORTAbits.RA10 = 0;
   
    //Enter sequence
    PMWADDR = 0x5555;
    PMDOUT = 0xaa;
    while(PMMODEbits.BUSY == 1);  
   
    PMWADDR = 0x2aaa;
    PMDOUT = 0x55;
    while(PMMODEbits.BUSY == 1);  
    
    PMWADDR = 0x5555;
    PMDOUT = 0x80;
    while(PMMODEbits.BUSY == 1);  
            
    PMWADDR = 0x5555;
    PMDOUT = 0xaa;
    while(PMMODEbits.BUSY == 1);  
   
    PMWADDR = 0x2aaa;
    PMDOUT = 0x55;
    while(PMMODEbits.BUSY == 1);  
    
    PMWADDR = 0x5555;
    PMDOUT = 0x10;
    while(PMMODEbits.BUSY == 1);  
    
    while(data_flash != 0xff)
    {
        data_flash = PMRDIN;
        while(PMMODEbits.BUSY == 1);
    }

    //CS2
    PORTAbits.RA10 = 1;
}

void Flash_Get_Bytes_Used(void)
{  
    unsigned long i;
    Bytes_used = 0;
    
    if(Flash_DID == 0xd5)
    {
        for(i=0; i <= 0x1ffff; i++)
        {
            Flash_RD(i);
            if(data_flash != 0xff)
            {
                Bytes_used++;
            }
        }
    }
    if(Flash_DID == 0xd6)
    {
        for(i=0; i <= 0x2ffff; i++)
        {
            Flash_RD(i);
            if(data_flash != 0xff)
            {
                Bytes_used++;
            }
        }
    }
    
    if(Flash_DID == 0xd7)
    {
        for(i=0; i <= 0x7ffff; i++)
        {
            Flash_RD(i);
            if(data_flash != 0xff)
            {
                Bytes_used++;
            }
        }
    }
    
    PORTBbits.RB8 = 0;
    PORTAbits.RA5 = 0;
    PORTAbits.RA3 = 0;        

    return;
}


