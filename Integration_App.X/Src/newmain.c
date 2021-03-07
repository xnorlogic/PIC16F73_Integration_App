/*
 * File:   newmain.c
 * Author: arodr
 *
 * Created on March 6, 2021, 2:20 PM
 */

// PIC16F73 Configuration Bit Settings

// 'C' source line config statements

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config CP = OFF         // FLASH Program Memory Code Protection bit (Code protection off)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#define _XTAL_FREQ 16000000

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include "Context.h"
#include <stdlib.h>

/*Port configuration*/
/*PORT A -> All OUTPUTS*/
#define PORTA_CONFIG 0x00
/*PORT B -> RB0 and RB1 INPUTS, RB2 RB3 RB4 RB5 RB6 RB7 OUTPUTS*/
#define PORTB_CONFIG 0x00
/*PORT C -> All OUTPUTS*/
#define PORTC_CONFIG 0x00

/*Bank selectors*/
#define BANK_0 0x00
#define BANK_1 0x01
#define BANK_2 0x02
#define BANK_3 0x03

/*Configuration for the AD port A*/
#define ADCON1_CONFIG_0 0x00 /*All Analog*/
#define ADCON1_CONFIG_1 0x01 /*refere to page 84 of pic16f73 manual*/
#define ADCON1_CONFIG_2 0x02 /*refere to page 84 of pic16f73 manual*/
#define ADCON1_CONFIG_3 0x03 /*refere to page 84 of pic16f73 manual*/
#define ADCON1_CONFIG_4 0x04 /*refere to page 84 of pic16f73 manual*/
#define ADCON1_CONFIG_5 0x05 /*refere to page 84 of pic16f73 manual*/
#define ADCON1_CONFIG_6 0x06 /*All Digital*/

/*LCD IO*/
#define RS RB5
#define EN RC7

#define D4 RB1
#define D5 RB2
#define D6 RB3
#define D7 RB4

#define LCD_CURSOR_1 1
#define LCD_CURSOR_2 2

#define MAX_DUTY_CYCLE 255

#include "lcd.h"

/*Global VAriables*/
U16 tick_count = 0;
U16 Duty_Cycle = 0;
char *STRING = "DC -> ";
/*----------------*/

void Setup(void);
void Loop(void);

void PORT_setup(U8 P_A, U8 P_B, U8 P_C);
void PORT_A_setup(U8 configSelect);

void PWM_Config(void);
void TIMER_0_Config(void);
void Interrupts_Config(void);

void on_board_LED_ON(void);
void on_board_LED_OFF(void);

void LCD_Reset(char *STRING,U8 Cursor_Location);
void Duty_Cycle_Write(U16 Duty_Cycle, U8 Cursor_Location);

/*
void mydelay_ms(U16 cycles);
void mydelay_sec(U16 cycles);
*/

void main(void) {
    
    static U8 buffer[2];
    
    /*Setup function call*/
    Setup();
    
    LCD_Reset("",LCD_CURSOR_1);
    
    /*Main loop*/
    while(1){ 
        
        utoa(buffer,tick_count,10);
        Lcd4_Set_Cursor (LCD_CURSOR_2, 0);
        Lcd4_Write_String(buffer);
        
        Duty_Cycle_Write(tick_count,LCD_CURSOR_1);
        
        /*Loop();*/
    }
    
}

void Loop(void){    
    /*Loop for increasing Duty cycle*/
    for(Duty_Cycle = 0;Duty_Cycle <= MAX_DUTY_CYCLE;Duty_Cycle++)
    {
        Duty_Cycle_Write(Duty_Cycle,LCD_CURSOR_2);
    }

    LCD_Reset(STRING,LCD_CURSOR_1);

    /*Loop for decreasing Duty cycle*/
    for(Duty_Cycle = MAX_DUTY_CYCLE;Duty_Cycle > 0;Duty_Cycle--)
    {
        Duty_Cycle_Write(Duty_Cycle,LCD_CURSOR_2);
    }
}

void PORT_setup(U8 P_A, U8 P_B, U8 P_C){
    STATUS = BANK_1;
    TRISA=P_A;
    TRISB=P_B;
    TRISC=P_C;
    PORTA = P_A;
    PORTB = P_B;
    PORTC = P_C;
    STATUS = BANK_0;
}

void PORT_A_setup(U8 configSelect){
    STATUS = BANK_1;
    ADCON1 = configSelect;
    STATUS = BANK_0;  
}

void PWM_Config(void){
    /*Test at 50% duty cycle*/
    PR2     = 0xFF;
    CCPR1L  = 0x80;
    CCP1CON = 0x0C;
    T2CON   = 0x04;
}

void TIMER_0_Config(void)
{
    /*Test with Prescaler value = 1:2 -> 000*/
    OPTION_REG = 0xC0;
}

void Interrupts_Config(void)
{
    INTCON = 0xE0;
}

void Setup(void){
    /*Port A Analog/Digital configuration*/
    PORT_A_setup(ADCON1_CONFIG_6);
    /*Port assignment configuration*/
    PORT_setup(PORTA_CONFIG,PORTB_CONFIG,PORTC_CONFIG);
    
    /*Configure PWM*/
    PWM_Config();
    /*Interrupts*/
    Interrupts_Config();
    /*TIMER 0*/
    TIMER_0_Config();
    
    Lcd4_Init();
    Lcd4_Clear();
}

/*
void mydelay_ms(U16 cycles) {
    U16 i;

    for (i = 1; i <= cycles; i++) {
        __delay_ms(1);
    }
}

void mydelay_sec(U16 cycles) {
    U16 i;

    for (i = 1; i <= cycles; i++) {
        mydelay_ms(1000);
    }
}
 * */

void on_board_LED_ON(void){
    RC0 = 1;
}

void on_board_LED_OFF(void){
    RC0 = 0;
}

void Duty_Cycle_Write(U16 Duty_Cycle, U8 Cursor_Location)
{
    static U8 buffer[4];
    
    utoa(buffer,Duty_Cycle,10);
    CCPR1L = (U8)Duty_Cycle;
    Lcd4_Set_Cursor (Cursor_Location, 0);
    Lcd4_Write_String(buffer);
}

void LCD_Reset(char *STRING,U8 Cursor_Location)
{
   Lcd4_Clear();
   Lcd4_Set_Cursor (Cursor_Location, 0);
   Lcd4_Write_String(STRING); 
}

void __interrupt(high_priority) tcInt(void)
{
    /*Any timer 0 interrupts*/
    if (TMR0IE && TMR0IF) 
    { 
        TMR0IF=0;
        
        if(tick_count<255)
        {
            ++tick_count;
        }
        else
        {
            tick_count = 0;
        }
    }

    return;
}
