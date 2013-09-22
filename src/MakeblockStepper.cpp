#include "MakeblockStepper.h"
#include <Me_BaseShield.h>

/*
 * two-wire constructor.
 * Sets which wires should control the motor.
 */
MakeblockStepper::MakeblockStepper(int number_of_steps, Me_BaseShield baseShield, int portNo )
{
  this->step_number = 0;      // which step the motor is on
  this->speed = 0;        // the motor speed, in revolutions per minute
  this->direction = 0;      // motor direction
  this->last_step_time = 0;    // time stamp in ms of the last step taken
  this->number_of_steps = number_of_steps;    // total number of steps for this motor
  
  // Port on BaseShield for the motor control connection:
  this->portNo = portNo;
    
  // pin_count is used by the stepMotor() method:
  this->pin_count = 2;
  
  this->baseShield = baseShield;
}

/*
  Sets the speed in revs per minute

*/
void MakeblockStepper::setSpeed(long whatSpeed)
{
  this->step_delay = 60L * 1000L / this->number_of_steps / whatSpeed;
}

/*
  Moves the motor steps_to_move steps.  If the number is negative, 
   the motor moves in the reverse direction.
 */
void MakeblockStepper::step(int steps_to_move)
{  
  int steps_left = abs(steps_to_move);  // how many steps to take
  
  // determine direction based on whether steps_to_mode is + or -:
  if (steps_to_move > 0) {this->direction = 1;}
  if (steps_to_move < 0) {this->direction = 0;}
    
    
  // decrement the number of steps, moving one step each time:
  while(steps_left > 0) {
  // move only if the appropriate delay has passed:
  if (millis() - this->last_step_time >= this->step_delay) {
      // get the timeStamp of when you stepped:
      this->last_step_time = millis();
      // increment or decrement the step number,
      // depending on direction:
      if (this->direction == 1) {
        this->step_number++;
        if (this->step_number == this->number_of_steps) {
          this->step_number = 0;
        }
      } 
      else { 
        if (this->step_number == 0) {
          this->step_number = this->number_of_steps;
        }
        this->step_number--;
      }
      // decrement the steps left:
      steps_left--;
      // step the motor to step number 0, 1, 2, or 3:
      stepMotor(this->step_number % 4);
    }
  }
}

/*
 * Moves the motor forward or backwards.
 */
void MakeblockStepper::stepMotor(int thisStep)
{
  if (this->pin_count == 2) {
    switch (thisStep) {
      case 0: /* 01 */
      baseShield.setMePort(portNo, LOW, HIGH);
      break;
      case 1: /* 11 */
      baseShield.setMePort(portNo, HIGH, HIGH);
      break;
      case 2: /* 10 */
      baseShield.setMePort(portNo, HIGH, LOW);
      break;
      case 3: /* 00 */
      baseShield.setMePort(portNo, LOW, LOW);
      break;
    } 
  }
}

/*
  version() returns the version of the library:
*/
int MakeblockStepper::version(void)
{
  return 1;
}

void MakeblockStepper::motorOff(void)
{
      baseShield.setMePort(portNo, LOW, LOW);
}
