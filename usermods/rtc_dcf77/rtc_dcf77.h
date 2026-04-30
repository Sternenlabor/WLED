#pragma once
#include "wled.h"
#include <Wire.h>

constexpr uint8_t ADDR_DCF77 = 0x21;
constexpr uint32_t I2C_CLOCK_HZ = 100000;

constexpr uint8_t REG_SECONDS = 0x00;
constexpr uint8_t REG_DCF77_CONFIG = 0x0D;
constexpr uint8_t REG_STATUS = 0x0E;

constexpr uint8_t DCF77_CONFIG_DCFE = bit(0);
constexpr uint8_t STATUS_DCFIF = bit(0);

struct RtcDateTime
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint8_t year;
};

/*

 */

class RtcDcf77 : public Usermod
{

private:
    bool usermodActive = false;
    unsigned long lastTime = 0;

    bool writeRegister(uint8_t reg, uint8_t value)
    {
        Wire.beginTransmission(ADDR_DCF77);
        Wire.write(reg & 0x0F);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    bool readRegisters(uint8_t startReg, uint8_t *buffer, uint8_t length)
    {
        Wire.beginTransmission(ADDR_DCF77);
        Wire.write(startReg & 0x0F);
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }

        if (Wire.requestFrom(ADDR_DCF77, length) != length)
        {
            return false;
        }

        for (uint8_t i = 0; i < length; ++i)
        {
            buffer[i] = Wire.read();
        }
        return true;
    }

    bool readRegister(uint8_t reg, uint8_t &value)
    {
        return readRegisters(reg, &value, 1);
    }

    uint8_t bcdToDecimal(uint8_t value)
    {
        return ((value >> 4) * 10) + (value & 0x0F);
    }

    bool readDateTime(RtcDateTime &dateTime)
    {

        uint8_t raw[7] = {};
        if (!readRegisters(REG_SECONDS, raw, sizeof(raw)))
        {
            return false;
        }

        dateTime.second = bcdToDecimal(raw[0] & 0x7F);
        dateTime.minute = bcdToDecimal(raw[1] & 0x7F);
        dateTime.hour = bcdToDecimal(raw[2] & 0x3F);
        dateTime.weekday = raw[3] & 0x07;
        dateTime.day = bcdToDecimal(raw[4] & 0x3F);
        dateTime.month = bcdToDecimal(raw[5] & 0x1F);
        dateTime.year = bcdToDecimal(raw[6]);
        return true;
    }

    bool enableDcf77Reception()
    {
        uint8_t config = 0;
        if (!readRegister(REG_DCF77_CONFIG, config))
        {
            return false;
        }

        config |= DCF77_CONFIG_DCFE;
        return writeRegister(REG_DCF77_CONFIG, config);
    }

    bool ensureDcf77ReceptionEnabled(bool &rewritten)
    {
        rewritten = false;

        uint8_t config = 0;
        if (!readRegister(REG_DCF77_CONFIG, config))
        {
            return false;
        }

        if (config & DCF77_CONFIG_DCFE)
        {
            return true;
        }

        rewritten = true;
        config |= DCF77_CONFIG_DCFE;
        return writeRegister(REG_DCF77_CONFIG, config);
    }

    const __FlashStringHelper *weekdayName(uint8_t weekday)
    {
        switch (weekday)
        {
        case 1:
            return F("Mo");
        case 2:
            return F("Di");
        case 3:
            return F("Mi");
        case 4:
            return F("Do");
        case 5:
            return F("Fr");
        case 6:
            return F("Sa");
        case 7:
            return F("So");
        default:
            return F("--");
        }
    }

    bool isDateTimePlausible(const RtcDateTime &dateTime)
    {
        return dateTime.second < 60 &&
               dateTime.minute < 60 &&
               dateTime.hour < 24 &&
               dateTime.weekday >= 1 &&
               dateTime.weekday <= 7 &&
               dateTime.day >= 1 &&
               dateTime.day <= 31 &&
               dateTime.month >= 1 &&
               dateTime.month <= 12;
    }

