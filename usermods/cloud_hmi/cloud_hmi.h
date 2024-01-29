#pragma once

#include "input_device.h"
#include "cloud_event.h"
#include <U8x8lib.h>
#include "wled.h"
#include <vector>

using namespace std::placeholders;

const char cstrCONFIG_CHMI_ROOT[] PROGMEM = "CloudUI";
const char cstrCONFIG_CHMI_STDBY_COLOR[] PROGMEM = "StdbyColorHsv";
const char cstrCONFIG_CHMI_STDBY_EFFECT[] PROGMEM = "StdbyEffect";

#define OLDE_PIN_SCL 22
#define OLED_PIN_SDA 21

#define iOLED_BRI_ON 10
#define iOLED_BRI_OFF 1

enum eEVENT_SOURCE_IDS
{
  ID_DEVICE_FIRST,
  ID_ROTARY_BRIGHTNESS = ID_DEVICE_FIRST,
  ID_BUT_ON_OFF,
  ID_ROTARY_EFFECT,
  ID_BUT_EFFECT,
  // ID_SWITCH_PULL,
  ID_DEVICE_LAST,
  ID_SM_MAIN = ID_DEVICE_LAST,
  ID_EVENT_SOURCE_LAST
};

struct stcPreset
{
  uint8_t m_uiId;
  String m_StrName;
};

typedef std::function<void(uint8_t, uint8_t)> funcSm_t;

class StateMachine : public Observer, public Observable
{
public:
  StateMachine(uint8_t uiId, uint8_t uiNumStates) : Observer(),
                                                    Observable(uiId),
                                                    m_arrFunc(new funcSm_t[uiNumStates])
  {
  }

  void Update(uint8_t uiSrc, uint8_t uiType, int32_t iPayload0 = 0, int32_t iPayload1 = 0)
  {
    if (!PreRun(uiSrc, uiType))
    {
      m_arrFunc[m_uiCurrState](uiSrc, uiType);
    }
  }

  void AddState(uint8_t uiStateId, funcSm_t func)
  {
    m_arrFunc[uiStateId] = func;
  }

  virtual bool PreRun(uint8_t uiSrc, uint8_t uiType)
  {
    return false;
  }

  void SetInitialState(uint8_t uiState)
  {
    m_uiCurrState = uiState;
    m_arrFunc[m_uiCurrState](ID_DEVICE_FIRST, EVT_STATE_ENTRY);
  }

  void NextState(uint8_t uiState)
  {
    m_arrFunc[m_uiCurrState](ID_DEVICE_FIRST, EVT_STATE_EXIT);
    m_uiCurrState = uiState;
    m_arrFunc[m_uiCurrState](ID_DEVICE_FIRST, EVT_STATE_ENTRY);
    NotifyObservers(EVT_STATE_CHANGED, uiState);
  }

  uint8_t GetCurrentState();

private:
  uint8_t m_uiCurrState;
  funcSm_t *m_arrFunc{nullptr};
};

enum eCLOUD_STATES
{
  STATE_OFF,
  STATE_ON,
  STATE_LAST
};

class SmCloud : public StateMachine
{
public:
  SmCloud() : StateMachine(ID_SM_MAIN, STATE_LAST),
              m_display(U8X8_PIN_NONE, OLDE_PIN_SCL, OLED_PIN_SDA),
              m_uiBrightness(0),
              m_uiLastPreset(0),
              m_vecPresets()
  {
  }

  void Init()
  {
    // init OLED display
    m_display.begin();
    m_display.setPowerSave(0);
    m_display.setFlipMode(1);
    m_display.setContrast(10); // Contrast setup will help to preserve OLED lifetime. In case OLED need to be brighter increase number up to 255
    m_display.setFont(u8x8_font_chroma48medium8_r);

    AddState(STATE_OFF, std::bind(&SmCloud::StOff, this, _1, _2));
    AddState(STATE_ON, std::bind(&SmCloud::StOn, this, _1, _2));
    SetInitialState(STATE_OFF);
  }

  void ApplyCurrentPreset()
  {
    if (m_vecPresets.size() > m_uiLastPreset)
    {
      applyPreset(m_vecPresets[m_uiLastPreset].m_uiId, CALL_MODE_BUTTON);
    }
  }

