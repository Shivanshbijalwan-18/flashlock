#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

CC=${CC:-cc}
STRICT_FLAGS="-std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -pthread"

if command -v timeout >/dev/null 2>&1; then
    run_timed() { timeout 20s "$@"; }
else
    echo "NOTE: timeout is unavailable; workload hang checks are disabled." >&2
    run_timed() { "$@"; }
fi

# Word splitting is intentional so callers may provide a compiler with flags.
# shellcheck disable=SC2086
$CC $STRICT_FLAGS "$ROOT/main.c" -o "$TMP_DIR/flashlock-engine"
# shellcheck disable=SC2086
$CC $STRICT_FLAGS "$ROOT/deadlock_demo.c" -o "$TMP_DIR/deadlock-demo"

check_run() {
    expected_requests=$1
    expected_successful=$2
    expected_rejected=$3
    capacity=$4
    shift 4
    output=$(run_timed "$TMP_DIR/flashlock-engine" "$@")

    submitted=$(printf '%s\n' "$output" | awk '/Requests submitted/ {print $4}')
    processed=$(printf '%s\n' "$output" | awk '/Requests processed/ {print $4}')
    successful=$(printf '%s\n' "$output" | awk '/Simulated success/ {print $4}')
    rejected=$(printf '%s\n' "$output" | awk '/Simulated rejected/ {print $4}')
    depth=$(printf '%s\n' "$output" | awk '/Maximum queue depth/ {print $4}')

    [ "$submitted" = "$expected_requests" ]
    [ "$processed" = "$submitted" ]
    [ "$successful" = "$expected_successful" ]
    [ "$rejected" = "$expected_rejected" ]
    [ "$((successful + rejected))" -eq "$processed" ]
    [ "$depth" -ge 1 ]
    [ "$depth" -le "$capacity" ]
    [ "$depth" -le "$expected_requests" ]
}

check_run 37 30 7 1 37 3 1 2
check_run 1 1 0 1 1 1 1 1
check_run 1 1 0 64 1 64 64 64
check_run 20 16 4 4 20 4 4 64

if run_timed "$TMP_DIR/flashlock-engine" 0 >/dev/null 2>&1; then
    echo "zero requests should be rejected" >&2
    exit 1
fi
if run_timed "$TMP_DIR/flashlock-engine" 10 2 4 1 extra >/dev/null 2>&1; then
    echo "extra arguments should be rejected" >&2
    exit 1
fi
if run_timed "$TMP_DIR/flashlock-engine" -1 >/dev/null 2>&1; then
    echo "negative requests should be rejected" >&2
    exit 1
fi

demo_output=$(run_timed "$TMP_DIR/deadlock-demo")
printf '%s\n' "$demo_output" | grep -F "Opposing lock order detected; trylock released safely (no hang)." >/dev/null

echo "All OS engine tests passed."
