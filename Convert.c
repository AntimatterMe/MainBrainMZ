#include "xc.h"
#include <p32xxxx.h>
#include "MainBrain.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/********************************************/
/*float to ASCII BCD Conversion Code        */
/*The fast and compact method by Cypress    */
/*PIC32			                    */
/********************************************/
void Float2ASCIIBCD(float number, char* output)
{
    if (number < 0) number = -number;           // handle negative numbers if needed

    uint8_t int_part = (uint8_t)number;         // get integer part (e.g., 3)
    float frac = number - int_part;             // get fractional part (e.g., 0.3)

    output[0] = '0' + int_part;                 // ASCII for integer digit
    output[1] = '.';                            // decimal point
    output[2] = '0' + (uint8_t)(frac * 10);     // one decimal digit as ASCII
}

/********************************************/
/*Binary to ASCII BCD Conversion Code       */
/*The fast and compact method by Cypress    */
/*PIC32                                     */
/********************************************/

uint8_t d0, d1, d2, d3, d4;

void Binary2ASCIIBCD(int bcd)
{
    unsigned char q;
        
    //Find n0-n3 numbers
    d0 = bcd & 0xF;
    d1 = (bcd>>4) & 0xF;
    d2 = (bcd>>8) & 0xF;
    d3 = (bcd>>12) & 0xF;
    d4 = (bcd>>16) & 0xF;
    
    //Calculate d0-d4 numbers
    d0 = 6*(d3 + d2 + d1) + d0; 
    q = d0 / 10; 
    d0 = d0 % 10;     
    d1 = q + 9*d3 + 5*d2 + d1;
    q = d1 / 10;
    d1 = d1 % 10;
    d2 = q + 2*d2;
    q = d2 / 10;
    d2 = d2 % 10;
    d3 = q + 4*d3;
    q = d3 / 10;
    d3 = d3 % 10;
    d4 = q;
    
    //ASCII
    d0 = d0 + 48;
    d1 = d1 + 48;
    d2 = d2 + 48;
    d3 = d3 + 48;
    d4 = d4 + 48;
}

/****************************************************/
/*32-bit Binary to ASCII Hexadecimal Conversion Code*/
/*The fast and compact method by Cypress            */
/*PIC32                                             */
/****************************************************/

int d_hex[8];

void Binary2ASCIIHex(int i_hex)
{
    unsigned t_hex;
    
    //digit 0
    t_hex = i_hex;
    d_hex[0] = t_hex & 0xf;
    
    //digit 1
    i_hex = i_hex>>4;
    t_hex = i_hex;
    d_hex[1] = t_hex & 0xf;

    //digit 2
    i_hex = i_hex>>4;
    t_hex = i_hex;
    d_hex[2] = t_hex & 0xf;

    //digit 3
    i_hex = i_hex>>4;
    t_hex = i_hex;
    d_hex[3] = t_hex & 0xf;

    //digit 4
    i_hex = i_hex>>4;
    t_hex = i_hex;
    d_hex[4] = t_hex & 0xf;

    //digit 5
    i_hex = i_hex>>4;
    t_hex = i_hex;
    d_hex[5] = t_hex & 0xf;

    //digit 6
    i_hex = i_hex>>4;
    t_hex = i_hex;
    d_hex[6] = t_hex & 0xf;

    //digit 7
    i_hex = i_hex>>4;
    t_hex = i_hex;
    d_hex[7] = t_hex & 0xf;

    //hex digits are separated
    //now convert to ascii
    int h;
    
    for(h=0;h<=7;h++)
    {
        if(d_hex[h] > 9)
        {
            d_hex[h] = d_hex[h] + 87;
        }
        else
        {
            d_hex[h] = d_hex[h] + 48;
        }
    }
    
    return;
}
