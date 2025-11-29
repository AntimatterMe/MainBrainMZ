/*********************************************************************
    FileName:     	Touch.c
    Dependencies:	See #includes
    Processor:		PIC32MZ
    Hardware:		MainBrain MZ
    Complier:		XC32 4.40, 4.45
    Author:		Larry Knight 2024

    Software License Agreement:
        This software is licensed under the Apache License Agreement

    Description:
        I2C code for capacitive touch display with FT5436 controller
        SCL frequency = 400KHz
        Address 0x38 (7-bit) or 0x70 (write) and 0x71 (read) (8-bit)
        5 touch points
    References:
        Understanding the I2C Bus - https://www.ti.com/lit/pdf/slva704 
        FocalTech Application Note for FT5426 - 5526 CTPM
        https://github.com/crystalfontz/CFAF320480C7-035Tx/blob/main/CFAF320480C7-035Tx/CFAF320480C7-035Tx.ino
        https://github.com/Bodmer/Touch_FT5436/blob/main/Touch_FT5436.cpp

    File Description:

    Change History:

/***********************************************************************/

#include <xc.h>
#include <p32xxxx.h>
#include <proc/p32mz2048efh100.h>
#include <sys/attribs.h>
#include <sys/kmem.h>
#include <stdint.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MainBrain.h"

//Register addresses
#define FT_REG_MODE 0x00                //Device mode, either WORKING or FACTORY
#define FT_REG_NUMTOUCHES 0x02          //Number of touch points
#define FT_REG_CALIBRATE 0x02           //Calibrate mode
#define FT_TP1_REG (0X03)
#define FT_TP2_REG (0X09)
#define FT_REG_FACTORYMODE 0x40         //Factory mode
#define FT_REG_THRESHHOLD 0x80          //Threshold for touch detection
#define FT_REG_POINTRATE 0x88           //Point rate
#define FT_ID_G_LIB_VERSION (0xA1)
#define FT_REG_FIRMVERS 0xA6            //Firmware version
#define FT_REG_CHIPID 0xA3              //Chip selecting
#define FT_ID_G_MODE (0xA4)
#define FT_REG_VENDID 0xA8              //FocalTech's panel ID
#define NVM_ADDR_DATA_WR (0xD0)

//FocalTech ID's
#define FT6234_VENVID 0x79  // FocalTech's panel ID
#define FT6236_VENDID 0x11  // FocalTech's panel ID
#define FT5436_VENDID 0x79  // FocalTech's panel ID
#define FT6206_CHIPID 0x06  // FT6206 ID
#define FT6234_CHIPID 0x54  // FT6234 ID
#define FT6236_CHIPID 0x36  // FT6236 ID
#define FT5436_CHIPID 0x54  // FT5436 ID
#define FT6236U_CHIPID 0x64 // FT6236U ID

uint8_t address;
uint32_t reg_data_buf;
uint8_t read_buf[16];
int wait_time = 10000;
uint16_t scn_pos_x;

uint16_t scn_pos_y;

uint8_t MenuLevel = 0;
uint8_t ButtonNum = 0;
bool ExitButton = false;
bool isActive = false;

void FT5436_Write_Reg(uint8_t reg, int8_t reg_data);
void FT5436_Read_Reg(uint8_t reg);
void I2C_wait_for_idle(void);
void I2C_start(void);
void I2C_stop(void);
void I2C_restart(void);
void I2C_ack(void);
void I2C_nack(void);
void FT5436_Read(uint8_t num_bytes);

