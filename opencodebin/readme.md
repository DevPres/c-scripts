
# Opencodebin
Simple script to compose a Docker command to open "opencode" in Docker.

If not present create a folder `$HOME/.local/share/opencode`, where opencode save session and auth stuff.
If you pass a configuration path , if the folder is not present it will create it.

Yes it could be a simple bash file. But i want power, so i'm learning C. 

## Options:
```sh
  -m PATH    Mount path (default: ./)
  -d PATH    Destination path (default: /workspace)
  -c PATH    Configuration path (default /$HOME/.config/opencode/)
  -w PATH    Working directory (default: same as -d)
  -e PATH    Environment file path
```

## Build

```sh
gcc -O3 opencode.c -o ~/.local/bin/opencodedc
chmod +x ~/.local/bin/opencodedc
```

## Usage

Be sure to set $HOME environment variables.

```sh
opencodedc -m /path/to/folder/to/mount -d /workspace -e
```

## Opencode

If you don't know opencode check [here](https://github.com/sst/opencode)

## Todo

Let user change some default, like configuation path or environment path, example by config file or env variables
