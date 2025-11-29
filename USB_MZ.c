/*********************************************************************
    FileName:     	USB_MZ.c
    Dependencies:	See #includes
    Processor:		PIC32MZ2048EFH100
    Hardware:		MainBrain MZ
    Complier:		XC32 4.40
    Author:		Larry Knight 2023

    Software License Agreement:
        This software is licensed under the Apache License Agreement

    Description:
        Enumerates as a High Speed interface class device,
        Uses Microsoft OS Descriptors to load a Winusb driver,
        Endpoint 1 is the receiving endpoint,
        Endpoint 2 is the transmitting endpoint,
        Host application sends commands to the device, 
        Device responds to the commands by sending requested data to the Host

    Device Interface GUID:
        2b8a8216-c82a-4a91-a8bc-a12129d2d70b

    References:        
        https://techcommunity.microsoft.com/t5/microsoft-usb-blog/how-does-usb-stack-enumerate-a-device/ba-p/270685#_Configuration_Descriptor_Request

    Registry Stuff:
        HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\usbflags\120900010200
        HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\USB\VID_1209&PID_0001\MainBrain_MZ

    Change History:
        Basic framework completed 06/16/2024

/***********************************************************************/

#include <xc.h>
#include <string.h>
#include <stdbool.h>
#include "MainBrain.h"

bool isConnected = false;
const uint8_t seqSize = 16;

union
{
    float fval;
    uint8_t bytes[4];
} converter;

typedef struct
{
    volatile unsigned char bmRequestType;
    volatile unsigned char bRequest;
    volatile unsigned short wValue;
    volatile unsigned short wIndex;
    volatile unsigned short wLength;
} USB_TRANSACTION;

USB_TRANSACTION USB_transaction;

USB_ENDPOINT EP[3];

uint8_t device_descriptor[] = 
{
    /* Descriptor Length			*/ 0x12, //Size of this descriptor in bytes
    /* DescriptorType: DEVICE			*/ 0x01,
    /* bcdUSB (ver 2.0)				*/ 0x00,0x02,
    /* bDeviceClass				*/ 0x00,
    /* bDeviceSubClass				*/ 0x00,
    /* bDeviceProtocol				*/ 0x00,
    /* bMaxPacketSize0				*/ 0x40, //0x40 for High Speed USB
    /* idVendor					*/ 0x09,0x12, /*VID */
    /* idProduct				*/ 0x01,0x00, 
    /* bcdDevice				*/ 0x00,0x02, 
    /* iManufacturer				*/ 0x01,
    /* iProduct					*/ 0x02,
    /* iSerialNumber				*/ 0x02, 
    /* bNumConfigurations			*/ 0x01
};

uint8_t config_descriptor[] = 
{
    // Configuration Descriptor
    0x09,                       //Descriptor size in bytes
    0x02,                       //Descriptor type
    0x20,0x00,                  //Total length of data
    0x01,                       //Number of interfaces
    0x01,                       //Index value of this configuration
    0x00,                       //Configuration string index
    0xc0,                       // Attributes, see usb_device.h
    0x32,                       // Max power consumption (2X mA)
							
    // Interface Descriptor
    0x09,                       // Size of this descriptor in bytes
    0x04,                       // INTERFACE descriptor type
    0x00,                       // Interface Number
    0x00,                       // Alternate Setting Number
    0x02,                       // Number of endpoints in this intf
    0x00,                       // Class code
    0x00,                       // Subclass code
    0x00,                       // Protocol code
    0x00,                       // Interface string index
    
    // Endpoint Descriptor
    //EP01 OUT
    0x07,                       //Size of this descriptor in bytes
    0x05,                       //Endpoint Descriptor
    0x01,                       //EndpointAddress
    0x02,                       //Attributes
    0x40,0x00,                  //size
    0x00,                       //Interval   
    //EP02 IN                      
    0x07,                       //Size of this descriptor in bytes
    0x05,                       //Endpoint Descriptor
    0x82,                       //EndpointAddress
    0x02,                       //Attributes
    0x40,0x00,                  //size
    0x00                        //Interval
};

uint8_t device_qualifier[] = 
{
    0x0a,                       //Size of this descriptor in bytes
    0x06,                       //Descriptor type (0x06)
    0x00, 0x02,                 //BCD - USB version number (Must be 0x200 or higher)
    0xff,                       //Class Code
    0xff,                       //Subclass Code
    0xff,                       //Protocol
    0x40,                       //bMaxPacketSize0
    0x01,                       //bNumConfigurations
    0x00                        //Reserved
};

uint8_t MSOSDescriptor[] =
{   
    //bLength - length of this descriptor in bytes
    0x0b,                           
    //bDescriptorType - "string"
    0x03,                           
    //qwSignature - special values that specifies the OS descriptor spec version that this firmware implements
    'M',0,'S',0,'F',0,'T',0,'1',0,'0',0,'0',0,
    //bMS_VendorCode - defines the "GET_MS_DESCRIPTOR" bRequest literal value
    0xee,
    //bFlags
    //a new flags field has been added to the Microsoft OS string descriptor that can be used to indicate support for the ContainerID descriptor
    //Bit 1 of this field is used to indicate support for the ContainerID descriptor
    0x00                            
};    

//Extended Compatability ID Feature Descriptor
uint8_t ExtCompatIDFeatureDescriptor[] =
{
    0x28, 0x00, 0x00, 0x00,                             /* dwLength Length of this descriptor */
    0x00, 0x01,                                         /* bcdVersion = Version 1.0 */
    0x04, 0x00,                                         /* wIndex = 0x0004 */
    0x01,                                               /* bCount = 1 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,           /* Reserved */
    0x00,                                               /* Interface number = 0 */
    0x01,                                               /* Reserved */
    0x57, 0x49, 0x4E, 0x55, 0x53, 0x42, 0x00, 0x00,     /* compatibleID */ //WINUSB
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /* subCompatibleID */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00                  /* Reserved */
};
    
