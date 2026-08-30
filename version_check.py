"""
Pre/post-build script: reminds you to check FIRMWARE_VERSION.
Reads the current value from Config.h and prints a colored warning
both before and after compilation.
"""

Import("env")

import re

# ANSI color codes
YELLOW = "\033[1;33m"
CYAN = "\033[1;36m"
RED = "\033[1;31m"
RESET = "\033[0m"

def get_version():
    try:
        with open("include/Config.h", "r") as f:
            for line in f:
                m = re.search(r'FIRMWARE_VERSION\s*=\s*"([^"]+)"', line)
                if m:
                    return m.group(1)
    except FileNotFoundError:
        pass
    return "unknown"

def print_version_reminder(*args, **kwargs):
    version = get_version()
    print()
    print(YELLOW + "!!" + RED + " " + "=" * 56 + " " + YELLOW + "!!" + RESET)
    print(YELLOW + "!!" + RESET + "   FIRMWARE_VERSION = " + CYAN + '"%s"' % version + RESET)
    print(YELLOW + "!!" + RESET + "   Update in include/Config.h if flashing a new release!")
    print(YELLOW + "!!" + RED + " " + "=" * 56 + " " + YELLOW + "!!" + RESET)
    print()

# Before build
print_version_reminder()

# After build
env.AddPostAction("buildprog", print_version_reminder)
