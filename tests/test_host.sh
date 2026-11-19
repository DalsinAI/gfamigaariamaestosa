#!/bin/sh
# Host tests (Cradle repo_test).
set -eu
cd "$(dirname "$0")"
exec make test CFLAGS="-std=c99 -Wall -Wextra -Werror -O1"
