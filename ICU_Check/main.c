#include <ch.h>
#include <hal.h>

#include <chprintf.h>

/*===========================================================================*/
/* Serial driver related.                                                    */
/*===========================================================================*/

// UART settings
static const SerialConfig sdcfg = {
  .speed = 115200,  // unexpected, but it is speed
  .cr1 = 0,
  .cr2 = USART_CR2_LINEN,// because it works only that way
  /* Just to make it more powerful =), RM says that it enables error detection,
     so this should work without this USART_CR2_LINEN */
  .cr3 = 0
};

/*===========================================================================*/
/* Mailbox code.                                                             */
/*===========================================================================*/
#define MAILBOX_SIZE 50             // why 50
/* Just because I wanted to keep no more 50 messages =) */
/* It`s up for you to set the depth of buffer, just keep in mind the speed of posting and fetching as
   it can overflow */
static mailbox_t steer_mb;          // name of mailbox for getting steering pulses
static msg_t b_steer[MAILBOX_SIZE]; // size for steer_mb

static mailbox_t speed_mb;          // name of mailbox for getting speed pulses
static msg_t b_speed[MAILBOX_SIZE]; // size for speed_mb

/*===========================================================================*/
/* ICU driver related.                                                       */
/*===========================================================================*/

// callback for steering pulses
static void icuwidthcb_steer(ICUDriver *icup)
{
  icucnt_t last_width = icuGetWidthX(icup);     // ...X - can work anywhere
                                                // return width in ticks
  chSysLockFromISR();                           // Critical Area
  chMBPostI(&steer_mb, (msg_t)last_width);      // send width
  chSysUnlockFromISR();                         // End of Critical Area
}

// callback for speed pulses
static void icuwidthcb_speed(ICUDriver *icup)
{
  icucnt_t last_width = icuGetWidthX(icup);     // ...X - can work anywhere
                                                // return width in ticks
  chSysLockFromISR();                           // Critical Area
  chMBPostI(&speed_mb, (msg_t)last_width);      // send width
  chSysUnlockFromISR();                         // End of Critical Area
}

// Configuration of Steer ICU
static const ICUConfig icucfg_steer = {
  .mode         = ICU_INPUT_ACTIVE_HIGH,        // Trigger on rising edge
  .frequency    = 100000,                       // do not depend on PWM freq
  .width_cb     = icuwidthcb_steer,             // callback for Steering
  .period_cb    = NULL,
  .overflow_cb  = NULL,
  .channel      = ICU_CHANNEL_1,                // for Timer
  .dier         = 0
};

// Configuration of Speed ICU
static const ICUConfig icucfg_speed = {
  .mode         = ICU_INPUT_ACTIVE_HIGH,        // Trigger on rising edge
  .frequency    = 100000,                       // do not depend on PWM freq
  .width_cb     = icuwidthcb_speed,             // callback for Speed
  .period_cb    = NULL,
  .overflow_cb  = NULL,
  .channel      = ICU_CHANNEL_1,                // for Timer
  .dier         = 0
};

/*===========================================================================*/
/* Application code.                                                         */
/*===========================================================================*/

int main(void)
{
    chSysInit();                // core function init
    halInit();                  // hal init

    // Serial driver
    sdStart( &SD7, &sdcfg );
    palSetPadMode( GPIOE, 8, PAL_MODE_ALTERNATE(8) );    // TX
    palSetPadMode( GPIOE, 7, PAL_MODE_ALTERNATE(8) );    // RX

    // Mailbox init
    chMBObjectInit(&steer_mb, b_steer, MAILBOX_SIZE);
    chMBObjectInit(&speed_mb, b_speed, MAILBOX_SIZE);

    // ICU driver
    icuStart(&ICUD9, &icucfg_steer);                    // Use Timer 9
    palSetPadMode( GPIOE, 5, PAL_MODE_ALTERNATE(3) );   // PE5
    icuStartCapture(&ICUD9);                            // Start Working ICU (Tmer 9)
    icuEnableNotifications(&ICUD9);                     // ??? Turn on notifications O_o

    icuStart(&ICUD8, &icucfg_speed);                    // Use Timer 8
    palSetPadMode( GPIOC, 6, PAL_MODE_ALTERNATE(3) );   // PC6
    icuStartCapture(&ICUD8);                            // Start Working ICU (Timer 8)
    icuEnableNotifications(&ICUD8);                     // ??? Turn on notifications O_o

    msg_t msg;  // only one, because it kind of rewrite each other ???

    while (true)
    {
      if ( chMBFetch(&steer_mb, &msg, TIME_IMMEDIATE) == MSG_OK )
      /* Get first in the mailbox message,
       * TIME_IMMEDIATE means to get the value without timeout
       * if no message in mailbox, chMBFetch returns MSG_TIMEOUT */
        chprintf(((BaseSequentialStream *)&SD7), "Steer      : %d\n", msg);
      /* send string */

      if ( chMBFetch(&speed_mb, &msg, TIME_IMMEDIATE) == MSG_OK )
        chprintf(((BaseSequentialStream *)&SD7), "Speed      : %d\n", msg); /* send string */
      /* BaseSequentialStream is type the just uses serial streaming (so called abstraction),
       * as we use Serial driver that has functions required for BaseSequentialStream,
       * than we can use chprintf */
      chThdSleepMilliseconds( 1 );
    }
}