uint8_t ExtPropertyFeatureDescriptor[] =
{
    //----------Header Section--------------
    0x8e, 0x00, 0x00, 0x00,                             //dwLength (4 bytes)
    0x00, 0x01,                                         //bcdVersion = 1.00
    0x05, 0x00,                                         //wIndex
    0x01, 0x00,                                         //wCount - 0x0001 "Property Sections" implemented in this descriptor
    //----------Property Section 1----------
    0x84, 0x00, 0x00, 0x00,                             //dwSize - 132 bytes in this Property Section
    0x01,0x00, 0x00, 0x00,                              //dwPropertyDataType (Unicode string)
    0x28, 0x00,                                         //wPropertyNameLength - 40 bytes in the bPropertyName field
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0,
        'e', 0, 'G', 0, 'U', 0, 'I', 0, 'D', 0,  0x00, 0x00,  //bPropertyName - "DeviceInterfaceGUID"
    0x4e, 0x00, 0x00, 0x00,                             //dwPropertyDataLength - 78 bytes in the bPropertyData field (GUID value in UNICODE formatted string, with braces and dashes)
    //Device Interface GUID
    //{2b8a8216-c82a-4a91-a8bc-a12129d2d70b}
    '{', 0, '2', 0, 'b', 0, '8', 0, 'a', 0, '8', 0, '2', 0, '1', 0, '6', 0, '-', 0, 'c', 0, '8', 0, '2', 0, 'a', 0, 
        '-', 0, '4', 0, 'a', 0, '9', 0, '1', 0, '-', 0, 'a', 0, '8', 0, 'b', 0, 'c', 0, '-', 0, 'a', 0, '1', 0, '2', 
        0, '1', 0, '2', 0, '9', 0, 'd', 0, '2', 0, 'd', 0, '7', 0, '0', 0, 'b', 0, '}', 0, 0x00, 0x00
    //{57c3b8e0-852f-11d0-bf32-00a0c90ab50f}
    //'{', 0, '5', 0, '7', 0, 'c', 0, '3', 0, 'b', 0, '8', 0, 'e', 0, '0', 0, '-', 0, '8', 0, '5', 0, '2', 0, 'f', 0, 
    //    '-', 0, '1', 0, '1', 0, 'd', 0, '0', 0, '-', 0, 'b', 0, 'f', 0, '3', 0, '2', 0, '-', 0, '0', 0, '0', 0, 'a', 
     //   0, '0', 0, 'c', 0, '9', 0, '0', 0, 'a', 0, 'b', 0, '5', 0, '0', 0, 'f', 0, '}', 0, 0x00, 0x00
};    

//Language - 0x0409 - English
uint8_t string0[] =  {4, 0x03, 0x09, 0x04};

//iManufacturer
uint8_t string1[] = {28, 3, 'A', 0, 'n', 0, 't', 0, 'i', 0, 'm', 0, 'a', 0, 't', 0, 't', 0, 'e', 0, 'r', 0, '.', 0, 'm', 0, 'e', 0};   

//iProduct	
uint8_t string2[] = {26, 3, 'M', 0, 'a', 0, 'i', 0, 'n', 0, 'B', 0, 'r', 0, 'a', 0, 'i', 0, 'n', 0, ' ', 0, 'M', 0, 'Z', 0};
 
//iSerialNumber	
uint8_t string3[] = {10, 3, '0', 0, '0', 0, '0', 0, '1', 0};

void EP0_control_transaction(void);
void USB_queue_EP0(uint8_t *buffer, int size, int max_size);
void EP0_RX(int length);
void EP0_TX(void);
void Host_CMDs(void);

int runCount = 0;
int EP1_RX(void);
int EP2_TX(volatile uint8_t *tx_buffer);
int EP0_Wait_TXRDY(void);
int EP2_Wait_TXRDY(void);
volatile uint8_t usbAddress;
volatile bool SetAddress = true;
volatile uint8_t USBState;
volatile uint8_t DeviceState;
uint8_t cmd = 0;
uint8_t data0;
uint8_t data1;
uint8_t data2;
uint8_t data3;
uint8_t data4;
bool inSequence = false;
bool isBusy = false;

void USB_init(void)
{
    //Disable the module
    USBCSR0bits.SOFTCONN = 0;     

    //Set the initial state of the device
    USBState = DETACHED;
    DeviceState = DISCONNECTED;
    
    //disable while module is setup
    USBCSR0bits.SOFTCONN = 0;   
    
    //EP 1
    //These bits select which endpoint registers are accessed through addresses 0x3010-0x301F
    USBCSR3bits.ENDPOINT = 1;

    //RX
    USBOTGbits.RXFIFOSZ = 0x06;
    USBIENCSR1bits.RXMAXP = 64;
    USBIENCSR3bits.RXFIFOSZ = 0x09;
    USBFIFOAbits.RXFIFOAD = 0x0280;
    USBIENCSR3bits.PROTOCOL = 0x02;
    USBIENCSR3bits.TEP = 0x01;
    USBIENCSR1bits.FLUSH = 1;
    
    
    //TX
    USBOTGbits.TXFIFOSZ = 0x06;
    USBIENCSR0bits.TXMAXP = 64;
    USBIENCSR3bits.TXFIFOSZ = 0x09;
    USBFIFOAbits.TXFIFOAD = 0x0080;
    USBIENCSR2bits.PROTOCOL = 0x02;
    USBIENCSR2bits.TEP = 0x02;
    USBIENCSR0bits.FLUSH = 1;

    //Endpoint 1 is RX
    USBE1CSR0bits.MODE = 0;
    
    //Endpoint 2 is TX
    USBE2CSR0bits.MODE = 1;
    
    // Set endpoint 0 buffer to 64 bytes (multiples of 8).
    USBE0CSR0bits.TXMAXP = 64; 

    //Clear the address
    usbAddress = 0;                
    USBCSR0bits.FUNC = 0;   
    
    //VBUS Monitoring ON
    USBCRCONbits.VBUSMONEN = 1;
    
    //Enable the reset interrupt
    USBCSR2bits.RESETIE = 1;    
    
    //Enable the USB interrupt
    IEC4bits.USBIE = 1;    
    
    //Enable USB module interrupt
    USBCRCONbits.USBIE = 1;     
    
    //Clear the USB interrupt flag.
    IFS4bits.USBIF = 0;         
    
    //USB Interrupt Priority 7
    //Must be 7. Cannot use any other priority.
    //Internally, the USB hardware expects SRS context switching 
    //to avoid stack usage ? that?s why priority of 7 is required 
    IPC33bits.USBIP = 7;        
    
    //See DISNYET (same bit as PIDERR)
    USBE1CSR1bits.PIDERR = 1;   

    //Enable High Speed (480Mbps) USB mode
    USBCSR0bits.HSEN = 1;      
    
    //Enable the module
    USBCSR0bits.SOFTCONN = 1;     
}

