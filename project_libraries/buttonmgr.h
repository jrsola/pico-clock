#pragma once

#include "pico/time.h"
#include "button.hpp"
#include "pico_display_2.hpp"

#include "icons.h"

using namespace pimoroni;

class ButtonManager {
    public:
        // class constructor
        ButtonManager();

        // Returns:
        // Action::None  -> no button pressed
        // Action::Empty -> a button was pressed, but it has no associated action
        // otherwise     -> action assigned to the pressed button.
        Action update();

        // assign actions to buttons
        void set_action(char button, Action action);
        void clear_action(char button);
        void clear_actions();

        // wait until a new button is pressed.
        void wait_for_button();

    private:
        Button button_a;
        Button button_b;
        Button button_x;
        Button button_y;

        // previous button states to detect changes
        bool last_a = false;
        bool last_b = false;
        bool last_x = false;
        bool last_y = false;

        // actions associated with A, B, X and Y
        Action button_actions[4] = {
            Action::Empty,
            Action::Empty,
            Action::Empty,
            Action::Empty
        };

        // BOOTSEL state
        bool bootsel_was_pressed = false;
        bool bootsel_long_handled = false;
        // this initializes bootsel_press_start with zero
        absolute_time_t bootsel_press_start {};

        // helpers
        int button_to_index(char button) const;
        bool any_pressed();
        static bool get_bootsel_button();        
};
