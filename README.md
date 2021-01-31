Simple log server
=================

This is a simple log server programa. It creates an unix socket (by default
`/dev/log`)  and read log messages from it and writes to the configured log
files.

Build
=====

```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
```

Run
===

To run it you can type './log-server -h' to the command line options.
When running in daemon mode, it writes the messages to `/var/log/messages`,
otherwise it writes to the files passed in the command line.

```sh
$ ./log-server -s /tmp/log /tmp/messages
```
