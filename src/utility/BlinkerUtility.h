#ifndef BlinkerUtility_H
#define BlinkerUtility_H

#if defined(ARDUINO)
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif
#endif

#define NUMBERIC_VALUE_SKIP                 2
#define ARRAY_NUMBERIC_VALUE_SKIP_START     3
#define ARRAY_NUMBERIC_VALUE_SKIP_IN        1
#define STRING_VALUE_SKIP                   3
#define NUMBERIC_VALUE_END_1                ","
#define NUMBERIC_VALUE_END_2                "}"
#define ARRAY_NUMBERIC_VALUE_END_1          ","
#define ARRAY_NUMBERIC_VALUE_END_2          "]"
#define STRING_VALUE_END                    "\""

#define FIND_KEY_VALUE_FAILED               -1000

extern "C" {
    typedef void (*callbackFunction)(void);

    typedef void (*callback_t)(void);
    typedef void (*callback_with_arg_t)(void*);
    typedef void (*callback_with_string_arg_t)(const String & data);
    typedef void (*callback_with_int32_arg_t)(int32_t data);
    typedef void (*callback_with_rgb_arg_t)(uint8_t r_data, uint8_t g_data, uint8_t b_data, uint8_t bright_data);
    typedef void (*callback_with_joy_arg_t)(uint8_t x_data, uint8_t y_data);
    typedef String (*callback_return_string_t)(void);
#if defined(ESP8266) || defined(ESP32)
    typedef bool (*callback_with_json_arg_t)(const JsonObject & data);
#endif
    // typedef void (*callback_with_bool_arg_t)(bool state);
}

template<class T>
String STRING_format(T p)
{
    return String(p);
}

#if defined(ESP8266)
extern "C" {
    #include "user_interface.h"
}

String macDeviceName()
{
    uint8_t mac[6];
    char macStr[13] = { 0 };
    #define STATION_IF 0x00
    wifi_get_macaddr(STATION_IF, mac);

    sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    String macStr_l = STRING_format(macStr);
    //macStr_l.toLowerCase();
    //BLINKER_LOG("MACADDR: ", macStr_l);
    return macStr_l;
}
#elif defined(ESP32)
#include "esp_wifi.h"

String macDeviceName()
{
    uint8_t mac[6];
    char macStr[13] = { 0 };
    // #define WIFI_IF_STA 0x00
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    String macStr_l = STRING_format(macStr);
    //macStr_l.toLowerCase();
    //BLINKER_LOG("MACADDR: ", macStr_l);
    return macStr_l;
}
#endif

template<class T>
const T& BlinkerMin(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template<class T>
const T& BlinkerMax(const T& a, const T& b)
{
    return (b < a) ? a : b;
}

String STRING_find_string(const String & src, const String & targetStart, const String & targetEnd, uint8_t skipNum) {
    int addr_start = src.indexOf(targetStart);
    int addr_end;
    if (targetEnd.length()) {
        addr_end = src.indexOf(targetEnd, addr_start + targetStart.length() + skipNum);
    }
    else {
        addr_end = src.length();
    }

    if (addr_start == -1 || addr_end == -1) {
        return "";
    }
    else {
        return src.substring(addr_start + targetStart.length() + skipNum, addr_end);
    }
}

bool STRING_contains_string(const String & src, const String & key)
{
    int addr_start = src.indexOf(key);
    uint8_t keyLen = key.length();

    return key == src.substring(addr_start, addr_start + keyLen);
}

bool STRING_find_string_value(const String & src, String & dst, const String & key)
{
    int addr_start = src.indexOf(key);
    uint8_t keyLen = key.length();
    // BLINKER_LOG("addr_start: ", addr_start);
    // BLINKER_LOG("keyLen: ", keyLen);

    int addr_end = src.indexOf(STRING_VALUE_END, addr_start + keyLen + STRING_VALUE_SKIP);
    // BLINKER_LOG("addr_end: ", addr_end);
    if (addr_start == -1 || addr_end == -1) {
        return false;
    }
    else {
        dst = src.substring(addr_start + keyLen + STRING_VALUE_SKIP, addr_end);
        // BLINKER_LOG("dst: ", dst);
        return true;
    }
}

int32_t STRING_find_numberic_value(const String & src, const String & key)
{
    int addr_start = src.indexOf(key);
    uint8_t keyLen = key.length();

    if ( key != src.substring(addr_start, addr_start + keyLen)) {
        return FIND_KEY_VALUE_FAILED;
    }

    int addr_end1 = src.indexOf(NUMBERIC_VALUE_END_1, addr_start + keyLen + NUMBERIC_VALUE_SKIP);
    int addr_end2 = src.indexOf(NUMBERIC_VALUE_END_2, addr_start + keyLen + NUMBERIC_VALUE_SKIP);
    int addr_end = BlinkerMin(addr_end1, addr_end2);

    if (addr_end == -1) {
        addr_end = BlinkerMax(addr_end1, addr_end2);
    }

    if (addr_start == -1 || addr_end == -1) {
        return FIND_KEY_VALUE_FAILED;
    }
    else {
        String value = src.substring(addr_start + keyLen + NUMBERIC_VALUE_SKIP, addr_end);
        return value.toInt();
    }
}

float STRING_find_float_value(const String & src, const String & key)
{
    int addr_start = src.indexOf(key);
    uint8_t keyLen = key.length();

    if ( key != src.substring(addr_start, addr_start + keyLen)) {
        return FIND_KEY_VALUE_FAILED;
    }

    int addr_end1 = src.indexOf(NUMBERIC_VALUE_END_1, addr_start + keyLen + NUMBERIC_VALUE_SKIP);
    int addr_end2 = src.indexOf(NUMBERIC_VALUE_END_2, addr_start + keyLen + NUMBERIC_VALUE_SKIP);
    int addr_end = BlinkerMin(addr_end1, addr_end2);

    if (addr_end == -1) {
        addr_end = BlinkerMax(addr_end1, addr_end2);
    }

    if (addr_start == -1 || addr_end == -1) {
        return (float)FIND_KEY_VALUE_FAILED;
    }
    else {
        String value = src.substring(addr_start + keyLen + NUMBERIC_VALUE_SKIP, addr_end);
        return value.toFloat();
    }
}

int32_t STRING_find_array_numberic_value(const String & src, const String & key, uint8_t num)
{
    int addr_start = src.indexOf(key);
    uint8_t keyLen = key.length();
    
    if ( key != src.substring(addr_start, addr_start + keyLen) ) {
        return FIND_KEY_VALUE_FAILED;
    }

    addr_start = addr_start + keyLen + ARRAY_NUMBERIC_VALUE_SKIP_START;

    if (num > 0) {
        int value_start1, value_start2, temp;
        
        for (uint8_t times = 0; times < num; ++times) {
            value_start1 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_1, addr_start);
            value_start2 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_2, addr_start);
            temp = BlinkerMin(value_start1, value_start2);

            if(temp == -1) {
                temp = BlinkerMax(value_start1, value_start2);
            }

            addr_start = temp + ARRAY_NUMBERIC_VALUE_SKIP_IN;
        }
    }

    int addr_end1 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_1, addr_start);
    int addr_end2 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_2, addr_start);
    int addr_end = BlinkerMin(addr_end1, addr_end2);

    if (addr_end == -1) {
        addr_end = BlinkerMax(addr_end1, addr_end2);
    }

    if (addr_start == -1 || addr_end == -1) {
        return FIND_KEY_VALUE_FAILED;
    }
    else {
        String value = src.substring(addr_start, addr_end);
        return value.toInt();
    }
}

