#pragma once

#include "../wled00/src/dependencies/touch-lib/TouchLib.h"
#include "pin_config.h"
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <vector>

TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS,
               PIN_TOUCH_RES);
#define TOUCH_GET_FORM_INT 0

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
lv_anim_t a;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

lv_obj_t *slider_label;
int screenWidth = TFT_HEIGHT;
int screenHeight = TFT_WIDTH;

std::vector<byte> g_vecPresetMap;
byte g_presetToApply = 0;

void ThreadGui(void *parameter)
{
   while (1)
   {
      lv_task_handler(); /* let the GUI do its work */
      vTaskDelay(5 / portTICK_PERIOD_MS);
   }
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p)
{
   uint32_t w = (area->x2 - area->x1 + 1);
   uint32_t h = (area->y2 - area->y1 + 1);

   tft.startWrite();
   tft.setAddrWindow(area->x1, area->y1, w, h);
   tft.pushColors(&color_p->full, w * h, true);
   tft.endWrite();

   lv_disp_flush_ready(disp);
}

bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
   if (touch.read())
   {
      int16_t touchX, touchY;
      bool touched;

      if (touch.getPointNum() > 0)
      {
         touched = true;
         TP_Point t = touch.getPoint(0);
         touchX = t.y;
         touchY = TFT_WIDTH - t.x;
      }
      else
      {
         touched = false;
      }

      if (touchX > screenWidth || touchY > screenHeight)
      {
         DEBUG_PRINTLN("Y or y outside of expected parameters..");
         DEBUG_PRINT("y:");
         DEBUG_PRINT(touchX);
         DEBUG_PRINT(" x:");
         DEBUG_PRINT(touchY);
      }
      else
      {
         data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

         /*Set the coordinates (if released use the last pressed coordinates)*/
         data->point.x = touchX;
         data->point.y = touchY;

         // DEBUG_PRINT("Data x");
         // DEBUG_PRINTLN(touchX);

         // DEBUG_PRINT("Data y");
         // DEBUG_PRINTLN(touchY);
      }
   }

   return false;
}

void printEvent(String Event, lv_event_t event)
{
   DEBUG_PRINT(Event);
   DEBUG_PRINTLN(" ");

   switch (event)
   {
   case LV_EVENT_PRESSED:
      DEBUG_PRINTLN("Pressed\n");
      break;

   case LV_EVENT_SHORT_CLICKED:
      DEBUG_PRINTLN("Short clicked\n");
      break;

   case LV_EVENT_CLICKED:
      DEBUG_PRINTLN("Clicked\n");
      break;

   case LV_EVENT_LONG_PRESSED:
      DEBUG_PRINTLN("Long press\n");
      break;

   case LV_EVENT_LONG_PRESSED_REPEAT:
      DEBUG_PRINTLN("Long press repeat\n");
      break;

   case LV_EVENT_RELEASED:
      DEBUG_PRINTLN("Released\n");
      break;
   }
}

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
   uint16_t uiSelected;
   if (event == LV_EVENT_VALUE_CHANGED)
   {
      uiSelected = lv_roller_get_selected(obj);
      if (g_vecPresetMap.size() > uiSelected && g_presetToApply != g_vecPresetMap[uiSelected])
      {
         g_presetToApply = g_vecPresetMap[uiSelected];
         applyPreset(g_vecPresetMap[uiSelected], CALL_MODE_BUTTON);
      }
   }
}

void UiAnimation(lv_obj_t *obj, lv_coord_t x)
{
   if (bri != 0)
   {
      lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hsv_to_rgb(360, 100, x));
   }
   else
   {
      lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
   }
}

class LilyGoUsermod : public Usermod
{
private:
   unsigned long lastTime = 0;

public:
   // LilyGoUsermod() :
   // {}
   // Functions called by WLED

   virtual void onStateChange(uint8_t mode) override
   {
   }

   /*
    * setup() is called once at boot. WiFi is not yet connected at this point.
    * You can use it to initialize variables, sensors or similar.
    */
   void setup()
   {
      pinMode(PIN_POWER_ON, OUTPUT);
      pinMode(PIN_LCD_BL, OUTPUT);

      digitalWrite(PIN_POWER_ON, HIGH);
      digitalWrite(PIN_LCD_BL, LOW);

      Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
      if (!touch.init())
      {
         DEBUG_PRINTLN("ERROR: Touch IC not found");
      }

      lv_init();

      tft.begin(); /* TFT init */
      tft.setRotation(1);

      lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

      /*Initialize the display*/
      lv_disp_drv_t disp_drv;
      lv_disp_drv_init(&disp_drv);
      disp_drv.hor_res = screenWidth;
      disp_drv.ver_res = screenHeight;
      disp_drv.flush_cb = my_disp_flush;
      disp_drv.buffer = &disp_buf;
      lv_disp_drv_register(&disp_drv);

      lv_indev_drv_t indev_drv;
      lv_indev_drv_init(&indev_drv);          /*Descriptor of a input device driver*/
      indev_drv.type = LV_INDEV_TYPE_POINTER; /*Touch pad is a pointer-like device*/
      indev_drv.read_cb = my_touchpad_read;   /*Set your driver function*/
      lv_indev_drv_register(&indev_drv);      /*Finally register the driver*/

      lv_obj_t *scr = lv_cont_create(NULL, NULL);
      lv_disp_load_scr(scr);

      lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_CYAN);

