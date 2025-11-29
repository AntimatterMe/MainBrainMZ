/********************************************************************* 
 * File:   MainBrain.h
 * Author: Antimatter
 *
 * Created on May 17, 2024, 4:45 AM
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

    File Description:

    Change History:
/*********************************************************************/

#ifndef MAINBRAIN_H
#define	MAINBRAIN_H
#include <stdbool.h>
#include <stdint.h> 

//Display
#define black   0x0000
#define white   0xffff
#define red     0xA186
#define blue    0x32b2
#define green   0x3546
#define gray    0x6B6E

#define HOME_SCREEN     0
#define INFO_SCREEN     1
#define MOTION_SCREEN   2
#define DRAW_SCREEN     3
#define DEBUG_SCREEN    4
#define CONFIG_SCREEN   5
#define ADC_SCREEN      6
#define BOARD_SCREEN    7
#define DAC_SCREEN      8
#define SCOPE_SCREEN    9
#define BSCOPE_SCREEN   10
#define MESSAGE_SCREEN  11

#define HOMELEVEL       0
#define MENULEVEL       1
#define LOWERLEVEL      2
#define UTILITYLEVEL    3

typedef enum
{
    DETACHED =      0x00,
    ATTACHED =      0x01
} USB_STATE;

extern volatile uint8_t USBState;

typedef enum
{
    CONNECTED =      0x00,
    DISCONNECTED =   0x01,
} DEVICE_STATE;

typedef struct
{
    volatile unsigned short rx_num_bytes;
    volatile unsigned short tx_num_bytes;
    volatile unsigned char tx_buffer[512];
    volatile unsigned char rx_buffer[512];
} USB_ENDPOINT;

extern USB_ENDPOINT EP[3];

//DEBUG
extern volatile bool SRAM_BUSY;
extern volatile uint32_t Last_Memory[10];
extern int sram_flag_fail_count;
extern int8_t dev_data;
extern volatile uint8_t DeviceState;
extern char MessageBoxTitle[];
extern char myStr[];
extern const uint16_t SplashImage[48000];
extern const uint16_t ConfigImage[307200];
extern const uint16_t DisplayMenu[22500];

//Flash Sector Size
extern const uint16_t SECTOR_SIZE;
extern uint8_t old_screen;
extern bool Message;
extern uint8_t back_level;
extern const uint16_t Button[6450];
extern int lut[];
extern int courier_new_16pt_bold[];
extern int ADC0_result;
extern float ADC6_result;
extern int CanvasColor;
extern int TextColor;
extern int RTC_delay_counter;
extern uint16_t hchar;
extern uint16_t vchar;
extern uint8_t d0, d1, d2, d3, d4, d5, d6, d7;
extern uint8_t read_buf[16];
extern uint8_t MenuLevel;
extern bool ExitButton;
extern bool isActive;
extern uint8_t ButtonNum;
extern char Button1Text[5];
extern char SRAMFailText[14];
extern char lastErrorText[9];
extern char SRAMLockedText[14];
extern int color1;
extern int color2;
extern bool DrawScreenOpen;
extern uint16_t scn_pos_x;
extern uint16_t scn_pos_y;
extern uint8_t Display_Read;
extern uint8_t disData[10];
extern uint8_t test_data;
extern uint8_t mdata_70V05;
extern uint32_t address_70V05;
extern char HeaderString[];
extern int d_hex[8];
extern uint8_t Flash_MID;
extern uint8_t Flash_DID;
extern bool test_result;
extern uint8_t data_flash;
extern bool full_access;
extern uint32_t lastError;
extern uint8_t flag_reg;
extern uint8_t Flag;
extern bool isConnected;
extern bool isAttached;
extern volatile bool inMotion;
extern bool NeedsRefresh;
extern uint8_t LastError;
extern uint8_t screen;
extern uint8_t BuildPList;
extern uint8_t requestDirective;
extern uint8_t PeripheralList[7];
extern uint8_t current_board_address;
extern bool ADC_Running;
extern char ADCtitleStr[4];
extern char BoardtitleStr[12];
extern bool isBusy;
extern uint8_t CurrentLevel;
extern uint8_t cmd;
extern uint8_t data0;
extern uint8_t data1;
extern uint8_t data2;
extern uint8_t data3;
extern uint8_t data4;
extern bool updated;
extern uint32_t start_count;
extern bool screenTouched;