//USB
void __attribute__((vector(_USB_VECTOR), interrupt(ipl7srs), nomips16)) USB_handler()
{       
    //Reset
    if(USBCSR2bits.RESETIF)
    {
        USBState = DETACHED;
        
        // 1 = Endpoint is TX
        USBE1CSR0bits.MODE = 1;     
        
        // Set endpoint 0 buffer to 64 bytes (multiples of 8)
        USBE0CSR0bits.TXMAXP = 64; 
        
        // Endpoint 0 Operating Speed Control bits
        USBE0CSR2bits.SPEED = 1;
        
        // Endpoint 1: TX Endpoint Operating Speed Control bits - High speed        
        USBE1CSR2bits.SPEED = 1;        
        
        // Endpoint 1 - Maximum TX Payload Per Transaction Control bits
        USBE1CSR0bits.TXMAXP = 64;
        
        // Endpoint 1 - TX Endpoint Protocol Control bits 
        USBE1CSR2bits.PROTOCOL = 2; 

        //PROTOCOL<1:0>: RX/TX Endpoint Protocol Control bits 
        //11 = Interrupt
        //10 = Bulk
        //01 = Isochronous
        //00 = Control
        
        USBCSR1bits.EP1TXIE = 1;    // Endpoint 1 TX interrupt enable
        USBCSR2bits.EP1RXIE = 1;    // Endpoint 1 RX interrupt enable
                    
        USBCSR2bits.RESETIF = 0;
    }
    
    /* Endpoint 0 Interrupt Handler */
    if(USBCSR0bits.EP0IF == 1)
    { 
        // Do we need the set the USB address?
        if (SetAddress == true)
        {
            //This sets a limit of 127
            USBCSR0bits.FUNC = usbAddress & 0x7F;
            SetAddress = false;
        }
        
        if(USBE0CSR0bits.RXRDY)
        {
            EP0_RX(USBE0CSR2bits.RXCNT);
            
            USB_transaction.bmRequestType = EP[0].rx_buffer[0];
            USB_transaction.bRequest = EP[0].rx_buffer[1];
            USB_transaction.wValue = (int)(EP[0].rx_buffer[3] << 8) | EP[0].rx_buffer[2];
            USB_transaction.wIndex = (int)(EP[0].rx_buffer[5] << 8) | EP[0].rx_buffer[4];
            USB_transaction.wLength = (int)(EP[0].rx_buffer[7] << 8) | EP[0].rx_buffer[6];
            
            EP0_control_transaction();
            
            // End of Data Control bit (Device mode) 
            if (USB_transaction.wLength == 0)
            {
                USBE0CSR0bits.DATAEND = 1; 
            }
        }
                
        if (USBE0CSR0bits.SETEND) 
        {
            USBE0CSR0bits.SETENDC = 1;
        }
        
        // Clear the USB EndPoint 0 Interrupt Flag.
        USBCSR0bits.EP0IF = 0;  
    }
    
    //Endpoint 1 Interrupt Handler
    if(USBCSR1bits.EP1RXIF == 1)
    { 
        EP1_RX();
		
        Host_CMDs();
	
        USBCSR1bits.EP1RXIF = 0;
    }

    IFS4bits.USBIF = 0;   
}

