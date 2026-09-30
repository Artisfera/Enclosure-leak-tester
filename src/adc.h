#ifndef ADC_H_
#define ADC_H_

typedef enum {
    ADC_OK, 
    ADC_CH_SETUP_ERR, 
    ADC_READ_ERR
} adc_status;


int adc_init(void);
int adc_read_flowrate(float *l_min);

#endif
