#include "Arduino.h"
#include "espasyncbutton.hpp"

#define G_C30_BUTTON_1  GPIO_NUM_0                 // Set GPIO for the button #1
#define G_C30_BUTTON_2  GPIO_NUM_35                // Set GPIO for the button #2

using ESPButton::event_t;

// Buttons

/**
 * @brief GPIOButton object construtor has the following parameters
 * 
 * @param gpio - GPIO number
 * @param logicLevel - Logic level state of the gpio when button is in 'PRESSED' state, LOW or HIGHT
 * @param pull - GPIO pull mode as defined in   https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html#_CPPv416gpio_pull_mode_t
 * @param mode - GPIO mode as defined in        https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html#_CPPv411gpio_mode_t
 * @param debounce = enable/disable debounce feature for gpio

    GPIOButton(gpio_num_t gpio, bool logicLevel, gpio_pull_mode_t pull = GPIO_PULLUP_ONLY, gpio_mode_t mode = GPIO_MODE_INPUT, bool debounce = true);
 */


/**
 * @brief gpio mapped button with Logic level LOW, i.e. button shorts gpio to the ground, gpio must be pulled high
 * 
 * @return GPIOButton<ESPEventPolicy> 
 */
GPIOButton<ESPEventPolicy> g_C30_b1(G_C30_BUTTON_1, LOW);

/**
 * @brief gpio mapped button with Logic level LOW, i.e. button shorts gpio to the ground, gpio must be pulled high
 * 
 * @return GPIOButton<ESPEventPolicy> 
 */
GPIOButton<ESPEventPolicy> g_C30_b2(G_C30_BUTTON_2, LOW);


/**
 * @brief Button callback menu object
 * it will maintain callback options for a group of button events
 * 
 */
ButtonCallbackMenu g_C30_menu;

// Declaring CallBack functions (see below for function definitions)

// an example function, pretend it could move a cursor via on-screeen menu
void C30_cursor_control(event_t e, const EventMsg* m);

// an example function, pretend it could control sound volume for some device
void C30_volume_control(event_t e, const EventMsg* m);
// an example function, it will just show click counts
void C30_counters(event_t e, const EventMsg* m);
// this function will toggle menu levels based on special button events
void C30_menu_toggle();


// Arduino's setup()
void C30_init(){
   // Serial.begin(115200);
    // We MUST create default event loop unless WiFi or Bluetooth is used
    // you do not need to call this if your scketch includes any WiFi/BT setup functions
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // attach our Menu callback handler to system event loop to listen for button events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(EBTN_EVENTS,
                            ESP_EVENT_ANY_ID,
                            // this lambda will simply translate loop events into btn_callback_t callback function
                            [](void* handler_args, esp_event_base_t base, int32_t id, void* event_data){
                                g_C30_menu.handleEvent(ESPButton::int2event_t(id), reinterpret_cast<EventMsg*>(event_data));
                            }, 
                            NULL, NULL)
                );

    // disable simple press/release events, we do not need them
    g_C30_b1.enableEvent(event_t::press,      false);
    g_C30_b1.enableEvent(event_t::release,    false);
    g_C30_b2.enableEvent(event_t::press,      false);
    g_C30_b2.enableEvent(event_t::release,    false);

    // enable clicks
    g_C30_b1.enableEvent(event_t::click);
    g_C30_b2.enableEvent(event_t::click);

    // enable longpress to cycle through menu
    g_C30_b1.enableEvent(event_t::longPress);
    g_C30_b2.enableEvent(event_t::longPress);

    // assign callback functions to Menu
    // I can assign same callbacks to different gpios
    // which button has triggered the event I can figure out in the event message struct and deal with each button sepparately
    // so this way we can process multiple buttons in a single context and code different scenarios and button combinations

    // menu  level 0 - assign Cursol control function (there is no cursor, we just pretend)
    g_C30_menu.assign(G_C30_BUTTON_1, 0, C30_cursor_control);
    g_C30_menu.assign(G_C30_BUTTON_2, 0, C30_cursor_control);

    // menu  level 1 - assign Volume control function (there is no sound, we just pretend)
    g_C30_menu.assign(G_C30_BUTTON_1, 1, C30_volume_control);
    g_C30_menu.assign(G_C30_BUTTON_2, 1, C30_volume_control);

    // menu  level 3 - assign click counters function
    g_C30_menu.assign(G_C30_BUTTON_1, 2, C30_counters);
    g_C30_menu.assign(G_C30_BUTTON_2, 2, C30_counters);

    // enable buttons
    g_C30_b1.enable();
    g_C30_b2.enable();

    // print a hint
    Serial.print("===");
    Serial.print("Use LongPress to switch menu levels, single press to do actions");
    Serial.print("At menu level 1 buttons will control cursor");
    Serial.print("At menu level 2 buttons will control sound volume");
    Serial.print("At menu level 3 buttons will just print chars on autorepeat on hold");
    Serial.print("===");
}

