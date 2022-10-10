#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "driver/gpio.h"
#include "esp_smartconfig.h"

#define ledgreen 27
#define ledyellow 26
#define ledred 25

#define red 35
#define green 34
#define yellow 33
#define reset 14

#define ESP_INTR_FLAG_DEFAULT 0


 int zliczeniagreen = 0;
 int zliczeniayellow = 0;
 int zliczeniared = 0;

char ssid[32]="FunBox2-70ASDSDSDSSDSDSDD";
char password[64]="A1ADE61239ED43DD1565932F5SDSDSDSDS";

esp_err_t err;
nvs_handle ssidh;
// Event group
static EventGroupHandle_t s_wifi_event_group;


static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "smartconfig_example";

unsigned char mac_base[6] = {0};

gpio_config_t pin_config = {};

struct states{
 bool gre;
 bool yell;
 bool re;
};

struct states colors;

TaskHandle_t xgreen = NULL;   //tworzymy uchwyt
TaskHandle_t xyellow = NULL;
TaskHandle_t xred = NULL;
TaskHandle_t xreset = NULL;

int level = 0;

void IRAM_ATTR buttongreen_isr_handler(void* arg) {

    // notify the button task
	xTaskResumeFromISR(xgreen);     //dekremenrtacja handlera dla przerwania
}

void IRAM_ATTR buttonyellow_isr_handler(void* arg) {

    // notify the button task
	xTaskResumeFromISR(xyellow);      //dekremenrtacja handlera dla przerwania
}

void IRAM_ATTR buttonred_isr_handler(void* arg) {

    // notify the button task
	xTaskResumeFromISR(xred);      //dekremenrtacja handlera dla przerwania
}

void IRAM_ATTR reset_isr_handler(void* arg) {

    // notify the button task
	xTaskResumeFromISR(xreset);      //dekremenrtacja handlera dla przerwania
}


esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}

void print_mac(const unsigned char *mac) {
	printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void getmacadress(){
    esp_efuse_mac_get_default(mac_base);
    esp_read_mac(mac_base, ESP_MAC_WIFI_STA);
    printf("MAC Address: \n");
    print_mac(mac_base);
    printf("\n\n");

}

void postmessage()
{
                char json[80];
                sprintf(json,"{'Logger':'%02X:%02X:%02X:%02X:%02X:%02X','pinstatus':{'p0':'%d','p1':'%d','p2':'%d'}}",mac_base[0],mac_base[1],mac_base[2],mac_base[3],mac_base[4],mac_base[5],colors.gre,colors.yell,colors.re);

                esp_http_client_config_t config_post = {
                .url = "http://httpbin.org/post",
                .method = HTTP_METHOD_POST,
                .cert_pem = NULL,
                .event_handler = client_event_post_handler};

                esp_http_client_handle_t client = esp_http_client_init(&config_post);

                char  *post_data = json;
                esp_http_client_set_post_field(client, post_data, strlen(post_data));
                esp_http_client_set_header(client, "Content-Type", "application/json");

                esp_http_client_perform(client);
                esp_http_client_cleanup(client);

}

void greentask(void *arg){

    while(1)
    {
    vTaskSuspend(NULL);
    zliczeniagreen++;
    printf("Przycisk zielony wcisniety %d razy.\n",zliczeniagreen);
    colors.gre =!colors.gre;
    gpio_set_level(ledgreen, colors.gre);
    getmacadress();
    postmessage();
    }

}

void yellowtask(void *arg){

    while(1)
    {
    vTaskSuspend(NULL);
    zliczeniayellow++;
    printf("Przycisk zolty wcisniety %d razy.\n",zliczeniayellow);
    colors.yell =!colors.yell;
    gpio_set_level(ledyellow, colors.yell);
    getmacadress();
    postmessage();
    }
}

void redtask(void *arg){

    while(1)
    {
    vTaskSuspend(NULL);
    zliczeniared++;
    printf("Przycisk czerwony wcisniety %d razy.\n",zliczeniared);
    colors.re =!colors.re;
    gpio_set_level(ledred, colors.re);
    getmacadress();
    postmessage();
    }
}

void restart(void *arg){
    while(1)
    {
        vTaskSuspend(NULL);
        nvs_flash_erase();
        printf("Resetuje pamiec i rdzenie...\n");
        esp_restart();
    }
}

// Wifi event handler
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

static void smartconfig_example_task(void * parm);

static void initialise_wifi(void)
{
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
       printf("zaczynamy laczenie\n\n");
     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        printf("poloczony..\n\n");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("rozlaczonyy..\n\n");
        initialise_wifi();
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        printf("\nTestuje wifi ssid: %s password: %s\n\n",ssid,password);


        err = nvs_open("storage",NVS_READWRITE,&ssidh); //odczytywanie loginu
         if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }

         printf("\n NVS SSID i PASSWORD...\n");


      err = nvs_set_str(ssidh, "ssid", (const char*)ssid);
      err = nvs_set_str(ssidh, "password", (const char*)password);

      printf((err != ESP_OK) ? "Failed ssid!\n" : "Done ssid\n");
      printf("Committing updates in NVS ...\n ");
      err = nvs_commit(ssidh);
      printf((err != ESP_OK) ? "Failed ssid save!\n" : "Done ssid save\n");
        nvs_close(ssidh);

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}



