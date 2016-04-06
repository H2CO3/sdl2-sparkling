# Audio class

Programming with Audio is considerably frustrating since it requires passing
around a lot of binary data, pointers and audio values.

Fear not: the combination of SDL's abstraction and Sparkling's convenience
*shall* deliver!

	nil close()

Closes the selected audio device.

	string getStatus()

Returns the audio device's current status.

	nil pause()
	nil resume()

Self-explanatory: respectively pause and resume the audio device's playback.

## AudioSpec

Creating an audio device for SDL is easier said than done.

Before you use `OpenAudioDevice()`, you need to craft an array with values in
a specific order. The order is as follows (along with additional information):

\#1 - **freq** specifies the number of sample frames sent to the sound device per
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
| F32MSB  | 32-bit floating point samples in system order                    |

*Note*: if you miswrite the format string, the program will halt to avoid further
trouble.

\#3 - **channels** specifies the number of output channels.
Supported values are 1 (mono), 2 (stereo), 4 (quad), and 6 (5.1).

\#4 - **samples** specifies the size of the audio buffer in sample frames.
A sample frame is a chunk of audio data of the size specified in format
multiplied by the number of channels. This field's value must be a power of two.

Here's a quick example:

	[48000, "F32", 2, 4096]
