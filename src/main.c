#include <pebble.h>

static Window *main_window, *s_menu_window, *config_window;
static MenuLayer *s_menu_layer;
TextLayer *output_layer, *number_layer;

#define SCREEN_TEXT_GAP 14

// Android Communication
#define REQUEST_LOCATION                0
#define REQUEST_FIX_LOCATION            1
#define REQUEST_START_THREADED_LOCATION 2
#define REQUEST_STOP_THREADED_LOCATION  3
#define REQUEST_ELEVATION               4
#define REQUEST_WEATHER_STATUS          5
#define REQUEST_WEATHER_TEMPERATURE     6
#define REQUEST_WEATHER_PRESSURE        7
#define REQUEST_WEATHER_HUMIDITY        8
#define REQUEST_WEATHER_WIND            9
#define REQUEST_WEATHER_SUNRISE        10
#define REQUEST_WEATHER_SUNSET         11
#define REQUEST_TRANSPORT              12
#define SHOW_UP_TIME                   13
#define SHOW_ACTIVE_TIME               14
#define SHOW_BATTERY_STATE             15

#define NUMBER_OF_ITEMS                16


// Pebble KEY
#define PEBBLE_KEY_VALUE        1
// Location APIpersist_write
#define KEY_LATITUDE        100
#define KEY_LONGITUDE       101
#define KEY_DISTANCE        102
#define KEY_DIRECTION       103
// Elevation API
#define KEY_ALTITUDE        200
// Weather API
#define KEY_STATUS          300
#define KEY_DESCRIPTION     301
#define KEY_TEMPERATURE     302
#define KEY_PRESSURE        303
#define KEY_HUMIDITY        304
#define KEY_WIND_SPEED      305
#define KEY_WIND_DIRECTION  306
#define KEY_SUNRISE         307
#define KEY_SUNSET          308
// Transport API
#define KEY_DEPARTURE       400
#define KEY_DEPARTURE_TIME  401
#define KEY_ARRIVAL         402
#define KEY_ARRIVAL_TIME    403


#define MAX_TEXT_SIZE       128
#define NUM_ACCEL_SAMPLES   10
#define GRAVITY             10000 // (1g)² = 10000
#define ACCEL_THRESHOLD     8000  // (1g)² = 10000

typedef struct {
  char name[16];  // Name of this tea
} ScreenInfo;

enum {
  PERSIST_SCREEN1,
  PERSIST_SCREEN2,
  PERSIST_SCREEN3,
  PERSIST_SCREEN4
};


ScreenInfo screen_array[] = {
   {"SCREEN 1"},
   {"SCREEN 2"},
   {"SCREEN 3"},
   {"SCREEN 4"}
};

int currentScreen = 0;

int counter = -1;

int nbItem = 0;

char text[MAX_TEXT_SIZE];
unsigned long int up_time = 0;      //in seconds
unsigned long int active_time = 0;  //in seconds/10

static char s_screen_text[32];

// menu select
static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, 
                            void *callback_context) {

  // Switch to config window
  currentScreen = cell_index->row;
  window_stack_push(config_window, false);
}

static void back_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, 
                            void *callback_context) {
  strcpy(text, "");
  text_layer_set_text(output_layer, text);
  window_stack_pop(true);   
}


void send(int key, char *value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, key, value);
  app_message_outbox_send();
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (counter ==  SHOW_UP_TIME) {
    // Get time since launch
    int seconds = up_time % 60;
    int minutes = (up_time % 3600) / 60;
    int hours = up_time / 3600;

    snprintf(text, MAX_TEXT_SIZE, "Uptime:\n%dh %dm %ds", hours, minutes, seconds);
    text_layer_set_text(output_layer, text);
  }
  else if (counter == SHOW_BATTERY_STATE) {
    BatteryChargeState charge_state = battery_state_service_peek();

    if (charge_state.is_charging) {
      snprintf(text, MAX_TEXT_SIZE, "Battery is charging");
    }
    else {
      snprintf(text, MAX_TEXT_SIZE, "Battery is\n%d%% charged", charge_state.charge_percent);
    }
    text_layer_set_text(output_layer, text);
  }  
  
  // Increment uptime
  up_time++;
}

 
uint16_t num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
  return 4;
}

