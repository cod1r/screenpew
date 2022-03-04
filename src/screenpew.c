#include <xcb/xcb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

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

void add_mouse_event(xcb_connection_t *connection, xcb_window_t window) {
  uint32_t values[2] = {XCB_EVENT_MASK_BUTTON_PRESS, XCB_EVENT_MASK_BUTTON_RELEASE};
  xcb_query_tree_cookie_t cookie = xcb_query_tree(connection, window);
  xcb_query_tree_reply_t *reply = xcb_query_tree_reply(connection, cookie, NULL);
  xcb_window_t *window_child = xcb_query_tree_children(reply);
  int length = xcb_query_tree_children_length(reply);
  for (int i = 0; i < length; i++) {
    xcb_change_window_attributes(
      connection, window_child[i], XCB_CW_EVENT_MASK, values
    );
    add_mouse_event(connection, window_child[i]);
  }
}

int main() {
  xcb_connection_t *connection = xcb_connect(NULL, NULL);
  const xcb_setup_t *setup = xcb_get_setup(connection);

  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

  int screen_number = 1;
  while (iter.rem != 0) {
    xcb_get_geometry_cookie_t geo_cookie = xcb_get_geometry(connection, iter.data->root);
    xcb_get_geometry_reply_t *geo_reply = xcb_get_geometry_reply(connection, geo_cookie, NULL);
    get_image(connection, iter.data->root, 0, 0, geo_reply->width, geo_reply->height, screen_number);
    screen_number++;
    xcb_screen_next(&iter);
  }

  xcb_flush(connection);
  // xcb_generic_event_t *event;
  // while ((event = xcb_wait_for_event(connection))) {
  //   printf("%d %d\n", ((xcb_motion_notify_event_t *)event)->event_x, ((xcb_motion_notify_event_t *)event)->event_y);
  //   free(event);
  // }
  return 0;
}