#pragma once

#include "pico/time.h"
#include "button.hpp"
#include "pico_display_2.hpp"

#include "icons.h"

using namespace pimoroni;

// creates a class ActionButton, extending Button from Pimoroni
// ActionButton
//     ├─ Pimoroni Button
//     ├─ action icon
//     └─ pressed
class ActionButton : public Button {
    public:
        ActionButton(uint pin)
            : Button(pin, Polarity::ACTIVE_LOW, 0) {}

        // by default, has (points to) an empty action icon 
        const ActionIcons::ActionIcon* action_icon = &ActionIcons::NONE;
        bool pressed = false;
        
        void update() {
            pressed = read();
        }
};

// area that covers the 4 buttons (x/y/a/b)
class ButtonGroup {
    public:
        //class constructor
        ButtonGroup();

        void update();
    
        // assign or clear actions to buttons
        void set_action_icon(char button, ActionIcons::ActionIcon& action_icon);
        void clear_action_icon(char button);
        void clear_action_icons();

        ActionButton button_a;
        ActionButton button_b;
        ActionButton button_x;
        ActionButton button_y;

    private:
    // helper function
    ActionButton* get_button(char button);
};

// handles the button area (a,b,x,y) and bootsel (back button)
class ButtonManager {
    public:
        // updates button states
        void update();
        
        // true if the action was triggered by buttons
        bool get_active_action(Action action);
                         
        // true if any button was newly pressed
        bool has_activity() const;

        // the x/y/a/b buttons to manage
        ButtonGroup button_group;

    private:
        // initialize bootsel button action to none
        Action bootsel_action = Action::None;
        
        // BOOTSEL state
        bool bootsel_was_pressed = false;
        bool bootsel_long_handled = false;
        bool bootsel_activity = false;
        // this initializes bootsel_press_start with zero
        absolute_time_t bootsel_press_start {};

        // helpers
        static bool get_bootsel_button();        
};
