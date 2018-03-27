#include <ch.h>
#include <hal.h>

#include <chprintf.h>


MUTEX_DECL(led_mtx);    // define and init variable, name = led_mtx

static THD_WORKING_AREA(waBlinkerGreen, 128);
static THD_FUNCTION(BlinkerGreen, arg)
{
  arg = arg;                        // Just to avoid Warning
  while ( true )
  {
    chMtxLock(&led_mtx);            // Lock mutex
    palClearLine( LINE_LED2 );
    for ( int i = 0; i < 4; i++ )
    {
      palToggleLine( LINE_LED1 );
      chThdSleepMilliseconds( 1000 );
    }
    chMtxUnlock(&led_mtx);          // Unlock mutex
  }
}

static THD_WORKING_AREA(waFastBlinkerGreen, 128);
static THD_FUNCTION(FastBlinkerGreen, arg)
{
  arg = arg;                        //Just to avoid Warning
  while ( true )
  {
    chMtxLock(&led_mtx);            // Lock mutex
    palSetLine( LINE_LED2 );
    for ( int i = 0; i < 10; i++ )
    {
      palToggleLine( LINE_LED1 );
      chThdSleepMilliseconds( 250 );
    }
    chMtxUnlock(&led_mtx);          // Unlock mutex
  }
}

int main(void)
{
    chSysInit();
    halInit();

    chThdCreateStatic( waBlinkerGreen, sizeof(waBlinkerGreen), NORMALPRIO, BlinkerGreen, NULL );
    chThdCreateStatic( waFastBlinkerGreen, sizeof(waFastBlinkerGreen), NORMALPRIO, FastBlinkerGreen, NULL );

    while (true)
    {
      chThdSleepSeconds( 1 );
    }
}
