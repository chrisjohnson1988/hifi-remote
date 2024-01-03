# Hi-Fi Remote

This is an IR relaying project for a yamaha Hi-Fi. It enables the relaying of
volume commands using the original remotes and defaults each source to a
preset default volume. In addition to the power state of the Hi-Fi can be controlled
by the original remote and using a 433mhz remote.

## Components

The project is build around a 5v 16mhz arduino and requires:

- A 433mhz receiver
- IR emitter
- IR receiver
- TIP120
- MT3608 DC-DC boost converter

Being a 5v arduino, the project can be powered by a USB power source using the
*VCC* and *GND* pins.

# Building

The [arduino code](remote/remote.ino) requires external libraries to be
installed in order to be able to compilation.

- [rc-switch](https://github.com/sui77/rc-switch) @ 436a74b03f3dc17a29ee327af29d5a05d77f94b9
- [ir-remote](https://github.com/z3t0/Arduino-IRremote) @ e1768b4debb03441e93a1f2d5f135111e98fdd97

The wiring diagram below describes how the arduino is wired up.

![Arduino Wiring](images/hifi-remote_bb.png)
