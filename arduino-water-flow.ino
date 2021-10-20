#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
  #include <avr/sleep.h>
  #include <avr/wdt.h>
  #include <avr/io.h>
  #include <avr/interrupt.h>
#endif

//#define SIMULATE

#define LEDS 6
#define NUM_LEDS 5
#define LEDS_INTENSITY 30
#define LEDS_INTENSITY_OVERFLOW_LITER 100

#define LED 9

// ATmega168 interrupt - INT0 is pin 4 (D2)
#define SENSOR_PIN 2
#define SENSOR_INTERRUPT digitalPinToInterrupt(SENSOR_PIN)

#define PULSE_FOR_LITER 300
#define TIME_BEFORE_SLEEP_MILLIS 300000 // 300s
#define COLOR_MIN 15000 // green
#define COLOR_MAX 65000 // red

Adafruit_NeoPixel pixels(NUM_LEDS, LEDS, NEO_GRB + NEO_KHZ800);

unsigned int pulseForLed = PULSE_FOR_LITER * NUM_LEDS;
unsigned long lastTime = 0;
bool hasOverflowLiter = false;

volatile long timeBeforeSleep = TIME_BEFORE_SLEEP_MILLIS;
volatile unsigned long pulseCount = 0;

ISR (INT0_vect) {
  timeBeforeSleep = TIME_BEFORE_SLEEP_MILLIS;
  
  digitalWrite(LED, HIGH);
  pulseCount++;
  digitalWrite(LED, LOW);
}

void blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
}

void goToSleep() {
  Serial.println(F("Sleep"));
  Serial.flush();
  
  sleep_mode();

  // ...
  
  sleep_disable();
  Serial.println(F("Wake up"));
  pulseCount = 0;
  hasOverflowLiter = false;
  pixels.setBrightness(LEDS_INTENSITY);
  blink();
}

unsigned long getColorHsvFromPulses(unsigned long pulses) {
  return getColorHsv(COLOR_MIN + ((pulseCount % pulseForLed) * (COLOR_MAX - COLOR_MIN) / pulseForLed));
}

unsigned long getColorHsv(unsigned long color) {
  return pixels.gamma32(pixels.ColorHSV(color));
}

void show() {
  pixels.clear();

  if (pulseCount > 30) { // 100mL
    byte nbLedsToUse = pulseCount / pulseForLed;

    if (nbLedsToUse > NUM_LEDS) {
      nbLedsToUse = NUM_LEDS;

      if (!hasOverflowLiter) {
        pulseCount = 0;
        hasOverflowLiter = true;
        pixels.setBrightness(LEDS_INTENSITY_OVERFLOW_LITER);
      }
    }
    
    for (byte i = 0; i <= nbLedsToUse; i++) {
      if (pulseCount / pulseForLed == i) {
        pixels.setPixelColor(i, getColorHsvFromPulses(pulseCount));
      } else {
        pixels.setPixelColor(i, getColorHsv(COLOR_MAX));
      }
    }
  }
  
  pixels.show();
  
  Serial.print(millis()); Serial.print(F(" ")); Serial.print(timeBeforeSleep); Serial.print(F(" ")); Serial.println(pulseCount);
}

void setup() {
  Serial.begin(115200);

  #ifdef __AVR__
    Serial.println(F("AVR detected"));
    
    #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
      clock_prescale_set(clock_div_1);
    #endif

    set_sleep_mode(SLEEP_MODE_PWR_SAVE);    
  #endif

  pinMode(LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  
  digitalWrite(SENSOR_PIN, HIGH);

  pixels.begin();
  pixels.clear();
  pixels.setBrightness(LEDS_INTENSITY);
  pixels.show();

  blink();

  // turn off interrupts
  cli();

  // set INT0 to trigger on falling edge
  EICRA |= (1 << ISC01);
  // Turns on INT0
  EIMSK |= (1 << SENSOR_INTERRUPT);

  // turn on interrupts
  sei();
  
  Serial.println(F("Started"));

  delay(10);

  // last instruvctions triggered interupt function so we reset the counter
  pulseCount = 0;
}

void loop() {
  #ifdef SIMULATE
    digitalWrite(LED, HIGH);
    timeBeforeSleep = TIME_BEFORE_SLEEP_MILLIS;  
    pulseCount++;  
    digitalWrite(LED, LOW);
  #endif
  
  timeBeforeSleep -= millis() - lastTime;
  lastTime = millis();

  if (timeBeforeSleep < 0) {
    goToSleep();
  }
  
  show();
  
  delay(1);
}