void Host_CMDs()
{
  uint8_t test;
  uint8_t SeqNum;
  
  switch (EP[1].rx_buffer[0])
  {
      //connected
      case 0x00:
          if(EP[1].rx_buffer[1] == 0x02)
          {
              DeviceState = CONNECTED;
          }
          
          if(EP[1].rx_buffer[1] == 0x05)
          {
              DeviceState = DISCONNECTED;
          }
        break;
          
      //Data check
      case 0x01:
          EP[2].tx_buffer[0] = 0x55;
          EP2_TX(EP[2].tx_buffer);
        break;
        
      //Send Message
      case 0x02:
	strcpy(myStr, "USB - Test");
	  
	//set message pending
	Message = 1;
        break;
        
      //Back light
      case 0x03:
          Backlight_Control(EP[1].rx_buffer[1]); 
        break;
        
    
      case 0x04:
	  
        break;
        
      case 0x05:
	  
          break;
          
      case 0x06:
          //Run Sequence
          //Motion Command Byte
          REN70V05_WR(0x3ff, EP[1].rx_buffer[1]) ;
        break;
        
      case 0x07:    
          //Board comm check
          REN70V05_WR(0x3d3, 37);
          test = REN70V05_RD(0x3d4);
          EP[2].tx_buffer[0] = test;
          EP2_TX(EP[2].tx_buffer);
	  screen = DEBUG_SCREEN;
        break;
                        
      case 0x09:
          //Update Button
          //Update Speed
          REN70V05_WR(0x3f8, EP[1].rx_buffer[1]) ;
          
          //Update Direction
          REN70V05_WR(0x3ff, EP[1].rx_buffer[2]) ;
          
        break;

      case 0x0a:
          //Save Sequence
          //Sequence Number
          SeqNum = (EP[1].rx_buffer[1]) - 1;

          REN70V05_WR(0x300 + (SeqNum * seqSize), EP[1].rx_buffer[1]) ;
          //Flash_WR(0x300 + (SeqNum * seqSize), EP[1].rx_buffer[1]);
                  
          //Sequence Direction
          REN70V05_WR(0x307 + (SeqNum * seqSize), EP[1].rx_buffer[2]) ;
          //Flash_WR(0x307 + (SeqNum * seqSize), EP[1].rx_buffer[2]);
          
          //Sequence Acceleration
          REN70V05_WR(0x306 + (SeqNum * seqSize), EP[1].rx_buffer[3]) ;
          //Flash_WR(0x306 + (SeqNum * seqSize), EP[1].rx_buffer[3]);
          
          //Sequence Speed
          REN70V05_WR(0x308 + (SeqNum * seqSize), EP[1].rx_buffer[4]) ;
          //Flash_WR(0x308 + (SeqNum * seqSize), EP[1].rx_buffer[4]);
          
          //Sequence Deceleration
          REN70V05_WR(0x301 + (SeqNum * seqSize), EP[1].rx_buffer[5]) ;
          //Flash_WR(0x301 + (SeqNum * seqSize), EP[1].rx_buffer[5]);
          
          //Sequence Run Distance
          REN70V05_WR(0x302 + (SeqNum * seqSize), EP[1].rx_buffer[6]) ;
          REN70V05_WR(0x303 + (SeqNum * seqSize), EP[1].rx_buffer[7]) ;
          REN70V05_WR(0x304 + (SeqNum * seqSize), EP[1].rx_buffer[8]) ;
          REN70V05_WR(0x305 + (SeqNum * seqSize), EP[1].rx_buffer[9]) ;
                   
          //Flash_WR(0x302 + (SeqNum * seqSize), EP[1].rx_buffer[6]);
          //Flash_WR(0x303 + (SeqNum * seqSize), EP[1].rx_buffer[7]);
          //Flash_WR(0x304 + (SeqNum * seqSize), EP[1].rx_buffer[8]);
          //Flash_WR(0x305 + (SeqNum * seqSize), EP[1].rx_buffer[9]);

          //Sequence Stop Distance
          REN70V05_WR(0x309 + (SeqNum * seqSize), EP[1].rx_buffer[10]) ;
          REN70V05_WR(0x30a + (SeqNum * seqSize), EP[1].rx_buffer[11]) ;
          REN70V05_WR(0x30b + (SeqNum * seqSize), EP[1].rx_buffer[12]) ;
          REN70V05_WR(0x30c + (SeqNum * seqSize), EP[1].rx_buffer[13]) ;

          //Flash_WR(0x309 + (SeqNum * seqSize), EP[1].rx_buffer[10]);
          //Flash_WR(0x30a + (SeqNum * seqSize), EP[1].rx_buffer[11]);
          //Flash_WR(0x30b + (SeqNum * seqSize), EP[1].rx_buffer[12]);
          //Flash_WR(0x30c + (SeqNum * seqSize), EP[1].rx_buffer[13]);

          //Delay between Sequences
          REN70V05_WR(0x30d + (SeqNum * seqSize), EP[1].rx_buffer[14]) ;
          //Flash_WR(0x30d + (SeqNum * seqSize), EP[1].rx_buffer[14]);
          
          //Total Number of Sequences to run
          REN70V05_WR(0x30e + (SeqNum * seqSize), EP[1].rx_buffer[15]) ;
          //Flash_WR(0x30e + (SeqNum * seqSize), EP[1].rx_buffer[15]);
          
          //seqLoop
          REN70V05_WR(0x30f + (SeqNum * seqSize), EP[1].rx_buffer[16]) ;
          //Flash_WR(0x30f + (SeqNum * seqSize), EP[1].rx_buffer[16]);

          NeedsRefresh = false;
          
        break;

	case 0x0b:
	    screen = DEBUG_SCREEN;
	    break;
      //This is where we send the full 64 bytes of data whenever the 
      //Host requests it
      case 0x64:	  	

	//read the current board data into the tx buffer
	SRAM2USB();	
	//REN70V05_WR(((((current_board_address) - 1) * 0x400) + 10), 0x89);  
	

	
	  
	//ADC Data
	//Current
	ADC0_result = ADC0_result * 0.00161172 * 342;
	converter.fval = ADC0_result;

	EP[2].tx_buffer[3] = converter.bytes[0];
	EP[2].tx_buffer[2] = converter.bytes[1];
	EP[2].tx_buffer[1] = converter.bytes[2];
	EP[2].tx_buffer[0] = converter.bytes[3];

	//4095 = 3.3v  3.3 / 4095 = 0.000805861v per ADC bit 6/3 = 2
	//so 3.0V = 24V input
	ADC6_result = (ADC6_result * 0.000805861 * 7.56) + 0.7;
          
        //voltage 
        converter.fval = ADC6_result;
        
        EP[2].tx_buffer[7] = converter.bytes[0];          
        EP[2].tx_buffer[6] = converter.bytes[1];
        EP[2].tx_buffer[5] = converter.bytes[2];          
        EP[2].tx_buffer[4] = converter.bytes[3];
        
	REN70V05_RD(((((current_board_address) - 1) * 0x400) + 10));
	EP[2].tx_buffer[10] = mdata_70V05;
	
	//return the status data
	REN70V05_RD(((((current_board_address) - 1) * 0x400) + 20));
	EP[2].tx_buffer[20] = mdata_70V05;
	
//        //Motion data
//        //PWM Frequency        
//        REN70V05_RD(0x3de);
//        EP[2].tx_buffer[8] = mdata_70V05;
//        REN70V05_RD(0x3df);
//        EP[2].tx_buffer[9] = mdata_70V05;
//        
//        //Encoder Position
//        //LSB
//        REN70V05_RD(0x3f0);
//        EP[2].tx_buffer[10] = mdata_70V05;
//        REN70V05_RD(0x3f1);
//        EP[2].tx_buffer[11] = mdata_70V05;
//        REN70V05_RD(0x3f2);
//        EP[2].tx_buffer[12] = mdata_70V05;
//        //MSB
//        REN70V05_RD(0x3f3);
//        EP[2].tx_buffer[13] = mdata_70V05;
//        
//        //PWM Duty Cycle
//        uint16_t tmp; 
//        
//        //Get the PG4DC from SRAM
//        REN70V05_RD(0x3d7);
//        tmp = mdata_70V05;
//        tmp = tmp << 8;
//        REN70V05_RD(0x3d6);
//        tmp = tmp + mdata_70V05;
//        
//        //Convert to float
//        converter.fval = tmp;
//        
//        //Send Float bytes to Host
//        EP[2].tx_buffer[14] = converter.bytes[0];
//        EP[2].tx_buffer[15] = converter.bytes[1];
//        EP[2].tx_buffer[16] = converter.bytes[2];
//        EP[2].tx_buffer[17] = converter.bytes[3];
//        
//        //Index Counter
//        //LSB
//        REN70V05_RD(0x3eb);
//        EP[2].tx_buffer[18] = mdata_70V05;
//        REN70V05_RD(0x3ec);
//        EP[2].tx_buffer[19] = mdata_70V05;
//        REN70V05_RD(0x3ed);
//        EP[2].tx_buffer[20] = mdata_70V05;
//        //MSB
//        REN70V05_RD(0x3ee);
//        EP[2].tx_buffer[21] = mdata_70V05;

//        //Speed
//        REN70V05_RD(0x3f8);
//        EP[2].tx_buffer[22] = mdata_70V05;
//        
//        //Control Byte
//        REN70V05_RD(0x3ff);
//        EP[2].tx_buffer[23] = mdata_70V05;
//        
//        //Status Byte
//        REN70V05_RD(0x3fe);
//        EP[2].tx_buffer[24] = mdata_70V05;
               
//        EP2_TX(EP[2].tx_buffer);  
        
//        //Get the Sequence data
//        //Sequence Number
//        REN70V05_RD(0x300);
//        EP[2].tx_buffer[25] = mdata_70V05;
//        
//        //Deceleration
//        REN70V05_RD(0x301);
//        EP[2].tx_buffer[26] = mdata_70V05;
//        
//        //Sequence Distance
//        REN70V05_RD(0x302);
//        EP[2].tx_buffer[27] = mdata_70V05;        
//        //Sequence Distance
//        REN70V05_RD(0x303);
//        EP[2].tx_buffer[28] = mdata_70V05;        
//        //Sequence Distance
//        REN70V05_RD(0x304);
//        EP[2].tx_buffer[29] = mdata_70V05;       
//        //Sequence Distance
//        REN70V05_RD(0x305);
//        EP[2].tx_buffer[30] = mdata_70V05;
//        
//       //Sequence Acceleration
//        REN70V05_RD(0x306);
//        EP[2].tx_buffer[31] = mdata_70V05;
//        
//        //Sequence Direction
//        REN70V05_RD(0x307);
//        //EP[2].tx_buffer[32] = mdata_70V05;
//        
//        //Sequence Speed
//        REN70V05_RD(0x308);
//        EP[2].tx_buffer[33] = mdata_70V05;
//        
//        //Sequence Stop Distance
//        REN70V05_RD(0x309);
//        EP[2].tx_buffer[34] = mdata_70V05;        
//        //Sequence Distance
//        REN70V05_RD(0x30a);
//        EP[2].tx_buffer[35] = mdata_70V05;        
//        //Sequence Distance
//        REN70V05_RD(0x30b);
//        EP[2].tx_buffer[36] = mdata_70V05;       
//        //Sequence Distance
//        REN70V05_RD(0x30c);
//        EP[2].tx_buffer[37] = mdata_70V05;
	
	//and writes it to the TX buffer	
	 EP2_TX(EP[2].tx_buffer);  
	
        break;
	
    /*****************************************
     This is the Peripheral command section
    *****************************************/
    //Command 0 - Send byte to board
    case 0x65:	
	//transfer USB data to SRAM
	BoardData2SRAM();
	
	requestDirective = 1;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x65;
	data0 = EP[1].rx_buffer[2];
	break;	
	
    //Command 2 - Set DAC
    case 0x67:
	//transfer USB data to SRAM
	BoardData2SRAM();
	
	requestDirective = 1;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x67;
	data0 = EP[1].rx_buffer[2];
	data1 = EP[1].rx_buffer[3];
	
	break;	
	
    //Command 3 - Set PWM
    case 0x68:
	//transfer USB data to SRAM
	BoardData2SRAM();
	
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x68;
	data0 = EP[1].rx_buffer[2];
	data1 = EP[1].rx_buffer[3];
	data2 = EP[1].rx_buffer[4];
	data3 = EP[1].rx_buffer[5];
		
	break;	
	
    //Command 4 - DAC - Wave Form
    case 0x69:
	//transfer USB data to SRAM
	BoardData2SRAM();
	
	requestDirective = 1;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x69;
	data0 = EP[1].rx_buffer[2];
	data1 = EP[1].rx_buffer[3];
	data2 = EP[1].rx_buffer[4];
	data3 = EP[1].rx_buffer[5];
	data4 = EP[1].rx_buffer[6];
	
	break;	
	
	//ADC Read Value
  case 0x6a:
	//transfer USB data to SRAM
	BoardData2SRAM();

	//sends command to current board to write it's data to the SRAM
	requestDirective = 1;
	
	//Set the current Screen
	screen = ADC_SCREEN;
	CurrentLevel = MENULEVEL;
	NeedsRefresh = true;
	
	break;
	
	//Get Board Data
  case 0x6b:
	//transfer USB data to SRAM
	//BoardData2SRAM();
	//Send Command
	current_board_address = EP[1].rx_buffer[1];
	
	REN70V05_WR(((((current_board_address) - 1) * 0x400)), EP[1].rx_buffer[0]);
	
	//sends command to current board to write it's data to the SRAM
	requestDirective = 1;
	
	//Set the current Screen
	screen = ADC_SCREEN;

	NeedsRefresh = true;
	
	break;
      
	//Switches
  case 0x6c:
	//transfer USB data to SRAM
	BoardData2SRAM();
	
	requestDirective = 1;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x6c;
	
	break;
      
	//Read Flash
  case 0x6d:
	current_board_address = EP[1].rx_buffer[1];

	//transfer USB data to SRAM
	BoardData2SRAM();

	requestDirective = 0;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x6d;
	Directive(current_board_address);
	break;

	//Write Flash
  case 0x6e:
	current_board_address = EP[1].rx_buffer[1];

	//transfer USB data to SRAM
	BoardData2SRAM();

	requestDirective = 1;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x6e;
	
	break;

	//Flash Chip Erase
  case 0x6f:
	current_board_address = EP[1].rx_buffer[1];

	//transfer USB data to SRAM
	BoardData2SRAM();

	requestDirective = 1;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x6f;
	
	break;

  case 0x70:
	current_board_address = EP[1].rx_buffer[1];

	//transfer USB data to SRAM
	BoardData2SRAM();

	requestDirective = 1;
	NeedsRefresh = true;
	screen = SCOPE_SCREEN;
	cmd = 0x70;

	break;

	//Flash Copy Buffer
  case 0x71:
	current_board_address = EP[1].rx_buffer[1];

	//transfer USB data to SRAM
	BoardData2SRAM();

	requestDirective = 1;
	NeedsRefresh = true;
	screen = BOARD_SCREEN;
	cmd = 0x71;

	break;

	//Copy File to I/O Board
   case 0x72:
	//Clear the Screen
	Display_CLRSCN(white);

	current_board_address = EP[1].rx_buffer[1];
	
	//Display the file size bytes	
	hchar = 10;
	vchar = 20;
	
	Binary2ASCIIHex(EP[1].rx_buffer[2]);
	WriteChar(hchar, vchar, d_hex[1], black, white);
	WriteChar(hchar, vchar, d_hex[0], black, white);

	Binary2ASCIIHex(EP[1].rx_buffer[3]);
	WriteChar(hchar, vchar, d_hex[1], black, white);
	WriteChar(hchar, vchar, d_hex[0], black, white);

	Binary2ASCIIHex(EP[1].rx_buffer[4]);
	WriteChar(hchar, vchar, d_hex[1], black, white);
	WriteChar(hchar, vchar, d_hex[0], black, white);

	Binary2ASCIIHex(EP[1].rx_buffer[5]);
	WriteChar(hchar, vchar, d_hex[1], black, white);
	WriteChar(hchar, vchar, d_hex[0], black, white);
	
	vchar = vchar + 25;
	
	for(int i=0;i<64;i++)
	{
	    Binary2ASCIIHex(EP[1].rx_buffer[i]);
	    WriteChar(hchar, vchar, d_hex[1], black, white);
	    WriteChar(hchar, vchar, d_hex[0], black, white);
	    hchar = hchar + 10;
	    if(hchar > 400)
	    {
		hchar = 10;
		vchar = vchar + 20;
	    }
	}
	
	//transfer USB data to SRAM
	USB2SRAM();

	//Write the command to address 0x00 each time
	REN70V05_WR((((current_board_address) - 1) * 0x400),EP[1].rx_buffer[0]);

	//This initiates an I/O cycle
	Directive(current_board_address);
	
	break;
	
   case 0x73:
       current_board_address = EP[1].rx_buffer[1];
	//transfer USB data to SRAM
	USB2SRAM();

//	//Write the command to address 0x00 each time
//	REN70V05_WR((((current_board_address) - 1) * 0x400),EP[1].rx_buffer[0]);
//
	//This initiates an I/O cycle
	Directive(current_board_address);
	
       break;
  default:
      //default
      break;	
  }
    updated = true;
}