void C30_run(){
    // Simply do nothing here
    delay(1000);
}


// this function will toggle menu-level on any button longPress
void C30_menu_toggle(){
    // cycle switch menu levels 0->1->2->0 ...
    g_C30_menu.setMenuLevel( (g_C30_menu.getMenuLevel()+1)%3 );

    // on menu level 2 we will enable autorepeat for  button_1 and multiclick counter for button_2
    if (g_C30_menu.getMenuLevel() == 2){
        g_C30_b1.enableEvent(event_t::autoRepeat);
        g_C30_b2.enableEvent(event_t::multiClick);
    } else {
        // on other levels but 2 we disable it
        g_C30_b1.enableEvent(event_t::autoRepeat, false);
        g_C30_b2.enableEvent(event_t::multiClick, false);
    }
    Serial.print("Switched to menu level:"); Serial.println(g_C30_menu.getMenuLevel());
};

// Let's pretend that this function is moving some cursor, but we will just print a message to Serial here as an example
void C30_cursor_control(event_t e, const EventMsg* m){
    switch(e){
        // Use click event to move cursor Up and Down (just print message)
        case event_t::click :
            // we catch all 'click' events and check which gpio triggered it
            // BUTTON_1 will move cursor "Up", BUTTON_2 will move it "down"
            Serial.print("Cursor "); Serial.println(m->gpio == G_C30_BUTTON_1 ? "Up" : "Down");
            break;
        // any button's longpress event will cycle menu level 0->1->2->3->0
        case event_t::longPress :
            menu_toggle();
            break;
    }
};

// Let's pretend that this function  is changing volume, but we just print messages here
void C30_volume_control(event_t e, const EventMsg* m){
    switch(e){
        // Use click event to control Volume Up and Down (just print)
        case event_t::click :
            Serial.print("Volume "); Serial.println(m->gpio == G_C30_BUTTON_1 ? "Up" : "Down");
            break;
        // any button longpress event will cycle menu level 0->1->2->0
        case event_t::longPress :
            menu_toggle();
            break;
    }
};

// this function will show how to use AutoRepeat and MultiCLicks
void C30_counters(event_t e, const EventMsg* m){
    switch(e){
    // on a single click we will instruct user to do something more with buttons
    case event_t::click :
        if (m->gpio == G_C30_BUTTON_1)
            Serial.println("Hold button 1 for autorepeat...");
        else
            Serial.println("Single click is not that fun, try double or triple clicks...");
        break;
    // autorepeat action
    case event_t::autoRepeat :
        // I'll print "*" on each autorepeat event for button_1 and '#' for button_2
        if (m->gpio == G_C30_BUTTON_1)
            Serial.print("*");
        else
            Serial.print("#");
        break;
    // multiclicks
    case event_t::multiClick :
        Serial.printf("gpio: %d clicked %u times. ", m->gpio, m->cntr);
        Serial.println("Click 4 times to enable autorepeat for Button 2");
        Serial.println("Click 5 times to exit to menu level 0");
        // toggle menu on 5th click
        if (m->cntr == 4){
            b2.enableEvent(event_t::autoRepeat);
            Serial.println("Button 2 autorepeats enabled, try it");
        }

        if (m->cntr == 5)
            menu_toggle();
        break;
    }

    /**
     * since we are using autorepeat above for astesk printing, we can no longer use 'longPress' event to toggle menu,
     * because 'autorepeat' is triggered after 'longPress' event
       so, let's ignore it here
    */
    //case event_t::longPress :
    //    menu_toggle();
    //    break;
};


