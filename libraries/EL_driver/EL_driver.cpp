/*
 * Uses PWM to drive transistor
 *
 * Fit a PNP transistor (6W min) with base to pin 3 (PWM control) and collector/emittor to the GND of an EL driver
 *
 */

#include "EL_driver.h"

EL_class::EL_class( int ctlPin ) {

  this->pin = ctlPin;

  // can't use set_min_intensity() and set_min_intensity yet.
  this->min_intensity = MINSCALE;  // default
  this->max_intensity = MAXSCALE;  // default
  this->min_pwm = MINPWM;
  this->max_pwm = MAXPWM;
  this->el_curve = ELCURVE;
  this->direction = 1;  // default

  // now it's safe
  this->set_ramp_time( RAMPTIME );  // default

  // set default off state
  this->set_light( 0 );  // default
}

void EL_class::set_direction( int direction ) {
  if( direction >= 0) this->direction=1;
  else this->direction=-1;
}

void EL_class::set_el_curve( int val ) {
  this->el_curve = val;
  Serial.print("el curve:");
  Serial.println(this->el_curve);
}

void EL_class::ramp_light( int intensity ) {
  int steps = abs(intensity - this->intensity);
  int pause_time = this->ramp_time / (this->max_intensity - this->min_intensity);

  int step = (intensity - this->intensity) / steps;

  for( int i=0;i<steps;i++) {
    this->set_light(this->intensity+step);
    delay(pause_time);
  }
}

void EL_class::set_ramp_time( long ramp_time ) {

  this->ramp_time = ramp_time;

  // how long is the ramp?
  int dI = this->max_intensity - this->min_intensity;

  // number of ms between each step.
  this->ramp_interval = ramp_time / dI;

//  Serial.print("Ramp interval now:");
//  Serial.println(this->ramp_interval);

}

void EL_class::set_min_intensity( int intensity ) {
  this->min_intensity = min(intensity,this->max_intensity-1);

  // need to adjust the ramp_interval
  this->set_ramp_time( this->ramp_time );
}

void EL_class::set_max_intensity( int intensity ) {
  this->max_intensity = max(intensity,this->min_intensity+1);

  // need to adjust the ramp_interval
  this->set_ramp_time( this->ramp_time );
}

void EL_class::set_min_pwm( int minVal ) {
  this->min_pwm = minVal;
}

void EL_class::set_max_pwm( int maxVal ) {
  this->max_pwm = maxVal;
}

void EL_class::update_light( ) {
  if (millis() - this->last_update >= this->ramp_interval + PAUSE_BETWEEN) {

    // don't make a big jump, even if we've been waiting multiple ramp_interval's, since that just looks ugly.
    this->set_light(this->intensity + this->direction);

    if( this->intensity >= this->max_intensity || this->intensity <= this->min_intensity )
      this->direction *= -1;  // swap directions

  }
  // else no need to do anything.
}

int EL_class::get_intensity() {
	return this->intensity;
}

void EL_class::set_light( int level ) {
/*
  Serial.print("Pin: ");
   Serial.print(this->pin);
   Serial.print(" set to: ");
   Serial.print(level);
   Serial.print(" -> ");
*/

  if( level==this->min_intensity ) {
    digitalWrite(this->pin, LOW);
//        Serial.println("LOW");
  }
  else if( level==this->max_intensity) {
    digitalWrite(this->pin, HIGH);
//        Serial.println("HIGH");
  }
  else {
    // power-function scaling from intensity->PWM duty.
    int alevel = int(this->fscale(this->min_intensity+1,this->max_intensity-1,min_pwm,max_pwm,level,el_curve));

    analogWrite(this->pin,alevel);
//        Serial.println(alevel);
  }

  //  Serial.println("Updating intensity.");
  this->intensity = level;
  this->last_update = millis();

  //  Serial.println("Exiting set_light().");
}

/* fscale
 Floating Point Autoscale Function V0.1
 Paul Badger 2007
 Modified from code by Greg Shakar
*/
float EL_class::fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  /*
   Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution
   Serial.println();
   */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  /*
  Serial.print(OriginalRange, DEC);
   Serial.print("   ");
   Serial.print(NewRange, DEC);
   Serial.print("   ");
   Serial.println(zeroRefCurVal, DEC);
   Serial.println();
   */

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}