//DEBUG
void dumpMem(void)
{
    //Command
    REN70V05_RD((((current_board_address) - 1) * 0x400) + 0);
    Binary2ASCIIHex(mdata_70V05);
    WriteChar(40, 10, 'M', black, white);
    WriteChar(55, 10, d_hex[1], black, white);
    WriteChar(70, 10, d_hex[0], black, white);

    //Command from USB
    WriteChar(140, 10, 'U', black, white);
    Binary2ASCIIHex(EP[1].rx_buffer[0]);
    WriteChar(155, 10, d_hex[1], black, white);
    WriteChar(170, 10, d_hex[0], black, white);

    //Board Address
    REN70V05_RD((((current_board_address) - 1) * 0x400) + 1);
    Binary2ASCIIHex(mdata_70V05);
    WriteChar(40, 30, d_hex[1], black, white);
    WriteChar(55, 30, d_hex[0], black, white);

    //Data 1 lo-byte
    REN70V05_RD((((current_board_address) - 1) * 0x400) + 2);
    Binary2ASCIIHex(mdata_70V05);
    WriteChar(70, 50, d_hex[1], black, white);
    WriteChar(85, 50, d_hex[0], black, white);

    //Data 1 hi-byte
    REN70V05_RD((((current_board_address) - 1) * 0x400) + 3);
    Binary2ASCIIHex(mdata_70V05);
    WriteChar(40, 50, d_hex[1], black, white);
    WriteChar(55, 50, d_hex[0], black, white);

    //Data 2 lo-byte
    REN70V05_RD((((current_board_address) - 1) * 0x400) + 4);
    Binary2ASCIIHex(mdata_70V05);
    WriteChar(70, 70, d_hex[1], black, white);
    WriteChar(85, 70, d_hex[0], black, white);	
    
    //Data 2 hi-byte
    REN70V05_RD((((current_board_address) - 1) * 0x400) + 5);
    Binary2ASCIIHex(mdata_70V05);
    WriteChar(40, 70, d_hex[1], black, white);
    WriteChar(55, 70, d_hex[0], black, white);	
    
    //Sub-Command
    REN70V05_RD((((current_board_address) - 1) * 0x400) + 6);
    Binary2ASCIIHex(mdata_70V05);
    WriteChar(40, 90, d_hex[1], black, white);
    WriteChar(55, 90, d_hex[0], black, white);	
}


