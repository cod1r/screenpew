#include <xcb/xcb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>

int max(int one, int two) {
  return one > two ? one : two;
}

int min(int one, int two) {
  return one < two ? one : two;
}

void create_png(uint8_t *img_data, xcb_window_t window, int width, int height, int screenN) {
  png_image img;
  img.opaque = NULL;
  img.version = PNG_IMAGE_VERSION;
  img.height = height;
  img.width = width;
  img.format = PNG_FORMAT_BGR | PNG_FORMAT_FLAG_ALPHA;
  char file_name[50];
  sprintf(file_name, "screenshot%d.png", screenN);
  png_image_write_to_file(
    &img, 
    file_name, 
    0, 
    img_data, 
    4 * width, 
    NULL
  );
}

void get_image(
  xcb_connection_t *connection, 
  xcb_window_t window, 
  int x1,
  int y1,
  int x2,
  int y2,
  int screenN
) {
  int min_y = min(y1, y2);
  int max_y = max(y1, y2);
  int min_x = min(x1, x2);
  int max_x = max(x1, x2);
  xcb_get_image_cookie_t cookie = xcb_get_image(
    connection, 
    XCB_IMAGE_FORMAT_Z_PIXMAP, 
    window,
    min_x,
    min_y,
    max_x - min_x,
    max_y - min_y,
    ~0
  );
  xcb_get_image_reply_t *img_reply = xcb_get_image_reply(connection, cookie, NULL);
  if (img_reply != NULL && img_reply->length > 0) {
    uint8_t *img_data = xcb_get_image_data(img_reply);
    create_png(img_data, window, max_x - min_x, max_y - min_y, screenN);
  }
  else if (img_reply == NULL) {
    printf("img_reply null\n");
  }
  else if (img_reply->length == 0) {
    printf("img_reply length is zero\n");
  }
}

void print_window_name(xcb_connection_t *connection, xcb_window_t window) {
  char *atom_name = "_NET_WM_NAME";
  xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(connection, 1, strlen(atom_name), atom_name);
  xcb_intern_atom_reply_t *atom_reply = xcb_intern_atom_reply(connection, atom_cookie, NULL);

  xcb_get_property_cookie_t prop_cookie = xcb_get_property(connection, 0, window, atom_reply->atom, XCB_ATOM_ANY, 0, 40);
  xcb_get_property_reply_t *prop_reply = xcb_get_property_reply(connection, prop_cookie, NULL);
  if (prop_reply != NULL) {
    int name_length = xcb_get_property_value_length(prop_reply);
    char *value = xcb_get_property_value(prop_reply);
    for (int i = 0; i < name_length; i++) {
      printf("%c", value[i]);
    }
    printf("\n");
  }
}

void print_window_state(xcb_connection_t *connection, xcb_window_t window) {
  printf("printing window state...\n");
  char *atom_state = "_NET_WM_STATE";
  xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(connection, 1, strlen(atom_state), atom_state);
  xcb_intern_atom_reply_t *atom_reply = xcb_intern_atom_reply(connection, atom_cookie, NULL);

  xcb_get_property_cookie_t prop_cookie = xcb_get_property(connection, 0, window, atom_reply->atom, XCB_ATOM_ANY, 0, 40);
  xcb_get_property_reply_t *prop_reply = xcb_get_property_reply(connection, prop_cookie, NULL);
  if (prop_reply->type != 0) {
    int value_len = xcb_get_property_value_length(prop_reply);
    int *value = xcb_get_property_value(prop_reply);
    printf("format: %d\n", prop_reply->format);
    printf("value len: %d, %d\n", value_len, prop_reply->length);
    printf("bytes after: %d\n", prop_reply->bytes_after);
    printf("value of state: %d\n", *value);
    xcb_get_atom_name_cookie_t name_cookie = xcb_get_atom_name(connection, *value);
    xcb_get_atom_name_reply_t *name_reply = xcb_get_atom_name_reply(connection, name_cookie, NULL);
    if (name_reply != NULL) {
      char *atom_name_str = xcb_get_atom_name_name(name_reply);
      int atom_name_len = xcb_get_atom_name_name_length(name_reply);
      for (int i = 0; i < atom_name_len; i++) {
        printf("%c", atom_name_str[i]);
      }
      printf("\n");
    }
  }
}

