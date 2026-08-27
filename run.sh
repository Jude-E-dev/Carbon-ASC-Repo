#!/bin/sh
#
# Convenience wrapper: sets up the environment (see env.sh) and runs the
# already-built groundCarbon binary, forwarding all arguments.
#
# Usage: ./run.sh soil-character binary 20000

sls_dir="$(cd "$(dirname "$0")" > /dev/null 2>&1 && pwd)"

# shellcheck disable=SC1091
. "$sls_dir/env.sh"

exec "$sls_dir/build/groundCarbon" "$@"