static void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, 
                             void *callback_context) {
  char* name = screen_array[cell_index->row].name;

  menu_cell_basic_draw(ctx, cell_layer, name, NULL, NULL);
}

static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, 
                                        void *callback_context) {
  return 60;
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = num_rows_callback,
    .get_cell_height = get_cell_height_callback,
    .draw_row = draw_row_handler,
    .select_click = select_callback
  }); 
  menu_layer_set_click_config_onto_window(s_menu_layer,	window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void menu_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}


static void data_handler(AccelData *data, uint32_t num_samples) {  // accel from -4000 to 4000, 1g = 1000 cm/s²
  if (counter == SHOW_ACTIVE_TIME) {
    int i, x, y, z, acc_norm_2;
    for (i = 0; i < NUM_ACCEL_SAMPLES; i++) {
                                             // Divide by 10 to avoid too high values. Now from -400 to 400
      x = data[i].x / 10;                    // accel in dm/s²
      y = data[i].y / 10;                    // accel in dm/s²
      z = data[i].z / 10;                    // accel in dm/s²
                                             // 1g = 100 dm/s²  
      acc_norm_2 = (x*x) + (y*y) + (z*z);    // (1g)² = 10000
      //APP_LOG(APP_LOG_LEVEL_INFO, "%d %d %d %d", x, y, z, acc_norm_2);
      if ( ((acc_norm_2 - GRAVITY) > ACCEL_THRESHOLD) || ((GRAVITY - acc_norm_2) > ACCEL_THRESHOLD) ) {
        active_time++;
      }
    }
    
    int active_time_s = active_time / 10;
    int seconds = active_time_s % 60;
    int minutes = (active_time_s % 3600) / 60;
    int hours = active_time_s / 3600;

    snprintf(text, MAX_TEXT_SIZE, "Active time:\n%dh %dm %ds", hours, minutes, seconds);
    text_layer_set_text(output_layer, text);
  }
}

void received_handler(DictionaryIterator *iter, void *context) {
  Tuple *result_tuple = dict_find(iter, PEBBLE_KEY_VALUE);
  switch(result_tuple->value->int32) {
    // Location API
    case REQUEST_LOCATION:
      strcpy(text, "lat : ");
      strcat(text, dict_find(iter, KEY_LATITUDE)->value->cstring);
      strcat(text, "\nlon : ");
      strcat(text, dict_find(iter, KEY_LONGITUDE)->value->cstring);
      break;
    case REQUEST_START_THREADED_LOCATION:
      strcpy(text, "distance : ");
      strcat(text, dict_find(iter, KEY_DISTANCE)->value->cstring);
      strcat(text, "\ndirection : ");
      strcat(text, dict_find(iter, KEY_DIRECTION)->value->cstring);
      break;
    // Elevation API
    case REQUEST_ELEVATION:
      strcpy(text, "altitude : ");
      strcat(text, dict_find(iter, KEY_ALTITUDE)->value->cstring);
      strcat(text, "m");
      break;
    // Weather API
    case REQUEST_WEATHER_STATUS:
      strcpy(text, dict_find(iter, KEY_STATUS)->value->cstring);
      strcat(text, "\n");
      strcat(text, dict_find(iter, KEY_DESCRIPTION)->value->cstring);
      break;
    case REQUEST_WEATHER_TEMPERATURE:
      strcpy(text, dict_find(iter, KEY_TEMPERATURE)->value->cstring);
      strcat(text, "°C");
      break;
    case REQUEST_WEATHER_PRESSURE:
      strcpy(text, "pressure : ");
      strcat(text, dict_find(iter, KEY_PRESSURE)->value->cstring);
      break;
    case REQUEST_WEATHER_HUMIDITY:
      strcpy(text, "humidity : ");
      strcat(text, dict_find(iter, KEY_HUMIDITY)->value->cstring);
      break;
    case REQUEST_WEATHER_WIND:
      strcpy(text, "wind speed : ");
      strcat(text, dict_find(iter, KEY_WIND_SPEED)->value->cstring);
      strcat(text, "km/h\nwind direction : ");
      strcat(text, dict_find(iter, KEY_WIND_DIRECTION)->value->cstring);
      break;
    case REQUEST_WEATHER_SUNRISE:
      strcpy(text, "sunrise : \n");
      strcat(text, dict_find(iter, KEY_SUNRISE)->value->cstring);
      break;
    case REQUEST_WEATHER_SUNSET:
      strcpy(text, "sunset : \n");
      strcat(text, dict_find(iter, KEY_SUNSET)->value->cstring);
      break;
    // Transport API
    case REQUEST_TRANSPORT:
      strcpy(text, dict_find(iter, KEY_DEPARTURE)->value->cstring);
      strcat(text, " : ");
      strcat(text, dict_find(iter, KEY_DEPARTURE_TIME)->value->cstring);
      strcat(text, "\n");
      strcat(text, dict_find(iter, KEY_ARRIVAL)->value->cstring);
      strcat(text, " : ");
      strcat(text, dict_find(iter, KEY_ARRIVAL_TIME)->value->cstring);
      break;
    default:
      strcpy(text, "Error.\nPlease check your dictionary KEYS");
      break;
  }
  text_layer_set_text(output_layer, text);
}

