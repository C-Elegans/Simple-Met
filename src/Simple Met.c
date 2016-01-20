#include <pebble.h>
#include <stdbool.h>
static Window *window;
static TextLayer *text_layer;
static int bpm;
static int delay;
static char text[4];
static bool enabled;
static const uint32_t const segments[] = { 50 };
static uint16_t taps[] = {0,0,0,0,0};
static uint8_t tapindex = 0;
bool arrayfull = false;
static time_t secs,last_secs;
static uint16_t msecs,last_msecs;
VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};
static AppTimer* timer;
static void update_bpm();
static void update_background();
void timer_callback(){
    
    
    if(enabled) vibes_enqueue_custom_pattern(pat);
    timer = app_timer_register(delay,(AppTimerCallback)timer_callback,NULL);
}
void average_bpm(){
    int total = 0;
    int i;
    for(i = 0; i<(arrayfull ? 5 : tapindex+1);i++){
        total += taps[i];
    }
    total = total /i;
    bpm = (60*1000)/total;
    update_bpm();

}
static void tap_handler(AccelAxisType axis, int32_t direction) {
    time_ms(&secs,&msecs);
    
    if(!(last_secs ==0 && last_msecs == 0)){
        int diff = secs-last_secs;
        diff *= 1000;
        diff += msecs-last_msecs;
        taps[tapindex++] = diff;
        if(tapindex>4){
            tapindex=0;
            arrayfull = true;
        }
        average_bpm();
    }
    last_secs = secs;
    last_msecs = msecs;
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  enabled = enabled ^ 1;
  update_background();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  bpm++;
  update_bpm();
  snprintf(text,sizeof(text),"%d",bpm);
  text_layer_set_text(text_layer, text);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  bpm--;
  update_bpm();
  snprintf(text,sizeof(text),"%d",bpm);
  text_layer_set_text(text_layer, text);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP , 50, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN , 50, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 70 }, .size = { bounds.size.w, 40 } });
  text_layer_set_background_color(text_layer,GColorFromRGB(255,127,127));
  text_layer_set_font(text_layer,fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

  text_layer_set_text(text_layer, "Press a button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_background_color(window,GColorFromRGB(255,127,127));
  
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}
static void update_background(){
    if(enabled)window_set_background_color(window,GColorFromRGB(127,255,127));
    else window_set_background_color(window,GColorFromRGB(255,127,127));
    if(enabled)text_layer_set_background_color(text_layer,GColorFromRGB(127,255,127));
    else text_layer_set_background_color(text_layer,GColorFromRGB(255,127,127));
}
static void update_bpm(){
    if(bpm <20)bpm=20;
    if(bpm>1000)bpm=1000;
    snprintf(text,sizeof(text),"%d",bpm);
    text_layer_set_text(text_layer, text);
    delay = (1000 * 60)/bpm;
}
int main(void) {
  bpm = 120;
  delay = 1000;
  
  enabled = false;
  init();
  update_bpm();
  //accel_tap_service_subscribe(tap_handler);
  timer = app_timer_register(delay,(AppTimerCallback)timer_callback,NULL);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
