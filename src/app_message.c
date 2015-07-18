#include <pebble.h>

#define WIDTH 144
#define HEIGHT 168
#define IMG_SIZE 32
#define MID_Y 62
#define WEATHER_Y 32
#define NR_ROWS 4
	
Window *window;	
TextLayer *txt_time;
TextLayer *txt_weather[NR_ROWS];
BitmapLayer *weather_icon_layer[NR_ROWS];
GBitmap *weather_icon_bitmap[NR_ROWS];
static char weather[NR_ROWS][32];
static bool firstTime = true;
Layer *graphics_layer;

void p(char* msg) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Log: %s", msg);
}
static void showReason(AppMessageResult reason) {
	static char reasonStr[20];
	switch(reason){
    case APP_MSG_OK:
      snprintf(reasonStr,20,"%s","APP_MSG_OK");
    break;
    case APP_MSG_SEND_TIMEOUT:
      snprintf(reasonStr,20,"%s","SEND TIMEOUT");
    break;
    case APP_MSG_SEND_REJECTED:
      snprintf(reasonStr,20,"%s","SEND REJECTED");
    break;
    case APP_MSG_NOT_CONNECTED:
      snprintf(reasonStr,20,"%s","NOT CONNECTED");
    break;
    case APP_MSG_APP_NOT_RUNNING:
      snprintf(reasonStr,20,"%s","NOT RUNNING");
    break;
    case APP_MSG_INVALID_ARGS:
      snprintf(reasonStr,20,"%s","INVALID ARGS");
    break;
    case APP_MSG_BUSY:
      snprintf(reasonStr,20,"%s","BUSY");
    break;
    case APP_MSG_BUFFER_OVERFLOW:
      snprintf(reasonStr,20,"%s","BUFFER OVERFLOW");
    break;
    case APP_MSG_ALREADY_RELEASED:
      snprintf(reasonStr,20,"%s","ALRDY RELEASED");
    break;
    case APP_MSG_CALLBACK_ALREADY_REGISTERED:
      snprintf(reasonStr,20,"%s","CLB ALR REG");
    break;
    case APP_MSG_CALLBACK_NOT_REGISTERED:
      snprintf(reasonStr,20,"%s","CLB NOT REG");
    break;
    case APP_MSG_OUT_OF_MEMORY:
      snprintf(reasonStr,20,"%s","OUT OF MEM");
    break;
    case APP_MSG_INTERNAL_ERROR:
      snprintf(reasonStr,20,"%s","INTERNAL ERROR");
    break;	
	 case APP_MSG_CLOSED:
      snprintf(reasonStr,20,"%s","CLOSED");
    break;	

  }
	p(reasonStr);
}
	
void send_message(void){
	p("============ sending message to phone ===========")		;
	DictionaryIterator *iter;	
	app_message_outbox_begin(&iter);	
	dict_write_uint8(iter, 0x1, 0x1);	
	dict_write_end(iter);
	app_message_outbox_send();
}


// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
	p("in_dropped_handler");
	showReason(reason);	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	p("out_failed_handler");
	showReason(reason);	
}

static int getY(int row) {
	return  MID_Y + (row-1) * IMG_SIZE+2;
}
static void graphics_layer_update_callback(Layer *layer, GContext *ctx) {
 	  GPoint p0 = GPoint(0,  getY(1));
  	  GPoint p1 = GPoint(WIDTH, getY(1));
      graphics_context_set_stroke_color(ctx, GColorBlack);
      graphics_draw_line(ctx, p0, p1);
  
}
static void addWeatherBitmap(int row, uint32_t resource_id) {	 
  int x = 1;
  int y = getY(row) -1;
  
  if (!firstTime) {
	  gbitmap_destroy(weather_icon_bitmap[row]);
  	  bitmap_layer_destroy(weather_icon_layer[row]); 		
  }
  weather_icon_bitmap[row] = gbitmap_create_with_resource(resource_id);
  weather_icon_layer[row] = bitmap_layer_create(GRect(x, y, IMG_SIZE, IMG_SIZE));
  bitmap_layer_set_bitmap(weather_icon_layer[row], weather_icon_bitmap[row]);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(weather_icon_layer[row]));
	
}	