// Select action
void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  strcpy(text, "");
  text_layer_set_text(output_layer, text);
  menu_window_load(main_window);
}

// Up action
void up_click_config_handler(ClickRecognizerRef recognizer, void *context) {
  if (nbItem + 1 > 15) {
    nbItem = 0;
  } else {
    nbItem = nbItem + 1;
  }
  //APP_LOG(APP_LOG_LEVEL_INFO, "UP : Sending request id : %d", nbItem);
  
  int val = -1;
  switch(currentScreen) {
    case 0:
      if (persist_exists(PERSIST_SCREEN1)) {
        val = persist_read_int(PERSIST_SCREEN1);
      }
    break;
    case 1:
      if (persist_exists(PERSIST_SCREEN2)) {
        val = persist_read_int(PERSIST_SCREEN2);
      }
    break;
    case 2:
      if (persist_exists(PERSIST_SCREEN3)) {
      val = persist_read_int(PERSIST_SCREEN3);
      }
    break;
    case 3:
    if (persist_exists(PERSIST_SCREEN4)) {
      val = persist_read_int(PERSIST_SCREEN4);
    }
    break;
  }
  
   //APP_LOG(APP_LOG_LEVEL_INFO, "Before UP valid, %d, %d == %d", val, nbItem, val);
  
  if (val != -1 && nbItem == val) {
    text_layer_set_text_color(output_layer, GColorWhite);
    text_layer_set_background_color(output_layer, GColorBlack);
    window_set_background_color(config_window, GColorBlack);
  } else {
    text_layer_set_text_color(output_layer, GColorBlack);
    text_layer_set_background_color(output_layer, GColorWhite);
    window_set_background_color(config_window, GColorWhite);
  }
 
  switch (nbItem) {
    case REQUEST_LOCATION:
      strcpy(text, "LOCATION");
      break;
    case REQUEST_FIX_LOCATION:
      strcpy(text, "FIXING TARGET");
      break;
    case REQUEST_START_THREADED_LOCATION:
      strcpy(text, "START THREAD NAVIGATION");
      break;
    case REQUEST_STOP_THREADED_LOCATION:
      strcpy(text, "STOP THREAD NAVIGATION");
      break;
    case REQUEST_ELEVATION:
      strcpy(text, "ELEVATION");
      break;
    case REQUEST_WEATHER_STATUS:
      strcpy(text, "WEATHER_STATUS");
      break;
    case REQUEST_WEATHER_TEMPERATURE:
      strcpy(text, "TEMPERATURE");
      break;
    case REQUEST_WEATHER_PRESSURE:
      strcpy(text, "PRESSURE");
      break;
    case REQUEST_WEATHER_HUMIDITY:
      strcpy(text, "HUMIDITY");
      break;
    case REQUEST_WEATHER_WIND:
      strcpy(text, "WIND");
      break;
    case REQUEST_WEATHER_SUNRISE:
      strcpy(text, "SUNRISE");
      break;
    case REQUEST_WEATHER_SUNSET:
      strcpy(text, "SUNSET");
      break;
    case REQUEST_TRANSPORT:
      strcpy(text, "TRANSPORT");
      break;
    case SHOW_UP_TIME:
      strcpy(text, "SHOW_UP_TIME");
      break;
    case SHOW_ACTIVE_TIME:
      strcpy(text, "SHOW_ACTIVE_TIME");
      break;
    case SHOW_BATTERY_STATE:
      strcpy(text, "SHOW_BATTERY_STATE");
      break;
    default:
      strcpy(text, "Error.\nPlease check if NUMBER_OF_ITEMS is OK");
      break;
  }
  text_layer_set_text(output_layer, text);
  text_layer_set_text_alignment(output_layer, GTextAlignmentCenter);
}

