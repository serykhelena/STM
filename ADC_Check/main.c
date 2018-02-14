#include <ch.h>
#include <hal.h>

#include <chprintf.h>

/*===========================================================================*/
/* Serial driver related.                                                    */
/*===========================================================================*/


static const SerialConfig sdcfg = {
  .speed = 460800,
  .cr1 = 0,
  .cr2 = USART_CR2_LINEN,
  .cr3 = 0
};

/*===========================================================================*/
/* GPT driver related.                                                       */
/*===========================================================================*/

/*
 * GPT4 configuration. This timer is used as trigger for the ADC.
 */
static const GPTConfig gpt4cfg1 = {
  .frequency =  100000,
  .callback  =  NULL,
  .cr2       =  TIM_CR2_MMS_1,  /* MMS = 010 = TRGO on Update Event.        */
  .dier      =  0U
};


/*===========================================================================*/
/* Mailbox code.                                                             */
/*===========================================================================*/

#define MAILBOX_SIZE 50

static mailbox_t test_mb;
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
static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
  (void)adcp;

  chSysLockFromISR();
  chMBPostI( &test_mb, samples1[0]);
  chMBPostI( &test_mb, samples1[1]);
  chSysUnlockFromISR();

}

/*
 * ADC errors callback, should never happen.
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
}

static const ADCConversionGroup adcgrpcfg1 = {
  .circular     = true,
  .num_channels = ADC_GRP1_NUM_CHANNELS,
  .end_cb       = adccallback,
  .error_cb     = adcerrorcallback,
  .cr1          = 0,
  .cr2          = ADC_CR2_EXTEN_RISING | ADC_CR2_EXTSEL_SRC(12),        // Commutated from GPT
  .smpr1        = ADC_SMPR1_SMP_AN10(ADC_SAMPLE_144),
  .smpr2        = ADC_SMPR2_SMP_AN3(ADC_SAMPLE_144),
  .sqr1         = ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),
  .sqr2         = 0,
  .sqr3         = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN3) |
                  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN10)
};


/*===========================================================================*/
/* Application code.                                                         */
/*===========================================================================*/

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED attached to TP1.
 */
static THD_WORKING_AREA(waBlinker, 128);
static THD_FUNCTION(Blinker, arg) {

  (void)arg;
  while (true)
  {
    palToggleLine(LINE_LED1);
    chThdSleepSeconds(1);
  }
}

int main(void)
{
    chSysInit();
    halInit();

    // Serial driver
    sdStart( &SD7, &sdcfg );
    palSetPadMode( GPIOE, 8, PAL_MODE_ALTERNATE(8) );    // TX
    palSetPadMode( GPIOE, 7, PAL_MODE_ALTERNATE(8) );    // RX

    // Mailbox init

    chMBObjectInit(&test_mb, buffer_test_mb, MAILBOX_SIZE);       // For external interrupt



    // GPT driver
    gptStart(&GPTD4, &gpt4cfg1);




#if 1
    /*
     * Fixed an errata on the STM32F7xx, the DAC clock is required for ADC
     * triggering.
     */
    rccEnableDAC1(false);

    // ADC driver
    adcStart(&ADCD1, NULL);
    palSetLineMode( LINE_ADC123_IN10, PAL_MODE_INPUT_ANALOG );
    palSetLineMode( LINE_ADC123_IN3, PAL_MODE_INPUT_ANALOG );

    adcStartConversion(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
    gptStartContinuous(&GPTD4, gpt4cfg1.frequency/10);
#endif
    chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, Blinker, NULL);



    msg_t msg1, msg2;

    while (true)
    {

        if ( chMBFetch(&test_mb, &msg1, TIME_IMMEDIATE) == MSG_OK )
        {
          chMBFetch(&test_mb, &msg2, TIME_IMMEDIATE);
          chprintf(((BaseSequentialStream *)&SD7), "ADC values: %d %d\n", msg1, msg2);

        }

        chThdSleepMilliseconds( 10 );
     }
}
