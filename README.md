# CHIP-8-interpreter
An interpreter for the CHIP-8 programming language written in C

# What is CHIP-8?


CHIP-8 CHIP-8 is an interpreted programming language developed by Joseph Weisbecker in the 1970's.
It was initially used on the COSMAC VIP and Telmac 1800 8-bit microcomputers. CHIP-8 programs are meant to be run on a virtual machine,
although call it an _emulator_, because most of the implementations try to emulate the machines in which it was originally developed and used.

# Screenshots

* Space Invaders - Initial screen
![Space invaders](Screenshots/Space-Invaders.png)

* Space Invaders - game screen
![Space invaders 2](Screenshots/Space-Invaders-2.png)

* Pong
![Pong](Screenshots/Pong.png)

* Tetris
![Tetris](Screenshots/Tetris.png)

* Tic Tac Toe
![TicTacToe](Screenshots/TicTacToe.png)

# Features

* It works (_cough_ sort of _cough_)
* It doesn't crash (most of the times)
* It doesn't kills your computer (hope so)
* Implemented all the opcodes

# Planned features
* Multi-platform support (main targets are Windows and OS X)
* Implement sound
* Command line options (like help, show keymaps, etc)
* Support for keys to, for example, reset the interpreter, change game without having to close the windows first, pause/unpause, etc.

# Building from sources

Until now it has only been tested on Debian GNU/Linux 9.3 (Stretch). Support for other platforms is planned.
The interpreter is written in C99, so a proper compiler is needed. A recent version of GCC or Clang should be fine.
The only dependencies are [SDL 2](https://www.libsdl.org/) and the make utility to build the project. Installation
of SDL is very platform specific, so links to the download pages are provided at the end of this file. To install make
you need to:

```
sudo apt install make
```

Then, to build the project, go to the root directory of it and type:

```
make
```

It will generate an executable file, called `chip8`. To run it, you need to pass it the path to the rom file, like shown below:

```
./chip8 roms/PONG
```

You could also build the debug version, which prints a lot of boring stuff to the screen:

```
make debug
```

It will generate a file called `debug`, which you can run the same way as explained above.

You can close the window pressing the ESCAPE key.


# Resources
This is a list of the various resources that I used to develop the interpreter:

* http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
* http://mattmik.com/files/chip8/mastering/chip8.html
* https://en.wikipedia.org/wiki/CHIP-8
https://www.libsdl.org/download-2.0.php (Main SDL download page)
* https://wiki.libsdl.org/Installation#Linux.2FUnix (Instructions to install SDL on Linux)
