

#pragma once


//////// uistrings.h
// Set of flash-strings that might be reused multiple times within the code

// General
static const char T_HEADLINE[] PROGMEM = "EmbUI Demo";    // имя проекта

// Our variable names
static const char V_LED[] PROGMEM = "vLED";
static const char V_VAR1[] PROGMEM = "v1";
static const char V_VAR2[] PROGMEM = "v2";

// UI blocks
static const char T_DEMO[] PROGMEM = "demo";     // генерация UI-секции "демо"
static const char T_MORE[] PROGMEM = "more";

// UI handlers
static const char T_SET_DEMO[] PROGMEM = "do_demo";     // обработка данных из секции "демо"
static const char T_SET_MORE[] PROGMEM = "do_more";
//


/////////

//#define BAUD_RATE       115200  

//// interface.h
void D10_create_parameters();
void D10_block_menu(Interface *interf, const JsonObject *data, const char* action);
void D10_block_demopage(Interface *interf, const JsonObject *data, const char* action);
void D10_action_demopage(Interface *interf, const JsonObject *data, const char* action);
void D10_action_blink(Interface *interf, const JsonObject *data, const char* action);

///// interface.cpp

//#include "main.h"

#include "EmbUI.h"
//#include "interface.h"

//#include "uistrings.h"   // non-localized text-strings

/**
 * можно нарисовать свой собственный интефейс/обработчики с нуля, либо
 * подключить статический класс с готовыми формами для базовых системных натсроек и дополнить интерфейс.
 * необходимо помнить что существуют системные переменные конфигурации с зарезервированными именами.
 * Список имен системных переменных можно найти в файле "constants.h"
 */
#include "basicui.h"

/**
 * Headline section
 * this is an overriden weak method that builds our WebUI interface from the top
 * ==
 * Головная секция,
 * переопределенный метод фреймфорка, который начинает строить корень нашего Web-интерфейса
 * 
 */
void D10_section_main_frame(Interface *interf, const JsonObject *data, const char* action){
  if (!interf) return;

  interf->json_frame_interface();                                 // open interface frame

  interf->json_section_manifest("EmbUI Example", embui.macid(), 0, "v1.0");      // app name/device id/jsapi/version manifest
  interf->json_section_end();                                     // manifest section MUST be closed!

  D10_block_menu(interf, data, NULL);                   // Строим UI блок с меню выбора других секций
  interf->json_frame_flush();                       // close frame

  if(WiFi.getMode() & WIFI_MODE_STA){            // if WiFI is no connected to external AP, than show page with WiFi setup
    D10_block_demopage(interf, data, NULL);                   // Строим блок с demo переключателями
  } else {
    LOG(println, "UI: Opening network setup page");
    basicui::page_settings_netw(interf, nullptr, NULL);
  }

};


/**
 * This code builds UI section with menu block on the left
 * 
 */
void D10_block_menu(Interface *interf, const JsonObject *data, const char* action){
    if (!interf) return;
    // создаем меню
    interf->json_section_menu();

    /**
     * пункт меню - "демо"
     */
    interf->option(T_DEMO, "UI Demo");

    /**
     * добавляем в меню пункт - настройки,
     * это автоматически даст доступ ко всем связанным секциям с интерфейсом для системных настроек
     * 
     */
    basicui::menuitem_settings(interf);       // пункт меню "настройки"
    interf->json_section_end();
}

/**
 * Demo controls
 * 
 */
void D10_block_demopage(Interface *interf, const JsonObject *data, const char* action){
    // Headline
    // параметр FPSTR(T_SET_DEMO) определяет зарегистрированный обработчик данных для секции
    interf->json_section_main(T_SET_DEMO, "Some demo controls");
    interf->comment("Комментарий: набор контролов для демонстрации");     // комментарий-описание секции

    // переключатель, связанный с переменной конфигурации V_LED - Изменяется синхронно
    interf->checkbox(V_LED, embui.paramVariant(V_LED),"Onboard LED", true);

    interf->text(V_VAR1, embui.paramVariant(V_VAR1), "text field label");   // create text field with value from the system config
    interf->text(V_VAR2, "some default val", "another text label");         // текстовое поле со значением "по-умолчанию"

    /*  кнопка отправки данных секции на обработку
     *  первый параметр FPSTR(T_DEMO) определяет алиас акшена обработчика данных формы 
     *  обработчк должен быть зарегистрирован через embui.section_handle_add()
     */ 
    interf->button(button_t::submit, T_SET_DEMO, T_DICT[lang][TD::D_Send], P_GRAY);   // button color
    interf->json_section_end(); // close json_section_main
    interf->json_frame_flush();
}

/**
 * @brief action handler for demo form data
 * 
 */
void D10_action_demopage(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;

    LOG(println, "processing section demo");

    // сохраняем значение 1-й переменной в конфиг фреймворка
    SETPARAM(V_VAR1);

    // выводим значение 1-й переменной в serial
    const char *text = (*data)[V_VAR1];
    Serial.printf("Varialble_1 value:%s\n", text );

    // берем указатель на 2-ю переменную
    text = (*data)[V_VAR2];
    // или так:
    // String var2 = (*data)[FPSTR(V_VAR2)];
    // выводим значение 2-й переменной в serial
    Serial.printf("Varialble_2 value:%s\n", text);
}

/**
 * @brief interactive handler for LED switchbox
 * 
 */
void D10_action_blink(Interface *interf, const JsonObject *data, const char* action){
  if (!data) return;  // здесь обрабатывает только данные

  SETPARAM(V_LED);  // save new LED state to the config

  // set LED state to the new checkbox state
  digitalWrite(LED_BUILTIN, !(*data)[V_LED]); // write inversed signal for build-in LED
  Serial.printf("LED: %u\n", (*data)[V_LED].as<bool>());
}

/**
 * функция регистрации переменных и активностей
 * 
 */
void D10_create_parameters(){
    LOG(println, F("UI: Creating application vars"));

    /**
     * регистрируем свои переменные
     */
    embui.var_create(V_LED, true);              // LED default status is on
    embui.var_create(V_VAR1, "ipsum lorum");    // заводим текстовую переменную V_VAR1 со значением по умолчанию "ipsum lorum"

    // регистрируем обработчики активностей
    embui.action.set_mainpage_cb(D10_section_main_frame);                            // заглавная страница веб-интерфейса

    /**
     * добавляем свои обрабочки на вывод UI-секций
     * и действий над данными
     */
    embui.action.add(T_DEMO, D10_block_demopage);                // generate "Demo" UI section

    // обработчики
    embui.action.add(T_SET_DEMO, D10_action_demopage);           // обработка данных из секции "Demo"

    embui.action.add(V_LED, D10_action_blink);                   // обработка рычажка светодиода
};


/// main.cpp


// Main headers
//#include "main.h"
#include "EmbUI.h"
//#include "uistrings.h"   // non-localized text-strings
//#include "interface.h"

/**
 * построение интерфейса осуществляется в файлах 'interface.*'
 *
 */

// MAIN Setup
void D10_init() {
  //Serial.begin(BAUD_RATE);
  Serial.println("Starting test...");

  pinMode(LED_BUILTIN, OUTPUT); // we are goning to blink this LED

  // Start EmbUI framework
  embui.begin();

  // register our actions
  D10_create_parameters();

  LOG(println, "restore LED state from configuration param");
  digitalWrite( LED_BUILTIN, !embui.paramVariant(FPSTR(V_LED)) );
}


// MAIN loop
void D10_run() {
  embui.handle();
}