      m_uiRollPreset = lv_roller_create(lv_scr_act(), NULL);
      lv_roller_set_visible_row_count(m_uiRollPreset, 4);
      lv_roller_set_auto_fit(m_uiRollPreset, false);
      lv_roller_set_fix_width(m_uiRollPreset, screenWidth);
      lv_obj_set_width(m_uiRollPreset, screenWidth - 70);
      lv_obj_set_height(m_uiRollPreset, screenHeight - 40);
      lv_obj_align(m_uiRollPreset, NULL, LV_ALIGN_CENTER, 0, 0);
      // lv_obj_set_style_local_border_width(m_uiRollPreset, LV_ROLLER_PART_BG, LV_STATE_DEFAULT, 20);
      lv_obj_set_event_cb(m_uiRollPreset, event_handler);

      UiUpdatePresetRoller();
      UiUpdateSelectedPreset(currentPreset);
      lv_obj_set_state(m_uiRollPreset, LV_STATE_FOCUSED);

      // Setup animation
      int16_t uiDurationMs = 500;

      lv_anim_init(&a);

      /* MANDATORY SETTINGS
       *------------------*/

      /*Set the "animator" function*/
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)UiAnimation);

      /*Set the variable to animate*/
      lv_anim_set_var(&a, lv_scr_act());

      /*Length of the animation [ms]*/
      lv_anim_set_time(&a, uiDurationMs);

      /*Set start and end values. E.g. 0, 150*/
      lv_anim_set_values(&a, 30, 100);

      /* OPTIONAL SETTINGS
       *------------------*/

      /*Time to wait before starting the animation [ms]*/
      lv_anim_set_delay(&a, 0);

      /*Play the animation backward too with this duration. Default is 0 (disabled) [ms]*/
      lv_anim_set_playback_time(&a, uiDurationMs);

      /*Number of repetitions. Default is 1.  LV_ANIM_REPEAT_INFINIT for infinite repetition*/
      lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

      /* START THE ANIMATION
       *------------------*/
      lv_anim_start(&a);

      xTaskCreatePinnedToCore(
          ThreadGui, /* Function to implement the task */
          "GUI",     /* Name of the task */
          10000,     /* Stack size in words */
          NULL,      /* Task input parameter */
          10,        /* Priority of the task */
          NULL,      /* Task handle. */
          0);        /* Core where the task should run */
   }

   void UiUpdatePresetRoller()
   {
      String strPresets = "";
      if (requestJSONBufferLock(9))
      {
         File hdlFile = WLED_FS.open("/presets.json", "r");
         if (hdlFile)
         {
            DeserializationError err = deserializeJson(doc, hdlFile);
            hdlFile.close();
            if (!err)
            {
               g_vecPresetMap.clear();
               bool bFirst = true;
               // iterate over all keys - preset IDs
               JsonObject jsRoot = doc.as<JsonObject>();
               for (JsonPair kv : jsRoot)
               {
                  if (doc[kv.key()].containsKey("n"))
                  {
                     uint8_t uiIdx;
                     uiIdx = strtoul(kv.key().c_str(), nullptr, 10);

                     g_vecPresetMap.push_back(uiIdx);
                     if (bFirst)
                     {
                        bFirst = false;
                     }
                     else
                     {
                        strPresets.concat("\n");
                     }
                     strPresets.concat(doc[kv.key()]["n"].as<String>());
                     DEBUG_PRINT(kv.key().c_str());
                     DEBUG_PRINT(":");
                     DEBUG_PRINTLN(doc[kv.key()]["n"].as<String>());
                  }
               }
            }
            else
            {
               DEBUG_PRINTLN("ERROR: Failed to read preset file.");
            }
         }
         releaseJSONBufferLock();
      }

      lv_roller_set_options(m_uiRollPreset,
                            strPresets.c_str(),
                            LV_ROLLER_MODE_NORMAL);
   }

   void UiUpdateSelectedPreset(byte uiPreset)
   {
      for (size_t i = 0; i < g_vecPresetMap.size(); i++)
      {
         if (g_vecPresetMap[i] == uiPreset)
         {
            lv_roller_set_selected(m_uiRollPreset, i, LV_ANIM_ON);
            break;
         }
      }
   }

   /*
    * connected() is called every time the WiFi is (re)connected
    * Use it to initialize network interfaces
    */
   void connected() {}

   /*
    * loop() is called continuously. Here you can check for events, read sensors,
    * etc.
    *
    * Tips:
    * 1. You can use "if (WLED_CONNECTED)" to check for a successful network
    * connection. Additionally, "if (WLED_MQTT_CONNECTED)" is available to check
    * for a connection to an MQTT broker.
    *
    * 2. Try to avoid using the delay() function. NEVER use delays longer than 10
    * milliseconds. Instead, use a timer check as shown here.
    */
   void loop()
   {

      // switch on BL after init
      digitalWrite(PIN_LCD_BL, HIGH);

      // do it every 5 seconds
      if (millis() - lastTime > 5000)
      {
         // UpdatePresets();
         lastTime = millis();
         // DEBUG_PRINT("Usermod running on Core: ");
         // DEBUG_PRINTLN(xPortGetCoreID());
      }
   }
   lv_obj_t *m_uiRollPreset = nullptr;
   lv_obj_t *m_uiBgObj = nullptr;

   // void addToJsonState(JsonObject &root)
   // {
   // }

   // void readFromJsonState(JsonObject &root)
   // {
   // }

   // void addToConfig(JsonObject &root)
   // {
   // }

   // bool readFromConfig(JsonObject &root)
   // {

   // }

   // void handleOverlayDraw()
   // {
   //    // check if usermod is active
   // }

   uint16_t getId() { return USERMOD_ID_LILYGO; }
};
