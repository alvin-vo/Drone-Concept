#include <Servo.h>
#include <SPI.h>

Servo ESC1;  // create servo object to control the first ESC
Servo ESC2;  // create servo object to control the second ESC
Servo ESC3;  // create servo object to control the third ESC
Servo ESC4;  // create servo object to control the fourth ESC

char buff[255];
volatile byte indx;
volatile boolean process;
int pwm1 = 0, pwm2 = 0;
int midpoint = 0;  // Initial midpoint value

void setup() {
  Serial.begin(9600);  // initialize serial communication at 115200 bits per second
  pinMode(MISO, OUTPUT); // have to send on master in so it set as output
  SPCR |= _BV(SPE); // turn on SPI in slave mode
  indx = 0; // buffer empty
  process = false;
  SPI.attachInterrupt(); // turn on interrupt

  ESC1.attach(2, 1000, 2000); // (pin, min pulse width, max pulse width in microseconds) for the first ESC
  ESC2.attach(3, 1000, 2000); // (pin, min pulse width, max pulse width in microseconds) for the second ESC
  ESC3.attach(5, 1000, 2000); // (pin, min pulse width, max pulse width in microseconds) for the third ESC
  ESC4.attach(6, 1000, 2000); // (pin, min pulse width, max pulse width in microseconds) for the fourth ESC

  Serial.println("Setup complete. Waiting for SPI data...");
}

ISR (SPI_STC_vect) // SPI interrupt routine
{
  byte c = SPDR; // read byte from SPI Data Register
  if (indx < sizeof(buff)) {
    buff[indx++] = c; // save data in the next index in the array buff
    if (c == '\n') {
      buff[indx - 1] = 0; // replace newline ('\n') with end of string (0)
      process = true;
      indx = 0; // reset index to zero
    }
  }
}

void loop() {

  if (process) {
    process = false; // reset the process
    Serial.println(buff); // print the array on serial monitor
  }
  sscanf(buff, "PWM1: %d, PWM2: %d\n", &pwm1, &pwm2);

  int esc1, esc2, esc3, esc4;

  // Handle directions
  if (pwm1 > 0) { // right
    // esc1 = pwm1*0.2;
    esc1 = 0;
    esc2 = pwm1;
    esc3 = pwm1;
    // esc4 = pwm1*0.2;
    esc4 = 0;
  } else if (pwm1 < 0) { // left
    esc1 = -pwm1;
    // esc2 = -pwm1*0.2;
    // esc3 = -pwm1*0.2;
    esc2 = 0;
    esc3 = 0;
    esc4 = -pwm1;
  } 

  if (pwm2 > 0) { // forward
    esc1 = pwm2;
    esc2 = pwm2;
    esc3 = 0;
    esc4 = 0;
    // esc3 = pwm2*0.2;
    // esc4 = pwm2*0.2;
  } else if (pwm2 < 0) { // backwards
    // esc1 = -pwm2*0.2;
    // esc2 = -pwm2*0.2;
    esc1 = 0;
    esc2 = 0;
    esc3 = -pwm2;
    esc4 = -pwm2;
  }

  // Handle diagonal movements
  if (pwm1 > 0 && pwm2 > 0) { // forward-right
    esc1 = pwm2;
    esc2 = pwm1;
    esc3 = pwm1;
    esc4 = 0;
  } else if (pwm1 < 0 && pwm2 > 0) { // forward-left
    esc1 = pwm2;
    esc2 = pwm2;
    esc3 = 0;
    esc4 = -pwm1;
  } else if (pwm1 > 0 && pwm2 < 0) { // backward-right
    esc1 = 0;
    esc2 = pwm1;
    esc3 = -pwm2;
    esc4 = pwm1;
  } else if (pwm1 < 0 && pwm2 < 0) { // backward-left
    esc1 = -pwm2;
    esc2 = 0;
    esc3 = -pwm2;
    esc4 = -pwm1;
  }
  
  if (pwm1 == 0 && pwm2 == 0) {
    esc1 = 0;
    esc2 = 0;
    esc3 = 0;
    esc4 = 0;
  }

  ESC1.write(esc1);
  ESC2.write(esc2);
  ESC3.write(esc3);
  ESC4.write(esc4);

  Serial.print("ESC1 set to: "); Serial.print(esc1);
  Serial.print(" ESC2 set to: "); Serial.print(esc2);
  Serial.print(" ESC3 set to: "); Serial.print(esc3);
  Serial.print(" ESC4 set to: "); Serial.println(esc4);
  Serial.println();

  delay(500);  // wait for a short time to avoid too much serial output
}