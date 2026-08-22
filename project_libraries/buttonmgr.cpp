#include "buttonmgr.h"
#include "bootsel.h"

#include "pico/stdlib.h"

using namespace pimoroni;

ButtonManager::ButtonManager()
    : button_a(PicoDisplay2::A),
      button_b(PicoDisplay2::B),
      button_x(PicoDisplay2::X),
      button_y(PicoDisplay2::Y)
{}

Action ButtonManager::update() {
    absolute_time_t now = get_absolute_time();

    //--------------------------
    // BOOTSEL button handling
    //--------------------------
    bool bootsel_pressed = get_bootsel_button();

    // BOOTSEL button pressed for the first time
    if (bootsel_pressed && !bootsel_was_pressed) {
        bootsel_press_start = now;
        bootsel_long_handled = false;
    }

    // BOOTSEL button still pressed
    if (bootsel_pressed && bootsel_was_pressed && !bootsel_long_handled) {
        int64_t press_duration_ms = absolute_time_diff_us(bootsel_press_start, now) / 1000;

        // long press
        if (press_duration_ms >= 1000) {
            bootsel_long_handled = true;
            bootsel_was_pressed = bootsel_pressed;
            return Action::UsbBoot;
        }
    }

    // BOOTSEL button released
    if (!bootsel_pressed && bootsel_was_pressed) {
        bootsel_was_pressed = false;

        // it was not a long press, regular reboot
        if (!bootsel_long_handled) {
            return Action::Reboot;
        }
    }
    
    bootsel_was_pressed = bootsel_pressed;

    // *********************
    // a/b/x/y button events
    // *********************
    bool current_a = button_a.raw();
    bool current_b = button_b.raw();
    bool current_x = button_x.raw();
    bool current_y = button_y.raw();

    Action action = Action::None;

    if (current_a && !last_a) action = button_actions[0];
    else if (current_b && !last_b) action = button_actions[1];
    else if (current_x && !last_x) action = button_actions[2];
    else if (current_y && !last_y) action = button_actions[3];

    last_a = current_a;
    last_b = current_b;
    last_x = current_x;
    last_y = current_y;

    return action;
}

int ButtonManager::button_to_index(char button) const {
    switch (button) {
        case 'a':
        case 'A':
            return 0;

        case 'b':
        case 'B':
            return 1;

        case 'x':
        case 'X':
            return 2;

        case 'y':
        case 'Y':
            return 3;

        default:
            return -1;
    }
}

void ButtonManager::set_action(char button, Action action) {
    int index = button_to_index(button);
    if (index != -1) button_actions[index] = action;
}

void ButtonManager::clear_action(char button) {
    int index = button_to_index(button);
    if (index != -1) button_actions[index] = Action::None;
}

void ButtonManager::clear_actions() {
    for (int i = 0; i < 4; ++i) {
        button_actions[i] = Action::None;
    }
}

bool ButtonManager::any_pressed() {
    return button_a.raw() ||
           button_b.raw() ||
           button_x.raw() ||
           button_y.raw() ||
           get_bootsel_button();
}

void ButtonManager::wait_for_any_button() {
    // wait until no button is pressed (if any)
    while (any_pressed()) {
        update();
        sleep_ms(10);
    }

    // then wait for any button to be pressed to continue
    while (!any_pressed()) {
        update();
        sleep_ms(10);
    }
}