static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while(1){
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }

}



// Main task
static void writenvs(){

    size_t required_size;

      printf("\n");             //otwieranie pamieci nvs
      printf("Otwieranie NVS handle...\n");



        printf("\nOdczytywanie stringa z NVS ssid...\n");   //odczytywanie danych
       nvs_get_str(ssidh, "ssid", NULL, &required_size );
       nvs_get_str(ssidh, "ssid", (char *)&ssid, &required_size);
       nvs_get_str(ssidh, "password", NULL, &required_size );
       nvs_get_str(ssidh, "password", (char *)&password, &required_size);

        printf("SSID : %s\n", ssid);
        printf("PSWD: %s\n", password);
}


// Main application
void app_main()
{
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS
	nvs_flash_init();
	// create the event group to handle wifi events
	s_wifi_event_group = xEventGroupCreate();

	// initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	esp_event_loop_create_default();

	// initialize the wifi stack in STAtion mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	 err = nvs_open("storage",NVS_READWRITE,&ssidh); //odczytywanie loginu
         if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }

    writenvs();
	// configure the wifi connection and start the interface
	 wifi_config_t sta_config = {};
    strcpy((char *)sta_config.sta.ssid, ssid);
    strcpy((char *)sta_config.sta.password, password);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
	printf("Connecting to %s\n", ssid);

	// start the main task
    esp_wifi_connect();

    pin_config.pin_bit_mask = GPIO_SEL_26 | GPIO_SEL_27 | GPIO_SEL_25;
    pin_config.mode = GPIO_MODE_OUTPUT;
    pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
    pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    pin_config.intr_type = GPIO_PIN_INTR_DISABLE;
    gpio_config(&pin_config);

    pin_config.pin_bit_mask = GPIO_SEL_34 | GPIO_SEL_33 | GPIO_SEL_35;
    pin_config.mode = GPIO_MODE_INPUT;
    pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
    pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    pin_config.intr_type = GPIO_PIN_INTR_ANYEDGE;
    gpio_config(&pin_config);

     pin_config.pin_bit_mask = GPIO_SEL_14;
    pin_config.mode = GPIO_MODE_INPUT;
    pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
    pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    pin_config.intr_type = GPIO_PIN_INTR_NEGEDGE;
    gpio_config(&pin_config);


    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(green, buttongreen_isr_handler,NULL);
    gpio_isr_handler_add(yellow, buttonyellow_isr_handler,NULL);
    gpio_isr_handler_add(red, buttonred_isr_handler,NULL);
    gpio_isr_handler_add(reset, reset_isr_handler,NULL);

    xTaskCreate(greentask, "greentask",5028,NULL,10,&xgreen);
    xTaskCreate(yellowtask, "yellowtask",5028,NULL,10,&xyellow);
    xTaskCreate(redtask, "redtask",5028,NULL,10,&xred);
    xTaskCreate(restart, "reset",1024,NULL,5,&xreset);


}
