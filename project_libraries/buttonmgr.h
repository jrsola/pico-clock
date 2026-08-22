#pragma once

#include "pico/time.h"
#include "button.hpp"
#include "pico_display_2.hpp"

#include "icons.h"

using namespace pimoroni;

class ButtonManager {

    public:
        ButtonManager();

        // update button states an return action associated with the 
        // button that was pressed. If no button was pressed, returns None.
        Action update();

        // this will assign actions to buttons
        void set_action(char button, Action action);
        void clear_action(char button);
        void clear_actions();

        // wait until a new button is pressed.
        void wait_for_any_button();

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
            Action::None,
            Action::None,
            Action::None,
            Action::None
        };

        // BOOTSEL state
        bool bootsel_was_pressed = false;
        bool bootsel_long_handled = false;
        absolute_time_t bootsel_press_start {};

        // Helpers
        int button_to_index(char button) const;
        bool any_pressed();
        
};
