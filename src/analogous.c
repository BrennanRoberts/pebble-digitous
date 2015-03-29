#include <pebble.h>

static Window *window;
static Layer *s_hands_layer;
static Layer *s_bg_layer;
static GPath *s_minute_path;
static GPath *s_hour_path;

static const GPathInfo HOUR_HAND_POINTS = {
  4,
  (GPoint []) {
    { -3, 10 },
    { 3, 10 },
    { 3, -40 },
    { -3, -40 }
  }
};

static const GPathInfo MINUTE_HAND_POINTS = {
  4,
  (GPoint []) {
    { -3, 10 },
    { 3, 10 },
    { 3, -70 },
    { -3, -70 }
  }
};

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
}

static void bg_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  GPoint tick;
  int32_t tick_angle;

  // minute ticks
  int16_t tick_length = bounds.size.w * 2;
  for (int i = 0; i < 60; i++) {
    if (i % 5 != 0) {
      tick_angle = TRIG_MAX_ANGLE * i / 60;
      GPoint tick = {
        .x = (int16_t)(sin_lookup(tick_angle) * (int32_t)tick_length / TRIG_MAX_RATIO) + center.x,
        .y = (int16_t)(-cos_lookup(tick_angle) * (int32_t)tick_length / TRIG_MAX_RATIO) + center.y,
      };
      graphics_context_set_stroke_color(ctx, GColorDarkGray);
      graphics_draw_line(ctx, center, tick);
    }
  }

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w * 4 / 100, bounds.size.h * 4 / 100, bounds.size.w * 92 / 100, bounds.size.h * 92 / 100), 8, GCornersAll);

  // hour ticks
  for (int i = 0; i < 12; i++) {
    tick_angle = TRIG_MAX_ANGLE * i / 12;
    GPoint tick = {
      .x = (int16_t)(sin_lookup(tick_angle) * (int32_t)tick_length / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(tick_angle) * (int32_t)tick_length / TRIG_MAX_RATIO) + center.y,
    };
    graphics_context_set_stroke_color(ctx, GColorLightGray);
    graphics_draw_line(ctx, center, tick);
  }

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w * 7 / 100, bounds.size.h * 7 / 100, bounds.size.w * 86 / 100, bounds.size.h * 86 / 100), 8, GCornersAll);
}

static void hands_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int16_t second_hand_length = bounds.size.w / 2;
  int16_t second_hand_behind_length = -bounds.size.w / 20;
  GColor second_hand_color = GColorIcterine;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_rotate_to(s_minute_path, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_path);
  gpath_draw_outline(ctx, s_minute_path);

  gpath_rotate_to(s_hour_path, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_path);
  gpath_draw_outline(ctx, s_hour_path);

  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };

  GPoint second_hand_behind = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_behind_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_behind_length / TRIG_MAX_RATIO) + center.y,
  };

  graphics_context_set_stroke_color(ctx, second_hand_color);
  graphics_draw_line(ctx, second_hand, second_hand_behind);

  graphics_context_set_fill_color(ctx, second_hand_color);
  graphics_fill_circle(ctx, center, 3);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  window_set_background_color(window, GColorBlack);

  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, bg_layer_update_proc);
  layer_add_child(window_layer, s_bg_layer);

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_layer_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_hands_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);

  s_minute_path = gpath_create(&MINUTE_HAND_POINTS);
  gpath_move_to(s_minute_path, center);

  s_hour_path = gpath_create(&HOUR_HAND_POINTS);
  gpath_move_to(s_hour_path, center);

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  gpath_destroy(s_minute_path);
  gpath_destroy(s_hour_path);
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
