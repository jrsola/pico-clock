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

class ButtonManager {
private:
    Button button_a;
    Button button_b;
    Button button_x;
    Button button_y;

    enum State { IDLE, PRESSED, RELEASED };
    State bootsel_state = IDLE;
    absolute_time_t last_bootsel_press = at_the_end_of_time;
    absolute_time_t bootsel_press_start;
    int bootsel_click_count = 0;
    BootselEvent bootsel_result = BootselEvent::None;

public:
    ButtonManager();

    void update();

    bool is_a() ;
    bool is_b() ;
    bool is_x() ;
    bool is_y() ;

    BootselEvent get_bootsel_event() const;
};