    bool isLeapYear(uint16_t year)
    {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    uint8_t daysInMonth(uint16_t year, uint8_t month)
    {
        static const uint8_t daysPerMonth[] = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        if (month == 2 && isLeapYear(year))
        {
            return 29;
        }
        return daysPerMonth[month - 1];
    }

    time_t rtcDateTimeToTimeT(const RtcDateTime &dateTime)
    {
        const uint16_t year = 2000 + dateTime.year;
        uint32_t days = 0;

        for (uint16_t y = 1970; y < year; ++y)
        {
            days += isLeapYear(y) ? 366 : 365;
        }

        for (uint8_t month = 1; month < dateTime.month; ++month)
        {
            days += daysInMonth(year, month);
        }

        days += dateTime.day - 1;

        return static_cast<time_t>(days) * 86400UL +
               static_cast<uint32_t>(dateTime.hour) * 3600UL +
               static_cast<uint32_t>(dateTime.minute) * 60UL +
               dateTime.second;
    }

public:
    // Functions called by WLED

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
    void setup()
    {
        DEBUG_PRINTLN(F("RTC DCF77: setup"));
        Wire.begin();
        Wire.setClock(I2C_CLOCK_HZ);
        if (enableDcf77Reception())
        {
            DEBUG_PRINTLN(F("RTC DCF77: reception enabled"));
        }
        else
        {
            DEBUG_PRINTLN(F("RTC DCF77: reception enable failed"));
        }
    }

    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected()
    {
    }

    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     *
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     *
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop()
    {
        // do it every 5 seconds
        if ((millis() - lastTime > 5000) && usermodActive)
        {
            lastTime = millis();

            RtcDateTime now;
            uint8_t status = 0;
            bool dcfConfigRewritten = false;
            bool dcfConfigOk = ensureDcf77ReceptionEnabled(dcfConfigRewritten);

            if (!readDateTime(now))
            {
                DEBUG_PRINTLN(F("Fehler: Zeit/Datum konnten nicht gelesen werden."));
                return;
            }

            if (readRegister(REG_STATUS, status) && (status & STATUS_DCFIF))
            {
                DEBUG_PRINTLN(F("  DCF77: neu synchronisiert"));
                writeRegister(REG_STATUS, STATUS_DCFIF);
            }
            else if (!dcfConfigOk)
            {
                DEBUG_PRINTLN(F("  DCF77: Config-Lesefehler"));
            }
            else if (dcfConfigRewritten)
            {
                DEBUG_PRINTLN(F("  DCF77: Empfang neu aktiviert"));
            }
            else
            {
                DEBUG_PRINTLN(F("  DCF77: Empfang aktiv"));
            }

            if (isDateTimePlausible(now))
            {
                time_t rtcTime = rtcDateTimeToTimeT(now);
                toki.setTime(rtcTime, TOKI_NO_MS_ACCURACY, TOKI_TS_RTC);
                updateLocalTime();
                DEBUG_PRINTLN(F("  Aktuallisiere Uhrzeit"));
            }
            else
            {
                DEBUG_PRINTLN(F("  Hinweis: Zeit/Datum noch nicht plausibel"));
            }
        }
    }

    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject &root)
    {
    }

    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject &root)
    {
    }

    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     *
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     *
     * addToConfig() will make your settings editable through the Usermod Settings page automatically.
     *
     * Usermod Settings Overview:
     * - Numeric values are treated as floats in the browser.
     *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
     *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
     *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
     *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
     *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
     *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
     *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
     *     used in the Usermod when reading the value from ArduinoJson.
     * - Pin values can be treated differently from an integer value by using the key name "pin"
     *   - "pin" can contain a single or array of integer values
     *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
     *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
     *   - Tip: use int8_t to store the pin value in the Usermod, so a  0 value (pin not set) can be used
     *
     * See usermod_v2_auto_save.h for an example that saves Flash space by reusing ArduinoJson key name strings
     *
     * If you need a dedicated settings page with custom layout for your Usermod, that takes a lot more work.
     * You will have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * See the WLED Soundreactive fork (code and wiki) for reference.  https://github.com/atuline/WLED
     *
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject &root)
    {
        JsonObject top = root.createNestedObject("RTC DCF77");
        top["active"] = usermodActive;
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
     *
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     *
     * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
     *
     * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
     * The configComplete variable is true only if the "exampleUsermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
     *
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject &root)
    {
        // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
        // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

        JsonObject top = root["RTC DCF77"];

        bool configComplete = !top.isNull();

        configComplete &= getJsonValue(top["active"], usermodActive);

        return configComplete;
    }

    virtual void appendConfigData() override
    {
    }

    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw()
    {
    }

    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
        return USERMOD_ID_RTC_DCF77;
    }

    // More methods can be added in the future, this example will then be extended.
    // Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};
