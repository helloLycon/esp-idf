#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "uart_echo_example_main.h"



#define TRANSMIT_METHOD  "transmit_method"


TransmitMethod  method;


void ble_app_main_init();
void tcpserver_app_main_init();
void uart_app_main();


esp_err_t  write_transmit_method(TransmitMethod method) {
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_ERR_NVS_BASE;
    }
    
    err = nvs_set_i32(my_handle, TRANSMIT_METHOD, (int32_t)method);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return ESP_ERR_NVS_BASE;
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return ESP_ERR_NVS_BASE;
    }
    nvs_close(my_handle);
    return ESP_OK;
}


void app_main() {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }
    err = nvs_get_i32(my_handle, TRANSMIT_METHOD, (int32_t *)&method);
    nvs_close(my_handle);
    if(err == ESP_OK && method == TRANSMIT_WIFI) {
        tcpserver_app_main_init();
    } else {
        ble_app_main_init();
    }
    uart_app_main();
}



