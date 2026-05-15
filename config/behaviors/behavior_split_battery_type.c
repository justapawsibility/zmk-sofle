#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/battery.h>
#include <zmk/split/bluetooth/peripheral_status.h>

#define DT_DRV_COMPAT zmk_behavior_macro

static void tap_key(uint8_t keycode) {
    zmk_hid_press(keycode);
    zmk_hid_send_report();
    k_msleep(8);

    zmk_hid_release(keycode);
    zmk_hid_send_report();
    k_msleep(8);
}

static void send_char(char c) {
    bool shift = false;
    uint8_t keycode = 0;

    if (c >= 'A' && c <= 'Z') {
        shift = true;
        keycode = HID_USAGE_KEY_KEYBOARD_A + (c - 'A');
    } else if (c >= 'a' && c <= 'z') {
        keycode = HID_USAGE_KEY_KEYBOARD_A + (c - 'a');
    } else if (c >= '1' && c <= '9') {
        keycode = HID_USAGE_KEY_KEYBOARD_1 + (c - '1');
    } else if (c == '0') {
        keycode = HID_USAGE_KEY_KEYBOARD_0;
    } else if (c == ':') {
        shift = true;
        keycode = HID_USAGE_KEY_KEYBOARD_SEMICOLON;
    } else if (c == ' ') {
        keycode = HID_USAGE_KEY_KEYBOARD_SPACEBAR;
    } else {
        return;
    }

    if (shift) {
        zmk_hid_register_mod(HID_USAGE_KEY_KEYBOARD_LEFTSHIFT);
    }

    tap_key(keycode);

    if (shift) {
        zmk_hid_unregister_mod(HID_USAGE_KEY_KEYBOARD_LEFTSHIFT);
        zmk_hid_send_report();
    }
}

static void send_string(const char *s) {
    while (*s) {
        send_char(*s++);
    }
}

static int split_battery_press(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {

    uint8_t local = zmk_battery_state_of_charge();

    uint8_t peripheral = 0xFF;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE)
    const struct zmk_peripheral_status *status =
        zmk_split_bt_peripheral_status(0);

    if (status) {
        peripheral = status->battery_level;
    }
#endif

    char buf[32];

    if (peripheral != 0xFF) {
        snprintk(buf, sizeof(buf), "L:%d R:%d", local, peripheral);
    } else {
        snprintk(buf, sizeof(buf), "BAT:%d", local);
    }

    send_string(buf);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int split_battery_release(struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api split_battery_driver_api = {
    .binding_pressed = split_battery_press,
    .binding_released = split_battery_release,
};

DEVICE_DT_INST_DEFINE(0,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      APPLICATION,
                      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                      &split_battery_driver_api);
