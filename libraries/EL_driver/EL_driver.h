/*
 * Uses PWM to drive transistor
 *
 * Fit a PNP transistor (6W min) with base to pin 3 (PWM control) and collector/emittor to the GND of an EL driver
 *
 */

#ifndef EL_driver_h
#define EL_driver_h

#include <math.h>
#include <Arduino.h>

#define MINPWM 1    // don't go less than this.  looks like 20.
#define MAXPWM 255  // don't go more than this.  some lines get flickery after this level.  looks like 200.
#define MINSCALE 0  // sensor scale min. set_min_intensity() overrides.
#define MAXSCALE 100 // sensor scale max. set_max_intensity() overrides.
#define RAMPTIME 60000  // default ms to run from MINSCALE to MAXSCALE.  set_ramp_time() overrides.
#define ELCURVE -5  // more negative values skew the procession to lower duty cycles.
#define PAUSE_BETWEEN 100  // ms delay after each light update

#define NEL 6	// six EL lines

class EL_class
{
public:
  EL_class(int ctlPin);

  // several options for changing the light levels.
  // 1) change light levels slowly, using prorated ramp_time to shift.  Blocking.
  void ramp_light(int intensity);
  // 2) write to the line directly. not blocking.
  void set_light(int intensity);
  // 3) update the lines over time.  not blocking.
  void update_light();

  // set the ramp time, or the amount of time to cycle from MINSCALE to MAXSCALE or vice-versa
  void set_ramp_time(long ramp_time);
  // set the maximum intensity from MINSCALE to MAXSCALE
  void set_max_intensity(int max_intensity);
  // set the minimum intensity from MINSCALE to MAXSCALE
  void set_min_intensity(int min_intensity);
  // set the direction update_light is going.  +1 is brighter, -1 is darker.
  void set_direction(int direction);

  void set_min_pwm(int minVal);
  void set_max_pwm(int maxVal);

  void set_el_curve(int val);
  int get_intensity();

private:
  long ramp_time;
  long ramp_interval;
  int min_pwm;
  int max_pwm;
  int el_curve;
  unsigned long last_update;

  int direction;

  int pin;

  int max_intensity;
  int intensity;
  int min_intensity;

  float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve);

};


extern EL_class EL[NEL];

#endif




