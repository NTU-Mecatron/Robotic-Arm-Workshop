/**
 * @file main.cpp
 * @author Thum MK
 * @author Scott CJX
 * @brief
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <Arduino.h>
#include <Servo.h>

#include <stdint.h>

// change timing delays for smoother runs
const uint8_t readJS_delay_ms = 10;
const uint8_t updateServo_delay_ms = 10;

const uint8_t js_rateOfChange = 4;
const uint8_t base_rateOfChange = 4;
const uint8_t shoulder_rateOfChange = 1;
const uint8_t elbow_rateOfChange = 1;
const uint8_t clamp_rateOfChange = 4;

// glob defs
#define servo_base_pin 8
#define servo_shoulder_pin 9
#define servo_elbow_pin 10
#define servo_clamp_pin 11
#define servo_tester_pin 12

#define joystick1_x_base A0
#define joystick1_y_shoulder A1
#define js_up_thresh 680
#define js_down_thresh 340

#define button_down_elbow_pin 2
#define button_up_elbow_pin 4
#define button_right_clamp_pin 5
#define button_left_clamp_pin 3

uint16_t js_adc_base;
uint16_t js_adc_shoulder;

uint8_t button_state_left_elbow = 0;
uint8_t button_state_right_elbow = 0;
uint8_t button_state_up_clamp = 0;
uint8_t button_state_down_clamp = 0;

uint8_t targ_base_deg = 0;
uint8_t targ_shoulder_deg = 0;
uint8_t targ_elbow_deg = 160;
uint8_t targ_clamp_deg = 92;

uint8_t real_base_deg = 0;
uint8_t real_shoulder_deg = 0;
uint8_t real_elbow_deg = 160;
uint8_t real_clamp_deg = 92;

const uint8_t max_base_deg = 180;
const uint8_t min_base_deg = 0;
const uint8_t max_shoulder_deg = 60;
const uint8_t min_shoulder_deg = 0;
const uint8_t max_elbow_deg = 160;
const uint8_t min_elbow_deg = 0;
const uint8_t max_clamp_deg = 124;
const uint8_t min_clamp_deg = 0;

// obj defs
Servo servo_base;
Servo servo_shoulder;
Servo servo_elbow;
Servo servo_clamp;
Servo servo_tester;

// vol defs
volatile uint64_t nextTs_ReadJS = 0;
volatile uint64_t nextTs_UpdatedServos = 0;
volatile uint64_t timeNow = 0;

// functions placeholders
void slowStartUp();
void readJoystick();
void moveServos();
void calcNewVals();

void setup()
{
  Serial.begin(9600);

  servo_base.attach(servo_base_pin);
  servo_shoulder.attach(servo_shoulder_pin);
  servo_elbow.attach(servo_elbow_pin);
  servo_clamp.attach(servo_clamp_pin);

  // servo tester set to 0deg for ease of arm assembly
  servo_tester.attach(servo_tester_pin);
  servo_tester.write(0);
  
  pinMode(button_up_elbow_pin, INPUT_PULLUP);
  pinMode(button_down_elbow_pin, INPUT_PULLUP);
  pinMode(button_right_clamp_pin, INPUT_PULLUP);
  pinMode(button_left_clamp_pin, INPUT_PULLUP);
}

void loop()
{
  slowStartUp();
  timeNow = millis();
  if (timeNow >= nextTs_ReadJS)
  {
    nextTs_ReadJS = timeNow + readJS_delay_ms;
    readJoystick();
  }
  if (timeNow >= nextTs_UpdatedServos)
  {
    nextTs_UpdatedServos = timeNow + updateServo_delay_ms;
    moveServos();
  }

  Serial.print("base degree : ");
  Serial.print(real_base_deg);
  Serial.print(", shoulder degree : ");
  Serial.print(real_shoulder_deg);
  Serial.print(", elbow degree : ");
  Serial.print(real_elbow_deg);
  Serial.print(", clamp degree : ");
  Serial.println(real_clamp_deg);
  Serial.println(analogRead(js_adc_base));
  delay(5);
}

void slowStartUp()
{
  while (real_base_deg != targ_base_deg ||
         real_shoulder_deg != targ_shoulder_deg ||
         real_elbow_deg != targ_elbow_deg ||
         real_clamp_deg != targ_clamp_deg)
  {
    if (real_base_deg != targ_base_deg)
    {
      real_base_deg += (real_base_deg > targ_base_deg) ? -base_rateOfChange : base_rateOfChange;
    }
    if (real_shoulder_deg != targ_shoulder_deg)
    {
      real_shoulder_deg += (real_shoulder_deg > targ_shoulder_deg) ? -shoulder_rateOfChange : shoulder_rateOfChange;
    }
    if (real_elbow_deg != targ_elbow_deg)
    {
      real_elbow_deg += (real_elbow_deg > targ_elbow_deg) ? -elbow_rateOfChange : elbow_rateOfChange;
    }
    if (real_clamp_deg != targ_clamp_deg)
    {
      real_clamp_deg += (real_clamp_deg > targ_clamp_deg) ? -clamp_rateOfChange : clamp_rateOfChange;
    }

    servo_base.write(real_base_deg);
    servo_shoulder.write(real_shoulder_deg);
    servo_elbow.write(real_elbow_deg);
    servo_clamp.write(real_clamp_deg);

    delay(10);
  }
}

/**
 * @brief read and change the target values of the servo positions
 *
 */
