#include <pebble.h>

static Window *main_window;
TextLayer *output_layer;

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
// Location API
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

int counter = -1;
char text[MAX_TEXT_SIZE];
unsigned long int up_time = 0;      //in seconds
unsigned long int active_time = 0;  //in seconds/10


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

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  counter = (counter + 1) % NUMBER_OF_ITEMS;
  APP_LOG(APP_LOG_LEVEL_INFO, "Sending request id : %d", counter);
  if (counter < 13) {
	  send(counter, "");
  }

  switch (counter) {
    case REQUEST_LOCATION:
      strcpy(text, "Request for:\nLOCATION\nsent");
      break;
    case REQUEST_FIX_LOCATION:
      strcpy(text, "Request for:\nFIXING TARGET\nsent");
      break;
    case REQUEST_START_THREADED_LOCATION:
      strcpy(text, "Request for:\nSTART THREAD NAVIGATION\nsent");
      break;
    case REQUEST_STOP_THREADED_LOCATION:
      strcpy(text, "Request for:\nSTOP THREAD NAVIGATION\nsent");
      break;
    case REQUEST_ELEVATION:
      strcpy(text, "Request for:\nELEVATION\nsent");
      break;
    case REQUEST_WEATHER_STATUS:
      strcpy(text, "Request for:\nWEATHER_STATUS\nsent");
      break;
    case REQUEST_WEATHER_TEMPERATURE:
      strcpy(text, "Request for:\nTEMPERATURE\nsent");
      break;
    case REQUEST_WEATHER_PRESSURE:
      strcpy(text, "Request for:\nPRESSURE\nsent");
      break;
    case REQUEST_WEATHER_HUMIDITY:
      strcpy(text, "Request for:\nHUMIDITY\nsent");
      break;
    case REQUEST_WEATHER_WIND:
      strcpy(text, "Request for:\nWIND\nsent");
      break;
    case REQUEST_WEATHER_SUNRISE:
      strcpy(text, "Request for:\nSUNRISE\nsent");
      break;
    case REQUEST_WEATHER_SUNSET:
      strcpy(text, "Request for:\nSUNSET\nsent");
      break;
    case REQUEST_TRANSPORT:
      strcpy(text, "Request for:\nTRANSPORT\nsent");
      break;
    case SHOW_UP_TIME:
      strcpy(text, "Mode:\nSHOW_UP_TIME\nset");
      break;
    case SHOW_ACTIVE_TIME:
      strcpy(text, "Mode:\nSHOW_ACTIVE_TIME\nset");
      break;
    case SHOW_BATTERY_STATE:
      strcpy(text, "Mode:\nSHOW_BATTERY_STATE\nset");
      break;
    default:
      strcpy(text, "Error.\nPlease check if NUMBER_OF_ITEMS is OK");
      break;
  }
  text_layer_set_text(output_layer, text);
}

void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  output_layer = text_layer_create(GRect(0, 60, bounds.size.w, bounds.size.h)); // Change if you use PEBBLE_SDK 3
  text_layer_set_text(output_layer, "Welcome Pebble :-)\nPlease UP click !");
  text_layer_set_text_alignment(output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(output_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(output_layer);
}

/**
 * Initializes
 */
static void init(void) {
  main_window = window_create();
  window_set_click_config_provider(main_window, click_config_provider);
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(main_window, true);

  // Subscribe to TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Subscribe to the accelerometer data service
  accel_data_service_subscribe(NUM_ACCEL_SAMPLES, data_handler);
  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);

  app_message_register_inbox_received(received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
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