int EP2_TX(volatile uint8_t* tx_buffer)
{
    int cnt = 0;
    
    //Load the data to TX in array
    EP[2].tx_num_bytes = 64;
    
    for (cnt = 0; cnt < 64; cnt++)
    {
        EP[2].tx_buffer[cnt] = tx_buffer[cnt];
    }       
        
    //a pointer
    uint8_t *FIFO_buffer;

    //load the pointer with the address of the TX buffer
    FIFO_buffer = (uint8_t *)&USBFIFO2;
    
    //return if the TX buffer is empty
    if (EP2_Wait_TXRDY())
    {
        return 0;
    }
    
    //reset cnt
    cnt = 0;
    
    //send data until the TX buffer is empty
    while (cnt < EP[2].tx_num_bytes)
    {
        *FIFO_buffer = EP[2].tx_buffer[cnt]; // Send the bytes

        cnt++;
        
        // Have we sent 64 bytes?
        if ((cnt > 0) && (cnt % 64 == 0))
        {
            //Set TXRDY and wait for it to be cleared before sending any more bytes
            USBE2CSR0bits.TXPKTRDY = 1;            
            if(EP2_Wait_TXRDY())
            {
                return 0;
            }            
        }
    }

    USBE2CSR0bits.TXPKTRDY = 1;            
}

