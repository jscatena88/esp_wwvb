#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "time.h"
#include "sys/time.h"
#include "protocol_examples_common.h"

#include "wwvb_utils.h"
#include "dac-cosine.h"

static const char *TAG = "esp_wwvb";

static void obtain_time(void);
static void initialize_sntp(void);
void printTime( time_t time);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

TaskHandle_t ModulateTaskHandle = NULL;
TaskHandle_t DataGenTaskHandle = NULL;

QueueHandle_t amp_enc_time_queue;

void modulateTask(void *params) {

    TickType_t delay_0 = 200/portTICK_PERIOD_MS;
    TickType_t delay_1 = 500/portTICK_PERIOD_MS;
    TickType_t delay_M = 800/portTICK_PERIOD_MS;

    while(1) {
        //read 60 uint8_t from queue
        uint8_t *amp_enc_time = NULL;
        ESP_LOGI(TAG, "Immediatly before recieve");
        xQueueReceive(amp_enc_time_queue, &amp_enc_time, portMAX_DELAY);
        ESP_LOGI(TAG, "Immediatly after");

        for (int i = 0; i < 60; i++) {
            //drop powertime_enc
            dac_scale_set(DAC_CHANNEL_1, 3);
            //delay time based on 2 bits
            TickType_t this_delay;
            if (amp_enc_time[i] == 0)
                this_delay = delay_0;
            else if (amp_enc_time[i] == 1)
                this_delay = delay_1;
            else
                this_delay = delay_M;
            vTaskDelay(this_delay);
            //raise power
            dac_scale_set(DAC_CHANNEL_1, 0);
            //delay until end of second
            vTaskDelay(1000/portTICK_PERIOD_MS - this_delay);
        }
        free(amp_enc_time);
    }
}

void DataGenTask(void *params) {
    while(1) {
        //sync time
        obtain_time();
        for (int i = 0; i<60; i++) {
            //get time
            time_t wwvb_time;
            time(&wwvb_time);
            //add 1 second
            wwvb_time++;
            //gen wwvb encoding
            uint8_t *time_enc = malloc(sizeof(uint8_t) * 60);
            ampEncTime(time_enc, wwvb_time);
            vTaskDelay(500/portTICK_PERIOD_MS);

            //wait for minute tick-over
            ESP_LOGI(TAG, "start wait for minute kick over");
            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            printTime(tv_now.tv_sec);
            struct tm *time_in_struct;
            time_in_struct = gmtime(&tv_now.tv_sec);
            int seconds_delay = 60 - time_in_struct->tm_sec - 1;
            ESP_LOGI(TAG, "tm_sec = %d, seconds delay = %d", time_in_struct->tm_sec, seconds_delay);
            ESP_LOGI(TAG, "going to wait %f seconds", (seconds_delay*1.0 + ((1000 - tv_now.tv_usec/1000)+1)/1000.0));
            TickType_t delay = ((1000 - tv_now.tv_usec/1000)+1 + seconds_delay*1000)/portTICK_PERIOD_MS;
            vTaskDelay(delay);
            ESP_LOGI(TAG, "minute has kicked over");
            gettimeofday(&tv_now, NULL);
            printTime(tv_now.tv_sec);

            //place on queue
            ESP_LOGI(TAG, "pushing encoding onto queue");
            uint8_t *ptr = time_enc;
            xQueueSendToBack(amp_enc_time_queue, &ptr, portMAX_DELAY);
        }
    }
}

void app_main(void)
{
    amp_enc_time_queue = xQueueCreate(10, sizeof(uint8_t*));
    int clk_8m_div = 1;      // RTC 8M clock divider (division is by clk_8m_div+1, i.e. 0 means 8MHz frequency)
    int frequency_step = 931;  // Frequency step for CW generator
    int scale = 0;
    int offset = 0;
    int invert = 2;  
    dac_frequency_set(clk_8m_div, frequency_step);
    dac_scale_set(DAC_CHANNEL_1, scale);
    dac_offset_set(DAC_CHANNEL_1, offset);
    dac_invert_set(DAC_CHANNEL_1, invert);
    dac_cosine_enable(DAC_CHANNEL_1);
    dac_output_enable(DAC_CHANNEL_1);
    xTaskCreate(DataGenTask, "data_gen_task", 10000, NULL, 1, DataGenTaskHandle);
    xTaskCreate(modulateTask, "modulate_task", 10000, NULL, 1, ModulateTaskHandle);
}

static void obtain_time(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_ERROR_CHECK( example_disconnect() );
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}

void printTime( time_t time) {
    struct tm *time_in_struct;
    time_in_struct = gmtime(&time);
    ESP_LOGI(TAG, "Time is %d:%d.%d", time_in_struct->tm_hour, time_in_struct->tm_min, time_in_struct->tm_sec);
}