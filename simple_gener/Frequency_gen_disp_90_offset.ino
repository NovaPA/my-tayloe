#include <SPI.h>
#include "s5351.h"
#include <Wire.h>
#include <string.h>
#include <stdlib.h>
#include <TM1637TinyDisplay6.h>
#include <Arduino.h> 

//#define CHECK_FREQUENCY_BOUNDS

// Define Digital Pins
#define CLK 3
#define DIO 2
int dispfr;

TM1637TinyDisplay6 display(CLK, DIO); // 6-Digit Display Class


enum FreqTarget {
  TARGET_CLK01, 
  TARGET_CLK2
};

Si5351 si5351;
static FreqTarget s_tgt = TARGET_CLK01;
static uint64_t s_freq = 5000000;

void frequency_disp(int64_t freqq, FreqTarget target)
{
    if (target == TARGET_CLK2)
    {
        Serial.print("CURRENT_TARGET_CLK01: ");
    }
    else
    {
        Serial.print("CURRENT_TARGET_CLK2: ");
    }
    Serial.println((long)freqq);
}

int8_t  frequency_set(uint64_t freq, FreqTarget target)
{
  int8_t res = 0;
  if (target == TARGET_CLK2)
  {
    res = si5351.set_freq(freq, SI5351_CLK2);
  }
  else
  {
    res = si5351.set_freq(freq, SI5351_CLK0);
    si5351.set_freq(freq, SI5351_CLK1);
    si5351.set_phase(SI5351_CLK0, 0);
    si5351.set_phase(SI5351_CLK1, 90);
  }

  return !res;
}

void setup() {

  bool i2c_found;

  Serial.begin(9600);
  Wire.setClock(400000);
  display.setBrightness(BRIGHT_HIGH);
  display.clear();
         display.showNumber(654321);
  i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);

  if (!i2c_found)
  {
    Serial.println("SI5351 not found on I2C bus!");
    delay(10);
    while (1);
  }

  si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
  si5351.set_ms_source(SI5351_CLK1, SI5351_PLLA);
  si5351.set_ms_source(SI5351_CLK2, SI5351_PLLB);

  Serial.println("Square wave generator");
  Serial.println("Welcome F4IFB");
  Serial.println("V0.5");
  Serial.println("Beta tests");

  frequency_set(5000000, TARGET_CLK01);
  frequency_set(6000000, TARGET_CLK2);

  frequency_disp(5000000, TARGET_CLK01);
  frequency_disp(6000000, TARGET_CLK2);
}

void loop() {
  static int timepressed = 0;
  char cmd[32];
  memset(cmd, 0, sizeof(cmd));

  if (Serial.available() > 0)
  {
    Serial.readBytesUntil('\n', cmd, sizeof(cmd) - 1);
    if (!strncmp("tg:", cmd, 3))
    {
      s_tgt = (FreqTarget)atoi(cmd + 3);
      if (s_tgt != TARGET_CLK01 && s_tgt != TARGET_CLK2)
      {
        s_tgt = TARGET_CLK01;
        Serial.println("!!! SET_TARGET: Bad argument !!!");
      }
      Serial.print("SET_TARGET: ");
      Serial.println(s_tgt);
    }
    else if (!strncmp("fr:", cmd, 3))
    {
      s_freq = strtoul(cmd + 3, NULL, 10);

#ifdef CHECK_FREQUENCY_BOUNDS
      if (s_freq > 200000000 && s_freq < 2500)
      {
        s_freq = 5000000;
        Serial.println("!!! SET_FREQUENCY: Bad argument !!!");
      }
#endif

      Serial.print("SET_FREQUENCY: ");
      dispfr = s_freq/1000;
         display.showNumber(dispfr);
      Serial.println((unsigned long)s_freq);

      if (!frequency_set(s_freq * 100, s_tgt))
      {
        frequency_set(5000000 * 100, s_tgt);
      }
      frequency_disp(s_freq, s_tgt);
    }
  }
}
