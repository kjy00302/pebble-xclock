#include <pebble.h>

const int MINOR_TICK_FRACT = 95;
const int SECOND_HAND_FRACT = 90;
const int MINUTE_HAND_FRACT = 70;
const int HOUR_HAND_FRACT = 40;
const int HAND_WIDTH_FRACT = 7;

static Window *s_window;
static Layer *s_face_layer;

static GPoint s_hand_points[3];
static const GPathInfo s_hand_path_info = {
  .num_points = 3,
  .points = s_hand_points
};
static GPath *s_hand_path;

static void clock_angle(int32_t tick_units, int32_t *sin, int32_t *cos) {
  int32_t angle = tick_units * TRIG_MAX_ANGLE / 3600;
  *sin = sin_lookup(angle);
  *cos = cos_lookup(angle);
}

static int16_t x_coord(int32_t x) {
  return ((x * 144 * 45 / 100) / TRIG_MAX_RATIO) + 72;
}

static int16_t y_coord(int32_t y) {
  return ((y * 144 * 45 / 100) / TRIG_MAX_RATIO) + 84;
}

static void render_rotate(GPoint *out, int32_t x, int32_t y, int32_t s, int32_t c) {
  out->x = x_coord(x * c - y * s);
  out->y = y_coord(x * c + y * s);
} 

static void draw_clock_face(GContext *ctx) {
  for (int i=0; i < 60; i++) {
    int32_t s, c;
    GPoint p1, p2;
    clock_angle(i * 60, &s, &c);
    p1.x = x_coord(c);
    p1.y = y_coord(s);
    if (i % 5) {
      p2.x = x_coord(c * MINOR_TICK_FRACT / 100);
      p2.y = y_coord(s * MINOR_TICK_FRACT / 100);
    } else {
      p2.x = x_coord(c * SECOND_HAND_FRACT / 100);
      p2.y = y_coord(s * SECOND_HAND_FRACT / 100);
    }
    graphics_draw_line(ctx, p1, p2);
  }
}

static void draw_hand(GContext *ctx, int32_t tick_units, int32_t size) {
  int16_t outer_x = (size * 144 * 45 / 10000) + 1;
  int16_t inner_y = (HAND_WIDTH_FRACT * 144 * 45 / 10000) + 1;
  s_hand_points[0].x = 0;
  s_hand_points[0].y = -outer_x;
  s_hand_points[1].x = inner_y;
  s_hand_points[1].y = inner_y;
  s_hand_points[2].x = -inner_y;
  s_hand_points[2].y = inner_y;
  gpath_rotate_to(s_hand_path, tick_units * TRIG_MAX_ANGLE / 3600);
  //gpath_draw_outline(ctx, s_hand_path);
  gpath_draw_filled(ctx, s_hand_path);
}

static void face_update(Layer *layer, GContext *ctx) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_hand_path, center);
  draw_hand(ctx, tick_time->tm_hour * 300 + tick_time->tm_min * 5, HOUR_HAND_FRACT);
  draw_hand(ctx, tick_time->tm_min * 60, MINUTE_HAND_FRACT);
  draw_clock_face(ctx);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_hand_path = gpath_create(&s_hand_path_info);
  s_face_layer = layer_create(bounds);
  layer_set_update_proc(s_face_layer, face_update);
  layer_add_child(window_layer, s_face_layer);
}

static void prv_window_unload(Window *window) {
  gpath_destroy(s_hand_path);
  layer_destroy(s_face_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  layer_mark_dirty(s_face_layer);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
