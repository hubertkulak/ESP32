// Receive analog input

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"



TaskHandle_t waterlevel = NULL;

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void water(void *arg)
{
    int voltage;
    int *ten=&voltage;
    const char msg[]="POZIOM ZAWILGOCENIA GLEBY:";
   for(;;){
     
    char bufor[34];
        adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, ten);
        voltage=map(voltage,4095,1900,0,100);
        sprintf(bufor,"%s %d",msg, voltage);
        printf("%s %%\n", bufor);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
   }
}

void app_main(void)
{

    adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_6);
    
    xTaskCreate(water, "INPUT LEVEL",2048,NULL,2,&waterlevel);
}