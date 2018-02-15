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
/* PWM Config.                                                       */
/*===========================================================================*/

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
                  {.mode = PWM_OUTPUT_ACTIVE_HIGH,  .callback = NULL},   // PD12 => Driving wheels
                  {.mode = PWM_OUTPUT_ACTIVE_HIGH,  .callback = NULL},   // PD13 => Steering wheels
                  {.mode = PWM_OUTPUT_DISABLED,     .callback = NULL},
                  {.mode = PWM_OUTPUT_DISABLED,     .callback = NULL}
                  },
    .cr2        = 0,
    .dier       = 0
};

// Delay in sec = tick/freq

/*===========================================================================*/
/* Application code.                                                         */
/*===========================================================================*/
/* Can I use define like this ? to calculate constant value ???????????????????
#define icu_freq_s 1/icucfg_steer.frequency */

#define icu_freq_s 0.00001


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

    // PWM init
    PWMDriver *pwmDriver      = &PWMD4;                 // Use Timer 4 for PWM
    palSetLineMode( LINE_QSPI_BK1_IO1, PAL_MODE_ALTERNATE(2) );   // PD12 (Steering wheels)
    palSetLineMode(  LINE_ZIO_D28, PAL_MODE_ALTERNATE(2) );       // PD13
    pwmStart( pwmDriver, &pwm4conf );                   // Start PWM

    msg_t msg;  // only one, because it kind of rewrite each other ???
    int32_t msg_s, width_steer, width_speed;
    pwmEnableChannel( pwmDriver, 1, 6070 );
    //pwmEnableChannel( pwmDriver, 0, 6070 );
    while (true)
    {

      if ( chMBFetch(&steer_mb, &msg, TIME_IMMEDIATE) == MSG_OK )
      {
      /* Get first in the mailbox message,
       * TIME_IMMEDIATE means to get the value without timeout
       * if no message in mailbox, chMBFetch returns MSG_TIMEOUT */
        //msg_s = msg * icu_freq_s;
        width_steer = msg * 40;
        pwmEnableChannel( pwmDriver, 1, width_steer );      // PD13 = Steering wheels
        chprintf(((BaseSequentialStream *)&SD7), "Steer      : %d \n", width_steer);
      /* send string */
      }

      if ( chMBFetch(&speed_mb, &msg, TIME_IMMEDIATE) == MSG_OK )
      {
        //msg_s = msg * 0.00001;
        width_speed = msg * 40;
        pwmEnableChannel( pwmDriver, 0, width_speed );      // PD12 = Driving wheels
        chprintf(((BaseSequentialStream *)&SD7), "Speed      : %d \n", width_speed); /* send string */
        /* BaseSequentialStream is type the just uses serial streaming (so called abstraction),
         * as we use Serial driver that has functions required for BaseSequentialStream,
         * than we can use chprintf */
      }
      chThdSleepMilliseconds( 1 );
    }
}
