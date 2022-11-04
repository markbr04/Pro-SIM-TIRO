/* IR protocols example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "ir_tools.h"

static const char *TAG = "example";

static rmt_channel_t tx_channel = 0; //CONFIG_EXAMPLE_RMT_TX_CHANNEL;

#define RMT_TX_GPIO 18

#define RMT_CONFIG_TX(RMT_TX_GPIO, tx_channel)      \
    {                                                \
        .rmt_mode = RMT_MODE_TX,                     \
        .channel = tx_channel,                       \
        .gpio_num = RMT_TX_GPIO,                            \
        .clk_div = 80,                               \
        .mem_block_num = 1,                          \
        .flags = 0,                                  \
        .tx_config = {                               \
            .carrier_freq_hz = 38000,                \
            .carrier_level = RMT_CARRIER_LEVEL_HIGH, \
            .idle_level = RMT_IDLE_LEVEL_LOW,        \
            .carrier_duty_percent = 33,              \
            .carrier_en = false,                     \
            .loop_en = false,                        \
            .idle_output_en = true,                  \
        }                                            \
    }

#define IR_BUILDER(dev) \
    {                                  \
        .buffer_size = 64,             \
        .dev_hdl = dev,                \
        .flags = 0,                    \
    }


/**
 * @brief RMT Transmit Task
 *
 */
static void ir_tx_task(void *arg)
{
    uint32_t addr = 0x10;
    uint32_t cmd = 0xba45;
    rmt_item32_t *items = NULL;
    size_t length = 0;
    ir_builder_t *ir_builder = NULL;

    rmt_config_t rmt_tx_config = RMT_CONFIG_TX (RMT_TX_GPIO, tx_channel);
    rmt_tx_config.tx_config.carrier_en = true;
    rmt_config(&rmt_tx_config);
    rmt_driver_install(tx_channel, 0, 0);
    ir_builder_config_t ir_builder_config = IR_BUILDER((ir_dev_t)tx_channel);
    ir_builder_config.flags |= IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5 have extended version)
    ir_builder = ir_builder_rmt_new_nec(&ir_builder_config);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        ESP_LOGI(TAG, "Send command 0x%x to address 0x%x", cmd, addr);
        // Send new key code
        ESP_ERROR_CHECK(ir_builder->build_frame(ir_builder, addr, cmd));
        ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
        //To send data according to the waveform items.
        rmt_write_items(tx_channel, items, length, false);
        // Send repeat code
        //vTaskDelay(pdMS_TO_TICKS(ir_builder->repeat_period_ms));
        //ESP_ERROR_CHECK(ir_builder->build_repeat_frame(ir_builder));
        //ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
        //rmt_write_items(tx_channel, items, length, false);
        cmd++;
    }
    ir_builder->del(ir_builder);
    rmt_driver_uninstall(tx_channel);
    vTaskDelete(NULL);
}

void app_main(void)
{

    xTaskCreate(ir_tx_task, "ir_tx_task", 2048, NULL, 10, NULL);
}