// down click
void down_click_config_handler(ClickRecognizerRef recognizer, void *context) {
  if (nbItem - 1 < 0) {
    nbItem = 15;
  } else {
    nbItem = nbItem - 1;
  }
  //APP_LOG(APP_LOG_LEVEL_INFO, "DOWN : Sending request id : %d", nbItem);
  int val = -1;
  switch(currentScreen) {
    case 0:
      if (persist_exists(PERSIST_SCREEN1)) {
        val = persist_read_int(PERSIST_SCREEN1);
      }
    break;
    case 1:
      if (persist_exists(PERSIST_SCREEN2)) {
        val = persist_read_int(PERSIST_SCREEN2);
      }
    break;
    case 2:
      if (persist_exists(PERSIST_SCREEN3)) {
      val = persist_read_int(PERSIST_SCREEN3);
      }
    break;
    case 3:
    if (persist_exists(PERSIST_SCREEN4)) {
      val = persist_read_int(PERSIST_SCREEN4);
    }
    break;
  }
  
  if (val != -1 && nbItem == val) {
    text_layer_set_text_color(output_layer, GColorWhite);
    text_layer_set_background_color(output_layer, GColorBlack);
    window_set_background_color(config_window, GColorBlack);
  } else {
    text_layer_set_text_color(output_layer, GColorBlack);
    text_layer_set_background_color(output_layer, GColorWhite);
    window_set_background_color(config_window, GColorWhite);
  }

  switch (nbItem) {
    case REQUEST_LOCATION:
      strcpy(text, "LOCATION");
      break;
    case REQUEST_FIX_LOCATION:
      strcpy(text, "FIXING TARGET");
      break;
    case REQUEST_START_THREADED_LOCATION:
      strcpy(text, "START THREAD NAVIGATION");
      break;
    case REQUEST_STOP_THREADED_LOCATION:
      strcpy(text, "STOP THREAD NAVIGATION");
      break;
    case REQUEST_ELEVATION:
      strcpy(text, "ELEVATION");
      break;
    case REQUEST_WEATHER_STATUS:
      strcpy(text, "WEATHER_STATUS");
      break;
    case REQUEST_WEATHER_TEMPERATURE:
      strcpy(text, "TEMPERATURE");
      break;
    case REQUEST_WEATHER_PRESSURE:
      strcpy(text, "PRESSURE");
      break;
    case REQUEST_WEATHER_HUMIDITY:
      strcpy(text, "HUMIDITY");
      break;
    case REQUEST_WEATHER_WIND:
      strcpy(text, "WIND");
      break;
    case REQUEST_WEATHER_SUNRISE:
      strcpy(text, "SUNRISE");
      break;
    case REQUEST_WEATHER_SUNSET:
      strcpy(text, "SUNSET");
      break;
    case REQUEST_TRANSPORT:
      strcpy(text, "TRANSPORT");
      break;
    case SHOW_UP_TIME:
      strcpy(text, "SHOW_UP_TIME");
      break;
    case SHOW_ACTIVE_TIME:
      strcpy(text, "SHOW_ACTIVE_TIME");
      break;
    case SHOW_BATTERY_STATE:
      strcpy(text, "SHOW_BATTERY_STATE");
      break;
    default:
      strcpy(text, "Error.\nPlease check if NUMBER_OF_ITEMS is OK");
      break;
  }
  text_layer_set_text(output_layer, text);
  text_layer_set_text_alignment(output_layer, GTextAlignmentCenter);
}
void up_main_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (currentScreen + 1 > 3) {
    currentScreen = 0;
  } else {
    currentScreen = currentScreen + 1;
  }
  int val = 0;
 switch(currentScreen) {
    case 0:
      if (persist_exists(PERSIST_SCREEN1)) {
        val = persist_read_int(PERSIST_SCREEN1);
      }
    break;
    case 1:
      if (persist_exists(PERSIST_SCREEN2)) {
        val = persist_read_int(PERSIST_SCREEN2);
      }
    break;
    case 2:
      if (persist_exists(PERSIST_SCREEN3)) {
      val = persist_read_int(PERSIST_SCREEN3);
      }
    break;
    case 3:
    if (persist_exists(PERSIST_SCREEN4)) {
      val = persist_read_int(PERSIST_SCREEN4);
    }
    break;
  }
  
  if (val < 13) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Nav send : %d", val);
	  send(val, "");
  }
  
}

