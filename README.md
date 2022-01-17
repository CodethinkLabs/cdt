`cdt`: Chrome DevTools Tool
===========================

This is a simple example client for the
[Chrome DevTools Protocol](https://chromedevtools.github.io/devtools-protocol/)
written in C using [Libwebsockets](https://libwebsockets.org/).

It serves as a simple example for how to connect to a Chrome DevTools Protocol using Libwebsockets.

Building
--------

This has a few dependencies, which are all packaged in Debian stable (version 11, codename Bullseye). The best way to see the dependencies is to consult the [.gitlab-ci.yml](.gitlab-ci.yml) file, which is maintained for CI purposes.

Once you have the dependencies installed, simply run:

```
make
```

That should build the executable, `cdt`.

Using
-----

At the moment the `cdt` tool expects to find a Chrome DevTools Protocol server at the fixed address of `localhost:9222`. To connect to a remote server I have used SSH port forwarding.

```
Usage:
  ./cdt <DISPLAY> <CMD>
```

All `cdt` commands take two common parameters. These are the DISPLAY to connect to, and the CMD (command) to run.

### DISPLAY

The DISPLAY is a string representing the display you want to interact with. If the DISPLAY you provide begins with a '/' character, it will treat it as the display's path and connect to `ws://localhost:9222[DISPLAY]`.

If the DISPLAY string does not start with a '/' character, it will fetch a display spec from `http://localhost:9222/json` and use the first display's webSocketDebuggerUrl for which the display has a "title" field containing the substring DISPLAY. So if a display has a "title" field of "FOOmy-targetBAR911", a DISPLAY of "my-target" would cause that display to match.

### CMD

If you run `cdt`  with a valid display, it will list the available commands.

At the moment, the available commands are:

| Command    | Function                                                        |
| ---------- | --------------------------------------------------------------- |
| sdl        | Interactive front end that renders display and supports tapping |
| tap        | Issues touch events to simulate tapping at given coordinate     |
| run        | Runs the supplied JavaScript script on the remote               |
| run-log    | Runs the supplied JavaScript on remote, capturing console.log   |
| screencast | Fetches continuous screenshots and saves locally                |
| screenshot | Fetches screenshot of the remote and saves locally              |

For example, if you run:

```
./cdt my-target tap
```

It will tell you that it needs X and Y coordinates, so you would run it like so:

```
./cdt my-target tap 100 100
```

Design
------

The source files in [src/cmd/handler/](src/cmd/handler/) each implement a self-contained example
command. The command handlers implement the callback table in
[src/cmd/private.h]([src/cmd/private.h]). The callback table represents the command lifecycle,
and is documented in [src/cmd/cmd.h](src/cmd/cmd.h). It should be simple to add more commands
by adding additional command handler implementations.

The main `cdt` code looks at all the available command callback tables for
one with a `cmd` name matching the given CMD argument, and uses that callback
table for the rest of the execution of the program.

Sometimes support for new messages will be needed, and support for converting
messages into a format for sending over the protocol is handled in the
[src/msg/handler/](src/msg/handler/) source files.
