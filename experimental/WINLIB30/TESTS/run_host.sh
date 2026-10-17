#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cc -x c -std=c89 -Wall -Wextra -Wno-misleading-indentation -Werror \
 -fsanitize=address,undefined -g TESTS/host.c SOURCE/RAWFILE.C SOURCE/TESTCORE.C -o /tmp/tandy-format-host
/tmp/tandy-format-host