void down_main_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (currentScreen - 1 < 0) {
    currentScreen = 3;
  } else {
    currentScreen = currentScreen - 1;
  }
  int val = 0;
  switch(currentScreen) {
    case 0:
      if (persist_exists(PERSIST_SCREEN1)) {
        val = persist_read_int(PERSIST_SCREEN1);
      }
    break;
    case 1:
      if (persist_exists(PERSIST_SCREEN2)) {
        val = persist_read_int(PERSIST_SCREEN2);
      }
    break;
    case 2:
      if (persist_exists(PERSIST_SCREEN3)) {
      val = persist_read_int(PERSIST_SCREEN3);
      }
    break;
    case 3:
    if (persist_exists(PERSIST_SCREEN4)) {
      val = persist_read_int(PERSIST_SCREEN4);
    }
    break;
  }
  
  if (val < 13) {
   // APP_LOG(APP_LOG_LEVEL_INFO, "Nav send : %d", val);
	  send(val, "");
  }
  
}

void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_UP, up_main_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_main_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  char window_number[MAX_TEXT_SIZE];
  switch(currentScreen) {
    case 0:
      strcpy(window_number, "Screen 1");
    break;
    case 1:
      strcpy(window_number, "Screen 2");
    break;
    case 2:
      strcpy(window_number, "Screen 3");
    break;
    case 3:
      strcpy(window_number, "Screen 4");
    break;
  }
  number_layer = text_layer_create(GRect(0, 0, bounds.size.w, 19)); // Change if you use PEBBLE_SDK 3
  text_layer_set_text(number_layer, window_number);
  text_layer_set_text_alignment(number_layer, GTextAlignmentCenter);
  text_layer_set_text_color(number_layer, GColorWhite);
  text_layer_set_background_color(number_layer, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(number_layer));


  output_layer = text_layer_create(GRect(0, 60, bounds.size.w, bounds.size.h)); // Change if you use PEBBLE_SDK 3
  int cpt = persist_read_int(PERSIST_SCREEN1) ? persist_read_int(PERSIST_SCREEN1) : 0;
  if (cpt < 13) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Nav send : %d", cpt);
	  send(cpt, "");
  }
  text_layer_set_text_alignment(output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(output_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(output_layer);
}

static void config_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Exit app after tea is done
  //APP_LOG(APP_LOG_LEVEL_INFO, "Current screen and nbItem : %d %d", currentScreen, nbItem);
  switch(currentScreen) {
    case 0:
      persist_write_int(PERSIST_SCREEN1, nbItem);
    break;
    case 1:
      persist_write_int(PERSIST_SCREEN2, nbItem);
    break;
    case 2:
      persist_write_int(PERSIST_SCREEN3, nbItem);
    break;
    case 3:
      persist_write_int(PERSIST_SCREEN4, nbItem);
    break;
  }
}

static void config_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  strcpy(text, "");
  text_layer_set_text(output_layer, text);
  window_stack_pop(true); 
}

static void config_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, config_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_config_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_config_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, config_back_click_handler);
}