void readJoystick()
{
  js_adc_base = analogRead(joystick1_x_base);
  if (js_adc_base > js_up_thresh)
  {
    targ_base_deg = min(targ_base_deg + js_rateOfChange, max_base_deg);
  }
  else if (js_adc_base < js_down_thresh)
  {
    targ_base_deg = max(targ_base_deg - js_rateOfChange, min_base_deg);
  }

  js_adc_shoulder = analogRead(joystick1_y_shoulder);
  if (js_adc_shoulder > js_up_thresh)
  {
    targ_shoulder_deg = min(targ_shoulder_deg + js_rateOfChange, max_shoulder_deg);
  }
  else if (js_adc_shoulder < js_down_thresh)
  {
    targ_shoulder_deg = max(targ_shoulder_deg - js_rateOfChange, min_shoulder_deg);
  }

  button_state_left_elbow = digitalRead(button_down_elbow_pin);
  if (button_state_left_elbow == 0)
  {
    targ_elbow_deg = max(targ_elbow_deg - js_rateOfChange, min_elbow_deg);
  }
  button_state_right_elbow = digitalRead(button_up_elbow_pin);
  if (button_state_right_elbow == 0)
  {
    targ_elbow_deg = min(targ_elbow_deg + js_rateOfChange, max_elbow_deg);
  }
  button_state_up_clamp = digitalRead(button_right_clamp_pin);
  if (button_state_up_clamp == 0)
  {
    targ_clamp_deg = min(targ_clamp_deg + js_rateOfChange, max_clamp_deg);
  }
  button_state_down_clamp = digitalRead(button_left_clamp_pin);
  if (button_state_down_clamp == 0)
  {
    targ_clamp_deg = max(targ_clamp_deg - js_rateOfChange, min_clamp_deg);
  }
}

/**
 * @brief calculate real values
 * @note assumes the target val shall not exceed the max/min thrsh mtrs set
 */
void calcNewVals()
{
  if (abs((int)real_base_deg - (int)targ_base_deg) > (base_rateOfChange * 2))
  {
    real_base_deg += (real_base_deg > targ_base_deg) ? -base_rateOfChange : base_rateOfChange;
  }
  if (abs((int)real_shoulder_deg - (int)targ_shoulder_deg) > (shoulder_rateOfChange * 2))
  {
    real_shoulder_deg += (real_shoulder_deg > targ_shoulder_deg) ? -shoulder_rateOfChange : shoulder_rateOfChange;
  }
  if (abs((int)real_elbow_deg - (int)targ_elbow_deg) > (elbow_rateOfChange * 2))
  {
    real_elbow_deg += (real_elbow_deg > targ_elbow_deg) ? -elbow_rateOfChange : elbow_rateOfChange;
  }
  if (abs((int)real_clamp_deg - (int)targ_clamp_deg) > (clamp_rateOfChange * 2))
  {
    real_clamp_deg += (real_clamp_deg > targ_clamp_deg) ? -clamp_rateOfChange : clamp_rateOfChange;
  }
}

/**
 * @brief move servos to their real vals
 *
 */
void moveServos()
{
  calcNewVals();
  servo_base.write(real_base_deg);
  servo_shoulder.write(real_shoulder_deg);
  servo_elbow.write(real_elbow_deg);
  servo_clamp.write(real_clamp_deg);
}

