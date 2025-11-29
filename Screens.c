/*********************************************************************
    FileName:     	Screens.c
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
#include <math.h>
#include "MainBrain.h"

char HeaderString[10] = {"MainBrain"};
char DisplayString[36] = {"480x320 16-bit RGB 65K Touch"};
char MessageBoxTitle[8] = {"Message"};
char ProcessorArray[18] = {"PIC32MZ2048EFH100"};
char SRAMsize[10] = {"8Kx8 SRAM"};
char PeripheralsStr[7] = {"Boards"};
char SRAMStatusPass[5] = {"Pass"};
char failStr[5] = {"Fail"};
char FlashSize[13] = {"512Kx8 Flash"};
char USBver[8] = {"USB 2.0"};
char ExitButtonText[5] = {"Exit"};
char Button1Text[5] = {"Info"};
char Button2Text[8] = {"Draw"};
char MenuText[20] = {"Draw"};
char SRAMFailText[14] = {"SRAM Fail!"};
char SRAMLockedText[14] = {"SRAM Locked!"};
char lastErrorText[9] = {"Errors: "};
char noneStr[5] = {"none"};
char vbusStr[5] = {"VBUS"};
char PrimaryOsc[8] = {"Pri OSC"};
char OtherOsc[6] = {"Other"};
char SequenceStr[15] = {"Sequence Data:"};
char M300Str[] = {"0x300:"};
char M301Str[] = {"0x301:"};
char M302Str[] = {"0x302:"};
char M306Str[] = {"0x306:"};
char M307Str[] = {"0x307:"};
char M308Str[] = {"0x308:"};
char StatusStr[8] = {"Status:"};
char ControlStr[9] = {"Control:"};
char ADCtitleStr[4] = {"ADC"};
char BoardtitleStr[12] = {"Peripherals"};
uint8_t LastError = 0;
int ButtonTopColor = 0xef7d;
int ButtonLowerColor = 0xd6ba;
bool DrawScreenOpen = false;
bool motionScreenActive = false;

void ShowSplashScreen(uint8_t option)
{
    int i;
    
    //Clear the Screen
    Display_CLRSCN(white);
        
    //Load Splash Image
    /****************************/
    Display_CASET(0, 479);
    Display_RASET(120, 170);
    Display_RAMWR();

    //Data
    //RF3 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;       
    
    for(i=0;i<=23040; i++)
    {
        PMDOUT = SplashImage[i*2];
        while(PMMODEbits.BUSY == 1);    
    }
    
    //This must be here to prevent the Display /CS from remaining low
    //after the function ends
    PORTAbits.RA9 = 1;

    if(option == 1)
    {
        LongDelay(2);

        //Clear the Screen
        Display_CLRSCN(white);

        //short delay to let operation complete
        Delay32(0, 1000);
    } 
}

void ShowSRAM_FailScreen(void)
{

}

void ShowDrawScreen(void)
{
    DrawScreenOpen = true;
    
    //Clear the Screen
    Display_CLRSCN(white);

    //DrawHeader();
    
    //draw menu col start 
    Display_Rect(10, 100, 40, 90, 0xd591);  
    WriteString(25, 50, MenuText, black, 0xd591);
    
    
}

void DrawHeader()
{
    //Draw Header Bar
    Display_Rect(0, 480, 0, 40, 0x04D3);  
}

void DrawMenu(void)
{
    int i;
    
    Display_CASET(100, 249);
    Display_RASET(80, 229);
    Display_RAMWR();

    //Data
    //RF3 = D/C 1=Data, 0=Command
    PORTBbits.RB1 = 1;       

    for(i=0;i<=22500; i++)
    {
        PMDOUT = DisplayMenu[i];
        while(PMMODEbits.BUSY == 1);    
    }

    WriteChar(192, 120, '5', black, white);
    
    //This must be here to prevent the Display /CS from remaining low
    //after the function ends
    PORTAbits.RA9 = 1;
                    
}

void DrawCanvas(uint8_t screen_, uint32_t begin_xpos, uint32_t begin_ypos, uint32_t _count)
{
    uint32_t i;
    uint32_t array_begin;
    uint32_t array_count;
    
    //CONFIG_SCREEN
    if(screen_ == CONFIG_SCREEN)    
    {        
        //calculate the start position in the array
        array_begin = (((begin_ypos + 1) * 480) - (480 - (begin_xpos + 1))) - 1;
        
        //set the frame size to full screen
        Display_CASET(0, 479);
        Display_RASET(0, 319);
        
        //Enable display memory write
        //Data will be written until another command is received by the display
        Display_RAMWR();

        //Set Display Data Mode
        //RF3 = D/C 1=Data, 0=Command
        PORTBbits.RB1 = 1;       

        for(i=0;i<=_count; i++)
        {
            PMDOUT = ConfigImage[i + array_begin];
            while(PMMODEbits.BUSY == 1);    
        }
    }
}