static void config_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_click_config_provider(window, config_click_config_provider);

  output_layer = text_layer_create(GRect(0, 60, bounds.size.w, bounds.size.h)); // Change if you use PEBBLE_SDK 3
  int val = -1;
  switch(currentScreen) {
    case 0:
      nbItem = persist_exists(PERSIST_SCREEN1) ? persist_read_int(PERSIST_SCREEN1) : 0;
      val = persist_exists(PERSIST_SCREEN1) ? persist_read_int(PERSIST_SCREEN1) : -1;
    break;
    case 1:
      nbItem = persist_exists(PERSIST_SCREEN2) ? persist_read_int(PERSIST_SCREEN2) : 0;
      val = persist_exists(PERSIST_SCREEN2) ? persist_read_int(PERSIST_SCREEN2) : -1;
    break;
    case 2:
      nbItem = persist_exists(PERSIST_SCREEN3) ? persist_read_int(PERSIST_SCREEN3) : 0;
      val = persist_exists(PERSIST_SCREEN3) ? persist_read_int(PERSIST_SCREEN3) : -1;
    break;
    case 3:
      nbItem = persist_exists(PERSIST_SCREEN4) ? persist_read_int(PERSIST_SCREEN4) : 0;
      val = persist_exists(PERSIST_SCREEN4) ? persist_read_int(PERSIST_SCREEN4) : -1;
    break;
  }
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "Config load : %d %d", currentScreen, nbItem);

  if (val != -1 && nbItem == val) {
    text_layer_set_text_color(output_layer, GColorWhite);
    text_layer_set_background_color(output_layer, GColorBlack);
    window_set_background_color(config_window, GColorBlack);
  } else {
    text_layer_set_text_color(output_layer, GColorBlack);
    text_layer_set_background_color(output_layer, GColorWhite);
    window_set_background_color(config_window, GColorWhite);
  }
  
  switch (nbItem) {
    case REQUEST_LOCATION:
      strcpy(text, "LOCATION");
      break;
    case REQUEST_FIX_LOCATION:
      strcpy(text, "FIXING TARGET");
      break;
    case REQUEST_START_THREADED_LOCATION:
      strcpy(text, "START THREAD NAVIGATION");
      break;
    case REQUEST_STOP_THREADED_LOCATION:
      strcpy(text, "STOP THREAD NAVIGATION");
      break;
    case REQUEST_ELEVATION:
      strcpy(text, "ELEVATION");
      break;
    case REQUEST_WEATHER_STATUS:
      strcpy(text, "WEATHER_STATUS");
      break;
    case REQUEST_WEATHER_TEMPERATURE:
      strcpy(text, "TEMPERATURE");
      break;
    case REQUEST_WEATHER_PRESSURE:
      strcpy(text, "PRESSURE");
      break;
    case REQUEST_WEATHER_HUMIDITY:
      strcpy(text, "HUMIDITY");
      break;
    case REQUEST_WEATHER_WIND:
      strcpy(text, "WIND");
      break;
    case REQUEST_WEATHER_SUNRISE:
      strcpy(text, "SUNRISE");
      break;
    case REQUEST_WEATHER_SUNSET:
      strcpy(text, "SUNSET");
      break;
    case REQUEST_TRANSPORT:
      strcpy(text, "TRANSPORT");
      break;
    case SHOW_UP_TIME:
      strcpy(text, "SHOW_UP_TIME");
      break;
    case SHOW_ACTIVE_TIME:
      strcpy(text, "SHOW_ACTIVE_TIME");
      break;
    case SHOW_BATTERY_STATE:
      strcpy(text, "SHOW_BATTERY_STATE");
      break;
    default:
      strcpy(text, "Error.\nPlease check if NUMBER_OF_ITEMS is OK");
      break;
  }
  text_layer_set_text(output_layer, text);
  text_layer_set_text_alignment(output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(output_layer));
}

static void config_window_unload(Window *window) {
  text_layer_destroy(output_layer);
}

void out_sent_handler(DictionaryIterator *sent, void *context){}
static void out_fail_handler(DictionaryIterator *failed, AppMessageResult reason, void* context){}

static void in_received_handler(DictionaryIterator *iter, void* context){}

void in_drop_handler(AppMessageResult reason, void *context){}

/**
 * Initializes
 */
static void init(void) {

  
  // Subscribe to TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Subscribe to the accelerometer data service
  accel_data_service_subscribe(NUM_ACCEL_SAMPLES, data_handler);
  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);

  app_message_register_inbox_received(received_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_inbox_dropped(in_drop_handler);
  app_message_register_outbox_failed(out_fail_handler);
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  
  
  main_window = window_create();
  window_set_click_config_provider(main_window, click_config_provider);
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(main_window, true);
  
   s_menu_window = window_create();
   window_set_window_handlers(s_menu_window, (WindowHandlers){
     .load = menu_window_load,
     .unload = menu_window_unload,
   });
  
  config_window = window_create();
  window_set_window_handlers(config_window, (WindowHandlers){
    .load = config_window_load,
    .unload = config_window_unload,
  });
}
  
static void deinit(void) {
  window_destroy(main_window);
}

/**
 * Starts the Pebble app.
 */
int main(void) {
  init();
  app_event_loop();
  deinit();
}