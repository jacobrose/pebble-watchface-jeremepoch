#include <pebble.h>

static Window *s_main_window;
static GBitmap *jeremy_bitmap;
static GBitmap *jeremy_jaw_white_bitmap;
static GBitmap *jeremy_jaw_black_bitmap;
static BitmapLayer *jeremy_layer;
static BitmapLayer *jaw_black_layer;
static BitmapLayer *jaw_white_layer;
static PropertyAnimation *jaw_black_animation;
static PropertyAnimation *jaw_white_animation;
static TextLayer *time_layer;
static GRect *jaw_start_bounds;
static GRect *jaw_end_bounds;
static bool time_shown = false;

#define JAW_ANIMATION_LENGTH 1000

#define CLOSED_JAW_X 40
#define CLOSED_JAW_Y 83

#define OPEN_JAW_X 40
#define OPEN_JAW_Y 104
  
#define TIME_X 40
#define TIME_Y 80
  

// AnimationStartedHandler: noop
static void noop(Animation *animation, void *data) {
}

// AnimationStoppedHandler: noop
static void noop_afterward(Animation *animation, bool finished, void *data) {
}

// Update the text layer that shows the time
static void set_time() {
  time_t epoch = time(NULL);
  struct tm *posixtime = localtime(&epoch);
  
  // must be static, because time_layer references this!
  static char time_buffer[] = "00:00";
  
  if (clock_is_24h_style()) {
    strftime(time_buffer, sizeof("00:00"), "%H:%M", posixtime);
  } else {
    strftime(time_buffer, sizeof("00:00"), "%I:%M", posixtime);
  }
  
  text_layer_set_text(time_layer, time_buffer);
}

// AnimationStoppedHandler: when the jaw is fully opened, the time is revealed
static void show_time(Animation *animation, bool finished, void *data) {
  time_shown = true;
  set_time();
  layer_set_hidden(text_layer_get_layer(time_layer), false);
}

static void start_animation(uint32_t delay, GRect *start_position, GRect *end_position, AnimationStoppedHandler do_afterward) {
  // Set up animation for both layers of the jaw
  jaw_black_animation = property_animation_create_layer_frame(
    bitmap_layer_get_layer(jaw_black_layer),
    start_position,
    end_position
  );
  animation_set_duration((Animation *)jaw_black_animation, JAW_ANIMATION_LENGTH);
  animation_set_curve((Animation *)jaw_black_animation, AnimationCurveEaseInOut);
  
  jaw_white_animation = property_animation_create_layer_frame(
    bitmap_layer_get_layer(jaw_white_layer),
    start_position,
    end_position
  );
  animation_set_duration((Animation *)jaw_white_animation, JAW_ANIMATION_LENGTH);
  animation_set_curve((Animation *)jaw_white_animation, AnimationCurveEaseInOut);

  if (delay > 0) {
    animation_set_delay((Animation *)jaw_white_animation, delay);
    animation_set_delay((Animation *)jaw_black_animation, delay);
  }
  
  // set up an event to run when this finishes
  animation_set_handlers(
    (Animation*)jaw_black_animation,
    (AnimationHandlers) {
      .started = (AnimationStartedHandler)noop,
      .stopped = (AnimationStoppedHandler)do_afterward,
    },
    NULL
  );
  
  animation_schedule((Animation *)jaw_black_animation);
  animation_schedule((Animation *)jaw_white_animation);
}

// AnimationStoppedHandler: animate the mouth opening, trigger time display
static void open_mouth(Animation *animation, bool finished, void *data) {
  layer_set_hidden(text_layer_get_layer(time_layer), true);
  start_animation(0, jaw_start_bounds, jaw_end_bounds, show_time);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_frame(window_layer);

  jeremy_bitmap = gbitmap_create_with_resource(RESOURCE_ID_jeremy_background);
  jeremy_layer = bitmap_layer_create(window_bounds);
  bitmap_layer_set_bitmap(jeremy_layer, jeremy_bitmap);
 
  jeremy_jaw_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_jeremy_jaw_WHITE);
  jeremy_jaw_black_bitmap = gbitmap_create_with_resource(RESOURCE_ID_jeremy_jaw_BLACK);
  
  // Pick up the size from either of the jaw bitmaps
  jaw_start_bounds = (GRect *)malloc(sizeof(GRect));
  *jaw_start_bounds = jeremy_jaw_white_bitmap->bounds;
  // Set start position
  jaw_start_bounds->origin.x = CLOSED_JAW_X;
  jaw_start_bounds->origin.y = CLOSED_JAW_Y;
  // Set end position
  jaw_end_bounds = (GRect *)malloc(sizeof(GRect));
  *jaw_end_bounds = *jaw_start_bounds;
  jaw_end_bounds->origin.y = OPEN_JAW_Y;
  
  // Make the time layer the same size as the jaw (sloppy!)
  GRect time_bounds = *jaw_start_bounds;
  time_bounds.origin.x = TIME_X;
  time_bounds.origin.y = TIME_Y;
  time_layer = text_layer_create(time_bounds);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text(time_layer, "00:00");
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer(time_layer), true);
  
  jaw_black_layer = bitmap_layer_create(*jaw_start_bounds);
  bitmap_layer_set_bitmap(jaw_black_layer, jeremy_jaw_black_bitmap);
  bitmap_layer_set_compositing_mode(jaw_black_layer, GCompOpClear);
  
  jaw_white_layer = bitmap_layer_create(*jaw_start_bounds);
  bitmap_layer_set_bitmap(jaw_white_layer, jeremy_jaw_white_bitmap);
  bitmap_layer_set_compositing_mode(jaw_white_layer, GCompOpOr);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(jeremy_layer));
  layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(time_layer));
  set_time();

  layer_add_child(window_layer, bitmap_layer_get_layer(jaw_black_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(jaw_white_layer));

  start_animation(1000, jaw_start_bounds, jaw_end_bounds, show_time);
}

static void main_window_unload(Window *window) {
  gbitmap_destroy(jeremy_bitmap);
  gbitmap_destroy(jeremy_jaw_white_bitmap);
  gbitmap_destroy(jeremy_jaw_black_bitmap);
  bitmap_layer_destroy(jeremy_layer);
  bitmap_layer_destroy(jaw_black_layer);
  bitmap_layer_destroy(jaw_white_layer);
  text_layer_destroy(time_layer);
  
  free(jaw_start_bounds);
  free(jaw_end_bounds);

  animation_destroy((Animation *)jaw_black_animation);
  animation_destroy((Animation *)jaw_white_animation);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Don't start ticking until we've shown the time once
  if (time_shown) {
    start_animation(0, jaw_end_bounds, jaw_start_bounds, open_mouth);
  }
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(
    s_main_window,
    (WindowHandlers) {
      .load = main_window_load,
      .unload = main_window_unload,
    }
  );
  window_stack_push(s_main_window, true /* animate */);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}