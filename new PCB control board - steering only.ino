/* Code for V2 of Box Bots robot control board
 * 
 * Arduino Nano clone w/ CH340 USB connected to TB6612 motor driver board
 * hooked up to receiver of 2.4GHz TX/RX 
 * 
 * standard model: FlySky FS-GT2B 3-Channel RC 
 * 
 * CH1 - Steering
 * CH2 - Throttle
 * CH3 - Switch
 * 
 */

#include <EnableInterrupt.h>

#define   lpwm    3     // pulse width modulation for left motor is pin 3
#define   lpin1   4     // left control pin one is pin 4
#define   lpin2   5     // left control pin two is pin 5
#define   standby 7     // standby pin is 7 - LOW=motor driver off, HIGH=motor driver on
#define   rpwm    6     // pulse width modulation for right motor is pin 6
#define   rpin1   8     // right control pin one is pin8
#define   rpin2   9     // right control pin two is pin 9


#define   forward 0     
#define   reverse 1
#define   coast   2
#define   brake   3

#define   ch1_pin   10    // input channel one is on pin 10
#define   ch2_pin  11    // input channel two is on pin 11
#define   ch3_pin   12    // input channel three is on pin 12

#define   ch1_index  0
#define   ch2_index  1
#define   ch3_index  2

int mtrspeed = 110 ;

int ch1; // Steering
int ch2; // Thottle
int ch3; // Weapon Switch
int tdeadband = 10;  // How much in the throttle neutral position does it count as neutral centered on 255  (exp: for 15, deadband is from 240 to 270, 15 each side of 255) 
int sdeadband = 5;  // how much in the steering neutral position does it count as neutral centered on 255  (exp: for 15, deadband is from 240 to 270, 15 each side of 255) 
int spd = 0;
byte neutral = 255;

int count = 0;

uint16_t rc_values[3];    //array of PWM values rec'd 
uint32_t rc_start[3];     //time at start of data collection
volatile uint16_t rc_shared[3];     //temp array for PWM values during reception


//--------------------------------------------------------------------------------------  
//--------------------------------------------------------------------------------------     
void setup() {
  // put your setup code here, to run once:

 // for debugging
  Serial.begin(9600); // Pour a bowl of Serial (for debugging)    
  
  //setup output pins
  pinMode(lpwm, OUTPUT);
  pinMode(lpin1, OUTPUT);
  pinMode(lpin2, OUTPUT);
  pinMode(rpwm, OUTPUT);
  pinMode(rpin1, OUTPUT);
  pinMode(rpin2, OUTPUT);
  pinMode(standby, OUTPUT);

  //setup input pins
  pinMode(ch1_pin, INPUT);       // channel one of RC receiver, steering
  pinMode(ch2_pin, INPUT);       // channel two of RC receiver, throttle
  pinMode(ch3_pin, INPUT);       // channel three of RC receiver, switch

  //turn on interrupts
  enableInterrupt(ch1_pin, get_ch1, CHANGE);
  enableInterrupt(ch2_pin, get_ch2, CHANGE);
  enableInterrupt(ch3_pin, get_ch3, CHANGE);

  // turn on the things
  digitalWrite(standby, HIGH);  

  //start writing to output pins
  digitalWrite(lpin1, LOW);
  digitalWrite(lpin2, LOW);
  digitalWrite(rpin1, LOW);
  digitalWrite(rpin2, LOW);
  analogWrite(lpwm, mtrspeed);
  analogWrite(rpwm, mtrspeed);
}


