// Ensure this library description is only included once
#ifndef MakeblockStepper_h
#define MakeblockStepper_h

#include <Me_BaseShield.h>

// library interface description
class MakeblockStepper {
public:
  // constructors:
  MakeblockStepper(int number_of_steps, Me_BaseShield baseShield, int portNo);
  // Stepper(int number_of_steps, int motor_pin_1, int motor_pin_2, int motor_pin_3, int motor_pin_4);

  // speed setter method:
  void setSpeed(long whatSpeed);

  // mover method:
  void step(int number_of_steps);

  // turn motor off
  void motorOff();

  int version(void);

private:
  Me_BaseShield baseShield;
  int portNo;

  void stepMotor(int this_step);

  int direction;        // Direction of rotation
  int speed;          // Speed in RPMs
  unsigned long step_delay;    // delay between steps, in ms, based on speed
  int number_of_steps;      // total number of steps this motor can take
  int pin_count;        // whether you're driving the motor with 2 or 4 pins
  int step_number;        // which step the motor is on

  long last_step_time;      // time stamp in ms of when the last step was taken
};

#endif