void traverse_windows(xcb_connection_t *connection, xcb_window_t window) {
  xcb_query_tree_cookie_t query_cookie = xcb_query_tree(connection, window);
  xcb_query_tree_reply_t *query_reply = xcb_query_tree_reply(connection, query_cookie, NULL);
  xcb_window_t *children = xcb_query_tree_children(query_reply);
  printf("window: %d\n", window);

  print_window_state(connection, window);
  print_window_name(connection, window);

  for (int i = 0; i < query_reply->children_len; i++) {
    traverse_windows(connection, children[i]);
  }
}

void add_mouse_event(xcb_connection_t *connection, xcb_window_t window) {
  uint32_t values[] = {XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE};
  xcb_change_window_attributes(
    connection, 
    window, 
    XCB_CW_EVENT_MASK, 
    values
  );
}

int main() {
  xcb_connection_t *connection = xcb_connect(NULL, NULL);
  const xcb_setup_t *setup = xcb_get_setup(connection);

  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  xcb_window_t root = iter.data->root;
  int screen_number = 1;
  while (iter.rem != 0) {
    xcb_get_geometry_cookie_t geo_cookie = xcb_get_geometry(connection, iter.data->root);
    xcb_get_geometry_reply_t *geo_reply = xcb_get_geometry_reply(connection, geo_cookie, NULL);

    xcb_window_t window = xcb_generate_id(connection);
    xcb_create_window(
      connection, 
      0, 
      window, 
      iter.data->root, 
      0, 
      0, 
      geo_reply->width, 
      geo_reply->height, 
      0, 
      XCB_WINDOW_CLASS_INPUT_OUTPUT, 
      iter.data->root_visual,
      0,
      NULL
    );
    xcb_intern_atom_cookie_t intern_cookie = xcb_intern_atom(connection, 1, strlen("_NET_WM_STATE"), "_NET_WM_STATE");
    xcb_intern_atom_reply_t *intern_reply = xcb_intern_atom_reply(connection, intern_cookie, NULL);

    xcb_intern_atom_cookie_t intern_fullscrn_cookie = xcb_intern_atom(connection, 1, strlen("_NET_WM_STATE_FULLSCREEN"), "_NET_WM_STATE_FULLSCREEN");
    xcb_intern_atom_reply_t *intern_fullscrn_reply = xcb_intern_atom_reply(connection, intern_fullscrn_cookie, NULL);

    int atom[] = {intern_fullscrn_reply->atom};
    // cannot put XCB_ATOM_ANY into change property. WHY!!!!!!
    // there has to be someway to get ATOM TYPE. If not, this is going to be super confusing moving on...
    xcb_change_property(
      connection,
      XCB_PROP_MODE_REPLACE,
      window,
      intern_reply->atom,
      XCB_ATOM_INTEGER,
      32,
      1,
      atom
    );
    xcb_change_property (
      connection,
      XCB_PROP_MODE_REPLACE,
      window,
      XCB_ATOM_WM_NAME,
      XCB_ATOM_STRING,
      8,
      strlen ("screenpew"),
      "screenpew" 
    );
    add_mouse_event(connection, window);
    xcb_map_window(connection, window);
    screen_number++;
    xcb_screen_next(&iter);
  }
  xcb_flush(connection);
  xcb_generic_event_t *event;
  int press_x;
  int release_x;
  int press_y;
  int release_y;
  while ((event = xcb_wait_for_event(connection))) {
    int event_type = event->response_type & ~0x80;
    if (event_type == XCB_BUTTON_PRESS) {
      press_x = ((xcb_button_press_event_t *)event)->event_x;
      press_y = ((xcb_button_press_event_t *)event)->event_y;
    } else if (event_type == XCB_BUTTON_RELEASE) {
      release_x = ((xcb_button_release_event_t *)event)->event_x;
      release_y = ((xcb_button_release_event_t *)event)->event_y;
      break;
    }
    free(event);
  }
  get_image(connection, root, press_x, press_y, release_x, release_y, 1);
  xcb_disconnect(connection);
  return 0;
}