//--------------------------------------------------------------------------------------
void loop() {
// put your main code here, to run repeatedly:


// get PWM data from transmitter through pin interrupts
  rc_read_values();
   
  ch1 = rc_values[ch1_index]; // Steering : 1000 Left, 2000 Right
  ch2 = rc_values[ch2_index]; // Throttle : 1000 Reverse, 2000 Forward
  ch3 = rc_values[ch3_index]; // Switch, toggle switch between 1000 (99x) and 2000 (199x) value

  ch1 = map(ch1, 1000,2000, 0, 1000); //center over 500
  ch2 = map(ch2, 1000,2000, 0, 1000); //center over 500
  ch3 = map(ch3, 1000,2000, 0, 1000); //center over 500

  ch1 = ch1 >> 1;  // right bit shift, divide by 2
  ch2 = ch2 >> 1;  // right bit shift, divide by 2

  ch1 = constrain(ch1, 0, 512);
  ch2 = constrain(ch2, 0, 512);
  ch3 = constrain(ch3, 0, 1000);
  
  // So now both ch1 and ch2 are in the range of 0 to 512, with 255 being neutral
  // ch3 is either 0ish or 1000ish

//  Serial.print("ch1:"); Serial.print(ch1);    Serial.print("\t");
//  Serial.print("ch2:"); Serial.print(ch2);    Serial.print("\t");
//  Serial.print("ch3:"); Serial.println(ch3);
//  
//  Serial.print(ch1);    Serial.print("\t");
//  Serial.print(ch2);    Serial.print("\t");
//  Serial.println(ch3);
  
  delay(100);
  
  spd = abs(neutral-ch2);
  
  if (ch2<(neutral-tdeadband)) {   // outside deadband, in reverse
    // if throttle in reverse do this
    motordirection(reverse);   
    steering();
  }
  else { 
    if (ch2>(neutral+tdeadband)) {  // outside deadband, going forward
      // throttle in forward do this
      motordirection(forward);
      steering();
    }
    else { // in deadband, bring both motors to a stop
      motordirection(brake);
    }
  }
}

//-------------------------------------------------------------------------------------- 
//  get pwm data from channel using interupts & return

void rc_read_values() {
  noInterrupts();
  memcpy(rc_values, (const void *)rc_shared, sizeof(rc_shared));
  interrupts();
}

void get_input(uint8_t channel, uint8_t input_pin) {
  if (digitalRead(input_pin) == HIGH) {
    rc_start[channel] = micros();
  } else {
    uint16_t rc_compare = (uint16_t)(micros() - rc_start[channel]);
    rc_shared[channel] = rc_compare;
  }

//  Serial.print("count # = ");
//  Serial.print(count);          Serial.print("\t");
//  Serial.print(rc_shared[0]);   Serial.print("\t");
//  Serial.print(rc_shared[1]);   Serial.print("\t");
//  Serial.println(rc_shared[2]);
//  count ++;    
}

void get_ch1() { get_input(ch1_index, ch1_pin); }
void get_ch2() { get_input(ch2_index, ch2_pin); }
void get_ch3() { get_input(ch3_index, ch3_pin); }

//-------------------------------------------------------------------------------------- 

//--------------------------------------------------------------------------------------
void motordirection(byte direction) {

  switch (direction) {
    
    case forward:
      digitalWrite(lpin1, HIGH);
      digitalWrite(lpin2, LOW);
      digitalWrite(rpin1, HIGH);
      digitalWrite(rpin2, LOW);
      break;

    case reverse:
      digitalWrite(lpin1, LOW);
      digitalWrite(lpin2, HIGH);
      digitalWrite(rpin1, LOW);
      digitalWrite(rpin2, HIGH);
      break;

    case brake:
      digitalWrite(lpin1, HIGH);
      digitalWrite(lpin2, HIGH);
      digitalWrite(rpin1, HIGH);
      digitalWrite(rpin2, HIGH);
      break;

    default:  // coast condition
      digitalWrite(lpin1, LOW);
      digitalWrite(lpin2, LOW);
      digitalWrite(rpin1, LOW);
      digitalWrite(rpin2, LOW);
    
  }
}

//--------------------------------------------------------------------------------------
void steering() {
  int turn = abs(255-ch1);
  // turn = turn >> 1;    // making steering less sensitive by dividing turn result by 4.
 
  int drag = (spd-turn);
  drag = constrain(drag,0,255);


//  Serial.print("spd:");
//  Serial.print(spd);
//  Serial.print("\t");
//  Serial.print("turn:");
//  Serial.print(turn);
//  Serial.print("\t");
//  Serial.print("drag:");
//  Serial.println(drag);

  
  if (turn > sdeadband) {   // outside the steering deadband
    if (ch1<neutral) { //steering left
       analogWrite(lpwm, spd);
       analogWrite(rpwm, drag);
    }
    else {  // steering right
      analogWrite(lpwm, drag);
      analogWrite(rpwm, spd);
    }
  }
  else { // in the steering deadband
      analogWrite(lpwm, spd) ;
      analogWrite(rpwm, spd);
    
  }
}