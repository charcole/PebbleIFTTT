#include <pebble.h>

static Window *window;
static ActionMenu *s_action_menu;
static ActionMenuLevel *s_root_level;
static TextLayer *text_layer;
static int numNames=0;
static char *names[256], *keys[256];
static bool bRejiggingMenu=false;

enum
{
	KEY_RECIEVECONFIG = 0,	
	KEY_SENDCOMMAND = 1,
  KEY_STATUS = 2,
  KEY_MENU = 3
};

static void action_performed_callback(ActionMenu *action_menu, const ActionMenuItem *action, void *context)
{
  char *cmd = (char*)action_menu_item_get_action_data(action);
  static char buf[128];
  DictionaryIterator *iter;
  
  strcpy(buf, "Sending Event: ");
  strcat(buf, cmd);
  text_layer_set_text(text_layer, buf);
	
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, KEY_SENDCOMMAND, cmd);
	
	dict_write_end(iter);
  app_message_outbox_send();
}

static void init_action_menu()
{
  if (s_root_level)
  {
    action_menu_hierarchy_destroy(s_root_level, NULL, NULL);
    s_root_level=NULL;
  }
  
  s_root_level = action_menu_level_create(numNames);
  int depth=0;
  ActionMenuLevel *level[32];
  level[0]=s_root_level;
  for (int i=0; i<numNames; i++)
  {
    if (keys[i])
    {
      action_menu_level_add_action(level[depth], names[i], action_performed_callback, (void*)keys[i]);
    }
    else
    {
      bool isBlank=true;
      int k=0;
      while (names[i][k])
      {
        if (names[i][k]!=' ' && names[i][k]!='\t')
        {
          isBlank=false;
          break;
        }
        k++;
      }
      if (isBlank)
      {
        if (depth>0)
          depth--;
      }
      else if (depth<(int)(sizeof(level)/sizeof(level[0])))
      {
        level[++depth]=action_menu_level_create(numNames-1-i);
        action_menu_level_add_child(level[depth-1], level[depth], names[i]);
      }
    }
  }
}

static void ActionMenuDidClose(ActionMenu *menu, const ActionMenuItem *performed_action, void *context)
{
  if (!bRejiggingMenu && performed_action==NULL)
    window_stack_remove(window, true);
}

static void open_action_menu()
{
  if (s_action_menu)
  {
    bRejiggingMenu=true;
    action_menu_close(s_action_menu, false);
    bRejiggingMenu=false;
    s_action_menu=NULL;
  }
  if (numNames)
  {
    init_action_menu();
    ActionMenuConfig config = (ActionMenuConfig) {
      .root_level = s_root_level,
      .colors = {
        .background = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
        .foreground = GColorBlack,
      },
      .align = ActionMenuAlignCenter,
      .did_close = ActionMenuDidClose
    };
    s_action_menu = action_menu_open(&config);
  }
}

static void click_handler(ClickRecognizerRef recognizer, void *context)
{
  open_action_menu();
}

static void click_config_provider(void *context)
{
  window_single_click_subscribe(BUTTON_ID_SELECT, click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_handler);
}

static void window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create(GRect(0, bounds.size.h/2-20, bounds.size.w, bounds.size.h/2+20));
  if (numNames==0)
    text_layer_set_text(text_layer, "Please Configure");
  else
    text_layer_set_text(text_layer, "Press Button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window)
{
  action_menu_hierarchy_destroy(s_root_level, NULL, NULL);
  text_layer_destroy(text_layer);
  s_root_level=NULL;
}

static void SetupMenu()
{
  int numSlots=persist_read_int(0);
  char name[PERSIST_STRING_MAX_LENGTH];
  for (int i=0; i<numNames; i++)
  {
    free(names[i]);
    names[i]=NULL;
    keys[i]=NULL;
  }
  numNames=0;
  for (int i=0; i<numSlots; i++)
  {
    int bytes=persist_read_string (i+1, name, sizeof(name));
    if (bytes==E_DOES_NOT_EXIST)
      break;
    names[numNames]=malloc(bytes);
    strcpy(names[numNames], name);
    char *sep=strchr(names[numNames], ':');
    if (sep)
    {
      sep[0]=0;
      keys[numNames]=sep+1;
    }
    numNames++;
  }
  if (text_layer)
  {
    if (numNames==0)
    {
      text_layer_set_text(text_layer, "Please Configure");
    }
    else
    {
      text_layer_set_text(text_layer, "Press Button");
      open_action_menu();
    }
  }
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *status = dict_find(received, KEY_STATUS);
  if (status)
  {
    switch (status->value->int32)
    {
      case 0: text_layer_set_text(text_layer, "Sent"); break;
      case 1: text_layer_set_text(text_layer, "Bad HTTP request. Network trouble?"); break;
      case 2: text_layer_set_text(text_layer, "Got error. Wrong key?"); break;
      default: text_layer_set_text(text_layer, "Returned unknown status"); break;
    }
  }
  
  Tuple *menu = dict_find(received, KEY_MENU);
  if (menu)
  {
    char name[PERSIST_STRING_MAX_LENGTH];
    int offset=0;
    int num=0;
    while (offset<menu->length)
    {
      char *end=strchr((char*)&menu->value->data[offset], '\n');
      char *send=strchr((char*)&menu->value->data[offset], '\r');
      if (send && (send<end || !end))
        send=end;
      int length, origLength;
      if (end)
        length=end-(char*)&menu->value->data[offset];
      else
        length=menu->length-offset;
      origLength=length;
      if (length>PERSIST_STRING_MAX_LENGTH-1)
        length=PERSIST_STRING_MAX_LENGTH-1;
      strncpy(name, (char*)&menu->value->data[offset], length);
      name[length]='\0';
      persist_write_string(++num, name);
      offset+=origLength+1;
    }
    persist_write_int(0, num);
    if (s_action_menu)
    {
      bRejiggingMenu=true;
      action_menu_close(s_action_menu, false);
      bRejiggingMenu=false;
      s_action_menu=NULL;
    }
    SetupMenu();
  }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  text_layer_set_text(text_layer, "Phone didn't acknoledge message");
}

static void init(void)
{
  SetupMenu();
  
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  
  // Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);

  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = app_message_inbox_size_maximum();
  const int outbox_size = app_message_outbox_size_maximum();
	app_message_open(inbox_size, outbox_size);
  
  open_action_menu();
}

static void deinit(void)
{
  app_message_deregister_callbacks();
  window_destroy(window);
}

int main(void)
{
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
