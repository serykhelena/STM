#include "ch.h"
#include "hal.h"

/*
GPTConfig gpt4conf = {
    .frequency    = 10000,
    .callback     = NULL,
    .cr2          = 0,
    .dier         = 0
};
*/

PWMConfig pwm4conf = {
    .frequency = 4411764, // частота тиков таймера
    .period    = 60000, /*1/100 s = 10 ms*/  // кол-во тиков  на 1 период шим этого таймера (с исп. frequency)
    .callback  = NULL, // PWM period = period/frequency [s]
    .channels  = {     // в конце каждого периода ШИМ (callback)
                  {.mode = PWM_OUTPUT_ACTIVE_HIGH,  .callback = NULL},      // PD12 => Driving wheels
                  {.mode = PWM_OUTPUT_ACTIVE_HIGH,     .callback = NULL},   // PD13 => Steering wheels
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

  //GPTDriver *delayDriver    = &GPTD4;
  PWMDriver *pwmDriver      = &PWMD4;

  palSetLineMode( LINE_QSPI_BK1_IO1, PAL_MODE_ALTERNATE(2) );   // PD12
  palSetLineMode(  LINE_ZIO_D28, PAL_MODE_ALTERNATE(2) );       // PD13


  pwmStart( pwmDriver, &pwm4conf );

  //gptStart( delayDriver, &gpt4conf );

  pwmEnableChannel( pwmDriver, 0, 30000 );      // Driving wheels
  pwmEnableChannel( pwmDriver, 1, 30000 );      // Steering wheels

  while (true)
  {
    //palToggleLine( LINE_LED1 );
    //
    //pwmEnableChannel( pwmDriver, 2, 10000 );
    //gptPolledDelay( delayDriver, 10000 );
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
