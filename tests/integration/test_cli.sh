#!/usr/bin/env bash
# Integration tests for the compiled ./maze_escape binary.
#
#   bash tests/integration/test_cli.sh
#
# Environment:
#   BIN                  path to the binary          (default: <repo>/maze_escape)
#   STRICT_KNOWN_BUGS=1  known bugs count as failures
#   REQUIRE_XVFB=1       FAIL (instead of skip) when no display / xvfb-run is available
#   SMOKE_SECONDS=N      how long each window smoke-test keeps the game open (default 1)
#   REPORT               JUnit XML output            (default: tests/reports/integration.xml)
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${BIN:-$ROOT/maze_escape}"
STRICT="${STRICT_KNOWN_BUGS:-0}"
REQUIRE_XVFB="${REQUIRE_XVFB:-0}"
SMOKE_SECONDS="${SMOKE_SECONDS:-1}"
REPORT="${REPORT:-$ROOT/tests/reports/integration.xml}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# The window smoke tests need an X display. If there is none, re-run this
# very script inside a virtual one (xvfb-run) so everything shares it.
if [ -z "${DISPLAY:-}" ] && [ -z "${MAZE_IN_XVFB:-}" ] && command -v xvfb-run >/dev/null 2>&1; then
	export MAZE_IN_XVFB=1
	exec xvfb-run -a -s "-screen 0 1920x1080x24" bash "${BASH_SOURCE[0]}" "$@"
fi

PASSED=0; FAILED=0; KNOWN=0; SKIPPED=0
XML_BODY=""

xml_escape() { sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e 's/"/\&quot;/g'; }

record() { # status name [message]
	local status="$1" name="$2" msg="${3:-}" esc
	esc="$(printf '%s' "$msg" | xml_escape)"
	case "$status" in
		pass)  PASSED=$((PASSED+1));  printf '[  PASS ] %s\n' "$name"
		       XML_BODY+="  <testcase classname=\"integration\" name=\"$name\"/>"$'\n' ;;
		fail)  FAILED=$((FAILED+1));  printf '[  FAIL ] %s\n         %s\n' "$name" "${msg//$'\n'/$'\n         '}"
		       XML_BODY+="  <testcase classname=\"integration\" name=\"$name\"><failure message=\"$esc\"/></testcase>"$'\n' ;;
		known) printf '[ XFAIL ] %s  -- KNOWN BUG: %s\n' "$name" "$msg"
		       if [ "$STRICT" = "1" ]; then
		           FAILED=$((FAILED+1))
		           XML_BODY+="  <testcase classname=\"integration\" name=\"$name\"><failure message=\"known bug still present: $esc\"/></testcase>"$'\n'
		       else
		           KNOWN=$((KNOWN+1))
		           XML_BODY+="  <testcase classname=\"integration\" name=\"$name\"><skipped message=\"KNOWN BUG: $esc\"/></testcase>"$'\n'
		       fi ;;
		xpass) printf '[ XPASS ] %s  -- known bug seems FIXED, remove its known_bug marker\n' "$name"
		       PASSED=$((PASSED+1))
		       XML_BODY+="  <testcase classname=\"integration\" name=\"$name\"/>"$'\n' ;;
		skip)  SKIPPED=$((SKIPPED+1)); printf '[  SKIP ] %s  (%s)\n' "$name" "$msg"
		       XML_BODY+="  <testcase classname=\"integration\" name=\"$name\"><skipped message=\"$esc\"/></testcase>"$'\n' ;;
	esac
}

# check NAME COND-RESULT MESSAGE   (COND-RESULT: 0 = ok)
check() { if [ "$2" -eq 0 ]; then record pass "$1"; else record fail "$1" "$3"; fi; }
known_bug() { # NAME COND-RESULT REASON  (COND-RESULT 0 = behaviour is correct, i.e. bug fixed)
	if [ "$2" -eq 0 ]; then record xpass "$1"; else record known "$1" "$3"; fi
}

# Run the game with NO display, so any window creation would fail loudly:
# proves a given map is rejected (or accepted) *before* MLX is touched.
run_nodisplay() { # args...  -> sets OUT, RC
	OUT="$(env -u DISPLAY timeout 10 "$BIN" "$@" 2>&1)"; RC=$?
}

