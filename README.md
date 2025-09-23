# Ash Shell 🐚
[![AUR version](https://img.shields.io/aur/version/ash-shell-git)](https://aur.archlinux.org/packages/ash-shell-git)
[![AUR votes](https://img.shields.io/aur/votes/ash-shell-git)](https://aur.archlinux.org/packages/ash-shell-git)
[![License](https://img.shields.io/github/license/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell/blob/main/LICENSE)
[![Top language](https://img.shields.io/github/languages/top/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell)
[![Languages count](https://img.shields.io/github/languages/count/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell)
[![Lines of code](https://img.shields.io/tokei/lines/github/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell)
[![Repo size](https://img.shields.io/github/repo-size/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell)
[![Last commit](https://img.shields.io/github/last-commit/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell/commits/main)
[![Commit activity](https://img.shields.io/github/commit-activity/m/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell/commits/main)
[![Contributors](https://img.shields.io/github/contributors/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell/graphs/contributors)
[![Open issues](https://img.shields.io/github/issues/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell/issues)
[![Pull requests](https://img.shields.io/github/issues-pr/aserdevyt/ash-shell)](https://github.com/aserdevyt/ash-shell/pulls)
[![Releases](https://img.shields.io/github/v/release/aserdevyt/ash-shell?include_prereleases)](https://github.com/aserdevyt/ash-shell/releases)
[![GitHub stars](https://img.shields.io/github/stars/aserdevyt/ash-shell?style=social)](https://github.com/aserdevyt/ash-shell/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/aserdevyt/ash-shell?style=social)](https://github.com/aserdevyt/ash-shell/network/members)
[![GitHub watchers](https://img.shields.io/github/watchers/aserdevyt/ash-shell?style=social)](https://github.com/aserdevyt/ash-shell/watchers)


__ash-shell__ is a linux shell designed for gnu/linux 

## ⚠️warning:
this shell is full of buggs and in an early stage i don't recomend using it daily untill i make sure that it is safe enough to use, but feel free to use in a __VM__ or in a test enviroment

# features 
__POSIX-Compliant Core:__  The fundamental process management, I/O redirection, and signal handling are built on POSIX standards for reliability and portability.

__Advanced Command Execution:__ Supports pipelines ```|```, logical AND ```&&```, logical OR ```||``` and running apps in the background ```&```(buggy) 

__I/O Redirection:__ Handles input```<``` and output ```>```(buggy) and redirecting to a text file ```>>```

__Built-in Commands:__  Includes essential commands like ```cd```, ```exit```, ```history```, ```help```, ```jobs```,```fg```, and ```bg```(sometimes buggy)

__Job Control:__(bugged): manage em with ```jobs``` make em foreground ```fg``` and move em to background with ```bg```

__Configuration:__ Supports a customizable user experience through a startup script ```~/.ashrc``` and a config file ```~/.config/ash.conf``` 

__prompt:__  Displays the current working directory and a customizable icon based on your Linux distribution.
.
<br>
<img width="212" height="28" alt="2025-08-03T18:53:07,872125896+03:00" src="https://github.com/user-attachments/assets/a34fc61d-536f-488a-be9c-f7cd4cc4e6b6" />
<br>
prompt
<br>
<img width="1140" height="401" alt="2025-08-03T18:53:17,947623250+03:00" src="https://github.com/user-attachments/assets/550df21f-a69f-4809-b01e-cb2156e4a921" />
<br>
help command
<br>

__Alias Support:__ Define custom aliases in ```~/.ashrc``` to simplify complex or frequently used commands.

```ash
alias <alias_name>='<command>'
```

__Command History:__ Utilizes ```readline``` for a familiar interactive history and line editing experience, with history saved to ```~/.ashhistory``` 

__Variable Support:__ Assign and expand shell variables.

# how to script (so bugged)
You can write standard shell scripts and execute them with ```ash```. The scripting syntax is highly compatible with other POSIX-compliant shells(kinda). warning⚠️: run the script inside the shell not outside of it like do ```./script``` insted ``` ash script``` cuss it will freak out
```ash
#!/bin/ash
# A simple example script for the ash shell.

# 1. Variable Assignment
# Sets the name of the log file we'll be working with.
LOG_FILE="script_output.log"
echo "Using log file: $LOG_FILE"

# 2. Command Execution and Output Redirection (>)
# Creates a new file and writes the first line to it.
echo "Starting script at $(date)" > "$LOG_FILE"

# 3. Appending to a file (>>)
# Adds a second line without overwriting the first one.
echo "---" >> "$LOG_FILE"
echo "This is a second line of text." >> "$LOG_FILE"

# 4. Pipelines (|)
# Lists files and pipes the output to grep to check for the log file.
echo
echo "Verifying file creation with a pipeline:"
ls -l | grep "$LOG_FILE"

# 5. Logical AND (&&)
# This command runs only if the previous grep command was successful.
grep "$LOG_FILE" "$LOG_FILE" && echo "File check successful!"

# 6. Basic command execution
# Displays the final contents of the log file.
echo
echo "Final contents of the log file:"
cat "$LOG_FILE"

# 7. Clean up
# Removes the created file.
rm "$LOG_FILE"
echo
echo "Script finished. File '$LOG_FILE' has been removed."
```

# using ```~/.ashrc```

The ```~/.ashrc``` file is executed every time the shell starts up. This is the ideal place to define aliases and set up your environment.

```conf
# Set aliases for common commands
alias ll='ls -alF'
alias h='history'
alias gcl='git clone'

# Add custom paths to the PATH environment variable
export PATH+=:/path/to/my/bin
```

Modifying ```~/.config/ash.conf```

```conf
# ash-shell configuration file
#
# first_time: Whether this is the first time running the shell.
#   - This is automatically set to true after the first run.
#
# hide_icon: Set to true to hide the Linux distro icon from the prompt.
#   - To hide the icon, change this to `hide_icon=true`.

first_time=true
hide_icon=false
```

# installation

### pacman repo
```
bash <(curl -fsSL https://raw.githubusercontent.com/aserdevyt/aserdev-repo/refs/heads/main/install.sh) && sudo pacman -Sy ash-shell
```

### from the aur(archlinux)

```bash
# Using yay
yay -S ash-shell-git

# Or using paru
paru -S ash-shell-git
```
This will automatically build the latest version of the shell from the Git repository.

### building 

If you prefer to build the shell manually, you can use CMake and Make.

```bash
git clone https://github.com/aserdevyt/ash-shell.git
cd ash-shell
mkdir build
cd build
cmake ..
make
```
#### install
```bash
sudo make install
```
# contribute 

fell free to __contribute__ to this project in github
