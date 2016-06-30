/* Code for V2 of Box Bots 
 * robot control board
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
//#include <Servo.h>

#define   lpwm    3     // pulse width modulation for left motor is pin 3
#define   lpin1   4     // left control pin one is pin 4
#define   lpin2   5     // left control pin two is pin 5
#define   standby 6     // standby pin is 6 - LOW=motor driver off, HIGH=motor driver on
#define   rpin1   7     // right control pin one is pin 7
#define   rpin2   8     // right control pin two is pin 8
#define   rpwm    9     // pulse width modulation for right motor is pin 9

#define   forward 0     
#define   reverse 1
#define   coast   2
#define   brake   3

#define   in_ch1   10    // input channel one is on pin 10
#define   in_ch2   13    // input channel two is on pin 11
// #define   in_ch3   12    // input channel three is on pin 12

// Servo myservo; 

int mtrspeed = 110 ;

int ch1; // Steering
int ch2; // Thottle
// int ch3; // Weapon Switch
int tdeadband = 10;  // How much in the throttle neutral position does it count as neutral centered on 255  (exp: for 15, deadband is from 240 to 270, 15 each side of 255) 
int sdeadband = 5;  // how much in the steering neutral position does it count as neutral centered on 255  (exp: for 15, deadband is from 240 to 270, 15 each side of 255) 
int spd = 0;
byte neutral = 255;

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


void steering() {
  int turn = abs(255-ch1);
  
  // turn = turn >> 1;    // making steering less sensitive by dividing turn result by 4.

  
  int drag = (spd-turn);
  drag = constrain(drag,0,255);

/*
  Serial.print("spd:");
  Serial.print(spd);
  Serial.print("\t");
  Serial.print("turn:");
  Serial.print(turn);
  Serial.print("\t");
  Serial.print("drag:");
  Serial.println(drag);
*/
  
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
          
void setup() {
  // put your setup code here, to run once:
  pinMode(lpwm, OUTPUT);
  pinMode(lpin1, OUTPUT);
  pinMode(lpin2, OUTPUT);
  pinMode(rpwm, OUTPUT);
  pinMode(rpin1, OUTPUT);
  pinMode(rpin2, OUTPUT);
  pinMode(standby, OUTPUT);
  
  pinMode(in_ch1, INPUT);       // channel one of RC receiver, steering
  pinMode(in_ch2, INPUT);       // channel two of RC receiver, throttle
  // pinMode(in_ch3, INPUT);       // channel three of RC receiver, switch

  digitalWrite(lpin1, LOW);
  digitalWrite(lpin2, LOW);
  digitalWrite(rpin1, LOW);
  digitalWrite(rpin2, LOW);

  digitalWrite(standby, HIGH);  // turn on the things

  analogWrite(lpwm, mtrspeed);
  analogWrite(rpwm, mtrspeed);

  //myservo.attach(weaponpin);    // activate servo output on the weapon 
  //myservo.write(0);             // set weapon speed at zero

 //
 // Serial.begin(9600); // Pour a bowl of Serial (for debugging)    
}


void loop() {
  // put your main code here, to run repeatedly:


  // pulsein returning value of 1000 to 2000 (1500 default neutral position) 
  // All Numbers are with transmitter channels in Normal position
  ch1 = pulseIn(in_ch1, HIGH, 25000); // Steering : 1000 Left, 2000 Right
  ch2 = pulseIn(in_ch2, HIGH, 25000); // Throttle : 1000 Reverse, 2000 Forward
 // ch3 = pulseIn(in_ch3, HIGH, 25000); // Switch, toggle switch between 1000 (99x) and 2000 (199x) value

  ch1 = map(ch1, 1000,2000, 0, 1000); //center over 500
  ch2 = map(ch2, 1000,2000, 0, 1000); //center over 500
//  ch3 = map(ch3, 1000,2000, 0, 1000); //center over 500

  ch1 = ch1 >> 1;  // right bit shift, divide by 2
  ch2 = ch2 >> 1;  // right bit shift, divide by 2

  ch1 = constrain(ch1, 0, 512);
  ch2 = constrain(ch2, 0, 512);
 // ch3 = constrain(ch3, 0, 1000);

  // So now both ch1 and ch2 are in the range of 0 to 512, with 255 being neutral
  // ch3 is either 0ish or 1000ish

   
/* 
 *  
  Serial.print(ch1);
  Serial.print("\t");
  Serial.print(ch2);
  Serial.print("\t");
  Serial.println(ch3);
  delay(100);
  
*/

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

/*  
  if (ch3 < weaponlvl) {    // If the switch if in "off" position and outputing less than the weaponlvl trigger number
    myservo.write(0);  // give the weaspon ESC a zero for off
  }
  else {
    if (ch3 > weaponlvl) {
      myservo.write(maxweapon);   // turn on the weapon to the max setting
    }
  }
*/

  
}