void I2C_init()
{    
    //FT5436 Device Address
    address = 0x38;
    
    //Touch Controller Reset
    TRISBbits.TRISB13 = 0;
    PORTBbits.RB13 = 1;
    
    //INT pin 
    TRISDbits.TRISD0 = 1;
    //PORTDbits.RD0 = 1;
    
    //RA2 - Pin 59 - SCL2
    TRISAbits.TRISA2 = 1;
    
    //RA3 - Pin 60 - SDA2
    TRISAbits.TRISA3 = 1;
     
    //Disable Slew Rate Control bit
    //I2C2CONbits.DISSLW = 1;
    
    //BAUD rate - 400KHz
    //set with the scope where one clock period is 2.5uS (400KHz)
    I2C2BRG = 0x0069; 
    
    //I2C-2 Interrupt
    IFS0bits.IC2IF = 0; 
    IEC0bits.IC2IE = 1;
    
    //Master
    IPC37bits.I2C2MIP = 4;
    IPC37bits.I2C2MIS = 3;
    
    IFS4bits.I2C2MIF = 0;
    IEC4bits.I2C2MIE = 0;
    
    //Slave
    IPC37bits.I2C2SIP = 4;
    IPC37bits.I2C2SIS = 2;
    
    IEC4bits.I2C2SIE = 0;
    IFS4bits.I2C2SIF = 0;
            
    //INT0 
    IPC0bits.INT0IP = 4;
    IPC0bits.INT0IS = 1;
    
    IFS0bits.INT0IF = 0;
    IEC0bits.INT0IE = 1;
    
    
    //SDA Hold Time 0 = 100nS, 1 = 300nS
    I2C2CONbits.SDAHT = 0;
    
    I2C2CONbits.PCIE = 1;

    //Enable module
    I2C2CONbits.ON = 1;
    
    //RESET Touch Controller
    //Should be low for 10mS
    int i;
    for(i=0;i<10000;i++)
    {
        PORTBbits.RB13 = 0;
    }
   
    PORTBbits.RB13 = 1;
    for(i=0;i<100000;i++);
    
//    //If there is no touch panel is detected by the time Timer 5 rolls over
//    //the wait for the device completes and returns to the calling function
//    //Start Timer 5
//    T5CONbits.TCKPS = 0;
//    IFS0bits.T5IF = 0;
//    T5CONbits.ON = 0;
//    IFS0bits.T5IF
//    
    //Check for device present    
    if(!Device_Present())
    {
        return;
    }
    
    //Set mode
    FT5436_Write_Reg(FT_REG_MODE, 0);
    
    //ID_G_MODE set interrupt
	//0x00 - Polling Mode
	//0x01 - Trigger Mode
    FT5436_Write_Reg(FT_ID_G_MODE, 0x00);
    
    //Touch Threshold
    FT5436_Write_Reg(FT_REG_THRESHHOLD, 20);

	//Reporting rate
	FT5436_Write_Reg(FT_REG_POINTRATE, 12);   
    
    FT5436_Write_Reg(0x86, 0x0);
}

//INT0 Interrupt handler
void __attribute__((vector(_EXTERNAL_0_VECTOR), interrupt(ipl4srs), nomips16)) INT0_Handler()
{
    FT5436_Read(16);
    
    //if there is a touch but only 1 touch point
    if(read_buf[2] == 1)
    {
        scn_pos_x = read_buf[5];
        scn_pos_x = scn_pos_x << 8;
        scn_pos_x = scn_pos_x + read_buf[6];

        scn_pos_y = read_buf[3] - 128;
        scn_pos_y = scn_pos_y << 8;
        scn_pos_y = scn_pos_y + read_buf[4];

        if(scn_pos_y > 320)
        {
            scn_pos_y = 320;
        }

        scn_pos_y = 320 - scn_pos_y;        
    }
    
    IFS0bits.INT0IF = 0;
}

bool Device_Present(void)
{
    int i;
    //******************************************************
    //Now we have to determine if there is a device present
    //The display is optional and should not be required
    //******************************************************
    I2C_start();
    
    //write to slave
    I2C2TRN = 0x70;                         // Send slave address with Read/Write bit cleared
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle

    //wait a finite amount of time then RETURN if not acknowledged by device
    for(i=0;i<wait_time;i++)
    {
        if(I2C2STATbits.ACKSTAT == 0)
        {
            return true;
            i = wait_time;           
        }
    }
    if(I2C2STATbits.ACKSTAT == 1)
    {
        return false;
    }   
    //******************************************************
}

//Writes a byte to the specified register
void FT5436_Write_Reg(uint8_t reg, int8_t reg_data)
{
    I2C_start();
    
    //write to slave
    I2C2TRN = 0x70;                         // Send slave address with Read/Write bit cleared
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received  

    I2C2TRN = reg;                          // Register
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received
    
    I2C2TRN = reg_data;                     // Register data
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received
    
    I2C_stop();
    
}

//Reads a byte from the specified register and stores it in reg_data_buf
void FT5436_Read_Reg(uint8_t reg)
{
    I2C_start();
    
    //write to slave
    I2C2TRN = 0x70;                         // Send slave address with Read/Write bit cleared
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received  

    I2C2TRN = reg;                          // Register
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received
    
    I2C_restart();
    
    //write to slave
    I2C2TRN = 0x71;                         // Send slave address with Read/Write bit cleared
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received  

    I2C2CONbits.RCEN = 1;                   // Receive enable
    while (I2C2CONbits.RCEN);               // Wait until RCEN is cleared (automatic)  
    while (!I2C2STATbits.RBF);              // Wait until Receive Buffer is Full (RBF flag)  
    reg_data_buf = I2C2RCV;                 // Retrieve value from I2C2RCV

    I2C_nack();
    I2C_stop(); 
}

