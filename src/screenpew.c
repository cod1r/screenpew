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

void create_png(uint8_t *img_data, xcb_window_t window, int width, int height) {
  png_image img;
  img.opaque = NULL;
  img.version = PNG_IMAGE_VERSION;
  img.height = height;
  img.width = width;
  img.format = PNG_FORMAT_BGR | PNG_FORMAT_FLAG_ALPHA;
  char *home = getenv("HOME");
  char *file_name = "/Pictures/screenshot.png";
  int string_len = strlen(home) + strlen(file_name);
  char *home_plus_file_name = malloc(string_len * sizeof(char));
  strcpy(home_plus_file_name, home);
  strcat(home_plus_file_name, file_name);
  png_image_write_to_file(
    &img,    home_plus_file_name,    0,    img_data,    4 * width,    NULL
  );
}

void get_image(
  xcb_connection_t *connection,  xcb_window_t window,  int x1,
  int y1,
  int x2,
  int y2
) {
  int min_y = min(y1, y2);
  int max_y = max(y1, y2);
  int min_x = min(x1, x2);
  int max_x = max(x1, x2);
  xcb_get_image_cookie_t cookie = xcb_get_image(
    connection,    XCB_IMAGE_FORMAT_Z_PIXMAP,    window,
    min_x,
    min_y,
    max_x - min_x,
    max_y - min_y,
    ~0
  );
  xcb_get_image_reply_t *img_reply = xcb_get_image_reply(connection, cookie, NULL);
  if (img_reply != NULL && img_reply->length > 0) {
    uint8_t *img_data = xcb_get_image_data(img_reply);
    create_png(img_data, window, max_x - min_x, max_y - min_y);
  }
  else if (img_reply == NULL) {
    printf("img_reply null\n");
  }
  else if (img_reply->length == 0) {
    printf("img_reply length is zero\n");
  }
}

void add_mouse_event(xcb_connection_t *connection, xcb_window_t window) {
  uint32_t values[] = {XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE};
  xcb_change_window_attributes(
    connection,    window,    XCB_CW_EVENT_MASK,    values
  );
}

int main() {
  xcb_connection_t *connection = xcb_connect(NULL, NULL);
  const xcb_setup_t *setup = xcb_get_setup(connection);

  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  xcb_window_t root = iter.data->root;
	xcb_window_t window = xcb_generate_id(connection);
	xcb_create_window(
		connection,
		0,
		window,
		iter.data->root,
		0,
		0,
		iter.data->width_in_pixels,
		iter.data->height_in_pixels,
		0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,		iter.data->root_visual,
		0,
		NULL
	);
	xcb_intern_atom_cookie_t intern_state_cookie = xcb_intern_atom(
			connection, 1, strlen("_NET_WM_STATE"), "_NET_WM_STATE"
	);
	xcb_intern_atom_reply_t *intern_state_reply = xcb_intern_atom_reply(
			connection, intern_state_cookie, NULL
	);

	xcb_intern_atom_cookie_t intern_fullscrn_cookie = xcb_intern_atom(
			connection, 1, strlen("_NET_WM_STATE_FULLSCREEN"), "_NET_WM_STATE_FULLSCREEN"
	);
	xcb_intern_atom_reply_t *intern_fullscrn_reply = xcb_intern_atom_reply(
			connection, intern_fullscrn_cookie, NULL
	);

	int atom_states[] = {intern_fullscrn_reply->atom};
	// cannot put XCB_ATOM_ANY into change property. WHY!!!!!!
	// there has to be someway to get ATOM TYPE. If not, this is going to be super confusing moving on...
	xcb_change_property(
		connection,
		XCB_PROP_MODE_APPEND,
		window,
		intern_state_reply->atom,
		XCB_ATOM_ATOM,
		32,
		1,
		atom_states
	);
	xcb_change_property (
		connection,
		XCB_PROP_MODE_REPLACE,
		window,
		XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING,
		8,
		strlen ("screenpew"),
		"screenpew"	);
	add_mouse_event(connection, window);
	xcb_map_window(connection, window);
	xcb_screen_next(&iter);
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
	xcb_translate_coordinates_reply_t *trans1 = xcb_translate_coordinates_reply (
			connection,	
			xcb_translate_coordinates(connection, window, root, press_x, press_y),
			NULL
	);
	xcb_translate_coordinates_reply_t *trans2 = xcb_translate_coordinates_reply (
			connection,
			xcb_translate_coordinates(connection, window, root, release_x, release_y),
			NULL
	);
  get_image(connection, root, trans1->dst_x, trans1->dst_y, trans2->dst_x, trans2->dst_y);
  xcb_disconnect(connection);
  return 0;
}
