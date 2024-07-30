/*********************************************************************
    FileName:     	USB_MZ.c
    Dependencies:	See #includes
    Processor:		PIC32MZ
    Hardware:		MainBrain32 rev 0.20
    Complier:  	    XC32 4.40
    Author:         Larry Knight 2023

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
#include <p32xxxx.h>
#include <proc/p32mz0512efk100.h>
#include <sys/attribs.h>
#include <sys/kmem.h>
#include <stdint.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MainBrain.h"

enum USB_State
{
    POWERED,
    DEFAULT,
    ADDRESS,
    CONFIGURED,
    ATTACHED,
    SUSPEND
};

enum USB_State DeviceState;

typedef struct
{
    volatile unsigned char bmRequestType;
    volatile unsigned char bRequest;
    volatile unsigned short wValue;
    volatile unsigned short wIndex;
    volatile unsigned short wLength;
} USB_TRANSACTION;

USB_TRANSACTION USB_transaction;

typedef struct
{
    volatile unsigned short rx_num_bytes;
    volatile unsigned short tx_num_bytes;
    volatile unsigned char tx_buffer[512];
    volatile unsigned char rx_buffer[512];
} USB_ENDPOINT;

USB_ENDPOINT EP[3];

uint8_t device_descriptor[] = 
{
    /* Descriptor Length						*/ 0x12, //Size of this descriptor in bytes
    /* DescriptorType: DEVICE					*/ 0x01,
    /* bcdUSB (ver 2.0)							*/ 0x00,0x02,
    /* bDeviceClass								*/ 0x00,
    /* bDeviceSubClass							*/ 0x00,
    /* bDeviceProtocol							*/ 0x00,
    /* bMaxPacketSize0							*/ 0x40, //0x40 for High Speed USB
    /* idVendor									*/ 0x09,0x12, /*VID */
    /* idProduct								*/ 0x01,0x00, 
    /* bcdDevice								*/ 0x00,0x02, 
    /* iManufacturer							*/ 0x01,
    /* iProduct									*/ 0x02,
    /* iSerialNumber							*/ 0x02, 
    /* bNumConfigurations						*/ 0x01
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
    0xff,                       // Class code
    0xff,                       // Subclass code
    0xff,                       // Protocol code
    0x00,                       // Interface string index
    
    // Endpoint Descriptor
    //EP01 OUT
    0x07,                       //Size of this descriptor in bytes
    0x05,                       //Endpoint Descriptor
    0x01,                       //EndpointAddress
    0x02,                       //Attributes
    0x40,0x00,                  //size
    0x01,                       //Interval   
    //EP02 IN                      
    0x07,                       //Size of this descriptor in bytes
    0x05,                       //Endpoint Descriptor
    0x82,                       //EndpointAddress
    0x02,                       //Attributes
    0x40,0x00,                  //size
    0x01                        //Interval
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
    'M','S','F','T','1','0','0',    
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
};    

//Language - 0x0409 - English
uint8_t string0[] =  {4, 0x03, 0x09, 0x04};

//iManufacturer
uint8_t string1[] = {26, 3, 'L', 0, 'a', 0, 'r', 0, 'r', 0, 'y', 0, ' ', 0, 'K', 0, 'n', 0, 'i', 0, 'g', 0, 'h', 0, 't', 0};   

//iProduct	
uint8_t string2[] = {26, 3, 'M', 0, 'a', 0, 'i', 0, 'n', 0, 'B', 0, 'r', 0, 'a', 0, 'i', 0, 'n', 0, ' ', 0, 'M', 0, 'Z', 0};
 
//iSerialNumber	
uint8_t string3[] = {10, 3, '0', 0, '0', 0, '0', 0, '1', 0};

void EP0_control_transaction(void);
void USB_queue_EP0(uint8_t *buffer, int size, int max_size);
void EP0_RX(int length);
void EP0_TX(void);
void Host_CMDs(void);
int EP1_RX(void);
int EP2_TX(volatile uint8_t *tx_buffer);
int EP0_Wait_TXRDY(void);
int EP2_Wait_TXRDY(void);

volatile uint8_t usbAddress;
volatile bool SetAddress = true;

void USB_init(void)
{
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
    
    // Enable the reset interrupt
    USBCSR2bits.RESETIE = 1;    
    
    // Enable the USB interrupt
    IEC4bits.USBIE = 1;       
    
    // Enable USB module interrupt
    USBCRCONbits.USBIE = 1;     
    
    // Clear the USB interrupt flag.
    IFS4bits.USBIF = 0;         
    
    // USB Interrupt Priority 7
    IPC33bits.USBIP = 7;        
    
    // USB Interrupt Sub-Priority 1
    IPC33bits.USBIS = 1;   
    
    // See DISNYET (same bit as PIDERR)
    USBE1CSR1bits.PIDERR = 1;   

    // Enable High Speed (480Mbps) USB mode
    USBCSR0bits.HSEN = 1;       

    USBCSR0bits.SOFTCONN = 1;
    
    DeviceState = ATTACHED;
}

//USB
void __attribute__((vector(_USB_VECTOR), interrupt(ipl7srs), nomips16)) USB_handler()
{   
    //Reset
    if(USBCSR2bits.RESETIF)
    {
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
  switch (EP[1].rx_buffer[0])
  {
      case 0x00:
          EP[2].tx_buffer[0] = 0x55;
          EP2_TX(EP[2].tx_buffer);
        break;
      case 0x01:
          LED_Port(EP[1].rx_buffer[1]);
        break;
  }
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

// Send EP[0].tx_num_bytes from EP[0].tx_buffer on endpoint 0
void EP0_TX()
{    
     int cnt = 0;
    
    //a pointer
    uint8_t *FIFO_buffer;

    //load the pointer with the address of the TX buffer
    FIFO_buffer = (uint8_t *)&USBFIFO0;
    
    //return if the TX buffer is empty
    if (EP0_Wait_TXRDY())
    {
        return;
    }
        
    //send data until the TX buffer is empty
    while (cnt < EP[0].tx_num_bytes)
    {
        *FIFO_buffer = EP[0].tx_buffer[cnt]; // Send the bytes

        cnt++;
        
        // Have we sent 64 bytes?
        if ((cnt > 0) && (cnt % 64 == 0))
        {
            //Set TXRDY and wait for it to be cleared before sending any more bytes
            USBE0CSR0bits.TXRDY = 1; 
            
            //wait while the data is being sent
            while(!EP0_Wait_TXRDY());
        }   
        
        //if the descriptor is larger than the 64 byte buffer size
        //then send another chunk
        if ((cnt > 0) && (cnt % 64 == 0))
        {
            //Set TXRDY and wait for it to be cleared before sending any more bytes
            USBE0CSR0bits.TXRDY = 1;            
            if(EP0_Wait_TXRDY())
            {
                return;
            }            
        }
    }

    USBE0CSR0bits.TXRDY = 1;            
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