void DrawScreen(uint8_t scrn, char title[])
{
    //Clear the Screen
    Display_CLRSCN(white);
    
    //Draw Header Bar
    DrawHeader();

    switch(scrn)
    {
        //Information
        case INFO_SCREEN:
            //Draw Title text        
            WriteString(170, 5, title, black, 0x04D3);
            
            hchar = 10;
            vchar = 50;
            
            //Display last error - 0 = no error
            WriteString(hchar, vchar, lastErrorText, black, white);
            
            if(!lastError)
            {
                WriteString(hchar, vchar, noneStr, green, white);
            }
            else
            {
                Binary2ASCIIBCD(lastError);
                WriteChar(hchar, vchar, d1, red, white);
                WriteChar(hchar, vchar, d0, red, white);
            }
	   
            //Display device information       
            //Processor
            hchar = 10;
            vchar = vchar + 25;
            WriteString(hchar, vchar, ProcessorArray, black, white);

            //Display
            hchar = 10;
            vchar = vchar + 25;
            WriteString(hchar, vchar, DisplayString, black, white);
            
            //SRAM size
            hchar = 10;
            vchar = vchar + 25;
            WriteString(hchar, vchar, SRAMsize, black, white);
            
            hchar = hchar + 15;
            
            if((lastError & 0x4) != 0)
            {
                WriteString(hchar, vchar, failStr, red, white);
            }
            else
            {
                WriteString(hchar, vchar, SRAMStatusPass, green, white);
            }

            hchar = hchar + 15;

            //Flash Size
            hchar = 10;
            vchar = vchar + 25;
            WriteString(hchar, vchar, FlashSize, black, white);
            
            //Flash MID	    
            hchar = hchar + 15;
            Binary2ASCIIHex(Flash_MID);
            WriteChar(hchar, vchar, d_hex[1], black, white);
            WriteChar(hchar, vchar, d_hex[0], black, white);            

            //Flash DID
            hchar = hchar + 15;
            Binary2ASCIIHex(Flash_DID);
            WriteChar(hchar, vchar, d_hex[1], black, white);
            WriteChar(hchar, vchar, d_hex[0], black, white);            

            hchar = 10;
            vchar = vchar + 25;
            
            //Check what boards are present
            WriteString(hchar, vchar, PeripheralsStr, black, white);
            
            hchar = hchar + 15;
            
            for(int i=1;i<8;i++)
            {            
                Binary2ASCIIHex(PeripheralList[i]);
                WriteChar(hchar, vchar, d_hex[1], blue, white);
                WriteChar(hchar, vchar, d_hex[0], blue, white);
                hchar = hchar + 10;
            }
            
           //USB ver
            hchar = 10;
            vchar = vchar + 25;
            WriteString(hchar, vchar, USBver, black, white);
            
            //Oscillator
            hchar = 10;
            vchar = vchar + 25;
            
            switch(OSCCONbits.COSC)
            {
                case 1:
                    WriteString(hchar, vchar, PrimaryOsc, green, white);
                break;
                
                default:
                    WriteString(hchar, vchar, OtherOsc, green, white);
                break;
            }

            vchar = vchar + 25;
            hchar = 10;
            
            WriteString(hchar, vchar, vbusStr, black, white);

            break;
            
        case CONFIG_SCREEN:
            
            //Load Splash Image
            /****************************/
            Display_CASET(0, 479);
            Display_RASET(0, 319);
            Display_RAMWR();

            //Data
            //RF3 = D/C 1=Data, 0=Command
            PORTBbits.RB1 = 1;       

            for(int i=0;i<=153600; i++)
            {
                PMDOUT = ConfigImage[i];
                while(PMMODEbits.BUSY == 1);    
            }
            
            break;
            
        //Home
        case HOME_SCREEN:
            //Draw Title text        
            WriteString(120, 5, title, black, 0x04D3);

            //Button1 - 'Info'
            DrawButton(20, 50, 150, 50, ButtonTopColor, ButtonLowerColor, black, Button1Text); 

            //Button2 - 'Draw'
            DrawButton(20, 125, 150, 50, ButtonTopColor, ButtonLowerColor, black, Button2Text); 

            break;
            
        case MOTION_SCREEN:
            //Draw Title text        
            WriteString(120, 5, title, black, 0x04D3);

            WriteString(10, 50, StatusStr, black, white);
            
            WriteString(180, 50, ControlStr, black, white);

            break;
            
        case DEBUG_SCREEN:
            
            WriteString(180, 5, HeaderString, black, 0x04D3);

            WriteString(10, 50, StatusStr, black, white);
            
            WriteString(180, 50, ControlStr, black, white);
            
            break;
    
        case ADC_SCREEN:
	    
            WriteString(10, 50, StatusStr, black, white);
            
            break;
	case MESSAGE_SCREEN:
	    Display_Rect(100, 200, 100, 200, blue);
	break;
    }   

}

void DrawCircle(void)
{
    int theta = 0;  // angle that will be increased each loop
    int h = 200;      // x coordinate of circle center
    int k = 150;      // y coordinate of circle center
    int step = 15;  // amount to add to theta each time (degrees)
    int x;
    int y;
    int r = 100;
    
    while(theta < 360)
    { 
        x = h + r*cos(theta);
        y = k + r*sin(theta);
        Display_Rect(x, x+5, y, y+5, blue);
        theta = theta + 1;
    }
}

void MessageBox(char text[], uint8_t show_time)
{
    //Border
    Display_Rect(100, 300, 220, 222, blue);
    Display_Rect(98, 100, 100, 222, blue);
    Display_Rect(300, 302, 100, 222, blue);    
    
    //header
    Display_Rect(100, 300, 100, 125, blue);
    WriteString(105, 100, MessageBoxTitle, white, blue);
    
    //body
    Display_Rect(100, 300, 125, 180, white);
    
    //footer
    Display_Rect(100, 300, 180, 220, 0xe73c);
    
    //string
    WriteString(104, 140, text, black, white);
    
    //shadow
    Display_Rect(302, 330, 130, 240, 0xef9d);
    Display_Rect(130, 330, 222, 250, 0xef9d);
    
    LongDelay(show_time);
    
    screen = old_screen;
}

