/*
 * Copyright (c) 2024 TBog.rocks
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT tbog_lock_indicator

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid.h>

struct tbog_lock_indicator_data {
    const struct gpio_dt_spec led_gpio;
    uint8_t indicator_mask;
};

#define LOCK_INDICATOR_DEFINE(inst)                                                           \
    static struct tbog_lock_indicator_data tbog_lock_indicator_data_##inst = {                     \
        .led_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, gpios, {0}),                                    \
        .indicator_mask = DT_INST_PROP_OR(inst, indicator_mask, HID_KBD_LED_CAPS_LOCK),            \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(inst, tbog_lock_indicator_init, NULL, &tbog_lock_indicator_data_##inst,  \
                        NULL, POST_KERNEL,                                                      \
                        CONFIG_APPLICATION_INIT_PRIORITY, NULL);

DT_INST_FOREACH_CHILD(0, LOCK_INDICATOR_DEFINE)

#define LOCK_INDICATOR_DATA_REF_AND_COMMA(n) \
    &tbog_lock_indicator_data_##n,

static struct tbog_lock_indicator_data *lock_indicator_instances[] = {
    DT_INST_FOREACH_CHILD(0, LOCK_INDICATOR_DATA_REF_AND_COMMA)
};

#define LOCK_INDICATOR_INSTANCE_COUNT ARRAY_SIZE(lock_indicator_instances)

//  static void tbog_lock_indicator_handler(const struct zmk_event_t *eh) {
//      if (is_zmk_hid_indicators_changed(eh)) {
//          const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);

//          DT_INST_FOREACH_CHILD(0, tbog_lock_indicator, tbog_lock_indicator_instance) {
//              const struct device *dev = DEVICE_DT_GET(DT_CHILD(DT_INST(0, tbog_lock_indicator), tbog_lock_indicator_instance));
//              struct tbog_lock_indicator_data *data = dev->data;

//              bool new_led_state = (ev->indicators & data->indicator_mask) != 0;

//              if (new_led_state != data->led_state) {
//                  data->led_state = new_led_state;
//                  gpio_pin_set_dt(&data->led_gpio, data->led_state);
//              }
//          }
//      }
//  }

static int tbog_lock_indicator_handler(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);
    struct tbog_lock_indicator_data *data;

    // Iterate over all instances
    for (int i = 0; i < LOCK_INDICATOR_INSTANCE_COUNT; i++) {
        data = lock_indicator_instances[i];
        const bool new_led_state = (ev->indicators & data->indicator_mask) != 0;
        gpio_pin_set_dt(&data->led_gpio, new_led_state);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int tbog_lock_indicator_init(const struct device *dev) {
    struct tbog_lock_indicator_data *data = dev->data;
    int ret;

    if (!device_is_ready(data->led_gpio.port)) {
        printk("Lock Indicator GPIO device not ready\n");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&data->led_gpio, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to configure Lock Indicator GPIO: %d\n", ret);
        return ret;
    }

    return 0;
}

ZMK_LISTENER(tbog_lock_indicator, tbog_lock_indicator_handler);
ZMK_SUBSCRIPTION(tbog_lock_indicator, zmk_hid_indicators_changed);