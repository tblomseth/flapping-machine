#include <Me_BaseShield.h>
#include <Me_InfraredReceiver.h>
#include <Me_UltrasonicSensor.h>
#include "MakeblockStepper.h"
#include <AccelStepper.h>
#include <Servo.h>
#include <Me_ServoDriver.h>

// Finite state machine

#define STATE_OFF 0
#define STATE_WAIT_FOR_ITEM 1
#define STATE_MOVE_ITEM_IN 2
#define STATE_STOP_ITEM 3
#define STATE_PULL_BACK_FLAP 4
#define STATE_LOWER_TOP_FLAP 5
#define STATE_RELEASE_FLAP 6
#define STATE_RAISE_TOP_FLAP 7
#define STATE_MOVE_ITEM_OUT_PHASE_ONE 8
#define STATE_MOVE_ITEM_OUT_PHASE_TWO 9

int fsmState;

Me_BaseShield baseShield;
Me_InfraredReceiver infraredReceiver; /* Note: As the code need to use interrupt 0, it works only when the Me - Infrared Receiver module is connected to the port 4(PORT_4) of Me - Base Shield. */
Me_UltrasonicSensor ultraSensor( PORT_5 ); //Ultrasonic module can ONLY be connected to port 3, 4, 5, 6, 7, 8 of base shield. ultraSensor.distanceCm()
MakeblockStepper moveItemStepper( 3200, baseShield, PORT_3 );
Me_ServoDriver servoDriver( PORT_2 ); //can ONLY be PORT_1,PORT_2

int SERVO1_UP = 130;
int SERVO1_DOWN = 115;

void setup() {
  // Initailize FSM
  fsmState = STATE_OFF;
  
  // Initialize communications
  Serial.begin(9600);

  // Initialize components
  baseShield.begin();  
  infraredReceiver.begin();  
  ultraSensor.begin();
  moveItemStepper.setSpeed( 12 );
  servoDriver.Servo1_begin();
  servoDriver.writeServo1( SERVO1_UP );
  delay( 500 );
  servoDriver.writeServo1( SERVO1_DOWN );
  delay( 500 );
  servoDriver.writeServo1( SERVO1_UP );  
}

void loop() {
  int remoteKey = infraredReceiver.read();
  int usDistance = ultraSensor.distanceCm();
  int stopMicroSwitch = baseShield.readMePortOutsidePin( PORT_8 ); 

  switch ( fsmState ) {
    case STATE_OFF:
      Serial.println( "OFF" ); 
      if ( remoteKey == IR_POWER_BUTTON ) {
        fsmState = STATE_WAIT_FOR_ITEM;          
      }
      break;
      
    case STATE_WAIT_FOR_ITEM:
      Serial.println( "WAIT FOR ITEM" );
      Serial.println( "usDistance = ");
      Serial.println( usDistance );
      Serial.println( "stopMicroSwitch = ");
      Serial.println( stopMicroSwitch );
      delay( 200 );
      if ( usDistance < 10 && stopMicroSwitch == HIGH ) {
        moveItemStepper.step( 100 );
        fsmState = STATE_MOVE_ITEM_IN;
      }
      break;
      
    case STATE_MOVE_ITEM_IN:
      Serial.println( "MOVE ITEM IN" );
      Serial.println( "usDistance = ");
      Serial.println( usDistance );
      Serial.println( "stopMicroSwitch = ");
      Serial.println( stopMicroSwitch );
      if ( stopMicroSwitch == HIGH ) {
        moveItemStepper.step( 100 );
        fsmState = STATE_MOVE_ITEM_IN;
      } else if ( stopMicroSwitch == LOW ) {
        fsmState = STATE_PULL_BACK_FLAP;
      }
      break;
      
    case STATE_PULL_BACK_FLAP: 
      Serial.println( "PULL BACK FLAP" );
      if ( stopMicroSwitch == LOW ) {
        moveItemStepper.step( 300 );
        fsmState = STATE_LOWER_TOP_FLAP;
      } 
      break;
    
    case STATE_LOWER_TOP_FLAP:
      Serial.println( "LOWER TOP FLAP" );    
      servoDriver.writeServo1( SERVO1_DOWN );
      delay( 1000 );
      fsmState = STATE_RELEASE_FLAP; 
    break;  
    
    case STATE_RELEASE_FLAP: 
      Serial.println( "RELEASE FLAP" );
      moveItemStepper.step( -300 );
      fsmState = STATE_RAISE_TOP_FLAP;    
      break;
    
    case STATE_RAISE_TOP_FLAP: 
      Serial.println( "RAISE TOP FLAP" );
      servoDriver.writeServo1( SERVO1_UP );
      fsmState = STATE_MOVE_ITEM_OUT_PHASE_ONE;
      break;

    case STATE_MOVE_ITEM_OUT_PHASE_ONE: 
      Serial.println( "MOVE ITEM OUT PHASE ONE" );
      if ( usDistance > 10 ) {
        moveItemStepper.step( -600 );
        fsmState = STATE_MOVE_ITEM_OUT_PHASE_ONE;
      } else {
        fsmState = STATE_MOVE_ITEM_OUT_PHASE_TWO;
      }
      break;

    case STATE_MOVE_ITEM_OUT_PHASE_TWO: 
      Serial.println( "MOVE ITEM OUT PHASE TWO" );
      if ( usDistance < 10 ) {
        moveItemStepper.step( -600 );
        fsmState = STATE_MOVE_ITEM_OUT_PHASE_TWO;
      } else {
        fsmState = STATE_WAIT_FOR_ITEM;
      }
      break;
    
    default: 
      Serial.println( "INVALID STATE" );
      break;
  
  }
}
