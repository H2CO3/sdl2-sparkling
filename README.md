# SDL2 bindings for Sparkling

## Basic usage

1. Import SDL
2. Open window(s) to draw on
3. Draw window; render
4. Handle events
5. goto 3

<!-- commity-comment -->

    // 1.
    let SDL = dynld("sdl2");

    // 2.
    let window = SDL::OpenWindow("Title", optionalWidth, optionalHeight);

    while true {
        // 3.
        window.fillRect(0, 0, window.width, window.height);
        window.refresh();

        // 4.
        let event;
        while (event = SDL::PollEvent()) != nil {
            // do something with 'event' here...
        }

        // 5.
    }

Check out the Documentation, starting with [SDL.md](../tree/master/doc/SDL.md),
on how to get started and for further reference.

## Installing

First things first, make sure that [Sparkling](https://github.com/H2CO3/Sparkling)
is installed.
Then, compile this with `make` (no extra flags are necessary).  
Finally, you must place the resulting `.dylib` file in the same directory as
`libspn.dylib`, which should be `/usr/local/lib` (unless you specified another
installation directory).

## Examples

For example code, see the Sparkling files:

- [`example_docsig.spn`](../tree/master/examples/example_docsig.spn)
- [`example_graph.spn`](../tree/master/examples/example_graph.spn)
- [`demo_linear_gradient.spn`](../tree/master/examples/demo_linear_gradient.spn)
- [`demo_radial_gradient.spn`](../tree/master/examples/demo_radial_gradient.spn)
- [`demo_conical_gradient.spn`](../tree/master/examples/demo_conical_gradient.spn)
