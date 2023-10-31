#pragma once

#include "wled.h"
#include "cloud_event.h"

#define NUM_MAX_OBSERVERS 5

#define PERIOD_ROTARY_MS 1
#define PERIOD_BUTTON_MS 20

#define TIME_LONG_PRESS_MS 1000
#define TIME_LONG_LONG_PRESS_MS 4000
#define TIME_END_MS 65500

#define KEY_PRESSED 0
#define KEY_RELEASED 1

class Observer
{
public:
    Observer() {}
    virtual ~Observer() {}
    virtual void Update(const uint8_t uiSrc, const uint8_t uiType, const int32_t iPayload0, const int32_t iPayload1) {}
};

class Observable
{
public:
    Observable(uint8_t uiId) : m_uiId(uiId)
    {
        for (int i = 0; i < NUM_MAX_OBSERVERS; i++)
        {
            m_arrObservers[i] = nullptr;
        }
    }

    virtual ~Observable() {}

    void Register(Observer *pObserver)
    {
        for (int i = 0; i < NUM_MAX_OBSERVERS; i++)
        {
            if (m_arrObservers[i] == nullptr)
            {
                m_arrObservers[i] = pObserver;
                break;
            }
        }
    }

    virtual void NotifyObservers(const uint8_t uiType, const int32_t iPayload0 = 0, const int32_t iPayload1 = 0)
    {
        for (int i = 0; i < NUM_MAX_OBSERVERS; i++)
        {
            if (m_arrObservers[i] != nullptr)
            {
                m_arrObservers[i]->Update(m_uiId, uiType, iPayload0, iPayload1);
            }
        }
    }

private:
    Observer *m_arrObservers[NUM_MAX_OBSERVERS];
    uint8_t m_uiId;
};

enum eINPUT_DEVICE_TYPE
{
    INP_TYPE_INVALID,
    INP_TYPE_ROTARY,
    INP_TYPE_BUTTON
};

class EventSource : public Observable
{
public:
    EventSource(uint8_t uiDevId, unsigned long uiPeriodMs) : Observable(uiDevId),
                                                             m_uiPeriodMs(uiPeriodMs),
                                                             m_ulTimLastCall(0)
    {
    }
    virtual ~EventSource() {}

    void loop()
    {
        unsigned long uiTimAct = millis();

        if (uiTimAct > (m_ulTimLastCall + m_uiPeriodMs))
        {
            exec();
            m_ulTimLastCall = uiTimAct;
        }
    }

    virtual void exec() = 0;

private:
    Observer *m_pObserver;
    unsigned long m_uiPeriodMs;
    unsigned long m_ulTimLastCall;
    
};

class IdRotaryEncoder : public EventSource
{
public:
    IdRotaryEncoder(uint8_t uiDevId, uint8_t uiPinA, uint8_t uiPinB) : EventSource(uiDevId, PERIOD_ROTARY_MS), m_uiPinA(uiPinA), m_uiPinB(uiPinB)
    {
        pinMode(m_uiPinA, INPUT_PULLUP);
        pinMode(m_uiPinB, INPUT_PULLUP);
        m_uiLastSateA = digitalRead(m_uiPinA); // first read
    }

    void exec() override
    {
        uint8_t uiEncA = digitalRead(m_uiPinA); // Read encoder pins
        uint8_t uiEncB = digitalRead(m_uiPinB);
        if ((!uiEncA) && (m_uiLastSateA))
        { // A has gone from high to low
            if (uiEncB == HIGH)
            {
                NotifyObservers(EVT_INP_INCREMENT, 0, 0);
            }
            else if (uiEncB == LOW)
            {
                NotifyObservers(EVT_INP_DECREMENT, 0, 0);
            }
        }
        m_uiLastSateA = uiEncA; // Store value of A for next time
    }

private:
    uint8_t m_uiPinA;
    uint8_t m_uiPinB;
    uint8_t m_uiLastSateA;
};

class IdButton : public EventSource
{
public:
    IdButton(uint8_t uiDevId, uint8_t uiPin) : EventSource(uiDevId, PERIOD_BUTTON_MS), m_uiPin(uiPin), m_uiPressTimeMs(0)
    {
        pinMode(m_uiPin, INPUT_PULLUP);
        m_uiButStateLast = KEY_RELEASED;
    }

    void exec() override
    {
        uint8_t uiButStateAct = digitalRead(m_uiPin);
        if (m_uiButStateLast == KEY_PRESSED)
        {
            // key is still pressd
            if (uiButStateAct == KEY_PRESSED)
            {
                if (m_uiPressTimeMs != TIME_END_MS)
                {
                    m_uiPressTimeMs += PERIOD_BUTTON_MS;
                    if (m_uiPressTimeMs == TIME_LONG_PRESS_MS)
                    {
                        NotifyObservers(EVT_INP_LONG_PRESS, 0, 0);
                    }
                    else if (m_uiPressTimeMs == TIME_LONG_LONG_PRESS_MS)
                    {
                        NotifyObservers(EVT_INP_LONG_LONG_PRESS, 0, 0);
                        m_uiPressTimeMs = TIME_END_MS;
                    }
                }
            }
            // key was released
            else
            {
                NotifyObservers(EVT_INP_RELEASED, 0, 0);
                m_uiPressTimeMs = 0;
            }
        }
        else
        {
            if (uiButStateAct == KEY_PRESSED)
            {
                NotifyObservers(EVT_INP_PRESSED, 0, 0);
                m_uiPressTimeMs = 0;
            }
        }
        m_uiButStateLast = uiButStateAct;
    }

private:
    uint8_t m_uiPin;
    uint8_t m_uiButStateLast;
    uint16_t m_uiPressTimeMs;
};