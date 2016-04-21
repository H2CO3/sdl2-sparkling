# Audio class

Programming with Audio is considerably frustrating since it requires passing
around a lot of binary data, pointers and audio values.

Fear not: the combination of SDL's abstraction and Sparkling's convenience
*shall* deliver!

## Observations

Audio is supported thanks to SDL2_mixer. Now, this means that if you wish
to use this class to its full potential, a.k.a. read formatted audio files
(like MP3, OGG, etc…), you must make sure that your SDL2_mixer build was compiled
with the appropriate libraries linked to it.

## Arguments

	Music OpenMusic(integer frequency, string format, integer channels, integer sample)

These are the available initializers for audio.
Before you can play around with audio, you need to pay special attention to the
required arguments:

\#1 - **frequency** specifies the number of sample frames sent to the sound device per
second. Common values are 11025, 22050, 44100 and 48000.
Larger values produce cleaner audio, in much the same way that larger
resolutions produce cleaner graphics.

\#2 - **format** specifies the size and type of each sample element and may be one
of the following (considering the left column is a string):

| |                           **8-bit support**                              |
| ------- | ---------------------------------------------------------------- |
| S8      | signed 8-bit samples                                             |
| U8      | unsigned 8-bit samples                                           |
| |                          **16-bit support**                              |
| S16     | signed 16-bit samples in little-endian byte order                |
| S16MSB  | signed 16-bit samples in big-endian byte order                   |
| S16SYS  | signed 16-bit samples in system order                            |
| U16     | unsigned 16-bit samples in little-endian byte order              |
| U16MSB  | unsigned 16-bit samples in big-endian byte order                 |
| U16SYS  | unsigned 16-bit samples in system order                          |
| |                         **32-bit support**                               |
| S32     | 32-bit integer samples in little-endian byte order               |
| S32MSB  | 32-bit integer samples in big-endian byte order                  |
| S32SYS  | 32-bit integer samples in system order                           |
| |                   **float support (new to SDL 2.0)**                     |
| F32     | 32-bit floating point samples in little-endian byte order        |
| F32MSB  | 32-bit floating point samples in big-endian byte order           |
| F32SYS  | 32-bit floating point samples in system order                    |

*Note*: if you miswrite the format string, the library will halt to avoid further
trouble.

\#3 - **channels** specifies the number of output channels.
Supported values are 1 (mono), 2 (stereo), 4 (quad), and 6 (5.1).

\#4 - **sample** specifies the size of the audio buffer in sample frames.
A sample frame is a chunk of audio data of the size specified in format
multiplied by the number of channels. This field's value must be a power of two.

Here's a quick example:

	SDL::OpenMusic(44100, "S16", 2, 2048)

To better diagnose issues coming from an irrelevant buffer size,
just hear the result if you are not satisfied with your sound output:

* crackle and unpleasant metal noises: the buffer size might be set **too low**
* lots of lag, sound effects seem to be output some time after their cause:
the buffer size might be set **too high**

## Shared Functionality

Audio class is composed of a Music subclass. Even though each (to-be-supported)
has their own set of methods, some are shared.

	array listDecoders()

Returns an array of available decoders. The list differs for either music or chunks.
The number of decoders can also be different for each run of a program due to the
change in availability of shared libraries that support each format.

## Music subclass

Music is an Audio subclass.

	boolean load(string filename)

Attempts to load the given filename as music.
Returns `true` if the file was loaded successfully, `false` otherwise.
Call `SDL::GetMixError()` to check what went wrong if the file wasn't loaded.

	getType()

Tells you the file format encoding of the music.
This may be handy when used with `setPosition()`, and other music functions
that vary based on the type of music being played.

	nil play(integer loops)

Plays the loaded music (meaning, requires calling `load()` first)
a `loops` number of times. If -1 is given, it'll loop indefinitely.

	nil fadeIn(integer loops, integer milliseconds [, number position])

Behave like `play()` with the addition of a fade-in in milliseconds and
an optional position to start the music from.

	nil fadeOut(integer milliseconds)

This one fades out the music from its current position.

	number volume([number volume])

Controls the volume, which can be set between 0 and 1.
Returns the previous set volume (or current, if no argument is given).

	nil pause()
	nil resume()
	nil rewind()

Self-explanatory: respectively pause, resume and rewind the music playback.

	boolean isPlaying()
	boolean isPaused()

Self-explanatory as well: respectively tell you if the music is playing or paused.

	[ nil | string ] isFading()

This one tells if the music is fading in, out, or not at all (in which case it'll
be `nil`).

	nil setPosition(number position)

Sets the "cursor" of the playback of the currently playing music

	nil halt()

Halts playback of music. This interrupts music fader effects.

	nil fromCMD(string command)

Sets up a command line music player to use to play music.
Any music playing will be halted.
You should set the music volume in the music player's command if the music
player supports that. Looping music works, by calling the command again when
the previous music player process has ended.

**Note**: Commands are not totally portable, so be careful.