PACMAN="$ROOT/textures/pacman"
echo "maze-escape integration tests (binary: $BIN)"; echo

# ---------------------------------------------------------------- binary ----
if [ ! -x "$BIN" ]; then
	record fail "binary_exists_and_is_executable" "$BIN not found - run 'make linux' first"
	FAILED=1
else
	record pass "binary_exists_and_is_executable"

	# ------------------------------------------------------------- help ----
	run_nodisplay
	[ $RC -eq 0 ] && [[ "$OUT" == *"Usage:"* ]]; check "no_arguments_prints_help_and_exits_0" $? "rc=$RC out=$OUT"
	run_nodisplay --help
	[ $RC -eq 0 ] && [[ "$OUT" == *"Usage:"* ]]; check "long_help_flag" $? "rc=$RC out=$OUT"
	run_nodisplay -h
	[ $RC -eq 0 ] && [[ "$OUT" == *"Usage:"* ]]; check "short_help_flag" $? "rc=$RC out=$OUT"
	run_nodisplay "$ROOT/levels/lvl1"
	[ $RC -eq 0 ] && [[ "$OUT" == *"Usage:"* ]]; check "one_argument_prints_help" $? "rc=$RC out=$OUT"
	run_nodisplay a b c
	[ $RC -eq 0 ] && [[ "$OUT" == *"Usage:"* ]]; check "too_many_arguments_prints_help" $? "rc=$RC out=$OUT"
	run_nodisplay --help "$PACMAN"
	[ $RC -eq 0 ] && [[ "$OUT" == *"Usage:"* ]]; check "help_flag_wins_over_other_arguments" $? "rc=$RC out=$OUT"
	[[ "$OUT" != *"Segmentation"* ]]; check "help_does_not_crash" $? "$OUT"

	# the help text must only advertise levels/textures that really exist
	run_nodisplay
	HELP="$OUT"; MISSING=""
	for p in $(printf '%s\n' "$HELP" | grep -oE '(levels/lvl[0-9]+|textures/[a-z-]+)'); do
		[ -e "$ROOT/$p" ] || MISSING+=" $p"
	done
	[ -z "$MISSING" ]; check "help_only_lists_existing_levels_and_textures" $? "missing:$MISSING"

	# -------------------------------------------------- invalid map files ----
	# name | file content (printf %b) | expected message
	while IFS='|' read -r name content expect; do
		[ -z "$name" ] && continue
		printf '%b' "$content" > "$WORK/$name.ber"
		run_nodisplay "$WORK/$name.ber" "$PACMAN"
		if [[ "$OUT" == *"$expect"* && "$OUT" != *"Error initializing MLX"* ]]; then
			record pass "rejects_$name"
		else
			record fail "rejects_$name" "expected message '$expect' before any window is created; rc=$RC out=$OUT"
		fi
	done <<'MAPS'
