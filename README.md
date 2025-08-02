# ash - A Modern Linux Shell in C

## Overview

**ash** is a secure, feature-rich Linux shell written in C. It supports advanced command execution, pipes, redirection, logical operators, custom prompts, tab completion, aliases, environment management, and more.

## Features

- Customizable prompt with Linux distro icon and current directory
- Tab completion for executables in `/bin`, `/usr/bin`, `~/.local/bin`, and custom paths via `PATH+=` in `~/.ashrc`
- Built-in commands: `cd`, `clear`, `history`, `version`, `status`, `export`, `help`
- Alias support via `~/.ashrc`
- Command history saved to `~/.ashhistory`
- Syntax highlighting for recognized commands
- Robust error handling and memory safety
- Background job support (`&`)
- Runs commands from `~/.ashrc` at startup
- Extensible and modular codebase

## Project Structure

```
ash/
├── CMakeLists.txt
├── Makefile (optional)
├── include/
│   └── ash.h
├── src/
│   ├── main.c
│   ├── builtins.c
│   ├── commands.c
│   ├── aliases.c
│   ├── ashrc.c
│   └── prompt.c
```

## Building

### Using CMake (recommended)

```sh
mkdir build
cd build
cmake ..
make
./ash
```

### Using Makefile (manual)

```sh
make
./ash
```

## Configuration

- **Aliases and PATH extensions:** Add to `~/.ashrc`
  ```
  alias ll='ls -l'
  PATH+=:/opt/bin
  ```

## Usage

- Type commands as in any shell.
- Use tab for completion.
- Built-in commands: `cd`, `clear`, `history`, `version`, `status`, `export`, `help`, `exit`
- Use `&` for background jobs.

## License

MIT License

## Author

aserdevyt (<aserdevyt@outlook.com>)
