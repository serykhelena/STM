#include <ch.h>
#include <hal.h>

#include <chprintf.h>

/*===========================================================================*/
/* Serial driver related.                                                    */
/*===========================================================================*/

static const SerialConfig sdcfg = {
  .speed = 460800,
  .cr1 = 0,
  .cr2 = USART_CR2_LINEN,   // because it works only that way ???
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
  .dier      =  0U          // ????
};

/*===========================================================================*/
/* Mailbox code.                                                             */
/*===========================================================================*/

#define MAILBOX_SIZE 50     // why 50?

static mailbox_t test_mb;       // name of mailbox
static msg_t buffer_test_mb[MAILBOX_SIZE];

/*===========================================================================*/
/* ADC driver related.                                                       */
/*===========================================================================*/

#define ADC_GRP1_NUM_CHANNELS   2
#define ADC_GRP1_BUF_DEPTH      1

static adcsample_t samples1[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];

/*
 * ADC streaming callback.
 */

// when ADC conversion ends, this func will be called
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
  .num_channels = ADC_GRP1_NUM_CHANNELS,    // number of channels
  .end_cb       = adccallback,              // after ADC conversion ends - call this func
  .error_cb     = adcerrorcallback,         // in case of errors, this func will be called
  .cr1          = 0,                        // just because it has to be 0
  .cr2          = ADC_CR2_EXTEN_RISING | ADC_CR2_EXTSEL_SRC(12),  // Commutated from GPT
  .smpr1        = ADC_SMPR1_SMP_AN10(ADC_SAMPLE_144),       // for AN10 - 144 samples
  .smpr2        = ADC_SMPR2_SMP_AN3(ADC_SAMPLE_144),        // for AN3  - 144 samples
  .sqr1         = ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),   //
  .sqr2         = 0,
  .sqr3         = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN3) |         // sequence of channels
                  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN10)
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
    chMBObjectInit(&test_mb, buffer_test_mb, MAILBOX_SIZE);  // For external interrupt

    // GPT driver
    gptStart(&GPTD4, &gpt4cfg1);

    /*
     * Fixed an errata on the STM32F7xx, the DAC clock is required for ADC
     * triggering.
     */
    rccEnableDAC1(false);       // ???

    // ADC driver
    adcStart(&ADCD1, NULL);
    palSetLineMode( LINE_ADC123_IN10, PAL_MODE_INPUT_ANALOG );  // PC0
    palSetLineMode( LINE_ADC123_IN3, PAL_MODE_INPUT_ANALOG );   // PA3

    adcStartConversion(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
    gptStartContinuous(&GPTD4, gpt4cfg1.frequency/10);          // how often we need ADC value

    msg_t msg1, msg2;

    while (true)
    {
        if ( chMBFetch(&test_mb, &msg1, TIME_IMMEDIATE) == MSG_OK ) // get the 1st ADC value
        {
          chMBFetch(&test_mb, &msg2, TIME_IMMEDIATE);       // get the 2nd ADC value
          chprintf(((BaseSequentialStream *)&SD7), "ADC values: %d %d\n", msg1, msg2);
        }
        chThdSleepMilliseconds( 10 );
     }
}