bool SetFreqPOSC(uint8_t f);
void Clock(uint8_t ClockData);
void IO_Board_Init(void);
void DrawMenu(void);
void DrawCanvas(uint8_t screen_, uint32_t begin_xpos, uint32_t begin_ypos, uint32_t _count);
void MotionScreen(void);
void ReleaseSRAM(void);
void MotionScreen(void);
void MotionData(void);
void DrawCircle(void);
void SystemReset(void);
void setTime(void);
void USB_init(void);
void SystemSetup(void);
void I2C_init(void);
void ADC_init(void);
void TMR1_init(void);
void TMR2_init(void);
void TMR3_init(void);
void TMR4_init(void);
void TMR5_init(void);
void TMR6_init(void);
void USB_init(void);
void PMP_init(void);
void Display_init(void);
void LED_Port(int8_t led_port_data);
void RTCC_init(void);
bool Device_Present(void);
void Delay32(uint8_t prescale, uint32_t _delay32);
void LongDelay(int delay_s);
void Backlight_Control(uint8_t back_level);
void ShowSplashScreen(uint8_t option);
void Display_RDDIM(void);
uint8_t Display_RDDIDIF(void);
void Display_RDDSM(void);
void Display_RDDSDR(void);
void Display_SLPIN(void);
void Display_SLPOUT(void);
void Display_PTLON(void);
void Display_NORON(void);
void Display_INVOFF(void);
void Display_INVON(void);
void Display_DISPOFF(void);
void Display_DISPON(void);
void Display_CASET(uint16_t col_start, uint16_t col_end);
void Display_NOP(void);
void Display_SWRESET(void);
void Display_RDDPM(void);
uint8_t Display_RDDMADCTL(void);
void Display_RDDCOLMOD(void);
void Display_RASET(unsigned row_start, unsigned row_end);
void Display_RAMWR(void);
void Display_RAMRD(void);
void Display_PLTAR(unsigned SR_HI, unsigned SR_LO, unsigned ER_HI, unsigned ER_LO);
void Display_GETDEVICEID(void);
void Display_Rect(unsigned col_start, unsigned col_end, unsigned row_start, unsigned row_end, unsigned rect_color);
void Display_CLRSCN(int CanvasColor);
void Display_DISPON(void);
void SetDisplayBrightness(void);
void Display_Rect(unsigned col_start, unsigned col_end, unsigned row_start, unsigned row_end, unsigned rect_color);
void WriteString(unsigned col_start, unsigned row_start, char array_name[], int TextColor, int CanvasColor);
void WriteChar(unsigned col_start, unsigned row_start, unsigned ascii_char, int TextColor, int CanvasColor);
void WriteButtonChar(unsigned col_start, unsigned row_start, unsigned ascii_char, int TextColor, int CanvasColor1, int CanvasColor2);
void DrawButton(unsigned col_start, unsigned row_start, uint8_t length, uint8_t height, int color1, int color2, int border_color, char array_name[]);
void WriteButtonString(unsigned col_start, unsigned row_start, char array_name[], int TextColor, int CanvasColor1, int CanvasColor2);
void Binary2ASCIIBCD(int bcd);
void DrawHeader();
void DrawScreen(uint8_t scrn, char title[]);
void ShowDrawScreen(void);
void Display_RDDST(void);
void Display_RDNUMED(void);
void MessageBox(char text[], uint8_t show_time);
void BuildPeripheralList(void);
void Directive(uint8_t badd);
void SetPeripheralAddress(uint8_t padd);
void SetPotentiometer(void);
void BoardData2SRAM(void);
void dumpMem(void);
void SRAM2USB(void);
void USB2SRAM(void);
void DMM(uint8_t data, uint16_t xchar, uint16_t ychar);
void Float2ASCIIBCD(float number, char* output);

//SRAM
void REN70V05_Init(void);
void REN70V05_WR(uint32_t address_70V05, uint8_t mdata_70V05);
int8_t REN70V05_RD(uint32_t address_70V05);
bool memtest_70V05(uint8_t test_data);
void ShowSRAM_FailScreen(void);
void Binary2ASCIIHex(int i_hex);
void SRAM_Semaphore_Test(void);
bool REN70V05_SEM(void);
void REN70V05_LOCK(void);
void REN70V05_RELEASE(void);

//Flash
void Flash_Init(void);
void Flash_RD(unsigned address_flash);
void Flash_WR(unsigned address_flash, uint8_t data_flash);
void Flash_Sector_Erase(int erase_sector);
void Flash_Chip_Erase(void);

//ADC
void ADC_init(void);

//Time
void GetTime(void);

//I/O Board
void IO_Board_init(void);

//Buzzer
void Beep(void);

#endif	/* MAINBRAIN_H */

