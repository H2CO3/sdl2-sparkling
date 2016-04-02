# Main SDL class

    let SDL = dynld("sdl2");
    SDL::Foo();

The code above gets you started on how to manage the functions mentioned in
this document. It's supposed to be straightforward, but the basic explanation is
that you need to declare a variable (or constant) that holds the information on
SDL2's bindings; in this case, you need to call `dynld()` in your script. Note
that the argument does not require the file extension (as Sparkling will do that
for you).

Do note that, although these are theoretically "classes objects", they are still
specifically crafted hashmaps.

<!-- commity-comment -->

    Window OpenWindow(string title [, integer width, integer height])

Opens a new window that you can draw on, returns the representing
window object. If width and height are omitted, window is opened
in fullscreen mode; `width` and `height` properties of window object
will reflect resolution of screen. The `ID` property of the window
object is an integer ID used throughout the event system.

    [ Event | nil ] PollEvent()

Returns an event object if there are any pending events to handle;
otherwise, returns nil.

<!-- commity-comment -->

    nil StartTimer(interval [, callback])

Sets up a timer on a background thread and returns the corresponding timer
object. `interval` is to be specified in seconds (may be fractional).
If present, `callback` must be a function which will be invoked on the
background thread every time the timer fires. If omitted, the timer
will generate a `timer` event instead (see above).

If the returned timer object is deallocated, the corresponding timer
object is stopped. (so, if you want to keep the timer alive, you must
hold a reference to the timer object, for example by storing it in a
variable or a data structure.)

    nil StopTimer(timer)

Stops the timer associated with `timer`. `timer` must be a timer
descriptor object returned by `StartTimer()`.

<!-- commity-comment -->

    hashmap GetPaths([string organization, string app])

Returns a hashmap with the current `base` path (where the executable is
located) and `pref` path (where you shall write files such as properties, saves,
among others). The arguments `organization` and `app` stand for the desired
organization name and the app's name respectively, and only serve to get
the `pref` path.

If `base` contains an empty string, then the current platform does not
support this feature. Note that `base` will point to the binary that is executing
your script.
If `pref` contains an empty string, either the given arguments aren't
correct (or given at all), or the current platform does not support
this feature.