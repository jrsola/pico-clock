#include "buttonmgr.h"
#include "bootsel.h"

using namespace pimoroni;

ButtonManager::ButtonManager()
    : button_a(PicoDisplay2::A),
      button_b(PicoDisplay2::B),
      button_x(PicoDisplay2::X),
      button_y(PicoDisplay2::Y)
{}

void ButtonManager::update() {

    absolute_time_t now = get_absolute_time();

    bootsel_result = BootselEvent::None;
    button_result = ButtonEvent::None;

    // **************
    // bootsel events
    // **************
    bool pressed = get_bootsel_button();

    static bool was_pressed = false;
    static bool first_press = true;

    if (pressed && !was_pressed) {
        int64_t elapsed_ms = absolute_time_diff_us(last_bootsel_press, now) / 1000;

        if (!first_press && elapsed_ms < 500) {
            bootsel_result = BootselEvent::DoublePress;
        } else {
            // Guardem l’hora actual per detectar doble clic més endavant
            last_bootsel_press = now;
        }

        bootsel_press_start = now;
        first_press = false;
    }

    if (!pressed && was_pressed) {
        int64_t press_duration_ms = absolute_time_diff_us(bootsel_press_start, now) / 1000;
        if (press_duration_ms >= 1000) {
            bootsel_result = BootselEvent::LongPress;
        } else if (bootsel_result != BootselEvent::DoublePress) {
            bootsel_result = BootselEvent::SinglePress;
        }
    }

    was_pressed = pressed;

    // *********************
    // a/b/x/y button events
    // *********************
    bool current_a = button_a.raw();
    bool current_b = button_b.raw();
    bool current_x = button_x.raw();
    bool current_y = button_y.raw();

    if (current_a && !last_a) button_result = ButtonEvent::A;
    else if (current_b && !last_b) button_result = ButtonEvent::B;
    else if (current_x && !last_x) button_result = ButtonEvent::X;
    else if (current_y && !last_y) button_result = ButtonEvent::Y;

    last_a = current_a;
    last_b = current_b;
    last_x = current_x;
    last_y = current_y;
}

bool ButtonManager::any_pressed() {
    return button_a.raw() ||
           button_b.raw() ||
           button_x.raw() ||
           button_y.raw() ||
           get_bootsel_button();
}

BootselEvent ButtonManager::get_bootsel_event() const {
    return bootsel_result;
}

ButtonEvent ButtonManager::get_button_event() const {
    return button_result;
}

bool ButtonManager::is_a()  {
    return button_result == ButtonEvent::A;
}

bool ButtonManager::is_b()  {
    return button_result == ButtonEvent::B;
}

bool ButtonManager::is_x()  {
    return button_result == ButtonEvent::X;
}

bool ButtonManager::is_y()  {
    return button_result == ButtonEvent::Y;
}

bool ButtonManager::is_bootsel_single(){
    return (bootsel_result==BootselEvent::SinglePress);
}

bool ButtonManager::is_bootsel_double(){
    return (bootsel_result==BootselEvent::DoublePress);
}

bool ButtonManager::is_bootsel_long(){
    return (bootsel_result==BootselEvent::LongPress);
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
