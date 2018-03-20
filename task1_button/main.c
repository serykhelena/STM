#include <ch.h>
#include <hal.h>


/*===========================================================================*/
/* EXT driver related.                                                       */
/*===========================================================================*/

static thread_reference_t trp_button = NULL;

// Callback for EXT
static void extcb1(EXTDriver *extp, expchannel_t channel) {

 extp = extp;
 channel = channel;

 chSysLockFromISR();                   // ISR Critical Area
 chThdResumeI( &trp_button, MSG_OK );
 chSysUnlockFromISR();                 // Close ISR Critical Area

}

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOC, extcb1}, //PC13 = Button
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};

/*===========================================================================*/
/* Application code.                                                         */
/*===========================================================================*/

uint8_t mode = 0;                       //  1 => enable, 2 = > disable
uint8_t red_mode = 0;                   //  3 => red toggle, another value => turn off
uint8_t counter = 0;                    // counter of button pressed
static THD_WORKING_AREA(waButton, 128); // 128 - stack size
static THD_FUNCTION(Button, arg)
{
  arg = arg;                            // just to avoid warnings

  msg_t msg_button;                     // to detect button press
  msg_t double_button;                  // to detect double button press

  while( true )
  {
    /* Waiting for the IRQ to happen */
    chSysLock();
    msg_button = chThdSuspendTimeoutS(&trp_button, MS2ST(5000));
    double_button = chThdSuspendTimeoutS(&trp_button, MS2ST(1000));
    chSysUnlock();

    if( msg_button == MSG_OK )
    {
      chThdSleepMicroseconds( 50 );
      if( palReadLine(LINE_BUTTON) == 0 )
      {
        counter += 1;
        palSetLine( LINE_LED1 );
        mode = 1;                       // enable mode
        if( double_button == MSG_OK )
        {
          if( counter == 2 )            // double press
          {
            red_mode = 3;               // red toggle
          }
        }
        if( double_button == MSG_TIMEOUT )
        {
          counter = 0;                  // reset counter of button press
          red_mode = 0;                 // turn off RED LED
        }
      }
    }
    if( msg_button == MSG_TIMEOUT )
    {

      palClearLine( LINE_LED1 );
      mode = 2;                         // disable mode
      red_mode = 0;                     // turn off RED LED
      counter = 0;                      // reset counter of button press
    }
  }
}

static THD_WORKING_AREA(waBlinker, 128);        // just for checking
static THD_FUNCTION(Blinker, arg)
{
    (void)arg;
    while (true)
    {
      if( mode == 1 )                            // enable mode
      {
        palToggleLine( LINE_LED2 );
        chThdSleepSeconds( 1 );                   // freq = 1 Hz
      }
      else if( mode == 2 )                       // disable mode
      {
        palToggleLine( LINE_LED2 );
        chThdSleepMilliseconds( 250 );            // freq = 4 Hz
      }
      else
      {
        palSetLine( LINE_LED2 );
        chThdSleepMilliseconds( 1 );            // to avoid hanging of prog
      }
    }
}

static THD_WORKING_AREA(waRedWarning, 128);        // just for checking
static THD_FUNCTION(RedWarning, arg)
{
    (void)arg;
    while (true)
    {
      if( red_mode == 3 )
      {
        palToggleLine( LINE_LED3 );
        chThdSleepMilliseconds( 100 );
      }
      else
      {
        palClearLine( LINE_LED3 );
        chThdSleepMilliseconds( 1 );                // to avoid hanging of prog
      }
    }
}

int main(void)
{
    chSysInit();
    halInit();

    // EXT driver
    extStart( &EXTD1, &extcfg );
    chThdCreateStatic(waButton, sizeof(waButton), NORMALPRIO, Button, NULL);
    chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO - 1, Blinker, NULL);
    chThdCreateStatic(waRedWarning, sizeof(waRedWarning), NORMALPRIO - 1, RedWarning, NULL);
    while (true)
    {
      chThdSleepMilliseconds( 10 );
    }
}
