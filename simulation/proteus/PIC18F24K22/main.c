/* Main.c file generated by New Project wizard
 *
 * Created:   Sa Nov 2 2013
 * Processor: PIC18F24K22
 * Compiler:  MPLAB XC8
 */

#include <xc.h>
#include <p18cxxx.h>
#include <stdint.h>
#include <spi.h>

#include "dp_adc.h"
#include "dp_led.h" 
#include "dp_spi.h"
#include "dp_spitypes.h"

#define _XTAL_FREQ 4000000
#define MAX (32)

void display_color(uint8_t raw_color)
{
    // TODO: decode color to rgb
    // ...
    led_set_rgb(raw_color, raw_color, raw_color);
}

void main(void)
{
    uint8_t max = MAX;
    dp_std_status_t current_status;
    dp_std_command_t cmd_from_master;
    int8_t adc = 1;
    
    // Debug LED
    TRISCbits.TRISC2 = 0;
    PORTCbits.RC2 = 0;

    // Initialize LEDs
    led_initialize();
    
    // Initialize ADC
    OpenADC ( 
        ADC_FOSC_16 & ADC_RIGHT_JUST & ADC_4_TAD, // ADC_4_TAD = Converting over 4*(1/(FOSC/16)) = 4*(1/250kHz) = 16us
        ADC_CH0 & ADC_INT_OFF & ADC_REF_VDD_VSS, // FOSC = 4 MHz
        0b0000000000000001 // wieso geht ADC_1ANA nicht?
    );

    // Open SPI
    OpenSPI1(SLV_SSON, MODE_00, SMPMID);

    // Initial ADC conversion and SPI reading
    ConvertADC();
    cmd_from_master.byte = spi_tranceive(0);
    
    /* MAIN LOOP:
     * At first we read the ADC value of the piezo-weight sensors if already present. 
     * Then we fill this data into a status package which is to be sent to our SPI master.
     * After that we await a command from our SPI master and send our status to it (tranceive).
     * If we got a color, we light our LEDs per PWM. If we got a command, we analyze it and take action.
     */
    while (1) {
        if (!BusyADC()) {
            adc = ReadADC() >> 2; // we just need 8bits from the returned 10bit sample
            ConvertADC(); // already start next conversion. Meanwhile we're gonna do stuff with SPI
        }
        
        current_status.data.value = adc;
        current_status.is_pressed = 0; // TODO: Analyze the adc value and set this one to 0 or 1
        cmd_from_master.byte = spi_tranceive(current_status.byte); // TODO: error handling needed: what if spi is not present? Will we operate on our own?
        if (cmd_from_master.is_rgb) {
            // we got a color
            PORTCbits.RC2 = 0; // Debug LED
            led_set_rgb(cmd_from_master.rgb.r*10, cmd_from_master.rgb.g*10, cmd_from_master.rgb.b*10); // TODO: introduce multiplicator to reach 100% PWM
        } else {
            // we got a command
            PORTCbits.RC2 = 1; // Debug LED
        }
    }
 }
 
