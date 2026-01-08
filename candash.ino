#include <Arduino.h>
#define LV_COLOR_16_SWAP 0
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <string>
#include <stdexcept>

#include <EMUcan.h>
#include "driver/twai.h"

extern const lv_font_t ui_font_JBM_18; //Mono-space font
using namespace std;

// CAN pins (SN65HVD230)
#define CAN_RX 21
#define CAN_TX 4

// EMUcan object
EMUcan emuCAN(0x600);

const int backLightPin = 27;
const int buzzerPin = 22;
bool buzzerOn = false;
bool canIconSts = false;
static lv_style_t style_can;
static bool style_initialized = false;

unsigned long lastCANFrame = 0;
const unsigned long canTimeout = 2000;

LV_FONT_DECLARE(lv_font_montserrat_28);
LV_FONT_DECLARE(ui_font_JBM_18);

lv_obj_t *can_icon_label;

// Display & LVGL setup
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 20];
lv_obj_t *table;

/* ===================== FORWARD DECLARATION ===================== */
void create_table();
/* =============================================================== */

// LVGL Display Flush Callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint16_t w = area->x2 - area->x1 + 1;
  uint16_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

void update_can_icon_color(bool ok, bool firstTime) {
  if (canIconSts != ok || firstTime) {
    if (!style_initialized) {
      lv_style_init(&style_can);
      style_initialized = true;
    }
    lv_style_set_text_color(&style_can,
                            ok ? lv_color_make(0, 255, 0)
                               : lv_color_make(0, 0, 255));
    lv_obj_add_style(can_icon_label, &style_can, 0);
    canIconSts = ok;
  }
}

void create_can_icon() {
  can_icon_label = lv_label_create(lv_scr_act());
  lv_label_set_text(can_icon_label, "CAN");
  lv_obj_set_style_text_font(can_icon_label, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_align(can_icon_label, LV_ALIGN_BOTTOM_RIGHT, -3, -5);
  update_can_icon_color(false, true);
}

// Cell alignment fix
void my_table_event_cb(lv_event_t * e) {
  lv_obj_t * table = lv_event_get_target(e);
  lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);

  if (dsc->part == LV_PART_ITEMS) {
    uint16_t row = dsc->id / lv_table_get_col_cnt(table);
    uint16_t col = dsc->id % lv_table_get_col_cnt(table);

    dsc->label_dsc->font = &ui_font_JBM_18;
    dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;

    if ((row == 0 && col == 1) || (row == 0 && col == 3) ||
        (row == 1 && col == 1) || (row == 1 && col == 3) ||
        (row == 2 && col == 1) || (row == 2 && col == 3) ||
        (row == 3 && col == 1) || (row == 3 && col == 3) ||
        (row == 4 && col == 1) || (row == 4 && col == 3)) {
      dsc->label_dsc->align = LV_TEXT_ALIGN_RIGHT;
    }
    if (row == 5 && col == 1) {
      dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
    }
  }
}

static void table_event_cb_bg(lv_event_t *e) {
  lv_obj_t *table = lv_event_get_target(e);
  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);

  if (!dsc || !dsc->rect_dsc) return;

  if (dsc->part == LV_PART_ITEMS) {
    uint16_t row = dsc->id / lv_table_get_col_cnt(table);
    uint16_t col = dsc->id % lv_table_get_col_cnt(table);

    const char *value_str = lv_table_get_cell_value(table, row, col);

    float value = 0.0f;
    if (value_str && value_str[0]) {
      value = atof(value_str);   // bewusst simpel, UI-unverändert
    }

    lv_color_t bg_color = lv_color_make(30, 30, 30);
    lv_color_t text_color = lv_color_white();

    if (row == 0 && col == 1 && value > 7000.0f) bg_color = lv_color_make(0, 0, 255);
    if (row == 1 && col == 3 && value > 100.0f) bg_color = lv_color_make(0, 0, 255);
    if (row == 1 && col == 3 && value < 55.0f && value > 1.0f) {
      bg_color = lv_color_make(0, 255, 255);
      text_color = lv_color_black();
    }
    if (row == 2 && col == 3 && value < 12.0f && value > 1.0f) bg_color = lv_color_make(0, 0, 255);
    if (row == 5 && col == 1 && value_str && value_str[0]) bg_color = lv_color_make(0, 0, 255);

    dsc->rect_dsc->bg_color = bg_color;
    dsc->rect_dsc->bg_opa = LV_OPA_COVER;
    dsc->label_dsc->color = text_color;
  }
}

