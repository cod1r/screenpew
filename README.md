# Screenshot software for X11 users

## Dependencies

1. X11 (obviously)
2. libpng
3. XCB (X11 C Bindings)

## Things to consider

- As of right now, the program doesn't account for MSB ( most significant byte ) first ordering or Big Endian ordering. If you still decide to use this, the colors will be messed up.

- The file name generated is not customizable. The default name is "screenshot1.png".

- The image data is not put into any clipboard buffers.

- You cannot specify where the screenshot image file will be generated. It will show up where the executable is ran.

- At the moment, the fullscreen 'screenpew' window does not make it clear that it is opened up, fullscreen and capturing user input.

## How it works

1. We use the xcb_get_image, xcb_get_image_reply, and xcb_get_image_data functions provided by XCB to get the image data from the root screen(s)/display(s) from X11.

2. We then use xcb_get_geometry, and xcb_get_geometry_reply to get the width and height information of the screen(s)/display(s).

3. A fullscreen 'screenpew' window is opened (using EWMH (Extended Window Manager Hints). We set the _NET_WM_STATE atom to have the value of _NET_WM_STATE_FULLSCREEN which is an XCB_ATOM_ATOM type. The type matters heavily when using xcb_change_property (I don't know why it doesn't work with XCB_ATOM_ANY)), with the root screen(s)/displays(s) image data on it, and the user can click and drag to specify what area of the captured root image data they want to be outputted.

- Once we have the image data we need, we use libpng's simplified api to produce a png file.

## Future plans

- Add methods to specify what areas of the screen(s)/display(s) we want to capture.
  - As of right now, screenpew will open a fullscreen window and allow you to press mouse 1 and release with mouse 1 to specify what area you would like to capture.

- Implement an method for users to specify what file name that is generated.

- Allow for the image data to be put into a clipboard buffer.

- Implement some indicator on the 'screenpew' fullscreen window so that the user knows screenpew is running.

## How to Build

- Download and install the XCB library from the official XCB website.

- Download and install the libpng library from the official libpng website.

```
gcc screenpew.c -lxcb -lpng
```

## Important things to note so that we do not get more confused later

- row_stride is basically how wide each row is, in the raw image data that we get.

- Read XCB documentation carefully.

- Software from the late 20th century and early 21st century need help with documenting things better. :tired_face:

- Atom type matters when using xcb_change_property. I do not know why it doesn't work with XCB_ATOM_ANY. The types are specified in the EWMH spec. Ex: the type of _NET_WM_STATE_FULLSCREEN is Atom which is an XCB_ATOM_ATOM type, and the type of _NET_WM_NAME is a utf8-string (XCB_ATOM_STRING).

- I do not know why we have to specify how many 32-bit multiples of data we want to retrieve when using xcb_get_property. Why not just give the entirety of the needed information? This makes me just want to put a big enough number just so I can get all the data. I do not know of a current way to know the data size before hand.
