 #include <Me_BaseShield.h>
#include <Me_BaseShieldMotorDriver.h>
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

/* Hardware components */
Me_BaseShield baseShield;
Me_InfraredReceiver infraredReceiver; /* Note: As the code need to use interrupt 0, it works only when the Me - Infrared Receiver module is connected to the port 4(PORT_4) of Me - Base Shield. */
Me_UltrasonicSensor ultraSensor( PORT_5 ); //Ultrasonic module can ONLY be connected to port 3, 4, 5, 6, 7, 8 of base shield. ultraSensor.distanceCm()
// MakeblockStepper moveItemStepper( 3200, baseShield, PORT_3 );
Me_ServoDriver servoDriver( PORT_1 ); //can ONLY be PORT_1,PORT_2
Me_BaseShieldMotorDriver baseShieldMotorDriver;

/* Flapping servo */
int SERVO1_UP = 133;
int SERVO1_DOWN = 110;
int lowerTopFlapDelay = 200;

/* Move item stepper */
int portNo = PORT_3;
AccelStepper moveItemStepper( forwardstep, backwardstep );
int MOVE_ITEM_STEPPER_PULSE_LENGTH = 30;
float MOVE_ITEM_STEPPER_MAX_SPEED = 2500.0;
float MOVE_ITEM_IN_MAX_SPEED = 2500.0;
float PULL_BACK_FLAP_MAX_SPEED = 2000.0;
float MOVE_ITEM_OUT_STEPPER_MAX_SPEED = 2500.0;
float MOVE_ITEM_STEPPER_ACCELERATION = 30000.0;
int MOVE_ITEM_IN_STEPS = 10000;
int PULL_BACK_FLAP_STEPS = 600;
int MOVE_ITEM_OUT_STEPS = 650;
boolean moveItemStepperIsRunning = false;

/* Solenoid contact servo */
int CONTACT_OFF_POSITION = 140;
int CONTACT_ON_POSITION = 180;

/* Output conveyor motor */
int OUTPUT_CONVEYANCE_SPEED = -190;
int OUTPUT_CONVEYANCE_DURATION = 155;

void setup() {
  // Initailize FSM
  fsmState = STATE_OFF;
  
  // Initialize communications
  Serial.begin( 9600 );

  // Initialize components
  baseShield.begin();
  //infraredReceiver.begin();  
  ultraSensor.begin();

  moveItemStepper.setMaxSpeed( MOVE_ITEM_STEPPER_MAX_SPEED );
  moveItemStepper.setAcceleration( MOVE_ITEM_STEPPER_ACCELERATION );

  servoDriver.Servo1_begin();
  servoDriver.Servo2_begin();

  /* Servo test */
  servoDriver.writeServo1( SERVO1_UP );
  delay( 300 );
  servoDriver.writeServo1( SERVO1_DOWN );
  delay( 300 );
  servoDriver.writeServo1( SERVO1_UP ); 
   
  servoDriver.writeServo2( CONTACT_OFF_POSITION );
  servoDriver.writeServo2( CONTACT_ON_POSITION );
  delay( 300 );
  servoDriver.writeServo2( CONTACT_OFF_POSITION );
   
  /* Move item stepper test */
  
  moveItemStepper.runToNewPosition( 800 );
  
  Serial.print( "setup: stopMicroSwitch = ");
  Serial.println( baseShield.readMePortOutsidePin( PORT_8 ) );
  
  baseShieldMotorDriver.begin();
  
}

