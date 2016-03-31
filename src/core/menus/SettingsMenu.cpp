/*
    cheali-charger - open source firmware for a variety of LiPo chargers
    Copyright (C) 2013  Paweł Stawicki. All right reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Hardware.h"
#include "SettingsMenu.h"
#include "Utils.h"
#include "Buzzer.h"
#include "SerialLog.h"
#include "EditMenu.h"
#include "memory.h"

//#define ENABLE_DEBUG
#include "debug.h"

namespace SettingsMenu {


const_char_ptr const PROGMEM SettingsFanOn[] = {
        string_disable,
        string_always,
        string_FanProgram,
        string_temperature,
        string_tempProgram
};

const PROGMEM struct cprintf::ArrayData FanOnData = CPRINTF_STRING_ARRAY_DATA(SettingsFanOn, settings.fanOn);

const_char_ptr const PROGMEM SettingsUART[] = {
        string_disable,
        string_normal,
        string_debug,
        string_extDebug,
        string_extDebugAdc
};
const PROGMEM struct cprintf::ArrayData UARTData = CPRINTF_STRING_ARRAY_DATA(SettingsUART, settings.UART);
const PROGMEM struct cprintf::ArrayData UARTSpeedsData = CPRINTF_UINT32_ARRAY_DATA(Settings::UARTSpeedValue, settings.UARTspeed);


const_char_ptr const PROGMEM SettingsUARToutput[] = {string_temp, string_pin7, string_pin38};
const PROGMEM struct cprintf::ArrayData UARToutputData = CPRINTF_STRING_ARRAY_DATA(SettingsUARToutput, settings.UARToutput);


const_char_ptr const PROGMEM SettingsMenuType[] = {string_simple, string_advanced};
const PROGMEM struct cprintf::ArrayData menuTypeData = CPRINTF_STRING_ARRAY_DATA(SettingsMenuType, settings.menuType);

const AnalogInputs::ValueType Tmin = (SETTINGS_FAN_TEMPERATURE_DIFF/ANALOG_CELCIUS(1) + 1)*ANALOG_CELCIUS(1);
const AnalogInputs::ValueType Tmax = ANALOG_CELCIUS(99);
const AnalogInputs::ValueType Tstep =  ANALOG_CELCIUS(1);


/*condition bits:*/
#define COND_FAN_ON_T       1
#define COND_UART_ON        2
#define COND_ALWAYS         EDIT_MENU_ALWAYS

uint16_t getSelector() {
    uint16_t result = EDIT_MENU_ALWAYS;
#ifdef ENABLE_FAN
    if(settings.fanOn != Settings::FanProgramTemperature && settings.fanOn != Settings::FanTemperature)
        result -= COND_FAN_ON_T;
#endif
    if(settings.UART == Settings::Disabled)
        result -= COND_UART_ON;

    return result;
}

#define SETTING_N(type, n, x)   {CP_TYPE_ ## type, n, {&settings.x}}
#define SETTING(type, x)        SETTING_N(type, 0, x)

/*
|static string          |when to display| how to display, see cprintf       | how to edit |
 */
const PROGMEM struct EditMenu::StaticEditData editData[] = {
#ifdef ENABLE_LCD_BACKLIGHT
{string_backlight,      COND_ALWAYS,    SETTING(UNSIGNED, backlight),       {1, 0, 100}},
#endif
#ifdef ENABLE_FAN
{string_fanOn,          COND_ALWAYS,    EDIT_STRING_ARRAY(FanOnData),       {1, 0, Settings::FanProgramTemperature}},
{string_fanTempOn,      COND_FAN_ON_T,  SETTING(TEMPERATURE, fanTempOn),    {Tstep, Tmin, Tmax}},
#endif
#ifdef ENABLE_T_INTERNAL
{string_dischOff,       COND_ALWAYS,    SETTING_N(TEMPERATURE, 3, dischargeTempOff), {Tstep, Tmin, Tmax}},
#endif
{string_AudioBeep,      COND_ALWAYS,    SETTING(ON_OFF, audioBeep),         {1, 0, 1}},
{string_minIc,          COND_ALWAYS,    SETTING(A, minIc),                  {CE_STEP_TYPE_KEY_SPEED, ANALOG_AMP(0.001), ANALOG_AMP(0.500)}},
{string_maxIc,          COND_ALWAYS,    SETTING(A, maxIc),                  {CE_STEP_TYPE_SMART, ANALOG_AMP(0.001), MAX_CHARGE_I}},
{string_minId,          COND_ALWAYS,    SETTING(A, minId),                  {CE_STEP_TYPE_KEY_SPEED, ANALOG_AMP(0.001), ANALOG_AMP(0.500)}},
{string_maxId,          COND_ALWAYS,    SETTING(A, maxId),                  {CE_STEP_TYPE_SMART, ANALOG_AMP(0.001), MAX_DISCHARGE_I}},
{string_inputLow,       COND_ALWAYS,    SETTING_N(V, 3, inputVoltageLow),   {ANALOG_VOLT(1), ANALOG_VOLT(7), ANALOG_VOLT(30)}},
#ifdef ENABLE_ANALOG_INPUTS_ADC_NOISE
{string_adcNoise,       COND_ALWAYS,    SETTING(ON_OFF, adcNoise),          {1, 0, 1}},
#endif
{string_UARTview,       COND_ALWAYS,    EDIT_STRING_ARRAY(UARTData),        {1, 0, Settings::ExtDebugAdc}},
{string_UARTspeed,      COND_UART_ON,   EDIT_UINT32_ARRAY(UARTSpeedsData),  {1, 0, SETTINGS_UART_SPEEDS-1}},
#ifdef ENABLE_TX_HW_SERIAL
{string_UARToutput,     COND_UART_ON,   EDIT_STRING_ARRAY(UARToutputData),  {1, 0, 2}},
#endif
{string_MenuType,       COND_ALWAYS,    EDIT_STRING_ARRAY(menuTypeData),    {1, 0, 1}},

#ifdef ENABLE_SETTINGS_MENU_RESET
{string_reset,          EDIT_MENU_ALWAYS, {0,0,{NULL}}},
#endif
{NULL,                  EDIT_MENU_LAST}
};

void editCallback(uint16_ptr adr) {
    EditMenu::setSelector(getSelector());
    Settings::check();
}

void run() {
#ifdef ENABLE_DEBUG
    settings.UART = Settings::Normal;
    SerialLog::powerOn();
#endif
    int8_t item;
    EditMenu::initialize(editData, editCallback);

    do {
        EditMenu::setSelector(getSelector());
        item = EditMenu::run();

        if(item < 0) break;

#ifdef ENABLE_SETTINGS_MENU_RESET
        if(EditMenu::getEditAddress(item) == NULL)  //reset
        {
            Settings::setDefault();
#ifdef ENABLE_DEBUG
            settings.UART = Settings::Normal;
            LogDebug("Reset");
#endif
            Buzzer::soundSelect();
        } else
#endif
        {
#ifndef SDCC_COMPILER
            Settings::Settings undo(settings);
#endif
            if(!EditMenu::runEdit()) {
#ifndef SDCC_COMPILER
                settings = undo;
#endif
            } else {
                Buzzer::soundSelect();
            }
            Settings::apply();
        }
    } while(true);
    Settings::save();
}

} //namespace SettingsMenu


#undef COND_ALWAYS   //needed when all files are packed into one cpp source