// Initialize LVGL Table
void create_table() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(30, 30, 30), LV_PART_MAIN);

  table = lv_table_create(lv_scr_act());
  lv_obj_align(table, LV_ALIGN_CENTER, -1, -1);
  lv_obj_set_style_text_opa(table, LV_OPA_COVER, 0);
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(table, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_text_color(table, lv_color_white(), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(table, lv_color_make(30, 30, 30), LV_PART_MAIN);

  static lv_style_t style_cell0;
  lv_style_init(&style_cell0);
  lv_style_set_pad_top(&style_cell0, 12.8);
  lv_style_set_pad_bottom(&style_cell0, 12.8);
  lv_style_set_pad_left(&style_cell0, 4);
  lv_style_set_pad_right(&style_cell0, 4);
  lv_obj_add_style(table, &style_cell0, LV_PART_ITEMS);

  lv_table_set_col_cnt(table, 4);
  lv_table_set_row_cnt(table, 6);

  lv_obj_set_style_border_width(table, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(table, lv_color_white(), LV_PART_ITEMS);
  lv_obj_set_style_border_side(table, LV_BORDER_SIDE_FULL, LV_PART_ITEMS);

  lv_table_set_col_width(table, 0, 47);
  lv_table_set_col_width(table, 1, 107);
  lv_table_set_col_width(table, 2, 47);
  lv_table_set_col_width(table, 3, 119);

  lv_table_add_cell_ctrl(table, 5, 1, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
  lv_table_add_cell_ctrl(table, 5, 2, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
  lv_table_add_cell_ctrl(table, 5, 3, LV_TABLE_CELL_CTRL_MERGE_RIGHT);

  lv_table_set_cell_value(table, 0, 0, "RPM");
  lv_table_set_cell_value(table, 0, 2, "TPS");
  lv_table_set_cell_value(table, 1, 0, "LAM");
  lv_table_set_cell_value(table, 1, 2, "CLT");
  lv_table_set_cell_value(table, 2, 0, "LT");
  lv_table_set_cell_value(table, 2, 2, "BAT");
  lv_table_set_cell_value(table, 3, 0, "OILP");
  lv_table_set_cell_value(table, 3, 2, "IAT");
  lv_table_set_cell_value(table, 4, 0, "OILT");
  lv_table_set_cell_value(table, 4, 2, "FP");
  lv_table_set_cell_value(table, 5, 0, "CEL");

  lv_obj_add_event_cb(table, my_table_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
  lv_obj_add_event_cb(table, table_event_cb_bg, LV_EVENT_DRAW_PART_BEGIN, NULL);

  create_can_icon();
  lv_timer_handler();
}

void setup() {
  tft.init();
  pinMode(backLightPin, OUTPUT);
  digitalWrite(backLightPin, LOW);
  uint16_t darkGray = ((30 & 0xF8) << 8) | ((30 & 0xFC) << 3) | (30 >> 3);
  tft.fillScreen(darkGray);
  tft.setRotation(1);
  Serial.begin(1000000);

  pinMode(buzzerPin, OUTPUT);

  lv_init();
  lv_refr_now(NULL);
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 320;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();

  uint32_t alerts =
    TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
  twai_reconfigure_alerts(alerts, NULL);

  create_table();
  digitalWrite(backLightPin, HIGH);
}

void loop() {
  uint32_t alerts_triggered;
  twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(1));

  if (alerts_triggered & TWAI_ALERT_RX_DATA) {
    twai_message_t msg;
    while (twai_receive(&msg, 0) == ESP_OK) {
      emuCAN.checkEMUcan(msg.identifier, msg.data_length_code, msg.data);
      lastCANFrame = millis();
      update_can_icon_color(true, false);
    }
  }

  if (millis() - lastCANFrame > canTimeout)
    update_can_icon_color(false, false);

  lv_table_set_cell_value(table, 0, 1, String(emuCAN.emu_data.RPM).c_str());
  lv_table_set_cell_value(table, 0, 3, (String(emuCAN.emu_data.TPS) + " %").c_str());
  lv_table_set_cell_value(table, 1, 1, String(emuCAN.emu_data.wboLambda, 2).c_str());
  lv_table_set_cell_value(table, 1, 3, (String(emuCAN.emu_data.CLT) + " °C").c_str());
  lv_table_set_cell_value(table, 2, 1, String(emuCAN.emu_data.lambdaTarget, 2).c_str());
  lv_table_set_cell_value(table, 2, 3, (String(emuCAN.emu_data.Batt) + " V").c_str());
  lv_table_set_cell_value(table, 3, 1, (String(emuCAN.emu_data.oilPressure) + " BAR").c_str());
  lv_table_set_cell_value(table, 3, 3, (String(emuCAN.emu_data.IAT) + " °C").c_str());
  lv_table_set_cell_value(table, 4, 1, (String(emuCAN.emu_data.oilTemperature) + " °C").c_str());
  lv_table_set_cell_value(table, 4, 3, (String(emuCAN.emu_data.fuelPressure) + " BAR").c_str());

  if (emuCAN.decodeCel())
    lv_table_set_cell_value(table, 5, 1, "CEL ACTIVE");
  else
    lv_table_set_cell_value(table, 5, 1, "");

  buzzerOn =
    emuCAN.decodeCel() ||
    emuCAN.emu_data.CLT > 105 ||
    emuCAN.emu_data.RPM > 7000 ||
    (emuCAN.emu_data.Batt < 12.0 && emuCAN.emu_data.Batt > 1.0) ||
    emuCAN.emu_data.fuelPressure < 2.0;

  digitalWrite(buzzerPin, (millis() % 600 < 300) && buzzerOn);

  lv_obj_invalidate(table);
  lv_timer_handler();
}
