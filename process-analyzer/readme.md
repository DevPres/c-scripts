# pkiller

Script that retrieves processes running under the init process and displays them in the format:

```
pid -> name -> cmdline
```

## Rofi Integration

This script is designed to work with [rofi](https://github.com/davatorium/rofi/tree/next).
It lets you choose a process and, after confirmation, sends a `SIGTERM` signal to gracefully kill it.

### Build

```sh
gcc -O3 pkiller.c -o ~/.config/rofi/pkiller
```

### Usage

```sh
rofi -show pk -modi pk:~/.config/rofi/pkiller
```

## TODO

- Add possibility to forcefully kill a process
- Create a theme for this
