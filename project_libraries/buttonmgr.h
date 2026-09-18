#pragma once

#include "pico/time.h"
#include "button.hpp"
#include "pico_display_2.hpp"

#include "icons.h"

using namespace pimoroni;

// creates a class ActionButton, extending Button from Pimoroni
class ActionButton : public Button {
    public:
        ActionButton(uint pin)
            : Button(pin, Polarity::ACTIVE_LOW, 0) {}

        Action action = Action::Empty;
        bool pressed = false;
        
        void update() {
            pressed = read();
        }
};

// area that covers the 4 buttons (x/y/a/b)
class ButtonArea {
    public:
        //class constructor
        ButtonArea();

        void update();
    
        // assign or clear actions to buttons
        void set_action(char button, Action action);
        void clear_action(char button);
        void clear_actions();

        // true if any button is pressed
        bool any_pressed();

        ActionButton button_a;
        ActionButton button_b;
        ActionButton button_x;
        ActionButton button_y;

    private:
    // helper function
    ActionButton* get_button(char button);
};

// this class handles the button area (a,b,x,y) and the bootsel
class ButtonManager {
    public:

        // Returns:
        // Action::None  -> no button pressed
        // Action::Empty -> a button was pressed, but it has no associated action
        // otherwise     -> action assigned to the pressed button.
        Action update();

        // wait until a new button is pressed.
        void wait_for_button();

        ButtonArea button_area;

    private:
        // BOOTSEL state
        bool bootsel_was_pressed = false;
        bool bootsel_long_handled = false;
        // this initializes bootsel_press_start with zero
        absolute_time_t bootsel_press_start {};

        // helpers
        bool any_pressed();
        static bool get_bootsel_button();        
};
