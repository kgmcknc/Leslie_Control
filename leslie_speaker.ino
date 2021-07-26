#include <Servo.h>
// you have to add the timerone library to the arduino project when compiling
#include <TimerOne.h>

//#define USE_SERIAL

#define TWEETER_MIN 370
#define TWEETER_MAX 650
#define TWEETER_RANGE (TWEETER_MAX - TWEETER_MIN)

#define WOOFER_MIN 380
#define WOOFER_MAX 520
#define WOOFER_RANGE (WOOFER_MAX - WOOFER_MIN)

#define ADC_MIN 0
#define ADC_MAX 1023
#define ADC_RANGE (ADC_MAX - ADC_MIN)
#define ADC_FLEX 5

#ifdef USE_SERIAL
#define MOTOR_OFF_COUNT 3
#else
#define MOTOR_OFF_COUNT 850
#endif

int switch_tip_pin = 7;
int switch_ring_pin = 6;
int switch_sleeve_pin = 5;
int tip_temp;
int switch_tip_value = -1;
int ring_temp;
int switch_ring_value = -1;
int sleeve_temp;
int switch_sleeve_value = -1;

int tweeter_reverse = 4;
int tweeter_pwm = 10;
int tweeter_forward = 2;
int woofer_reverse = 11;
int woofer_pwm = 9;
int woofer_forward = 8;

int pot_top_right_pin = A0;
int pot_bottom_left_pin = A1;
int pot_bottom_right_pin = A2;
int pot_top_left_pin = A3;
int pot_center_pin = A4;

int pot_top_left_value = (-1 - ADC_FLEX);
int pot_top_right_value = (-1 - ADC_FLEX);
int pot_bottom_left_value = (-1 - ADC_FLEX);
int pot_bottom_right_value = (-1 - ADC_FLEX);
int pot_center_value = (-1 - ADC_FLEX);
int tl_temp;
int tr_temp;
int bl_temp;
int br_temp;
int c_temp;

int tweeter_speed = TWEETER_MIN;
int woofer_speed = WOOFER_MIN;

int switch_type_pin = 13;
int switch_temp = 0;
int switch_type_value = -1;

int motor_off_counter = 0;

int update_speed;

void print_data(void);
void update_adc_values(void);
void set_motor_speed(void);

void set_tweeter_float(void);
void set_tweeter_forward(void);
void set_tweeter_reverse(void);

void set_woofer_float(void);
void set_woofer_forward(void);
void set_woofer_reverse(void);

int motors_on = 1;
int speed_select = 0;

void setup() {
  // put your setup code here, to run once:

  pinMode(tweeter_forward, OUTPUT);
  pinMode(tweeter_reverse, OUTPUT);
  pinMode(tweeter_pwm, OUTPUT);

  pinMode(woofer_forward, OUTPUT);
  pinMode(woofer_reverse, OUTPUT);
  pinMode(woofer_pwm, OUTPUT);
  
  pinMode(pot_top_right_pin, INPUT);
  pinMode(pot_bottom_left_pin, INPUT);
  pinMode(pot_bottom_right_pin, INPUT);
  pinMode(pot_top_left_pin, INPUT);
  pinMode(pot_center_pin, INPUT);

  pinMode(switch_type_pin, INPUT_PULLUP);
  
  pinMode(switch_tip_pin, INPUT_PULLUP);
  pinMode(switch_ring_pin, INPUT_PULLUP);
  pinMode(switch_sleeve_pin, OUTPUT);
    
  set_tweeter_float();
  set_woofer_float();

  Timer1.initialize(100);  //100us = 10khz
  
  update_adc(); // do initial read to start
  delay(1000); // wait for adc to init
  
  get_switch_type();
  get_switch_setting();
  update_adc();
  set_motor_speed();

  #ifdef USE_SERIAL
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Leslie Speaker\n");
  #endif

}

void loop() {
  // put your main code here, to run repeatedly:

  get_switch_type();
  get_switch_setting();
  update_adc();
  set_motor_speed();
  
  #ifdef USE_SERIAL
  print_data();
  #else
  //delay(50);
  #endif
}

void get_switch_type(void){
  switch_temp = digitalRead(switch_type_pin);
  switch_temp = !switch_temp; // invert cause it's pulled up when open, pulled down when connected
  if(switch_temp != switch_type_value){
    switch_type_value = switch_temp;
    if(switch_type_value){
      // trs switch
      
      // currently doing sleeve as "common"
      // switch will be:
      // "open" - meaning motors off
      // "speed 1" - ts
      // "speed 2" - rs

      pinMode(switch_tip_pin, INPUT_PULLUP);
      pinMode(switch_ring_pin, INPUT_PULLUP);
      pinMode(switch_sleeve_pin, OUTPUT);
      
      //digitalWrite(switch_tip_pin,0);
      //digitalWrite(switch_ring_pin,0);
      digitalWrite(switch_sleeve_pin, 0);
    } else {
      // ts switch

      // switch will be:
      // "open" - meaning keep previous setting
      // "closed" - change speed (turn motors on if they are off)
      // "closed held for period of time" - turn off motors

      pinMode(switch_tip_pin, INPUT_PULLUP);
      pinMode(switch_ring_pin, INPUT_PULLUP);
      pinMode(switch_sleeve_pin, OUTPUT);
      
      //digitalWrite(switch_tip_pin,0);
      //digitalWrite(switch_ring_pin,0);
      digitalWrite(switch_sleeve_pin, 0);
    }
    delay(200);
  }
}