int EP1_RX()
{
    unsigned char *FIFO_buffer;
    int cnt;
    int rx_bytes;
    //get the number of bytes received
    rx_bytes = USBE1CSR2bits.RXCNT;
    
    //USB FIFO Data Register 1
    FIFO_buffer = (unsigned char *)&USBFIFO1; 
    
    //load the array with the bytes in the buffer
    for(cnt = 0; cnt < rx_bytes; cnt++)
    {
        EP[1].rx_buffer[cnt] = *(FIFO_buffer + (cnt & 3));
    }
    
    //unload the RX FIFO
    USBE1CSR1bits.RXPKTRDY = 0;

    return rx_bytes;
}

void EP0_control_transaction()
{
    uint16_t length;

    if ((USB_transaction.bmRequestType == 0xC0) && (USB_transaction.wIndex == 0x04))
    {
       length = USB_transaction.wLength;
       if (length > sizeof(ExtCompatIDFeatureDescriptor))
       {
           length = sizeof(ExtCompatIDFeatureDescriptor);
       }
       
       USB_queue_EP0(ExtCompatIDFeatureDescriptor, sizeof(ExtCompatIDFeatureDescriptor), length); 
       
       return;
    }
    
    //Class specific, device to host, interface target
    if(USB_transaction.bmRequestType == 0xc1)    
    {
        //Check if the host is requesting an MS feature descriptor
        if(USB_transaction.bRequest == 0xee)
        {
            //Figure out which descriptor is being requested
            if(USB_transaction.wIndex == 0x05)    
            {
                //Determine number of bytes to send to host 
                //Lesser of: requested amount, or total size of the descriptor
                length = sizeof(ExtPropertyFeatureDescriptor);
                if(USB_transaction.wLength < length)
                {
                    length = USB_transaction.wLength;
                }
                
                USB_queue_EP0(ExtPropertyFeatureDescriptor, sizeof(ExtPropertyFeatureDescriptor), length);  
                        
                USBE0CSR0bits.TXRDY = 1;    
                
                return;
            }
        }
    }
    
    // We're not going to bother with whether bmRequestType is IN or OUT for the most part
    switch (USB_transaction.bRequest)
    {
        case 0xC:
        {
            USBE0CSR0bits.STALL = 1;
            break;
            
        }
        case 0x0: 
        {
            if (USB_transaction.bmRequestType == 0x80) // Get status
                USB_queue_EP0(device_descriptor, 0, 0);
            if (USB_transaction.bmRequestType == 0x00) // Select function
                USB_queue_EP0(device_descriptor, 0, 0);
            break;            
        }
        
        //Set USB address
        case 0x5: 
        {
            USBE0CSR0bits.RXRDYC = 1;
            usbAddress = EP[0].rx_buffer[2];

            SetAddress = true;
            break;
        }
	
	//added by chatgpt
	case 0xEE:  // Microsoft OS Descriptor request
	{
	    if (USB_transaction.bmRequestType == 0xC0) // Vendor-specific request
	    {
		USB_queue_EP0(MSOSDescriptor, sizeof(MSOSDescriptor), USB_transaction.wLength);
	    }
	    break;
	}
        
        //Get descriptor
        case 0x6: 
        {
            switch (USB_transaction.wValue >> 8)
            {
                //Device descriptor
                case 0x1: 
                {
                    USB_queue_EP0(device_descriptor, sizeof(device_descriptor), USB_transaction.wLength);                             
                    break;
                }
                
                //Configuration descriptor
                case 0x2: 
                {
                    USB_queue_EP0(config_descriptor, sizeof(config_descriptor), USB_transaction.wLength);
                    break;
                }
                
                //String descriptors
                case 0x3: 
                {          
                    switch (USB_transaction.wValue & 0xff)
                    {
                        //String 0 - Language ID
                        case 0x0: 
                        {
                            USB_queue_EP0(string0, sizeof(string0), USB_transaction.wLength);
                            break;
                        }
                        //String 1 - iManufacturer
                        case 0x1: 
                        {
                            USB_queue_EP0(string1, sizeof(string1), USB_transaction.wLength);                           
                            break;
                        }
                        //String 2 - iProduct
                        case 0x2: 
                        {
                            USB_queue_EP0(string2, sizeof(string2), USB_transaction.wLength);
                            break;
                        }
                        //String 3 - iSerialNumber
                        case 0x3: 
                        {
                            USB_queue_EP0(string3, sizeof(string3), USB_transaction.wLength);
                            break;
                        }
                        //MS OS Descriptor Query
                        case 0xee:
                        {
                            USB_queue_EP0(MSOSDescriptor, sizeof(MSOSDescriptor), USB_transaction.wLength);
                            break;
                        }                       
                        break;
                    }  
                    break;
                }
		
		case 0x04: // Extended Compatibility ID Feature Descriptor
		if (USB_transaction.wIndex == 0x0004)
		{
		    USB_queue_EP0(ExtCompatIDFeatureDescriptor, sizeof(ExtCompatIDFeatureDescriptor), USB_transaction.wLength);
		}
		break;
                
		case 0x05: // Extended Properties Feature Descriptor
		if (USB_transaction.wIndex == 0x0005)
		{
		    USB_queue_EP0(ExtPropertyFeatureDescriptor, sizeof(ExtPropertyFeatureDescriptor), USB_transaction.wLength);
		}
		break;

	    //Device Qualifier
                case 0x6: 
                {          
                    USB_queue_EP0(device_qualifier, sizeof(device_qualifier), USB_transaction.wLength);
                    break;
                }                        
            }
            break;
        }
        
        // Set configuration
        case 0x9: 
        {
            //Enumeration complete!
            USBState = ATTACHED;
            break;
        }
        
        default: 
        {
            USBE0CSR0bits.STALL = 1;
            break;
        }  
    }
}

