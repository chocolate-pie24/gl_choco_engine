#!/bin/bash

# Sanitizers
# Leak checking (LSan) is performed on Linux only.
# LeakSanitizer output may not appear in the VSCode Debug Console (it may only abort).
# If a leak is detected, run from a terminal to get the full leak report and stack traces.

./bin/gl_choco_engine halt_on_error=1:abort_on_error=1:detect_leaks=1, print_stacktrace=1:halt_on_error=1
