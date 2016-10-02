#include <pebble.h>
#define array_length 5
#define TIMESTAMP 1
typedef struct GTextAttributes GTextAttributes;
static Window *s_main_window;
//static ClickConfigProvider *config_provider;
static TextLayer *s_text_layer;
static GBitmap *s_bitmap;
static BitmapLayer *s_layer;
static Layer *window_layer;
static int countDownTimer = 0;
static TextLayer *arr[array_length];
static DataLoggingSessionRef logging_session;
static uint32_t num_samples = 10;
static uint32_t samples_taken = 0;
static int SQRT_EST_LOOP_ITERATIONS = 15;
static float SQRT_EST_ERROR = 0.01;
static int last;
static bool dataIsLog = false;

enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1
};

static void tick_handler(struct tm *tick_time, TimeUnits changed);

float my_sqrt( float num ) {

    float init, product;
    init = num;
    product = num * num;
    for ( int iter = 0 ; ( iter < SQRT_EST_LOOP_ITERATIONS ) && ( product - num >= SQRT_EST_ERROR ) ; iter++ ) {

        init = ((init + (num / init)) / 2);
        product = init * init;
    }
    return init;
}

float magnitude( int x, int y, int z ) {

    return my_sqrt( x * x + y * y + z * z );
}

//Accelerometer handling for punch
static void accel_data_handler( AccelData *data, uint32_t num_samples ) {

	// Read sample 0's x, y, and z values
    int16_t x = data[0].x;
    int16_t y = data[0].y;
    int16_t z = data[0].z;
	
    // Determine if the sample occured during vibration, and when it occured
    bool did_vibrate = data[0].did_vibrate;
    uint64_t timestamp = data[0].timestamp;

    if (!did_vibrate) {
		
        int mag = magnitude(x, y, z);
		APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d, mag: %d, delta: %d", timestamp, x, y, z, mag, mag - last);
		APP_LOG(APP_LOG_LEVEL_INFO, "LOG: %d data: %d", data_logging_log(logging_session, &mag, 1) , mag);
        last = mag;
		samples_taken ++;
    }
    else
		
        APP_LOG(APP_LOG_LEVEL_WARNING, "Vibration occured during collection");
	
	if ( samples_taken == num_samples ) { 
		dataIsLog = true;
		//accel_data_service_unsubscribe ();
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
		data_logging_finish(logging_session);
    
		samples_taken = 0;
		APP_LOG(APP_LOG_LEVEL_INFO, "Logging finished" );
	}
}

static void main_window_load(Window *window) {

  window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // We do this to account for the offset due to the status bar
  // at the top of the app window.
  

  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BEST_FIST);

  GPoint center = grect_center_point(&bounds);

  GSize image_size = gbitmap_get_bounds(s_bitmap).size;

  GRect image_frame = GRect(center.x, center.y, image_size.w, image_size.h);
  image_frame.origin.x -= image_size.w / 2;
  image_frame.origin.y -= image_size.h / 2;

  // Use GCompOpOr to display the white portions of the image
  s_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_bitmap(s_layer, s_bitmap);
  bitmap_layer_set_compositing_mode(s_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_layer));
  layer_mark_dirty(window_get_root_layer(s_main_window));
  
  
  GFont font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  GFont font2 = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  
  //sets the text layers for the countdown
  arr[0] = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));;
  text_layer_set_text(arr[0], "3");
  text_layer_set_text_alignment(arr[0], GTextAlignmentCenter);
  text_layer_set_font(arr[0], font);
 
  arr[1] = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));;
  text_layer_set_text(arr[1], "2");
  text_layer_set_text_alignment(arr[1], GTextAlignmentCenter);
  text_layer_set_font(arr[1], font);
  
  arr[2] = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));;
  text_layer_set_text(arr[2], "1");
  text_layer_set_text_alignment(arr[2], GTextAlignmentCenter);
  text_layer_set_font(arr[2], font);
  
  arr[3] = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));;
  text_layer_set_text(arr[3], "PUNCH!");
  text_layer_set_text_alignment(arr[3], GTextAlignmentCenter);
  text_layer_set_font(arr[3], font2);
 
} 

static void main_window_unload(Window *window) {
 // bitmap_layer_destroy(s_layer);
 // gbitmap_destroy(s_bitmap);
  
  text_layer_destroy(arr[0]);
  text_layer_destroy(arr[1]);
  text_layer_destroy(arr[2]);
  text_layer_destroy(arr[3]);  

}



static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  if(dataIsLog){
    accel_data_service_unsubscribe ();
    tick_timer_service_unsubscribe();
    APP_LOG(APP_LOG_LEVEL_INFO, "unsubbed both" );
    window_stack_pop_all(true);
  }
  else{
  if(countDownTimer == 3){
    vibes_short_pulse();
    tick_timer_service_unsubscribe();
    accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);
	  logging_session = data_logging_create(TIMESTAMP, DATA_LOGGING_INT, sizeof(int), true); 
    accel_data_service_subscribe(num_samples, accel_data_handler);
  }
  if(countDownTimer == 0){
    bitmap_layer_destroy(s_layer);
    gbitmap_destroy(s_bitmap);
    
  }
  layer_add_child(window_layer, text_layer_get_layer(arr[countDownTimer]));
  layer_mark_dirty(window_get_root_layer(s_main_window));
  countDownTimer++;
  }
  
}

static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Window *window = (Window *)context;
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void click_config_provider(void *context) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
}

static void init(void) {
  
  s_main_window = window_create();
  
  window_set_click_config_provider(s_main_window, click_config_provider);
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(s_main_window, true);
  
  APP_LOG(APP_LOG_LEVEL_INFO, "id: %d:",BUTTON_ID_SELECT);
  
 
	/*
  // Create a text layer and set the text
	s_text_layer = text_layer_create(bounds);
	text_layer_set_text(s_text_layer, "Hi, I'm a Pebble! 2");
  
  // Set the font and text alignment
	text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);

	// Add the text layer to the window
	layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_text_layer));
  
  // Enable text flow and paging on the text layer, with a slight inset of 10, for round screens
  text_layer_enable_screen_text_flow_and_paging(s_text_layer, 10);
*/
	// Push the window, setting the window animation to 'true'
	
  
  // accel_tap_service_subscribe(accel_tap_handler);
 // accel_data_service_subscribe(3, accel_data_handler);
	
	// App Logging!
//	APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");
  
}

static void deinit(void) {
	// Destroy the text layer
	//text_layer_destroy(s_text_layer);
	
	// Destroy the window
  //app_message_deregister_callbacks();
 
	window_destroy(s_main_window);
 
 // accel_tap_service_unsubscribe();
  //gbitmap_destroy(s_bitmap);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}

