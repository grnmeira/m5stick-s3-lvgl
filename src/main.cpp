#include <M5Unified.h>
#include <lvgl.h>

const auto BTNA_GPIO_PIN = GPIO_NUM_11;
const auto BTNB_GPIO_PIN = GPIO_NUM_12;
const auto DISPLAY_RES_V = 240;
const auto DISPLAY_RES_H = 135;

enum class EventType {
  LVGL_INDEV_READ,
  LVGL_TIMER_HANDLER
};

QueueHandle_t mainEventQueue;

const auto DRAW_BUF_SIZE = (DISPLAY_RES_H * DISPLAY_RES_V / 10 * (LV_COLOR_DEPTH / 8));
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

static uint32_t lvgl_tick_cb() {
    return millis();
}

void lvgl_disp_flush( lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    uint16_t* buf16 = (uint16_t*)px_map;
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5.Display.pushImageDMA(area->x1,area->y1, w, h, buf16);
    lv_display_flush_ready(disp);
}

void IRAM_ATTR lvgl_timer_cb(TimerHandle_t timer) {
    auto event = EventType::LVGL_TIMER_HANDLER;
    xQueueSendToBackFromISR(mainEventQueue, &event, nullptr);
}

volatile auto btnAState = LV_INDEV_STATE_RELEASED;
volatile auto btnBState = LV_INDEV_STATE_RELEASED;

void IRAM_ATTR button_changed_interrupt() {
  btnAState = digitalRead(BTNA_GPIO_PIN) ?  LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
  btnBState = digitalRead(BTNB_GPIO_PIN) ?  LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
  auto event = EventType::LVGL_INDEV_READ;
  xQueueSendToBackFromISR(mainEventQueue, &event, nullptr);
}

void lvgl_btna_indev_read(lv_indev_t* indev, lv_indev_data_t* data) {
  data->btn_id = 0;
  data->state = btnAState;
}

void lvgl_btnb_indev_read(lv_indev_t* indev, lv_indev_data_t* data) {
  data->btn_id = 0;
  data->state = btnBState;
}

TimerHandle_t lvgl_timer;

lv_indev_t* btnA;
lv_indev_t* btnB;

void lvgl_btna_cb(lv_event_t* event) {
  Serial.print("button A event: ");
  Serial.println(lv_event_code_get_name(lv_event_get_code(event)));
}

void lvgl_btnb_cb(lv_event_t* event) {
  Serial.print("button B event: ");
  Serial.println(lv_event_code_get_name(lv_event_get_code(event)));
}

void setup() {
  Serial.begin(115200);

  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.begin();
  M5.Display.setBrightness(50);

  lv_init();
  lv_tick_set_cb(lvgl_tick_cb);

  lv_display_t* disp;

  disp = lv_display_create(DISPLAY_RES_H, DISPLAY_RES_V);
  lv_display_set_flush_cb(disp, lvgl_disp_flush);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  btnA = lv_indev_create();
  lv_indev_set_type(btnA, LV_INDEV_TYPE_BUTTON);
  lv_indev_set_read_cb(btnA, lvgl_btna_indev_read);
  lv_indev_set_mode(btnA, LV_INDEV_MODE_EVENT);
  static const lv_point_t btn_points[1] = { {0, 0} };
  lv_indev_set_button_points(btnA, btn_points);
  lv_indev_add_event_cb(btnA, lvgl_btna_cb, LV_EVENT_ALL, nullptr);

  btnB = lv_indev_create();
  lv_indev_set_type(btnB, LV_INDEV_TYPE_BUTTON);
  lv_indev_set_read_cb(btnB, lvgl_btnb_indev_read);
  lv_indev_set_mode(btnB, LV_INDEV_MODE_EVENT);
  lv_indev_set_button_points(btnB, btn_points);
  lv_indev_add_event_cb(btnB, lvgl_btnb_cb, LV_EVENT_ALL, nullptr);

  lv_obj_t* label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "M5StickS3");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  mainEventQueue = xQueueCreate(10, sizeof(EventType));

  attachInterrupt(digitalPinToInterrupt(BTNA_GPIO_PIN), button_changed_interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTNB_GPIO_PIN), button_changed_interrupt, CHANGE);

  lvgl_timer = xTimerCreate("lvgl_timer", 100, pdFALSE, 0, lvgl_timer_cb);
  xTimerStart(lvgl_timer, 0);
}

void loop() {
  EventType receivedEvent;
  if(xQueueReceive(mainEventQueue, &receivedEvent, portMAX_DELAY) == pdPASS)
  {
    switch(receivedEvent) {
      case EventType::LVGL_INDEV_READ:
        lv_indev_read(btnA);
        lv_indev_read(btnB);
        break;
      case EventType::LVGL_TIMER_HANDLER:
        break;
    }
    uint32_t time_till_next = lv_timer_handler();
    xTimerChangePeriod(lvgl_timer, time_till_next, 0);
    xTimerReset(lvgl_timer, 0);
  }
}