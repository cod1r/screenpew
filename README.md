# Screenshot software for X11 users

# Dependencies

1. X11 (obviously)
2. libpng
3. XCB (X11 C Bindings)

# Things to consider

- As of right now, the program doesn't account for MSB ( most significant byte ) first ordering or Big Endian ordering. If you still decide to use this, the colors will be messed up.

- The file name generated is not customizable. The default name is "screenshot&lt;current number of screenshot&gt;.png".

- The image data is not put into any clipboard buffers.

- You cannot specify where the screenshot image file will be generated. It will show up where the executable is ran.

# How it works

- We use the xcb_get_image, xcb_get_image_reply, and xcb_get_image_data functions provided by XCB to get the image data from the root screen(s)/display(s) from X11.

- We also use xcb_get_geometry, and xcb_get_geometry_reply to get the width and height information of the screen(s)/display(s).

- Once we have the data, we use libpng to produce a png file.

# Future plans

- Add methods to specify what areas of the screen(s)/display(s) we want to capture.

- Implement an method for users to specify what file name that is generated.

- Allow for the image data to be put into a clipboard buffer.