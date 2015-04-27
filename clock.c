#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_output_layer;

void send_int(int key, int cmd);

bool currentlyRecording = false;

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!currentlyRecording) {
      text_layer_set_text(s_output_layer, "Recording Started!");
  static const uint32_t const segments[] = { 200, 100, 400 };
   VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};
vibes_enqueue_custom_pattern(pat);    
  } else {
    text_layer_set_text(s_output_layer, "Recording Stopped!");
  static const uint32_t const segments[] = { 000, 100, 300 };
   VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};
vibes_enqueue_custom_pattern(pat);    
  }
  currentlyRecording = !currentlyRecording;
  send_int(0, 0);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *conatext) {
  text_layer_set_text(s_output_layer, "Distress Activiated!");
  static const uint32_t const segments[] = { 1000, 000, 300 };
   VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};
vibes_enqueue_custom_pattern(pat);    
  send_int(0, 1);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_output_layer, "Video Recording!");
  static const uint32_t const segments[] = { 050, 000, 050 };
   VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};
vibes_enqueue_custom_pattern(pat);    
  send_int(0, 2);
}


static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void send_int(int key, int cmd)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
 
    Tuplet value = TupletInteger(key, cmd);
    dict_write_tuplet(iter, &value);
 
    app_message_outbox_send();
}


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == false) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void main_window_load(Window *window) {
   Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  // Create time TextLayer
  s_output_layer = text_layer_create(GRect(5, 0, window_bounds.size.w - 5, window_bounds.size.h / 5));
  s_time_layer = text_layer_create(GRect(0, 55, 144, 50));
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(s_output_layer, "...");
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_overflow_mode(s_output_layer, GTextOverflowModeWordWrap);
  
  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
//text_layer_destroy(s_output_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  text_layer_set_text(s_output_layer, "");
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
window_set_click_config_provider(s_main_window, click_config_provider);
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
