#include "pico/time.h"
#include "button.hpp"
#include "pico_display_2.hpp"

using namespace pimoroni;

enum class BootselEvent {
    None,
    SinglePress,
    DoublePress,
    LongPress
};

enum class ButtonEvent {
    None,
    A,
    B,
    X,
    Y
};

class ButtonManager {
private:
    Button button_a;
    Button button_b;
    Button button_x;
    Button button_y;

    bool last_a = false;
    bool last_b = false;
    bool last_x = false;
    bool last_y = false;

    enum State { IDLE, PRESSED, RELEASED };
    State bootsel_state = IDLE;
    absolute_time_t last_bootsel_press = at_the_end_of_time;
    absolute_time_t bootsel_press_start;
    int bootsel_click_count = 0;
    BootselEvent bootsel_result = BootselEvent::None;
    BootselEvent get_bootsel_event() const;
    ButtonEvent button_result = ButtonEvent::None;

public:
    ButtonManager();

    ButtonEvent get_button_event() const;

    void update();

    bool is_a() ;
    bool is_b() ;
    bool is_x() ;
    bool is_y() ;
    bool any_pressed();
    bool is_bootsel_single();
    bool is_bootsel_double();
    bool is_bootsel_long();
    void wait_for_any_button();

};
