# fsh

A simple bash clone shell written from scratch for learning purposes.

Right now, it supports basic command execution and filesystem navigation (with the traditional command `cd`). Commands are tokenized by `;` as in bash, which means that multiple commands can be entered in the same line, as long as they are separated by `;`, and they will be executed one after the other serially.

My goal with this project is to learn about UNIX/Linux internals. Here are some important milestones that I'd like to reach in the near future:

- [ ] Job control (e.g. `./myprog &` to run in the background) -- *UPDATE*: This is harder than it looks.
- [X] Pipes
- [X] stdin and stdout file redirection with `<` and `>` (and also support append mode with `>>`)

There is much, much more to say about a shell. I consider these to be part of a shell's core functionality, so I'll start by working on them.

## TODO

- [ ] Improve error handling
- [ ] Rewrite the parser using proper tools

## Installation and Usage

The shell is very primitive right now; it can be compiled and executed as any other program.

To really use it as a shell, create a user and edit that user's entry in `/etc/passwd` so as to point the shell executable path to the path of the compiled binary of `fsh`. Upon login, the system will run `fsh` as the user's shell.

Example for user `fshtest` after copying the compiled binary into `/bin/fsh`:

```
# emacs /etc/passwd
```

Find the entry for `fshtest`. It's similar to this:

```
fshtest:x:1001:1002:,,,:/home/fshtest:/bin/bash
```

Change `/bin/bash` to `/bin/fsh`, save and exit. Login with the user `fshtest` (you can switch to `fshtest` with `su - fshtest`) and you're done.