float STRING_find_array_float_value(const String & src, const String & key, uint8_t num)
{
    int addr_start = src.indexOf(key);
    uint8_t keyLen = key.length();

    if ( key != src.substring(addr_start, addr_start + keyLen)) {
        return FIND_KEY_VALUE_FAILED;
    }

    addr_start = addr_start + keyLen + ARRAY_NUMBERIC_VALUE_SKIP_START;

    if (num > 0) {
        int value_start1, value_start2, temp;
        for (uint8_t times = 0; times < num; ++times) {
            value_start1 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_1, addr_start);
            value_start2 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_2, addr_start);
            temp = BlinkerMin(value_start1, value_start2);

            if(temp == -1) {
                temp = BlinkerMax(value_start1, value_start2);
            }

            addr_start = temp + ARRAY_NUMBERIC_VALUE_SKIP_IN;
        }
    }

    int addr_end1 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_1, addr_start);
    int addr_end2 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_2, addr_start);
    int addr_end = BlinkerMin(addr_end1, addr_end2);

    if (addr_end == -1) {
        addr_end = BlinkerMax(addr_end1, addr_end2);
    }

    if (addr_start == -1 || addr_end == -1) {
        return (float)FIND_KEY_VALUE_FAILED;
    }
    else {
        String value = src.substring(addr_start, addr_end);
        return value.toFloat();
    }
}

String STRING_find_array_string_value(const String & src, const String & key, uint8_t num)
{
    int addr_start = src.indexOf(key);
    uint8_t keyLen = key.length();
    
    if ( key != src.substring(addr_start, addr_start + keyLen) ) {
        return "";
    }

    addr_start = addr_start + keyLen + ARRAY_NUMBERIC_VALUE_SKIP_START;

    if (num > 0) {
        int value_start1, value_start2, temp;
        
        for (uint8_t times = 0; times < num; ++times) {
            value_start1 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_1, addr_start);
            value_start2 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_2, addr_start);
            temp = BlinkerMin(value_start1, value_start2);

            if(temp == -1) {
                temp = BlinkerMax(value_start1, value_start2);
            }

            addr_start = temp + ARRAY_NUMBERIC_VALUE_SKIP_IN;
        }
    }

    int addr_end1 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_1, addr_start);
    int addr_end2 = src.indexOf(ARRAY_NUMBERIC_VALUE_END_2, addr_start);
    int addr_end = BlinkerMin(addr_end1, addr_end2);

    if (addr_end == -1) {
        addr_end = BlinkerMax(addr_end1, addr_end2);
    }

    if (addr_start == -1 || addr_end == -1) {
        return "";
    }
    else {
        String value = src.substring(addr_start, addr_end);
        return value;
    }
}

#endif