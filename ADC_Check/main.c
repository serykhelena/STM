#include <ch.h>
#include <hal.h>

#include <chprintf.h>

/*===========================================================================*/
/* Serial driver related.                                                    */
/*===========================================================================*/

// UART settings
static const SerialConfig sdcfg = {
  .speed = 460800,
  .cr1 = 0,
  .cr2 = USART_CR2_LINEN,   // because it works only that way ???
  /* Just to make it more powerful =), RM says that it enables error detection,
     so this should work without this USART_CR2_LINEN */
  .cr3 = 0
};

/*===========================================================================*/
/* GPT driver related.                                                       */
/*===========================================================================*/

/*
 * GPT4 configuration. This timer is used as trigger for the ADC.
 */
static const GPTConfig gpt4cfg1 = { // Timer 4 is used
  .frequency =  100000,
  .callback  =  NULL,
  .cr2       =  TIM_CR2_MMS_1,  /* MMS = 010 = TRGO on Update Event.        */
  .dier      =  0U          //
  /* .dier field is direct setup of register, we don`t need to set anything here until now */
};

/*===========================================================================*/
/* Mailbox code.                                                             */
/*===========================================================================*/

#define MAILBOX_SIZE 50     // why 50?
/* Just because I wanted to keep no more 50 messages =) */
/* It`s up for you to set the depth of buffer, just keep in mind the speed of posting and fetching as
   it can overflow */

static mailbox_t test_mb;       // name of mailbox
/* Quite right =) */
static msg_t buffer_test_mb[MAILBOX_SIZE];

/*===========================================================================*/
/* ADC driver related.                                                       */
/*===========================================================================*/

#define ADC1_NUM_CHANNELS   2
#define ADC1_BUF_DEPTH      1

static adcsample_t samples1[ADC1_NUM_CHANNELS * ADC1_BUF_DEPTH];

/*
 * ADC streaming callback.
 */

// when ADC conversion ends, this func will be called
/* if the depth is equal to 1, another way it will be called twice per conversion!
   conversion is sampling of all your cnahhel N times (N is buffer depth) */
static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
  (void)adcp;

  chSysLockFromISR();                   // Critical Area
  chMBPostI( &test_mb, samples1[0]);    // send 1st ADC-value (channel 1)
  chMBPostI( &test_mb, samples1[1]);    // send 2nd ADC-value (channel 2)
  chSysUnlockFromISR();                 // Close Critical Area

}

/*
 * ADC errors callback, should never happen.
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
}

static const ADCConversionGroup adcgrpcfg1 = {
  .circular     = true,                     // working mode = looped
  /* Buffer will continue writing to the beginning when it come to the end */
  .num_channels = ADC1_NUM_CHANNELS,    // number of channels
  .end_cb       = adccallback,              // after ADC conversion ends - call this func
  /* Don`t forget about depth of buffer */
  .error_cb     = adcerrorcallback,         // in case of errors, this func will be called
  .cr1          = 0,                        // just because it has to be 0
  /* Cause we don`t need to write something to the register */
  .cr2          = ADC_CR2_EXTEN_RISING | ADC_CR2_EXTSEL_SRC(12),  // Commutated from GPT
  /* 12 means 0b1100, and from RM (p.449) it is GPT4 */
  /* ADC_CR2_EXTEN_RISING - means to react on the rising signal (front) */
  .smpr1        = ADC_SMPR1_SMP_AN10(ADC_SAMPLE_144),       // for AN10 - 144 samples
  .smpr2        = ADC_SMPR2_SMP_AN3(ADC_SAMPLE_144),        // for AN3  - 144 samples
  .sqr1         = ADC_SQR1_NUM_CH(ADC1_NUM_CHANNELS),   //
  /* Usually this field is set to 0 as config already know the number of channels (.num_channels) */
  .sqr2         = 0,
  .sqr3         = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN3) |         // sequence of channels
                  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN10)
  /* If we can macro ADC_SQR2_SQ... we need to write to .sqr2 */
};

/*===========================================================================*/
/* Application code.                                                         */
/*===========================================================================*/

int main(void)
{
    chSysInit();            // core function init
    halInit();              // hal init

    // Serial driver
    sdStart( &SD7, &sdcfg );
    palSetPadMode( GPIOE, 8, PAL_MODE_ALTERNATE(8) );    // TX
    palSetPadMode( GPIOE, 7, PAL_MODE_ALTERNATE(8) );    // RX

    // Mailbox init
    chMBObjectInit(&test_mb, buffer_test_mb, MAILBOX_SIZE);

    // GPT driver
    gptStart(&GPTD4, &gpt4cfg1);

    /*
     * Fixed an errata on the STM32F7xx, the DAC clock is required for ADC
     * triggering.
     */
    rccEnableDAC1(false);
    /* This enables DAC clock that is required for ADC triggering,
     * it`s errata feature (not bug but not so pleased thing) =) */

    // ADC driver
    adcStart(&ADCD1, NULL);
    palSetLineMode( LINE_ADC123_IN10, PAL_MODE_INPUT_ANALOG );  // PC0
    palSetLineMode( LINE_ADC123_IN3, PAL_MODE_INPUT_ANALOG );   // PA3

    adcStartConversion(&ADCD1, &adcgrpcfg1, samples1, ADC1_BUF_DEPTH);
    gptStartContinuous(&GPTD4, gpt4cfg1.frequency/1000);          // how often we need ADC value
    /* Just set the limit (interval) of timer counter, you can use this function
       not only for ADC triggering, but start infinite counting of timer for callback processing */

    msg_t msg1, msg2;

    while (true)
    {
        if ( chMBFetch(&test_mb, &msg1, TIME_IMMEDIATE) == MSG_OK ) // get the 1st ADC value
          /* Get first in the mailbox message, TIME_IMMEDIATE means to get the value without timeout 
             if no message in mailbox, chMBFetch returns MSG_TIMEOUT */
        {
          chMBFetch(&test_mb, &msg2, TIME_IMMEDIATE);       // get the 2nd ADC value
          chprintf(((BaseSequentialStream *)&SD7), "ADC values: %d %d\n", msg1, msg2);
          /* BaseSequentialStream is type the just uses serial streaming (so called abstraction),
             as we use Serial driver that has functions required for BaseSequentialStream, than we can use 
             chprintf */
        }
        chThdSleepMilliseconds( 1 );
     }
}
