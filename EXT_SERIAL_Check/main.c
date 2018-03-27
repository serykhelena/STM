#include <ch.h>
#include <hal.h>

#include <chprintf.h>

/*===========================================================================*/
/* Serial driver related.                                                    */
/*===========================================================================*/

// UART settings
static const SerialConfig sdcfg = {
  .speed = 460800,  // unexpected, but it is speed
  .cr1 = 0,
  .cr2 = USART_CR2_LINEN,// because it works only that way
  /* Just to make it more powerful =), RM says that it enables error detection,
     so this should work without this USART_CR2_LINEN */
  .cr3 = 0
};

/*===========================================================================*/
/* Mailbox code.                                                             */
/*===========================================================================*/

#define MAILBOX_SIZE 50/* Just because I wanted to keep no more 50 messages =) */
/* It`s up for you to set the depth of buffer,
 * just keep in mind the speed of posting and
 * fetching as it can overflow */

static mailbox_t hall_1;        //
static msg_t b_hall_1[MAILBOX_SIZE];

static mailbox_t hall_2;
static msg_t b_hall_2[MAILBOX_SIZE];

static mailbox_t hall_3;
static msg_t b_hall_3[MAILBOX_SIZE];

/*===========================================================================*/
/* EXT driver related.                                                       */
/*===========================================================================*/

// Callback for EXT
static void extcb1(EXTDriver *extp, expchannel_t channel) {

 (void)extp;
 (void)channel;

 msg_t resHall = 0;

 if(palReadPad( GPIOD , 3 ))
   resHall |= 0b001;
 if(palReadPad( GPIOD , 4 ))
    resHall |= 0b010;
 if(palReadPad( GPIOD , 5 ))
    resHall |= 0b100;

  /*
  if ( palReadPad( GPIOC, 0 ) == PAL_HIGH )
  {
    front_time = gptGetCounterX( &GPTD4 );
  } else {
    edge_time  = gptGetCounterX( &GPTD4 );

    gptcnt_t result = edge_time < front_time ? edge_time + (UINT16_MAX - front_time) :
                                               edge_time - front_time;
*/
    chSysLockFromISR();
    chMBPostI(&hall_1, (msg_t)resHall);/*
    chMBPostI(&hall_2, (msg_t)hall2);
    chMBPostI(&hall_3, (msg_t)hall3);*/
    //chMBPostI(&hall_1, (msg_t)counter);
    chSysUnlockFromISR();
  }

static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOD , extcb1}, // PD3
    {EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOD , extcb1}, // PD4
    {EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOD , extcb1}, // PD5
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
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};

/*===========================================================================*/
/* Application code.                                                         */
/*===========================================================================*/

int main(void)
{
    chSysInit();
    halInit();

    // Serial driver
    sdStart( &SD7, &sdcfg );
    palSetPadMode( GPIOE, 8, PAL_MODE_ALTERNATE(8) );    // TX
    palSetPadMode( GPIOE, 7, PAL_MODE_ALTERNATE(8) );    // RX

    // Mailbox init
    chMBObjectInit(&hall_1, b_hall_1, MAILBOX_SIZE); // For external interrupt

    // EXT driver
    extStart( &EXTD1, &extcfg );

    msg_t msg;

    while (true)
    {
      if ( chMBFetch(&hall_1, &msg, TIME_IMMEDIATE) == MSG_OK )
        chprintf(((BaseSequentialStream *)&SD7), "Hall: %d \n \r ", msg);
/*
      if ( chMBFetch(&hall_2, &msg, TIME_IMMEDIATE) == MSG_OK )
              chprintf(((BaseSequentialStream *)&SD7), "Hall 2: %d ", msg);

      if ( chMBFetch(&hall_3, &msg, TIME_IMMEDIATE) == MSG_OK )
              chprintf(((BaseSequentialStream *)&SD7), "Hall 3: %d \n \r", msg);
              */
      chThdSleepMilliseconds( 1 );
    }
}
