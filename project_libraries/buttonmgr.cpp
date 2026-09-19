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
ButtonGroup::ButtonGroup()
    : button_a(PicoDisplay2::A),
      button_b(PicoDisplay2::B),
      button_x(PicoDisplay2::X),
      button_y(PicoDisplay2::Y)
{}

void ButtonGroup::update() {
    button_a.update();
    button_b.update();
    button_x.update();
    button_y.update();
}

ActionButton* ButtonGroup::get_button(char button) {
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

void ButtonGroup::set_action_icon(char button, const ActionIcons::ActionIcon& action_icon) {
    
    // returns pointer to the action button that is paired 
    // with the friendly name (x/y/a/b) passed as a char
    ActionButton* b = get_button(button);
    // if it's not null, point now to the new action button
    if (b) b->action_icon = &action_icon;
}

void ButtonGroup::clear_action_icon(char button) {
    ActionButton* b = get_button(button);
    if (b) b->action_icon = &ActionIcons::NONE;
}

void ButtonGroup::clear_actions() {
    button_a.action_icon = &ActionIcons::NONE;
    button_b.action_icon = &ActionIcons::NONE;
    button_x.action_icon = &ActionIcons::NONE;
    button_y.action_icon = &ActionIcons::NONE;
}

void ButtonManager::update() {
    absolute_time_t now = get_absolute_time();

    // update button group
    button_group.update();
    
    // reset bootsel action
    bootsel_action = Action::None;
    bootsel_activity = false;

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
        bootsel_activity = true;
        return;
    }

    // bootsel button was pressed and it's still pressed 
    // but long press action was not handled (we did not initiate any action) 
    // long press -> firmware load boot 
    // calculate if it qualifies for a long press (more than 1 second)
    if (bootsel_pressed && bootsel_was_pressed && !bootsel_long_handled) {
        if (absolute_time_diff_us(bootsel_press_start, now) / 1000 >= 1000) {
            bootsel_long_handled = true;
            bootsel_action = Action::UsbBoot;
            return;
        }
    }

    // bootsel button was pressed, but it's not pressed now 
    if (!bootsel_pressed && bootsel_was_pressed) {
        bootsel_was_pressed = false;

        // reboot if it's a short press
        if (!bootsel_long_handled) {
            bootsel_action = Action::Reboot;
            return;
        }
    }
    
    bootsel_was_pressed = bootsel_pressed;
}

bool ButtonManager::get_bootsel_button() {
    return read_bootsel_button();
}

bool ButtonManager::is_action_active(Action action) const {

    if (bootsel_action == action) {
        return true;
    }

    if (button_group.button_a.pressed &&
        button_group.button_a.action == action) {
        return true;
    }

    if (button_group.button_b.pressed &&
        button_group.button_b.action == action) {
        return true;
    }

    if (button_group.button_x.pressed &&
        button_group.button_x.action == action) {
        return true;
    }

    if (button_group.button_y.pressed &&
        button_group.button_y.action == action) {
        return true;
    }

    return false;
}

bool ButtonManager::has_activity() const {
    return bootsel_action != Action::None ||
           button_group.button_a.pressed ||
           button_group.button_b.pressed ||
           button_group.button_x.pressed ||
           button_group.button_y.pressed;
}

static bool __no_inline_not_in_flash_func(read_bootsel_button)() {
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