void get_switch_setting(void){
  if(switch_type_value){
    // trs switch
    tip_temp = digitalRead(switch_tip_pin);
    ring_temp = digitalRead(switch_ring_pin);
    //sleeve_temp = digitalRead(switch_sleeve_pin);

    if((tip_temp != switch_tip_value) || (ring_temp != switch_ring_value)){
      switch_tip_value = tip_temp;
      switch_ring_value = ring_temp;
      if((switch_tip_value == 1) && (switch_ring_value == 1)){
        speed_select = 0;
        motors_on = 0;
      } else {
        motors_on = 1;
        // speed 1 speed off ts closure
        if(switch_tip_value == 0){
          speed_select = 1;
        } else {
          speed_select = 0;
        }
      }
    }
  } else {
    // ts switch
    tip_temp = digitalRead(switch_tip_pin);
    //ring_temp = digitalRead(switch_ring_pin);
    //sleeve_temp = digitalRead(switch_sleeve_pin);

    if(tip_temp != switch_tip_value){
      switch_tip_value = tip_temp;
      switch_ring_value = ring_temp;
      if(switch_tip_value == 0){
        motor_off_counter = 0;
        speed_select = !speed_select;
        if(motors_on == 0){
          motors_on = 1;
        }
      }
    }
    
    // continue incrementing counter as long as switch is held
    if(switch_tip_value == 0){
      motor_off_counter = motor_off_counter + 1;
    }
  }
  
  if(motor_off_counter > MOTOR_OFF_COUNT){
    motor_off_counter = 0;
    motors_on = 0;
  }
}

void update_adc(void){
  tl_temp = analogRead(pot_top_left_pin);
  tr_temp = analogRead(pot_top_right_pin);
  bl_temp = analogRead(pot_bottom_left_pin);
  br_temp = analogRead(pot_bottom_right_pin);
  c_temp = analogRead(pot_center_pin);

  pot_top_left_value = ADC_MAX - tl_temp;
  pot_top_right_value = ADC_MAX - tr_temp;
  pot_bottom_left_value = ADC_MAX - bl_temp;
  pot_bottom_right_value = ADC_MAX - br_temp;
  pot_center_value = c_temp;
}

void set_motor_speed(void){
  if(motors_on){
    set_tweeter_forward();
    set_woofer_forward();
  } else {
    set_tweeter_float();
    set_woofer_float();
  }

  if(speed_select){
    tweeter_speed = TWEETER_MIN + (((long int) TWEETER_RANGE * pot_top_right_value) / ADC_MAX);
    woofer_speed = WOOFER_MIN + (((long int) WOOFER_RANGE * pot_bottom_right_value) / ADC_MAX);
  } else {
    tweeter_speed = TWEETER_MIN + (((long int) TWEETER_RANGE * pot_top_left_value) / ADC_MAX);
    woofer_speed = WOOFER_MIN + (((long int) WOOFER_RANGE * pot_bottom_left_value) / ADC_MAX);
  }
  Timer1.pwm(tweeter_pwm, tweeter_speed);   // 50% DC
  Timer1.pwm(woofer_pwm, woofer_speed);   // 50% DC
}

void print_data(void){
  Serial.print("\n");
  Serial.print("ADC: ");
  Serial.print(pot_bottom_left_value, DEC);
  Serial.print(", ");
  Serial.print(pot_top_left_value, DEC);
  Serial.print(", ");
  Serial.print(pot_bottom_right_value, DEC);
  Serial.print(", ");
  Serial.print(pot_top_right_value, DEC);
  Serial.print(", ");
  Serial.print(pot_center_value, DEC);
  Serial.print("\n");
  Serial.print("Switch Type: ");
  Serial.print(switch_type_value, DEC);
  Serial.print(", TRS: ");
  Serial.print(switch_tip_value, DEC);
  Serial.print(",");
  Serial.print(switch_ring_value, DEC);
  Serial.print(",");
  Serial.print(switch_sleeve_value, DEC);
  Serial.print("\n");
  Serial.print("Motors, On, Speed, Woofer, Tweeter: ");
  Serial.print(motors_on, DEC);
  Serial.print(",");
  Serial.print(speed_select, DEC);
  Serial.print(",");
  Serial.print(woofer_speed, DEC);
  Serial.print(",");
  Serial.print(tweeter_speed, DEC);
  Serial.print("\n");
  delay(1000);
}

void set_tweeter_float(void){
  digitalWrite(tweeter_forward, HIGH);
  digitalWrite(tweeter_reverse, HIGH);
}

void set_tweeter_forward(void){
  digitalWrite(tweeter_forward, HIGH);
  digitalWrite(tweeter_reverse, LOW);
}

void set_tweeter_reverse(void){
  digitalWrite(tweeter_forward, LOW);
  digitalWrite(tweeter_reverse, HIGH);
}

void set_woofer_float(void){
  digitalWrite(woofer_forward, HIGH);
  digitalWrite(woofer_reverse, HIGH);
}

void set_woofer_forward(void){
  digitalWrite(woofer_forward, HIGH);
  digitalWrite(woofer_reverse, LOW);
}

void set_woofer_reverse(void){
  digitalWrite(woofer_forward, LOW);
  digitalWrite(woofer_reverse, HIGH);
}