//Reads 16 bytes from the device
void FT5436_Read(uint8_t num_bytes)
{
    int i;
    
    //read at least 1 byte
    if(!num_bytes)
    {
        num_bytes;
    }
    
    //Send start condition
    I2C_start();
    
    //write to slave
    I2C2TRN = 0x70;                         // Send slave address with Read/Write bit cleared
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received  

    I2C2TRN = 0;                            // Device mode
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received
    
    I2C_restart();
    
    //Read from slave
    I2C2TRN = 0x71;                         // Send slave address with Read/Write bit set
    while (I2C2STATbits.TBF == 1);          // Wait until transmit buffer is empty
    I2C_wait_for_idle();                    // Wait until I2C bus is idle
    while (I2C2STATbits.ACKSTAT == 1);      // Wait until ACK is received  

    for(i=0;i<num_bytes;i++)
    {
        I2C2CONbits.RCEN = 1;                   // Receive enable
        while (I2C2CONbits.RCEN);               // Wait until RCEN is cleared (automatic)  
        while (!I2C2STATbits.RBF);              // Wait until Receive Buffer is Full (RBF flag)  
        read_buf[i] = I2C2RCV;                 // Retrieve value from I2C2RCV
        I2C_ack();
    }
    
    I2C2CONbits.RCEN = 1;                   // Receive enable
    while (I2C2CONbits.RCEN);               // Wait until RCEN is cleared (automatic)  
    while (!I2C2STATbits.RBF);              // Wait until Receive Buffer is Full (RBF flag)  
    read_buf[15] = I2C2RCV;                 // Retrieve value from I2C2RCV
    
    I2C_nack();
    I2C_stop(); 
}

//DRIVER FOUNDATION 
//Waits until the I2C peripheral is no longer doing anything  
void I2C_wait_for_idle(void)
{
    while(I2C2CON & 0x1F); // Acknowledge sequence not in progress
                                // Receive sequence not in progress
                                // Stop condition not in progress
                                // Repeated Start condition not in progress
                                // Start condition not in progress
    while(I2C2STATbits.TRSTAT); // Bit = 0 Master transmit is not in progress
}

//Sends a start condition  
void I2C_start(void)
{
    I2C_wait_for_idle();
    I2C2CONbits.SEN = 1;
    while (I2C2CONbits.SEN == 1);
}

//Sends a stop condition  
void I2C_stop(void)
{
    I2C_wait_for_idle();
    I2C2CONbits.PEN = 1;
}

//Sends a repeated start/restart condition
void I2C_restart(void)
{
    I2C_wait_for_idle();
    I2C2CONbits.RSEN = 1;
    while (I2C2CONbits.RSEN == 1);
}

//Sends an ACK condition
void I2C_ack(void)
{
    I2C_wait_for_idle();
    I2C2CONbits.ACKDT = 0; // Set hardware to send ACK bit
    I2C2CONbits.ACKEN = 1; // Send ACK bit, will be automatically cleared by hardware when sent  
    while(I2C2CONbits.ACKEN); // Wait until ACKEN bit is cleared, meaning ACK bit has been sent
}

//Sends a NACK condition
void I2C_nack(void) // Acknowledge Data bit
{
    I2C_wait_for_idle();
    I2C2CONbits.ACKDT = 1; // Set hardware to send NACK bit
    I2C2CONbits.ACKEN = 1; // Send NACK bit, will be automatically cleared by hardware when sent  
    while(I2C2CONbits.ACKEN); // Wait until ACKEN bit is cleared, meaning NACK bit has been sent
}

//Master Interrupt Handler
void __attribute__((vector(_I2C1_MASTER_VECTOR), interrupt(ipl4srs), nomips16)) I2C_Master_Handler()
{

    IFS4bits.I2C2MIF = 0;
}

//Slave Interrupt Handler
void __attribute__((vector(_I2C2_SLAVE_VECTOR), interrupt(ipl4srs), nomips16)) I2C_Slave_Handler()
{
    
    IFS0bits.IC2IF = 0;
}