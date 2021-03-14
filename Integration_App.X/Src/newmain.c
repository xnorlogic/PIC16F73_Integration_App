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
/*PORT A*/
/*0000 0000*/
#define PORTA_CONFIG 0x00
/*PORT B*/
/*0000 0001*/
#define PORTB_CONFIG 0x01
/*PORT C*/
/*0001 0000*/
#define PORTC_CONFIG 0x10

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
#define EN RA0
#define D4 RB1
#define D5 RB2
#define D6 RB3
#define D7 RB4

/*EEPROM IO*/
#define CS RA1

/*EEPROM Instructions*/
#define EEPROM_WRITE_ENABLE 0x06
#define EEPROM_WRITE        0x02
#define EEPROM_READ         0x03

#define LCD_CURSOR_1 1
#define LCD_CURSOR_2 2

#include "lcd.h"

/*Global VAriables------------------------------------------------------------*/
U16 tick_count = 0;
U16 write_counter = 0;
U16 Duty_Cycle = 0;
U16 external_interrupt_cnt = 0;
char *STRING = "DC -> ";
U16 address = 43690;
U16 buffer[2];
char  EEPROM_DATA;
/*----------------------------------------------------------------------------*/

void Setup(void);
void Loop(void);

void PORT_setup(U8 P_A, U8 P_B, U8 P_C);
void PORT_A_setup(U8 configSelect);

void PWM_Config(void);
void TIMER_0_Config(void);
void TIMER_1_Config(void);
void Interrupts_Config(void);
void SPI_Config(void);

U8 SPI_transfer(U8 data);
void EEPROM_25LC1024_WRITE_EN(void);
void EEPROM_25LC1024_WRITE(U16 address, U8 data);
char EEPROM_25LC1024_READ(U16 address);

void on_board_LED_ON(void);
void on_board_LED_OFF(void);

void LCD_Reset(char *STRING,U8 Cursor_Location);
void Duty_Cycle_Write(U16 Duty_Cycle, U8 Cursor_Location);

/*Tasks-----------------------------------------------------------------------*/
/*Task 0 @ 1:32 prescale*/
void TIMER_0_TASK(void);
/*Task 1 @ 1:8  prescale*/
void TIMER_1_TASK(void);
/*External interrupt at RB0*/
void EXTERNAL_INTERRUPT_TASK(void);
/*----------------------------------------------------------------------------*/

void main(void) {
    /*Setup function call*/
    Setup();
    
    /*Read the EEPROM and initialize the counter*/
    EEPROM_DATA = EEPROM_25LC1024_READ(address);
    write_counter = EEPROM_DATA;
    
    /*Initial LCD MEssage*/
    LCD_Reset("READY",LCD_CURSOR_1);
    __delay_ms(1000);
    LCD_Reset("",LCD_CURSOR_1);
    
    /*Main loop*/
    while(1){     
        Loop();
    }
}

void Loop(void){      
    /*Display the EEPROM data on the LCD*/
    utoa(buffer,EEPROM_DATA,10);
    Lcd4_Set_Cursor (LCD_CURSOR_1, 0);
    Lcd4_Write_String(buffer);
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
    /*Timer 1*/
    TIMER_1_Config();
    /*SPI Config*/
    SPI_Config();
    
    Lcd4_Init();
    Lcd4_Clear();
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
    nRBPU  = 1;
    INTEDG = 1;
    T0CS   = 0;
    T0SE   = 0;
    PSA    = 0;
    PS2    = 1;
    PS1    = 0;
    PS0    = 0;
}

