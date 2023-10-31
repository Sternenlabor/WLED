#pragma once

#include "input_device.h"
#include "cloud_event.h"
#include <U8x8lib.h>

using namespace std::placeholders;

const char cstrCONFIG_CHMI_ROOT[] PROGMEM = "CloudUI";
const char cstrCONFIG_CHMI_STDBY_COLOR[] PROGMEM = "StdbyColorHsv";
const char cstrCONFIG_CHMI_STDBY_EFFECT[] PROGMEM = "StdbyEffect";

#define OLDE_PIN_SCL 22
#define OLED_PIN_SDA 21

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
              m_uiEffect(FX_MODE_STATIC)
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

private:
  void StOff(uint8_t uiSrc, uint8_t uiType)
  {
    if (uiType == EVT_STATE_ENTRY)
    {
      m_uiBrightness = 0;
      m_display.drawString(0, 0, "Off    ");
      sprintf(m_cstrTmp, "Br: %4d", m_uiBrightness);
      m_display.drawString(0, 1, m_cstrTmp);
      sprintf(m_cstrTmp, "Ef: %4d", m_uiEffect);
      m_display.drawString(0, 2, m_cstrTmp);
      bri = m_uiBrightness;
      colorUpdated(CALL_MODE_NO_NOTIFY);
    }
    else if (uiSrc == ID_BUT_ON_OFF && uiType == EVT_INP_PRESSED)
    {
      NextState(STATE_ON);
    }
  }

  void StOn(uint8_t uiSrc, uint8_t uiType)
  {
    Segment &seg = strip.getSegment(0);
    if (uiType == EVT_STATE_ENTRY)
    {
      m_uiBrightness = 20;
      m_display.drawString(0, 0, "On     ");
      sprintf(m_cstrTmp, "Br: %4d", m_uiBrightness);
      m_display.drawString(0, 1, m_cstrTmp);
      sprintf(m_cstrTmp, "Ef: %4d", m_uiEffect);
      m_display.drawString(0, 2, m_cstrTmp);
      bri = m_uiBrightness;
      colorUpdated(CALL_MODE_NO_NOTIFY);
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
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
      else if (uiType == EVT_INP_DECREMENT)
      {
        m_uiBrightness -= 5;
        sprintf(m_cstrTmp, "Br: %4d", m_uiBrightness);
        m_display.drawString(0, 1, m_cstrTmp);
        bri = m_uiBrightness;
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
    }
    else if (uiSrc == ID_ROTARY_EFFECT || uiSrc == ID_BUT_EFFECT)
    {
      if (uiType == EVT_INP_INCREMENT)
      {
        m_uiEffect++;
        if (m_uiEffect > FX_MODE_DYNAMIC_SMOOTH)
        {
          m_uiEffect = FX_MODE_STATIC;
        }
        sprintf(m_cstrTmp, "Ef: %4d", m_uiEffect);
        m_display.drawString(0, 2, m_cstrTmp);
        seg.mode = m_uiEffect;
      }
      else if (uiType == EVT_INP_DECREMENT)
      {
        m_uiEffect--;
        if (m_uiEffect > FX_MODE_DYNAMIC_SMOOTH)
        {
          m_uiEffect = FX_MODE_DYNAMIC_SMOOTH;
        }
        sprintf(m_cstrTmp, "Ef: %4d", m_uiEffect);
        m_display.drawString(0, 2, m_cstrTmp);
        seg.mode = m_uiEffect;
      }
    }
  }

  U8X8_SSD1306_128X64_NONAME_HW_I2C m_display;
  char m_cstrTmp[80];
  uint8_t m_uiBrightness;
  uint8_t m_uiEffect;
};

class UsermodCloudHmi : public Usermod,
                        public Observer
{

public:
  UsermodCloudHmi() : Usermod(),
                      Observer(),
                      m_iCounter(0),
                      m_sm()
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
};
