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

#define TRANSITION_MIN_SPEED 0
#define TRANSITION_MAX_SPEED 1023
#define TRANSITION_RANGE (TRANSITION_MAX_SPEED - TRANSITION_MIN_SPEED)

#define ADC_MIN 0
#define ADC_MAX 1024
#define ADC_RANGE (ADC_MAX - ADC_MIN)
#define ADC_FLEX 5

#define PWM_MIN 1
#define PWM_MAX 1023

#define TWEETER_UPDATE_DELAY 25
#define WOOFER_UPDATE_DELAY 10
#define TWEETER_TRANSITION_RATIO 200
#define WOOFER_TRANSITION_RATIO 50
#define WOOFER_MAX_SPEEDUP 200

#define SWITCH_POLARITY 1 // 0 is normally open, 1 is normally closed

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
int adc_temp;

int tweeter_speed = TWEETER_MIN;
int woofer_speed = WOOFER_MIN;
int current_tweeter_speed = TWEETER_MIN;
int current_woofer_speed = WOOFER_MIN;
int transition_speed = TRANSITION_MIN_SPEED;
int tweeter_transition_speed = TRANSITION_MIN_SPEED;
int woofer_transition_speed = TRANSITION_MIN_SPEED;
int ramp_down;

int switch_type_pin = 13;
int switch_temp = 0;
int switch_type_value = -1;

int motor_off_counter = 0;

int update_speed;
int ramp_to_max;

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

int tweeter_update_count = 0;
int woofer_update_count = 0;

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
  get_motor_speed();
  
  current_tweeter_speed = tweeter_speed;
  current_woofer_speed = woofer_speed;
  update_speed = 0;
  ramp_to_max = 0;
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
  get_motor_speed();
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
        update_speed = 1;
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
      if(switch_tip_value == SWITCH_POLARITY){
        motor_off_counter = 0;
        update_speed = 1;
        if(motors_on == 0){
          motors_on = 1;
        } else {
          speed_select = !speed_select;
        }
      }
    }
    
    // continue incrementing counter as long as switch is held
    if(switch_tip_value == SWITCH_POLARITY){
      motor_off_counter = motor_off_counter + 1;
    }
  }

  if(update_speed){
    tweeter_update_count = 0;
    woofer_update_count = 0;
  } else {
    if(tweeter_update_count > TWEETER_UPDATE_DELAY){
      tweeter_update_count = 0;
    }
    if(woofer_update_count > WOOFER_UPDATE_DELAY){
      woofer_update_count = 0;
    }
  }

  ramp_to_max = update_speed;
  if(motor_off_counter > MOTOR_OFF_COUNT){
    motor_off_counter = 0;
    speed_select = 0;
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
  pot_center_value = ADC_MAX - c_temp;
}

void get_motor_speed(void){
  if(speed_select){
    tweeter_speed = TWEETER_MIN + (((long int) TWEETER_RANGE * pot_top_right_value) / ADC_MAX);
    woofer_speed = WOOFER_MIN + (((long int) WOOFER_RANGE * pot_bottom_right_value) / ADC_MAX);
  } else {
    tweeter_speed = TWEETER_MIN + (((long int) TWEETER_RANGE * pot_top_left_value) / ADC_MAX);
    woofer_speed = WOOFER_MIN + (((long int) WOOFER_RANGE * pot_bottom_left_value) / ADC_MAX);
  }

  tweeter_transition_speed = 1 + TRANSITION_MIN_SPEED + ((((long int) TRANSITION_RANGE * pot_center_value) / ADC_MAX) / TWEETER_TRANSITION_RATIO);
  
  ramp_down = (pot_center_value > (ADC_MAX/2));
  if(ramp_down){
    woofer_transition_speed = 1 + TRANSITION_MIN_SPEED + ((((long int) TRANSITION_RANGE * (ADC_MAX - pot_center_value)) / ADC_MAX) / WOOFER_TRANSITION_RATIO);
  } else {
    woofer_transition_speed = 1 + TRANSITION_MIN_SPEED + ((((long int) TRANSITION_RANGE * pot_center_value) / ADC_MAX) / WOOFER_TRANSITION_RATIO);
  }
}

void set_motor_speed(void){
  if(update_speed){
    update_speed = 0;
    if(ramp_down){
      if(woofer_speed > current_woofer_speed){
        ramp_to_max = 1;
      }
      else {
        ramp_to_max = 0;
      }
    } else {
      ramp_to_max = 0;
    }
  }

  if(tweeter_update_count == 0){
    if(current_tweeter_speed != tweeter_speed){
      if(current_tweeter_speed < tweeter_speed){
        if(((long int) current_tweeter_speed + tweeter_transition_speed) > tweeter_speed){
          current_tweeter_speed = tweeter_speed;
        } else {
          current_tweeter_speed = current_tweeter_speed + tweeter_transition_speed;
        }
      } else {
        if(current_tweeter_speed < ((long int) tweeter_speed + tweeter_transition_speed)){
          current_tweeter_speed = tweeter_speed;
        } else {
          current_tweeter_speed = current_tweeter_speed - tweeter_transition_speed;
        }
      }
    }
  }

  if(ramp_to_max || (woofer_update_count == 0)){
    if(ramp_to_max){
      if(current_woofer_speed < PWM_MAX){
        if(woofer_transition_speed > WOOFER_MAX_SPEEDUP){
          current_woofer_speed = current_woofer_speed + 1;
        } else {
          current_woofer_speed = current_woofer_speed + 1 + (WOOFER_MAX_SPEEDUP - woofer_transition_speed);
        }
      } else {
        ramp_to_max = 0;
      }
    } else {
      if(current_woofer_speed != woofer_speed){
        if(current_woofer_speed < woofer_speed){
          if(((long int) current_woofer_speed + woofer_transition_speed) > woofer_speed){
            current_woofer_speed = woofer_speed;
          } else {
            current_woofer_speed = current_woofer_speed + woofer_transition_speed;
          }
        } else {
          if(current_woofer_speed < ((long int) woofer_speed + woofer_transition_speed)){
            current_woofer_speed = woofer_speed;
          } else {
            current_woofer_speed = current_woofer_speed - woofer_transition_speed;
          }
        }
      }
    } 
  }

  tweeter_update_count = tweeter_update_count + 1;
  woofer_update_count = woofer_update_count + 1;
  
  Timer1.pwm(tweeter_pwm, current_tweeter_speed);   // 50% DC
  Timer1.pwm(woofer_pwm, current_woofer_speed);   // 50% DC

  if(motors_on){
    set_tweeter_forward();
    set_woofer_forward();
  } else {
    set_tweeter_float();
    set_woofer_float();
  }
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
