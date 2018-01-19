* There is a bug only seen when playing PONG. When the right side player moves off the screen (either up or down)
the interpreter receives a SIGSEGV and crashes. gdb reports that the crash occurs in libSDL2-2.0.so.0, but I don't
think that it's an SDL bug.
