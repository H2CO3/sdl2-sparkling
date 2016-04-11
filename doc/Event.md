# Event class

An object instanced by the `Event` class has the following properties:

 - `type`: string describing the type of the event
 - `timestamp`: implementation-specific integral timestamp

Depending on the `type` of the event, the following additional
properties are also available:

 - if type is `timer`, then `ID` will contain the timer object that
   generated the event.
 - if type is `quit`, then there are no additional properties.
 - if type is `window`, then:
   - `ID` is the integer window ID of the window that generated the event
   - `name` is the event name as a string, e. g. `focus_gained`,
     `focus_lost`, `resized`, etc.
   - `data1` and `data2` are two integers. Their semantics is dependent
      on the event name; see
      [the documentation of SDL_WindowEventID](https://wiki.libsdl.org/SDL_WindowEventID)
      for more info.
 - if type is `drop`, then:
 	- `value` is the filename (including path) of the dropped file
	- For Mac OS X, in order to enable drag&drop on an SDL app,
	`info.plist` must *also* be edited: simply Add/Modify **Document Types**.
	For example, to enable all document types, add the "public.data" mime
	type as a document type.
 - if type is `keyboard`, then:
   - `value` is a string describing the key that sent the event
   - `state` is `true` if the key was pressed, `false` if it was released
   - `modifier` is a hashmap of booleans with keys like `lshift`,
     `rshift`, `lctrl`, `lctrl`, etc. (see source code for all keys).
	 These flags represent the status of the modifier keys pressed
	 simultaneously with they key.
 - if type is `mousebutton`, then:
   - `button` is the name of the button
   - `state` is `true` for a pressed button, `false` for a released button
   - `count` is the (integer) number of clicks
   - `x` and `y` are the coordinates of the click
 - if type is `mousemove`, then:
   - `x` and `y` are the coordinates of the cursor
   - `dx` and `dy` are the relative movements since the last motion event
   - `buttons` are a set of Boolean flags, describing the state of the
     mouse buttons (see source for more info).
 - if type is `mousewheel`, then `x` and `y` are the relative changes in
   the motion of the wheel.
 - if type is `touchdown`, `touchup` or `touchmove`, then:
   - `finger` is an implementation-defined unique ID of the finger sending
     the event
   - `x`, `y`, `dx` and `dy` are normalized (to the [0...1] closed interval)
     values of the coordinates and relative displacement of the finger,
	 respectively
   - `pressure` is the applied force (normalized to the interval [0...1])
 - if type is `gesture`, then:
   - `fingerCount` is the number of fingers participating in the multi-touch
     gesture
   - `x` and `y` are the normalized coordinates of the centerpoint/centroid
     of the touches that form the gesture [0...1]
   - `rotation` is the angle of rotation, in radians, since the last gesture
     event
   - `distance` is the normalized [0...1] movement of the fingers since
     the last gesture
