#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

char ssid[] = "Robert";
char password[] = "123456a";
char bufor[100];
char bufor2[100];


void app_main()
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
        }
      ESP_ERROR_CHECK( err );

       nvs_handle ssidh;


       size_t required_size;
       int textsize = 0;

      printf("\n");
      printf("Otwieranie NVS handle...\n");

      err = nvs_open("storage",NVS_READWRITE,&ssidh); //odczytywanie loginu
         if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }
          printf("\nOdczytywanie stringa z NVS ssid...\n");
        err = nvs_get_str(ssidh, "ssid", NULL, &required_size );
        err = nvs_get_str(ssidh, "ssid", (char *)&bufor, &required_size);
        err = nvs_get_str(ssidh, "password", NULL, &required_size );
        err = nvs_get_str(ssidh, "password", (char *)&bufor2, &required_size);

        switch (err) {
        case ESP_OK:
            printf("Done\n\n");
            printf("Buffer ssid = %s\n\n", bufor);
            printf("Buffer password = %s\n\n", bufor2);
        break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            memset(bufor, 0, sizeof(bufor));
            memset(bufor2, 0, sizeof(bufor2));
            break;
        default :
            printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        textsize = strlen(bufor);

        printf("Buffer is %d characters in size\n", textsize);
        if (textsize > 940) {
            printf("Text is getting too large, so restarting the ESP32!\n\n");
            for (int i = 6; i == 0; i--) {
                printf("Restarting in %d seconds...\n", i);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            esp_restart();
        }

      printf("\nDodawanie tekstu do NVS SSID i PASSWORD...\n");
      strncat(bufor,ssid,sizeof(ssid)-1);
      strncat(bufor2,password,sizeof(password)-1);
      err = nvs_set_str(ssidh, "ssid", (const char*)bufor);
      err = nvs_set_str(ssidh, "password", (const char*)bufor2);

      printf((err != ESP_OK) ? "Failed ssid!\n" : "Done ssid\n");
      printf("Committing updates in NVS ...\n ");
      err = nvs_commit(ssidh);
      printf((err != ESP_OK) ? "Failed ssid save!\n" : "Done ssid save\n");
        nvs_close(ssidh);


}