static void addRow(int row, int iconid, char* desc) {
  int x = IMG_SIZE+4;	  
  int y = getY(row);
  int w = WIDTH-IMG_SIZE-5;
  int h = 35;
  
    TextLayer *txt = txt_weather[row];
	if (firstTime) {
		txt = text_layer_create(GRect(x, y,w , h));
		txt_weather[row] = txt;
	}
	
  snprintf(weather[row], sizeof(weather[row]), "%s", desc);	
  text_layer_set_text(txt, weather[row]);
  if (firstTime) {
	  text_layer_set_background_color(txt, GColorClear);
	  text_layer_set_text_color(txt, GColorBlack);
	  text_layer_set_text_alignment(txt, GTextAlignmentCenter);	 
	  text_layer_set_font(txt, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));	 
	  layer_add_child(window_get_root_layer(window), text_layer_get_layer(txt));
  }
}
// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	p("============ received answer from phone ===========");
	Tuple *tuple;
	int icon_id =0;
	int index = 0;
	int res_id = 0;
	int value=0;
    char weather[NR_ROWS][32];
	uint32_t resource_id[NR_ROWS];
	
	
	// first check for status 1 so that we can start sending
    tuple = dict_read_first(received);   
    while (tuple && tuple != NULL) {
	  APP_LOG(APP_LOG_LEVEL_INFO, "--- Reading next tuple with key %d", (int)tuple->key); 
	  
      int key = (int)tuple->key;
	  if (key == 0) {
		   value = (int)tuple->value->uint32;
           APP_LOG(APP_LOG_LEVEL_INFO, "Received STATUS_KEY: %d",value); 
		   if (value ==1) {
		   	  p("		Got status key 1, fetching weather");
           	  send_message();
			  return;
		   }		   
	  }
      int which = (key-1) % 3+1;
      index = (int)((key-1)/3);
      APP_LOG(APP_LOG_LEVEL_INFO, "		Received key: %d, which=%d, index=%d", key, which, index); 
      switch (which) {
        case 0:
          break;
        case 1:
          index = (int)((int)tuple->value->uint32);
          break;
        case 2:
		  icon_id = tuple->value->uint32;
          switch (icon_id) {
			case 1:
				res_id = RESOURCE_ID_IMAGE_01;
			break;
			case 2:
				res_id = RESOURCE_ID_IMAGE_02;
			break;
			case 3:
				res_id = RESOURCE_ID_IMAGE_03;
			break;
			case 4:
				res_id = RESOURCE_ID_IMAGE_04;
			break;
			case 9:
				res_id = RESOURCE_ID_IMAGE_09;
			break;
			case 10:
				res_id = RESOURCE_ID_IMAGE_10;
			break;
			case 11:
				res_id = RESOURCE_ID_IMAGE_11;
			break;
			case 13:
				res_id = RESOURCE_ID_IMAGE_13;
			break;
			case 50:
				res_id = RESOURCE_ID_IMAGE_50;
			break;
			default:
				res_id = RESOURCE_ID_IMAGE_50;
			break;
		  }		
		  resource_id[index] = res_id;
          break;
        case 3:          
	      snprintf(weather[index], sizeof(weather[index]), "%s", tuple->value->cstring);
          break;         
        default:
           p("		Unknown key");
         break;
      }
	  
      tuple = dict_read_next(received);
    }
	for (int i = 0; i < NR_ROWS; i++) {
	      addRow(i, resource_id[i], weather[i]);                   	
		  addWeatherBitmap(i, resource_id[i]);
	}
	firstTime = false;
}


void update_time() {
  
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00, 01.01";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof(buffer), "%H:%M, %d.%m", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof(buffer), "%I:%M, %d.%m", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(txt_time, buffer);
}



void main_window_load(Window *window) {
  // graphics layer
  graphics_layer = layer_create(GRect(0, 0, WIDTH, HEIGHT));
  layer_set_update_proc(graphics_layer, graphics_layer_update_callback);
  layer_add_child(window_get_root_layer(window), graphics_layer);
	
  // Create time TextLayer
  txt_time = text_layer_create(GRect(0, 0, WIDTH, 30));
  text_layer_set_background_color(txt_time, GColorClear);
  text_layer_set_text_color(txt_time, GColorBlack);
  text_layer_set_text(txt_time, "00:00");

  //Apply to TextLayer
  text_layer_set_font(txt_time, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(txt_time, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(txt_time));
  
  // Make sure the time is displayed from the start
  update_time();

}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    send_message();
  }
}

void main_window_unload(Window *window) {
  	for (int i =0; i <NR_ROWS; i++ ) {
  		gbitmap_destroy(weather_icon_bitmap[i]);
  		bitmap_layer_destroy(weather_icon_layer[i]);
 		text_layer_destroy(txt_weather[i]);
	}
  text_layer_destroy(txt_time);  
  layer_destroy(graphics_layer);
}
	
void init(void) {		
	window = window_create();
	// Set handlers to manage the elements inside the Window
	  window_set_window_handlers(window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	  });
	window_stack_push(window, true);
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	tick_timer_service_subscribe(MINUTE_UNIT,tick_handler );

}
void deinit(void) {
	app_message_deregister_callbacks();
	window_destroy(window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}