void loop() {
  //int remoteKey = infraredReceiver.read();
  int remoteKey = -1;
  int usDistance = 100;
  int stopMicroSwitch = baseShield.readMePortOutsidePin( PORT_8 ); 
/*
      Serial.println( "usDistance = ");
      Serial.println( usDistance );
delay( 500 );
  */
  
  switch ( fsmState ) {
    case STATE_OFF:
      Serial.println( "OFF" ); 
      if ( remoteKey == IR_POWER_BUTTON ) {
        fsmState = STATE_WAIT_FOR_ITEM;          
      }
      fsmState = STATE_WAIT_FOR_ITEM;
      break;
      
    case STATE_WAIT_FOR_ITEM:
      Serial.println( "WAIT FOR ITEM" );
      usDistance = ultraSensor.distanceCm();
      Serial.print( "usDistance = "); Serial.println( usDistance );
      delay( 200 );
      if ( usDistance < 10 && stopMicroSwitch == HIGH ) {
        fsmState = STATE_MOVE_ITEM_IN;
        Serial.println( "->> MOVE ITEM IN" );
      }
      break;
      
    case STATE_MOVE_ITEM_IN:
    /*
      Serial.print( "stopMicroSwitch = "); Serial.println( stopMicroSwitch );
      Serial.print( "distanceToGo = "); Serial.println( moveItemStepper.distanceToGo() );
      Serial.print( "speed = "); Serial.println( moveItemStepper.speed() );*/
      if ( stopMicroSwitch == HIGH && moveItemStepper.distanceToGo() == 0 ) {
        moveItemStepper.setMaxSpeed( MOVE_ITEM_IN_MAX_SPEED );
        moveItemStepper.move( MOVE_ITEM_IN_STEPS );
        Serial.print( "targetPosition = "); Serial.println( moveItemStepper.targetPosition() );
        fsmState = STATE_MOVE_ITEM_IN;
      } else if ( stopMicroSwitch == HIGH && moveItemStepper.distanceToGo() > 0 ) {
        moveItemStepper.run();
        fsmState = STATE_MOVE_ITEM_IN;
      } else if ( stopMicroSwitch == LOW ) {
        moveItemStepper.stop();
        moveItemStepper.move( 0 );
        moveItemStepperIsRunning = false;
        fsmState = STATE_PULL_BACK_FLAP;
        Serial.println( "->> PULL BACK FLAP" );
        Serial.print( "distanceToGo = "); Serial.println( moveItemStepper.distanceToGo() );
        Serial.print( "targetPosition = "); Serial.println( moveItemStepper.targetPosition() );
        //delay( 1000 );
      }
      break;
      
    case STATE_PULL_BACK_FLAP: 
      //Serial.println( "PULL BACK FLAP" );
      if ( moveItemStepper.distanceToGo() == 0 && !moveItemStepperIsRunning ) {
        moveItemStepper.setMaxSpeed( PULL_BACK_FLAP_MAX_SPEED ); 
        moveItemStepper.move( PULL_BACK_FLAP_STEPS );
        Serial.print( "distanceToGo = "); Serial.println( moveItemStepper.distanceToGo() );
        Serial.print( "targetPosition = "); Serial.println( moveItemStepper.targetPosition() );
        moveItemStepperIsRunning = true;
        fsmState = STATE_PULL_BACK_FLAP;
      } else if ( moveItemStepper.distanceToGo() == 0 && moveItemStepperIsRunning ) {
        moveItemStepperIsRunning = false;
        fsmState = STATE_LOWER_TOP_FLAP;
        Serial.println( "->> LOWER TOP FLAP" );
//delay( 1000 );        
      } else {
        moveItemStepper.run();
        fsmState = STATE_PULL_BACK_FLAP;
      }
      break;
    
    case STATE_LOWER_TOP_FLAP:
      Serial.println( "LOWER TOP FLAP" );    
      servoDriver.writeServo1( SERVO1_DOWN );
      delay( lowerTopFlapDelay );
      fsmState = STATE_RELEASE_FLAP;
      Serial.println( "->> RELEASE FLAP" ); 
    break;  
    
    case STATE_RELEASE_FLAP: 
      if ( moveItemStepper.distanceToGo() == 0 && !moveItemStepperIsRunning ) { 
        //moveItemStepper.move( -600 );
        moveItemStepperIsRunning = true;
        fsmState = STATE_RELEASE_FLAP;        
      } else if ( moveItemStepper.distanceToGo() == 0 && moveItemStepperIsRunning ) {
        moveItemStepperIsRunning = false;
        fsmState = STATE_RAISE_TOP_FLAP;
      } else {
        moveItemStepper.run();
        fsmState = STATE_RELEASE_FLAP;
      }
      break;
    
    case STATE_RAISE_TOP_FLAP: 
      Serial.println( "RAISE TOP FLAP" );
      servoDriver.writeServo1( SERVO1_UP );
      fsmState = STATE_MOVE_ITEM_OUT_PHASE_ONE;
      Serial.println( "->> MOVE ITEM OUT PHASE ONE" );
      break;

    case STATE_MOVE_ITEM_OUT_PHASE_ONE: 
      if (stopMicroSwitch == LOW && !moveItemStepperIsRunning && moveItemStepper.distanceToGo() == 0 ) {
        servoDriver.writeServo2( CONTACT_ON_POSITION );
        delay( 100 ); // In order to not putting to much pressure from the side on the solenoids
        moveItemStepper.setMaxSpeed( MOVE_ITEM_OUT_STEPPER_MAX_SPEED );
        moveItemStepper.move( MOVE_ITEM_OUT_STEPS );
        moveItemStepperIsRunning = true;
        baseShieldMotorDriver.runMotor1( OUTPUT_CONVEYANCE_SPEED );
        fsmState = STATE_MOVE_ITEM_OUT_PHASE_ONE;
      } else if ( moveItemStepper.distanceToGo() != 0) {
        moveItemStepper.run();
        fsmState = STATE_MOVE_ITEM_OUT_PHASE_ONE;
      } else {
        moveItemStepperIsRunning = false;
        fsmState = STATE_MOVE_ITEM_OUT_PHASE_TWO;
      }
      break;

    case STATE_MOVE_ITEM_OUT_PHASE_TWO: 
      Serial.println( "MOVE ITEM OUT PHASE TWO" );
        delay( OUTPUT_CONVEYANCE_DURATION );
        baseShieldMotorDriver.stopMotor1();
        servoDriver.writeServo2( CONTACT_OFF_POSITION );
        
      //if ( usDistance > 10 ) {
        //moveItemStepper.step( -600 );
        //moveItemStepper.move( -600 );
        //fsmState = STATE_MOVE_ITEM_OUT_PHASE_TWO;
      //} else {
        fsmState = STATE_WAIT_FOR_ITEM;
      //}
      break;
    
    default: 
      Serial.println( "INVALID STATE" );
      break;
  
  }
}

void forwardstep() { 
  baseShield.setMePort(portNo, LOW, HIGH);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
  baseShield.setMePort(portNo, HIGH, HIGH);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
  baseShield.setMePort(portNo, HIGH, LOW);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
  baseShield.setMePort(portNo, LOW, LOW);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
}
void backwardstep() { 
  baseShield.setMePort(portNo, LOW, LOW);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
  baseShield.setMePort(portNo, HIGH, LOW);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
  baseShield.setMePort(portNo, HIGH, HIGH);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
  baseShield.setMePort(portNo, LOW, HIGH);
  delayMicroseconds( MOVE_ITEM_STEPPER_PULSE_LENGTH );
}
