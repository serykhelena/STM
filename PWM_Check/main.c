#include "ch.h"
#include "hal.h"


GPTConfig gpt3conf = {
    .frequency    = 10000,
    .callback     = NULL,
    .cr2          = 0,
    .dier         = 0
};

/*
 * PWM-Timer4
 * frequency = 4 000 000
 * period = 54 400 (13.6 ms)
 *
 * PD12 => Steering wheels (Channel 1)
 *
 *        |  Clockwise  |  Center  | Counterclockwise
 * -------------------------------------------------
 * t, ms  |     1.27    |   1.52   |      1.79
 * -------------------------------------------------
 * width  |     5080    |   6070   |      7160
 * -------------------------------------------------
 *        |on the right |  Center  |   On the left
 *
 * PD13 => Driving wheels (Channel 2)
 *
 *        | Backward  |  Center  | Forward
 *-------------------------------------------
 * t, ms  |    1.05   |   1.51   |   1.92
 * ------------------------------------------
 * width  |    4230   |   6070   |   7700
 *
 */


PWMConfig pwm4conf = {
    .frequency = 4000000,   // frequency of timer ticks
    .period    = 54400,     /* 1/100 s = 10 ms
                             * tick number for 1 PWM period of this timer
                             * PWM period = period/frequency [s] */
    .callback  = NULL,      // in the end of each PWM period (callback)
    .channels  = {
                  {.mode = PWM_OUTPUT_ACTIVE_HIGH,  .callback = NULL},      // PD12 => Driving wheels
                  {.mode = PWM_OUTPUT_ACTIVE_HIGH,  .callback = NULL},   // PD13 => Steering wheels
                  {.mode = PWM_OUTPUT_DISABLED,     .callback = NULL},
                  {.mode = PWM_OUTPUT_DISABLED,     .callback = NULL}
                  },
    .cr2        = 0,
    .dier       = 0
};

// Delay in sec = tick/freq

int main(void)
{
  halInit();

  GPTDriver *delayDriver    = &GPTD3;
  PWMDriver *pwmDriver      = &PWMD4;

  palSetLineMode( LINE_QSPI_BK1_IO1, PAL_MODE_ALTERNATE(2) );   // PD12
  palSetLineMode(  LINE_ZIO_D28, PAL_MODE_ALTERNATE(2) );       // PD13


  pwmStart( pwmDriver, &pwm4conf );

  gptStart( delayDriver, &gpt3conf );

  pwmEnableChannel( pwmDriver, 0, 6500 );      // Driving wheels
  pwmEnableChannel( pwmDriver, 1, 5080 );      // Steering wheels

  while (true)
  {

    //palToggleLine( LINE_LED1 );
    //
    //pwmEnableChannel( pwmDriver, 2, 10000 );

    //pwmEnableChannel( pwmDriver, 2, 5000 );
    //gptPolledDelay( delayDriver, 10000 );
    //pwmEnableChannel( pwmDriver, 2, 2500 );
    //gptPolledDelay( delayDriver, 10000 );
    //pwmEnableChannel( pwmDriver, 2, 1000 );
    //gptPolledDelay( delayDriver, 10000 );
  }
}


//testhal = example <- chibios176
/*
int main(void)
{
  halInit();

  gptStart( &GPTD3, &gpt2conf );


  //palSetLine( PAL_LINE(GPIO, 5) );
  //palSetPad( GPIOB, 0 );

  while (true)
  {
    palToggleLine( LINE_LED1 ); // switch 1 -> 0, 0 -> 1

    //palSetLine( LINE_LED1 );
    gptPolledDelay( &GPTD4, 10000 );
    //palClearLine( LINE_LED1 );
    //gptPolledDelay( &GPTD3, 10000 );
  }
}
*/