  bool UpdatePresets()
  {
    bool bRetval = false;
    if (requestJSONBufferLock(9))
    {
      File hdlFile = WLED_FS.open("/presets.json", "r");
      if (hdlFile)
      {
        DeserializationError err = deserializeJson(doc, hdlFile);
        if (!err)
        {
          m_vecPresets.clear();
          // iterate over all keys - preset IDs
          JsonObject jsRoot = doc.as<JsonObject>();
          for (JsonPair kv : jsRoot)
          {
            if (doc[kv.key()].containsKey("n"))
            {
              uint8_t uiIdx;
              uiIdx = strtoul(kv.key().c_str(), nullptr, 10);
              m_vecPresets.push_back(stcPreset{uiIdx, doc[kv.key()]["n"].as<String>()});
              DEBUG_PRINT(kv.key().c_str());
              DEBUG_PRINT(":");
              // doc[kv.key()]["n"].as<const char*>();
              DEBUG_PRINTLN(doc[kv.key()]["n"].as<String>());
            }
            // Serial.println(kv.key().c_str());
            // Serial.println(kv.value().as<const char *>());
          }
          // serializeJsonPretty(doc, Serial);
          bRetval = true;
        }
        else
        {
          DEBUG_PRINTLN("ERROR: Failed to read preset file.");
        }
      }
    }
    releaseJSONBufferLock();
    DEBUG_PRINT("Updated num. presets:");
    DEBUG_PRINTLN(m_vecPresets.size());
    if (m_vecPresets.empty())
    {
      m_uiLastPreset = 0;
    }
    return bRetval;
  }

private:
  void StOff(uint8_t uiSrc, uint8_t uiType)
  {
    if (uiType == EVT_STATE_ENTRY)
    {
      m_display.setPowerSave(1);
      UpdatePresets();
      m_uiBrightness = 0;
      m_display.drawString(0, 0, "Off    ");
      sprintf(m_cstrTmp, "Br: %4d", m_uiBrightness);
      m_display.drawString(0, 1, m_cstrTmp);
      if (m_vecPresets.size() > m_uiLastPreset)
      {
        sprintf(m_cstrTmp, "%10s", m_vecPresets[m_uiLastPreset].m_StrName.c_str());
      }
      m_display.drawString(0, 2, m_cstrTmp);
      bri = m_uiBrightness;
            colorUpdated(CALL_MODE_BUTTON);
      m_display.setContrast(iOLED_BRI_OFF);
    }
    else if (uiSrc == ID_BUT_ON_OFF && uiType == EVT_INP_PRESSED)
    {
      NextState(STATE_ON);
    }
  }

  void StOn(uint8_t uiSrc, uint8_t uiType)
  {
    // Segment &seg = strip.getSegment(0);
    if (uiType == EVT_STATE_ENTRY)
    {
      m_display.setPowerSave(0);
      m_display.setContrast(iOLED_BRI_ON);
      m_uiBrightness = 20;
      m_display.drawString(0, 0, "On     ");
      sprintf(m_cstrTmp, "Br: %4d", m_uiBrightness);
      m_display.drawString(0, 1, m_cstrTmp);
      if (m_vecPresets.size() > m_uiLastPreset)
      {
        sprintf(m_cstrTmp, "%10s", m_vecPresets[m_uiLastPreset].m_StrName.c_str());
      }
      m_display.drawString(0, 2, m_cstrTmp);
      ApplyCurrentPreset();
      bri = m_uiBrightness;
      colorUpdated(CALL_MODE_BUTTON);
    }
    else if (uiSrc == ID_ROTARY_BRIGHTNESS || uiSrc == ID_BUT_ON_OFF)
    {
      if (uiType == EVT_INP_PRESSED)
      {
        NextState(STATE_OFF);
      }
      else if (uiType == EVT_INP_INCREMENT)
      {
        m_uiBrightness += 5;
        sprintf(m_cstrTmp, "Br: %4d", m_uiBrightness);
        m_display.drawString(0, 1, m_cstrTmp);
        bri = m_uiBrightness;
        colorUpdated(CALL_MODE_BUTTON);
      }
      else if (uiType == EVT_INP_DECREMENT)
      {
        m_uiBrightness -= 5;
        sprintf(m_cstrTmp, "Br: %4d", m_uiBrightness);
        m_display.drawString(0, 1, m_cstrTmp);
        bri = m_uiBrightness;
        colorUpdated(CALL_MODE_BUTTON);
      }
    }
    else if (uiSrc == ID_ROTARY_EFFECT || uiSrc == ID_BUT_EFFECT)
    {
      if (uiType == EVT_INP_INCREMENT)
      {
        if (!m_vecPresets.empty())
        {
          m_uiLastPreset++;
          if (m_uiLastPreset >= m_vecPresets.size())
          {
            m_uiLastPreset = 0;
          }
        }
        else
        {
          m_uiLastPreset = 0;
        }
      }
      else if (uiType == EVT_INP_DECREMENT)
      {
        if (!m_vecPresets.empty())
        {
          if (m_uiLastPreset == 0)
          {
            m_uiLastPreset = m_vecPresets.size() - 1;
          }
          else
          {
            m_uiLastPreset--;
          }
        }
        else
        {
          m_uiLastPreset = 0;
        }
      }
      if (m_vecPresets.size() > m_uiLastPreset)
      {
        sprintf(m_cstrTmp, "%10s", m_vecPresets[m_uiLastPreset].m_StrName.c_str());
      }
      m_display.drawString(0, 2, m_cstrTmp);
      ApplyCurrentPreset();
      bri = m_uiBrightness;
      colorUpdated(CALL_MODE_BUTTON);
    }
  }

