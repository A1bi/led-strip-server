# led-strip-server

This little daemon relays incoming color information via network to up to two connected WS2812b LED strips.

## Building

First make sure you have compiled the [rpi_ws281x](https://github.com/jgarff/rpi_ws281x) library. Let the compiler find it by running:

```sh
export CPATH=$CPATH:/path/to/rpi_ws281x LIBRARY_PATH=$LIBRARY_PATH:/path/to/rpi_ws281x
```

Then you'll be able to compile this tool:

```sh
make
```

Set `BUILD=debug` to include debugging symbols.

## Usage

You can run the binary without any arguments. It will then default to channel 1 on GPIO pin 18 with 24 LEDs and channel 2 disabled. If you want to change your setup you can pass these values like so:

```sh
./led-strip-server PIN:LEDS PIN:LEDS
```

`PIN` being the GPIO pin used for the channel (can only be PWM ones) and `LEDS` being the number of LEDs connected to that channel. The second set is for the second channel.

## LED interface

The daemon listens for UDP packets on port `29384` which are structured as follows:

```
|     1 byte    |   2 bytes  | 1 byte | 1 byte | 1 byte | 1 byte | 1 byte | ...
--------------------------------------------------------------------------------
| channel index | LED offset | LED1 R | LED1 G | LED1 B | LED2 R | LED2 G | ...
```

You can include color information for a maximum of 483 LEDs in one packet. To reach more LEDs split it up into multiple packets and set the LED offset accordingly.

## Control interface

The daemon listens for TCP connections on port `29384`. It is a binary interface to control the setup and is structured like so:

```
|     1 byte    |     1 byte     |    1 byte     |     1 byte     | 1 byte |  1 byte  |
---------------------------------------------------------------------------------------
| channel 1 pin | channel 1 LEDs | channel 2 pin | channel 2 LEDs |  FPS   | activity |
```

- `FPS`: LED rendering refresh rate (times per second)
- `activity`: toggle activity indicators on the LED strip on/off
    - the first LED on the strip will show rendering activity
    - the second LED will show incoming UDP packets

The interface will respond to valid requests with `OK`, otherwise `NOK`.