//USB_queue_EP0(config_descriptor, sizeof(config_descriptor), USB_transaction.wLength);
void USB_queue_EP0(uint8_t *buffer, int size, int max_size)
{
    int cnt;
    
    if (max_size < size)
        size = max_size;
    
    EP[0].tx_num_bytes = size;
    
    for (cnt = 0; cnt < size; cnt++)
    {
        EP[0].tx_buffer[cnt] = buffer[cnt];
    }       
    
    EP0_TX();
}

/* Non-blocking EP0_TX */
void EP0_TX()
{
    static int ep0_tx_cnt = 0;
    uint8_t *FIFO_buffer = (uint8_t *)&USBFIFO0;

    while ((ep0_tx_cnt < EP[0].tx_num_bytes) && !USBE0CSR0bits.TXRDY)
    {
	*FIFO_buffer = EP[0].tx_buffer[ep0_tx_cnt++];
	if ((ep0_tx_cnt % 64) == 0)
	{
	    USBE0CSR0bits.TXRDY = 1;
	    return; // Exit; next chunk will be sent in next ISR
	}
    }

    // Final chunk
    if (ep0_tx_cnt >= EP[0].tx_num_bytes)
    {
	USBE0CSR0bits.TXRDY = 1;
	ep0_tx_cnt = 0; // Reset for next transfer
    }

}

void EP0_RX(int length)
{
    int cnt;
    uint8_t *FIFO_buffer;
    
    // Store number of bytes received
    EP[0].rx_num_bytes = USBE0CSR2bits.RXCNT;
    
    // Get 8-bit pointer to USB FIFO for endpoint 0
    FIFO_buffer = (uint8_t *)&USBFIFO0;
    
    for(cnt = 0; cnt < length; cnt++)
    {
        // Read in one byte at a time
        EP[0].rx_buffer[cnt] = *(FIFO_buffer + (cnt & 3));
    }
     
    USBE0CSR0bits.RXRDYC = 1;
}

int EP0_Wait_TXRDY()
{
    int timeout;
    
    timeout = 0;
    
    while (USBE0CSR0bits.TXRDY)
    {
        timeout++;
        
        if (timeout > 5000)
        {
            return 1;
        }
    };
    
    return 0;
}

int EP2_Wait_TXRDY()
{
    int timeout;
    
    timeout = 0;
    
    while (USBE1CSR0bits.TXPKTRDY)
    {
        timeout++;
        
        if (timeout > 5000)
        {
            return 1;
        }
    };
    
    return 0;
}

void setTime(void)
{
    //Unlock the registers
    RTCCONbits.RTCWREN = 1;
    
//    //24 to 12 hour conversion
//    if(EP[1].rx_buffer[1] > 12)
//    {
//        Binary2ASCIIBCD(EP[1].rx_buffer[1] - 12);
//    }
//    else
//    {
        Binary2ASCIIBCD(EP[1].rx_buffer[1]);
//    }
        
    RTCTIMEbits.HR10 = d1;
    RTCTIMEbits.HR01 = d0;
    
    hchar = hchar + 20;
    Binary2ASCIIBCD(EP[1].rx_buffer[2]);
//    WriteChar(hchar, vchar, d1, 0x0);
//    WriteChar(hchar, vchar, d0, 0x0);
   
    RTCTIMEbits.MIN10 = d1;
    RTCTIMEbits.MIN01 = d0;

    hchar = hchar + 20;
    Binary2ASCIIBCD(EP[1].rx_buffer[3]);
//    WriteChar(hchar, vchar, d1, 0x0);
//    WriteChar(hchar, vchar, d0, 0x0);
   
    
    RTCTIMEbits.SEC10 = d1;
    RTCTIMEbits.SEC01 = d0;    

    //Re-lock the registers
    RTCCONbits.RTCWREN = 0;
}

void SRAM2USB(void)
{
    //Get Board Address
    current_board_address = EP[1].rx_buffer[1];

    for(int i=4;i<=63;i++)
    {
	EP[2].tx_buffer[i] = REN70V05_RD((((current_board_address) - 1) * 0x400) + i);
    }
}

void BoardData2SRAM(void)
{
    //Get Board Address
    current_board_address = EP[1].rx_buffer[1];
    
    for(int i=0;i<=63;i++)
    {
	REN70V05_WR(((((current_board_address) - 1) * 0x400) + i), EP[1].rx_buffer[i]);
    }
    
    //dumpMem();
}

void USB2SRAM(void)
{
    //Get Board Address
    current_board_address = EP[1].rx_buffer[1];
    
    for(int i=0;i<064;i++)
    {
	REN70V05_WR(((((current_board_address) - 1) * 0x400) + i), EP[1].rx_buffer[i]);
    }
}