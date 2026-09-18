#include "pico/stdlib.h"

#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "buttonmgr.h"

using namespace pimoroni;

// constructor, assigns the hardware buttons in PicoDisplay2
// to each of our 4 logical buttons
// buttonmgr
// ├─ button_a → A
// ├─ button_b → B
// ├─ button_x → X
// └─ button_y → Y
ButtonArea::ButtonArea()
    : button_a(PicoDisplay2::A),
      button_b(PicoDisplay2::B),
      button_x(PicoDisplay2::X),
      button_y(PicoDisplay2::Y)
{}

void ButtonArea::update() {
    button_a.update();
    button_b.update();
    button_x.update();
    button_y.update();
}

ActionButton* ButtonArea::get_button(char button) {
    switch (button) {
        case 'a':
        case 'A':
            return &button_a;

        case 'b':
        case 'B':
            return &button_b;

        case 'x':
        case 'X':
            return &button_x;

        case 'y':
        case 'Y':
            return &button_y;

        default:
            return nullptr;
    }
}

void ButtonArea::set_action(char button, Action action) {
    ActionButton* b = get_button(button);
    if (b) b->action = action;
}

void ButtonArea::clear_action(char button) {
    ActionButton* b = get_button(button);
    if (b) b->action = Action::Empty;
}

void ButtonArea::clear_actions() {
    button_a.action = Action::Empty;
    button_b.action = Action::Empty;
    button_x.action = Action::Empty;
    button_y.action = Action::Empty;
}

bool ButtonArea::any_pressed() {
    return button_a.raw() ||
           button_b.raw() ||
           button_x.raw() ||
           button_y.raw();
}

// class constructor
ButtonManager::ButtonManager();

Action ButtonManager::update() {

    absolute_time_t now = get_absolute_time();

    // update button area
    button_area.update();

    // *********************** 
    // BOOTSEL button handling
    // ***********************

    // is the bootsel button pressed?
    bool bootsel_pressed = get_bootsel_button();

    // bootsel button pressed for the first time, 
    // just log it and return
    if (bootsel_pressed && !bootsel_was_pressed) {
        bootsel_press_start = now; // time when first press detected
        bootsel_long_handled = false; // it's not a long press (yet?)
        bootsel_was_pressed = true; // register the button was already pressed
        return Action::Empty;
    }

    // bootsel button was pressed and it's still pressed 
    // but long press action was not handled (we did not initiate any action) 
    // long press -> firmware load boot 
    // calculate if it qualifies for a long press (more than 1 second)
    if (bootsel_pressed && bootsel_was_pressed && !bootsel_long_handled) {
        if (absolute_time_diff_us(bootsel_press_start, now) / 1000 >= 1000) {
            bootsel_long_handled = true;
            bootsel_was_pressed = bootsel_pressed;
            return Action::UsbBoot;
        }
    }

    // bootsel button was pressed, but it's not pressed now 
    if (!bootsel_pressed && bootsel_was_pressed) {
        bootsel_was_pressed = false;

        // reboot if it's a short press
        if (!bootsel_long_handled) {
            return Action::Reboot;
        }
    }
    
    bootsel_was_pressed = bootsel_pressed;

    return Action::None;
}

bool ButtonManager::any_pressed() {
    return button_area.any_pressed() ||
           get_bootsel_button();
}

void ButtonManager::wait_for_button() {
    // wait until no button is pressed
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

