/* MODULE main */
#include "MK64F12.h"

/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "Pins1.h"
#include "FX1.h"
#include "GI2C1.h"
#include "WAIT1.h"
#include "CI2C1.h"
#include "CsIO1.h"
#include "IO1.h"
#include "MCUC1.h"
#include "SM1.h"
#include "TU1.h"
#include "PWM1.h"
#include "PwmLdd1.h"
#include "PWM3.h"
#include "PwmLdd3.h"
#include "PWM4.h"
#include "PwmLdd4.h"
#include "TU3.h"
#include "TU4.h"
#include "PWM2.h"
#include "PwmLdd2.h"
#include "TU2.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "PDD_Includes.h"
#include "Init_Config.h"
/* User includes (#include below this line is not maintained by Processor Expert) */

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
#define DEADZONE 17000
unsigned char write[512];

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - out_min) + out_min;
}

void ADC0_Init(void) {
    SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK;  // Enable ADC0 clock
    ADC0_CFG1 = ADC_CFG1_MODE(3) | ADC_CFG1_ADICLK(0); // 16-bit conversion, bus clock
    ADC0_SC1A = ADC_SC1_ADCH(31);      // Disable ADC0
}

unsigned short ADC0_Read16b(unsigned char channel) {
    ADC0_SC1A = ADC_SC1_ADCH(channel); // Start conversion on specified channel
    while (ADC0_SC2 & ADC_SC2_ADACT_MASK); // Conversion in progress
    while (!(ADC0_SC1A & ADC_SC1_COCO_MASK)); // Wait until conversion complete
    return ADC0_RA; // Read conversion result
}
void ADC1_Init(void) {
    SIM_SCGC3 |= SIM_SCGC3_ADC1_MASK;  // Enable ADC1 clock
    ADC1_CFG1 = ADC_CFG1_MODE(3) | ADC_CFG1_ADICLK(0); // 16-bit conversion, bus clock
    ADC1_SC1A = ADC_SC1_ADCH(31);      // Disable ADC1
}
unsigned short ADC1_Read16b(uint8_t channel) {
    ADC1_SC1A = channel; // Start conversion on the specified channel
    while (ADC1_SC2 & ADC_SC2_ADACT_MASK); // Conversion in progress
    while (!(ADC1_SC1A & ADC_SC1_COCO_MASK)); // Wait until conversion complete
    return ADC1_RA; // Read conversion result
}

long apply_deadzone(long value, long center, long threshold) {
    if (value > center - threshold && value < center + threshold) {
        return center; // Within deadzone, set to center
    } else {
        return value; // Outside deadzone, return original value
    }
}
int main(void) {
    PE_low_level_init();
    uint32_t delay;
    long pwm1, pwm2, pwm3, pwm4;
    int len;
    int joy_x, joy_y, pot3;
    LDD_TDeviceData *SM1_DeviceData;
    SM1_DeviceData = SM1_Init(NULL);

//    SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK; // Enable Port B Clock Gate Control
//    PORTB_GPCLR = 0x00040100; // Configures pins 2 of Port B for GPIO
//    PORTD_GPCLR = 0x00000001; // Configures pins 0 of PORT D for GPIO
//    GPIOB_PDDR =  0x00000001; // Configures pins 0 for input
//    GPIOB_PDDR = 0x00000000; // Configures pins 2 of Port B for input

    FX1_Init();
    ADC0_Init(); // Initialize ADC0
    ADC1_Init();

    for (;;) {
    	joy_x = ADC0_Read16b(13); // Read ADC value from channel 13 (PTB3)
		joy_y = ADC0_Read16b(12); // Read ADC value from channel 11 (PTC2)
		pot3 = ADC1_Read16b(14); // Read ADC value from channel 14 (PTB10)
//		pot4 = ADC1_Read16b(15); // Read ADC value from channel 15 (PTB11)

		// Apply deadzone to joystick readings
		joy_x = apply_deadzone(joy_x, 32768, DEADZONE); // 32768 is the center value for 16-bit ADC
		joy_y = apply_deadzone(joy_y, 32768, DEADZONE);

		pwm3 = map(pot3, 0, 65535, 1, 100);

        pwm1 = (map(joy_x, 0, 65535, -170, 170) + 1) * (pwm3 * 0.01); // Map 16-bit ADC value to 8-bit PWM value
//        if (pwm1 < 0) {
//        	pwm1 *= -1;
//        }

        pwm2 = (map(joy_y, 0, 65535, -170, 170) + 1) * (pwm3 * 0.01);
//        if (pwm2 < 0) {
//			pwm2 *= -1;
//		}

//        pwm3 = map(pot3, 0, 65535, 0, 255);
//        pwm4 = map(pot4, 0, 65535, 0, 255);

        printf("pot1: %5d, pot2: %5d, pot3: %5d \n", joy_x, joy_y, pot3);
        printf("PWM1: %3d, PWM2: %3d\n\n", pwm1, pwm2);

        len = sprintf(write, "PWM1: %3d, PWM2: %3d\n", pwm1, pwm2);
//        GPIOB_PSOR = (1 << 2); // set high
        SM1_SendBlock(SM1_DeviceData, &write, len);
        for (delay = 0; delay < 100000; delay++); // delay
//        GPIOB_PCOR = (1 << 2); // set low
//        uint32_t button = GPIOB_PDIR & 0x04;
//        if (button == 1) {
//        	GPIOB_PTOR = (1 << 2); // toggle
//        }

    }
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
