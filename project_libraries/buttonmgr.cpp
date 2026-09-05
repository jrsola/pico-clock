#include "pico/stdlib.h"

#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "buttonmgr.h"

using namespace pimoroni;

ButtonManager::ButtonManager()
    : button_a(PicoDisplay2::A),
      button_b(PicoDisplay2::B),
      button_x(PicoDisplay2::X),
      button_y(PicoDisplay2::Y)
{}

ButtonEvent ButtonManager::update() {
    absolute_time_t now = get_absolute_time();

    ButtonEvent event;

    //--------------------------
    // BOOTSEL button handling
    //--------------------------
    bool bootsel_pressed = get_bootsel_button();

    // BOOTSEL button pressed for the first time
    if (bootsel_pressed && !bootsel_was_pressed) {
        bootsel_press_start = now;
        bootsel_long_handled = false;

        event.activity = true;
    }

    // BOOTSEL button still pressed -> firmware load 
    if (bootsel_pressed && bootsel_was_pressed && !bootsel_long_handled) {
        int64_t press_duration_ms = absolute_time_diff_us(bootsel_press_start, now) / 1000;

        // long press
        if (press_duration_ms >= 1000) {
            bootsel_long_handled = true;
            bootsel_was_pressed = bootsel_pressed;
            
            event.activity = true;
            event.action = Action::UsbBoot;

            return event;
        }
    }

    // BOOTSEL button released -> reboot
    if (!bootsel_pressed && bootsel_was_pressed) {
        bootsel_was_pressed = false;

        // it was not a long press, regular reboot
        if (!bootsel_long_handled) {
            event.activity = true;
            event.action = Action::Reboot;
            return event;
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

    if (current_a && !last_a) {
        event.activity = true;
        event.action = button_actions[0];
    }
    else if (current_b && !last_b) {
        event.activity = true;
        event.action = button_actions[1];
    }
    else if (current_x && !last_x) {
        event.activity = true;
        event.action = button_actions[2];
    }
    else if (current_y && !last_y) {
        event.activity = true;
        event.action = button_actions[3];
    }

    last_a = current_a;
    last_b = current_b;
    last_x = current_x;
    last_y = current_y;

    return event;
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

bool __no_inline_not_in_flash_func(ButtonManager::get_bootsel_button)() {
    const uint CS_PIN_INDEX = 1;

    // Interrupt handlers may live in flash, so disable interrupts while
    // temporarily taking control of QSPI CS.
    uint32_t flags = save_and_disable_interrupts();

    // Set QSPI chip select to Hi-Z
    hw_write_masked(
        &ioqspi_hw->io[CS_PIN_INDEX].ctrl,
        GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
        IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS
    );

    // Cannot call sleep functions while flash access is disabled
    for (volatile int i = 0; i < 1000; ++i);

#if PICO_RP2040
    constexpr uint32_t CS_BIT = (1u << 1);
#else
    constexpr uint32_t CS_BIT = SIO_GPIO_HI_IN_QSPI_CSN_BITS;
#endif

    // BOOTSEL pulls CS low when pressed
    bool pressed = !(sio_hw->gpio_hi_in & CS_BIT);

    // Restore normal QSPI chip select operation
    hw_write_masked(
        &ioqspi_hw->io[CS_PIN_INDEX].ctrl,
        GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
        IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS
    );

    restore_interrupts(flags);

    return pressed;
}