  U8X8_SSD1306_128X64_NONAME_HW_I2C m_display;
  char m_cstrTmp[80];
  uint8_t m_uiBrightness;
  uint8_t m_uiLastPreset;

public:
  std::vector<stcPreset> m_vecPresets;
};

class UsermodCloudHmi : public Usermod,
                        public Observer
{

public:
  UsermodCloudHmi() : Usermod(),
                      Observer(),
                      m_iCounter(0),
                      m_sm(),
                      m_ulLastTime(0)
  {
  }

  void setup()
  {

    m_sm.Init();

    // init input devices
    m_arrDev[ID_ROTARY_BRIGHTNESS] = new IdRotaryEncoder(ID_ROTARY_BRIGHTNESS, 19, 18);
    m_arrDev[ID_ROTARY_BRIGHTNESS]->Register(&m_sm);
    m_arrDev[ID_BUT_ON_OFF] = new IdButton(ID_BUT_ON_OFF, 17);
    m_arrDev[ID_BUT_ON_OFF]->Register(&m_sm);

    m_arrDev[ID_ROTARY_EFFECT] = new IdRotaryEncoder(ID_ROTARY_EFFECT, 35, 34);
    m_arrDev[ID_ROTARY_EFFECT]->Register(&m_sm);
    m_arrDev[ID_BUT_EFFECT] = new IdButton(ID_BUT_EFFECT, 33);
    m_arrDev[ID_BUT_EFFECT]->Register(&m_sm);

    // m_arrDev[ID_SWITCH_PULL] = new IdButton(ID_SWITCH_PULL, this, 0);
  }

  void loop()
  {
    for (uint8_t i = ID_DEVICE_FIRST; i < ID_DEVICE_LAST; i++)
    {
      m_arrDev[i]->loop();
    }

    if (millis() - m_ulLastTime > 5000)
    {
      // remember last update
      m_ulLastTime = millis();
    }
  }

  /**
   * addToConfig() (called from set.cpp) stores persistent properties to cfg.json
   */
  void addToConfig(JsonObject &root)
  {
    JsonObject top = root.createNestedObject(FPSTR(cstrCONFIG_CHMI_ROOT)); // usermodname

    top[FPSTR(cstrCONFIG_CHMI_STDBY_COLOR)] = 0xf5bca5;
    top[FPSTR(cstrCONFIG_CHMI_STDBY_EFFECT)] = -1;

    DEBUG_PRINTLN(F("Hardkey config saved."));
  }

  /**
   * readFromConfig() is called before setup() to populate properties from values stored in cfg.json
   *
   * The function should return true if configuration was successfully loaded or false if there was no configuration.
   */
  bool readFromConfig(JsonObject &root)
  {
    // we look for JSON object
    // JsonObject top = root[FPSTR(cstrCONFIG_CHMI_ROOT)];

    //  setup();

    return true;
  }

  uint16_t getId()
  {
    return USERMOD_ID_ROTARY_ENC_UI;
  }

private:
  EventSource *m_arrDev[ID_DEVICE_LAST];

  int32_t m_iCounter;
  SmCloud m_sm;
  uint64_t m_ulLastTime;
};
