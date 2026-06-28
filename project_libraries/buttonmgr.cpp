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
    bool pressed = get_bootsel_button();
    absolute_time_t now = get_absolute_time();
    bootsel_result = BootselEvent::None;

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
}

bool ButtonManager::is_a()  {
    return button_a.raw();
}

bool ButtonManager::is_b()  {
    return button_b.raw();
}

bool ButtonManager::is_x()  {
    return button_x.raw();
}

bool ButtonManager::is_y()  {
    return button_y.raw();
}

BootselEvent ButtonManager::get_bootsel_event() const {
    return bootsel_result;
}