no_player|11111\n100C1\n1E001\n11111|exactly one player
two_players|11111\n1P0C1\n1E00P\n11111|exactly one player
no_exit|11111\n1P0C1\n10001\n11111|exactly one exit
two_exits|11111\n1P0C1\n1E0E1\n11111|exactly one exit
no_collectible|11111\n1P001\n1E001\n11111|at least one collectible
not_rectangular|11111\n1P0C1\n1E01\n11111|not rectangular
hole_in_top_wall|11011\n1P0C1\n1E001\n11111|not enclosed
hole_in_left_wall|11111\n0P0C1\n1E001\n11111|not enclosed
hole_in_right_wall|11111\n1P0C1\n1E000\n11111|not enclosed
MAPS
	printf '11111\n1P0X1\n1E0C1\n11111' > "$WORK/badchar.ber"
	run_nodisplay "$WORK/badchar.ber" "$PACMAN"
	[ $RC -ne 0 ] && [[ "$OUT" == *"Invalid character 'X'"* ]]; check "rejects_invalid_character_with_nonzero_exit" $? "rc=$RC out=$OUT"

	run_nodisplay "$WORK/does_not_exist.ber" "$PACMAN"
	[ $RC -ne 0 ] && [[ "$OUT" == *"Error opening map file"* ]]; check "missing_map_file_fails_with_nonzero_exit" $? "rc=$RC out=$OUT"

	# every shipped level must get past validation (it then fails only at MLX init, as there is no display)
	for lvl in "$ROOT"/levels/*; do
		run_nodisplay "$lvl" "$PACMAN"
		[[ "$OUT" == *"Error initializing MLX"* ]]; check "shipped_level_passes_validation_$(basename "$lvl")" $? "rc=$RC out=$OUT"
	done

	# ---------------------------------------------------- known issues ------
	printf '11111\n1P0C1\n1E0E1\n11111' > "$WORK/exit_status.ber"
	run_nodisplay "$WORK/exit_status.ber" "$PACMAN"
	[ $RC -ne 0 ]; known_bug "invalid_map_gives_nonzero_exit_status" $? \
		"main() returns 0 after a map error, so scripts and CI cannot detect a failed run"

	printf '11111\n1P0C1\n1E001\n11011' > "$WORK/bottom.ber"
	run_nodisplay "$WORK/bottom.ber" "$PACMAN"
	[[ "$OUT" == *"not enclosed"* ]]; known_bug "binary_rejects_hole_in_bottom_wall" $? \
		"map with an open bottom wall is accepted"

	printf '11111\n1P0C1\n1E001\n11111\n' > "$WORK/newline.ber"
	run_nodisplay "$WORK/newline.ber" "$PACMAN"
	[[ "$OUT" != *"not rectangular"* ]]; known_bug "binary_accepts_map_with_trailing_newline" $? \
		"map files ending in a newline are rejected as 'not rectangular'"

	# ---------------------------------------- window smoke tests (X11) ------
	have_display=1
	if [ -z "${DISPLAY:-}" ]; then have_display=0; fi
	if ! command -v timeout >/dev/null 2>&1; then have_display=0; fi
	if [ $have_display -eq 0 ]; then
		if [ "$REQUIRE_XVFB" = "1" ]; then
			record fail "window_smoke_tests" "no X display and no xvfb-run (install xvfb), or 'timeout' missing; REQUIRE_XVFB=1"
		else
			record skip "window_smoke_tests" "no X display / xvfb-run / timeout available"
		fi
	else
		# The game keeps running in mlx_loop() until closed; "still alive when
		# timeout kills it" (rc 124) is the success signal. Anything else
		# (segfault 139, abort 134, early exit) is a failure.
		for lvl in "$ROOT"/levels/*; do
			for tex in "$ROOT"/textures/*/; do
				tex="${tex%/}"
				name="opens_window_$(basename "$lvl")_$(basename "$tex")"
				OUT="$(timeout "$SMOKE_SECONDS" "$BIN" "$lvl" "$tex" 2>&1)"; RC=$?
				if [ $RC -eq 124 ] && [[ "$OUT" != *"Error"* ]]; then
					record pass "$name"
				else
					record fail "$name" "expected game to keep running (rc 124), got rc=$RC out=$OUT"
				fi
			done
		done
		# known issue: a texture pack with a missing file makes the game segfault
		cp -r "$PACMAN" "$WORK/broken_pack" && rm -f "$WORK/broken_pack/wall.xpm"
		OUT="$(timeout "$SMOKE_SECONDS" "$BIN" "$ROOT/levels/lvl1" "$WORK/broken_pack" 2>&1)"; RC=$?
		[ $RC -lt 128 ]; known_bug "missing_texture_file_does_not_crash" $? \
			"set_img() never checks mlx_xpm_file_to_image() for NULL: a missing/invalid .xpm segfaults (rc=$RC)"
	fi
fi

# ----------------------------------------------------------------- report ---
mkdir -p "$(dirname "$REPORT")"
TOTAL=$((PASSED+FAILED+KNOWN+SKIPPED))
{
	echo '<?xml version="1.0" encoding="UTF-8"?>'
	echo "<testsuite name=\"maze-escape-integration\" tests=\"$TOTAL\" failures=\"$FAILED\" skipped=\"$((KNOWN+SKIPPED))\">"
	printf '%s' "$XML_BODY"
	echo '</testsuite>'
} > "$REPORT"

echo
echo "==== $TOTAL tests: $PASSED passed, $FAILED failed, $KNOWN known-bug (expected fail), $SKIPPED skipped ===="
[ "$FAILED" -eq 0 ]