void TIMER_1_Config(void)
{
    T1CKPS1 = 1;
    T1CKPS0 = 1;
    T1OSCEN = 0;
    nT1SYNC = 0;
    TMR1CS  = 0;
    TMR1ON  = 1;
}
void Interrupts_Config(void)
{
    GIE    = 1;
    PEIE   = 1;
    TMR0IE = 1;
    INTE   = 1;
    RBIE   = 0;
    /*PSPIE  = 0;*/ /*Only in 28 pin devices*/
    ADIE   = 0;
    RCIE   = 0;
    TXIE   = 0;
    SSPIE  = 0;
    CCP1IE = 0;
    TMR2IE = 0;
    TMR1IE = 1;
    CCP2IE = 0;
    CCP2IE = 0;
            
    TMR0IF = 0;
    INTF   = 0;
    RBIF   = 0;
    /*PSPIF  = 0;*/ /*Only in 28 pin devices*/
    ADIF   = 0;
    RCIF   = 0;
    TXIF   = 0; 
    SSPIF  = 0;
    CCP1IF = 0;
    TMR2IF = 0;
    TMR1IF = 0;
    CCP2IF = 0;
}

void SPI_Config(void)
{
    SMP = 0;
    CKE = 1;
    
    WCOL  = 0;
    SSPOV = 0;
    SSPEN = 1;
    CKP   = 0;
    SSPM3 = 0;
    SSPM2 = 0;
    SSPM1 = 1;
    SSPM0 = 0;
    
    CS=1;
}

U8 SPI_transfer(U8 data)
{
    SSPBUF = data;
    
    while(!BF)
    {
        
    }
    
    return SSPBUF;
}

void EEPROM_25LC1024_WRITE_EN(void)
{
    CS=0;
    SPI_transfer(EEPROM_WRITE_ENABLE);
    CS=1; 
}

void EEPROM_25LC1024_WRITE(U16 address, U8 data)
{
    CS=0;
    SPI_transfer(EEPROM_WRITE); // write instruction
    SPI_transfer(0x00);
    SPI_transfer((address >> 8) & 255);
    SPI_transfer(address & 255);
    SPI_transfer(data);
    CS=1; 
}

char EEPROM_25LC1024_READ(U16 address)
{
    static char buffer = 0;
    
    CS=0;
    SPI_transfer(EEPROM_READ); // write instruction
    SPI_transfer(0x00);
    SPI_transfer((address >> 8) & 255);
    SPI_transfer(address & 255);
    buffer = SPI_transfer(0); // Clock out the data
    CS=1;
    
    return buffer;
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

void on_board_LED_ON(void){
    RC0 = 1;
}

void on_board_LED_OFF(void){
    RC0 = 0;
}

void TIMER_0_TASK(void)
{
    /*write the TIMER 0 interrupt counter to the PWM */
    CCPR1L = (U8)tick_count;
}

void TIMER_1_TASK(void)
{
    /*Read the EEPROM*/
    EEPROM_DATA = EEPROM_25LC1024_READ(address);
}

void EXTERNAL_INTERRUPT_TASK(void)
{
    /*Write the new counter to the EEPROM*/
    EEPROM_25LC1024_WRITE_EN();
    EEPROM_25LC1024_WRITE(address,(U8)write_counter);
}

void __interrupt(high_priority) tcInt(void)
{
    /*timer 0 interrupts*/
    if (TMR0IE && TMR0IF) 
    { 
        /*reset TIMER 0 interrupt flag*/
        TMR0IF=0;
        
        /*increment the tick count for the TIMER 0 interrupt*/
        if(tick_count<255)
        {
            ++tick_count;
        }
        else
        {
            tick_count = 0;
        }
        /*Task Call*/
        TIMER_0_TASK();
    }
    
    /*timer 1 interrupts*/
    if (TMR1IE && TMR1IF) 
    { 
        /*reset TIMER 0 interrupt flag*/
        TMR1IF=0;
        /*Task Call*/
        TIMER_1_TASK();
    }
    
    /*External interrupt*/
    if (INTE && INTF) 
    { 
        /*Reset the external interrupt flag*/
        INTF=0;
        
        /*increment counter for external interrupt*/
        if(write_counter<255)
        {
            ++write_counter;
        }
        else
        {
            write_counter = 0;
        }
        /*Task Call*/
        EXTERNAL_INTERRUPT_TASK();
    }
}