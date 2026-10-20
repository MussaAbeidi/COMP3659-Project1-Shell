#!/usr/bin/env bash
#
# Test cases for mysh (COMP3659 Project 1 - Shell)
#
# Usage:
#   make
#   ./test_mysh.sh              # tests ./mysh
#   ./test_mysh.sh ./myshell    # or pass the path to your binary
#
# Each test feeds lines to the shell on standard input (one read() per line),
# then closes input like Ctrl+D, and checks stdout, stderr and any files made.
# Limits (BUFFER_SIZE, MAX_ARGS, MAX_TOKENS, MAX_PIPELINE_LEN, HEAP_SIZE) are
# read from your .h files so the boundary tests match your code.
#
# Best run on the course's Linux machine. On macOS, install `timeout` with
# `brew install coreutils` so a hanging test can't freeze the run, and expect
# small differences (macOS restarts read() after signals, and has extra
# commands like /usr/bin/exit).
#
# Tests that need a real keyboard (Ctrl+C, Ctrl+Z, fg/bg on stopped jobs,
# zombies) are listed in the MANUAL TESTS section at the bottom.

set -u

BIN=${1:-./mysh}
if [ ! -x "$BIN" ]; then
  echo "Can't find an executable shell at '$BIN'. Run make first, or pass the path: ./test_mysh.sh ./yourshell" >&2
  exit 2
fi
BIN=$(cd "$(dirname "$BIN")" && pwd -P)/$(basename "$BIN")
SRC=$(dirname "$BIN")

TMP=$(cd "$(mktemp -d)" && pwd -P)
WORK="$TMP/work"
mkdir -p "$WORK"
trap 'rm -rf "$TMP"' EXIT
printf 'banana\napple\ncherry\n' > "$WORK/in.txt"

TIMEOUT=""
if command -v timeout >/dev/null 2>&1; then TIMEOUT="timeout 10"
elif command -v gtimeout >/dev/null 2>&1; then TIMEOUT="gtimeout 10"; fi

pass=0; fail=0; skipped=0; NAME=""; STATUS=0

# ---------- helpers ----------

define() {   # value of a #define from your headers, or empty
  grep -hE "^[[:space:]]*#[[:space:]]*define[[:space:]]+$1[[:space:]]+[0-9]+" "$SRC"/*.h 2>/dev/null \
    | head -n1 | sed -E "s/.*define[[:space:]]+$1[[:space:]]+([0-9]+).*/\1/"
}
rep()   { local s="" i; for ((i=0; i<$2; i++)); do s+="$1"; done; printf '%s' "$s"; }
words() { local s="" i; for ((i=0; i<$1; i++)); do s+=" a"; done; printf '%s' "$s"; }

run() {      # run LINE... : one line per read, then end of input
  : > "$TMP/out"; : > "$TMP/err"
  ( cd "$WORK" && { for line in "$@"; do printf '%s\n' "$line"; sleep 0.3; done; } \
      | $TIMEOUT "$BIN" > "$TMP/out" 2> "$TMP/err" )
  STATUS=$?
}
stdout() { sed 's/\$ //g' "$TMP/out"; }   # stdout with "$ " prompts removed

t()    { NAME=$1; }
ok()   { pass=$((pass+1)); printf '  PASS  %s\n' "$NAME"; }
skip() { skipped=$((skipped+1)); printf '  SKIP  %s\n' "$1"; }
bad()  {
  fail=$((fail+1)); printf '  FAIL  %s\n        %s\n' "$NAME" "$1"
  printf '        stdout: %s\n' "$(stdout | head -c 200 | tr '\n' '|')"
  printf '        stderr: %s\n' "$(head -c 200 "$TMP/err" | tr '\n' '|')"
}
has_out()   { if stdout | grep -qF -- "$1"; then ok; else bad "stdout should contain: $1"; fi; }
has_line()  { if stdout | grep -qxF -- "$1"; then ok; else bad "stdout should have the line: $1"; fi; }
lacks_out() { if stdout | grep -qF -- "$1"; then bad "stdout should NOT contain: $1"; else ok; fi; }
has_err()   { if grep -qF -- "$1" "$TMP/err"; then ok; else bad "stderr should contain: $1"; fi; }
no_err()    { if [ -s "$TMP/err" ]; then bad "stderr should be empty"; else ok; fi; }
no_crash()  { if [ "$STATUS" -ge 124 ]; then bad "shell crashed or hung (exit status $STATUS)"; else ok; fi; }
file_is()   {
  if [ -f "$WORK/$1" ] && [ "$(cat "$WORK/$1")" = "$2" ]; then ok
  else bad "file $1 should contain: $(printf '%s' "$2" | tr '\n' '|')"; fi
}

# ---------- basic commands ----------
echo "== Basic commands"
t "runs a simple command";                run 'echo hello';                    has_line "hello"
t "simple command prints no error";                                             no_err
t "collapses extra spaces";               run '   echo   a    b   ';            has_line "a b"
t "treats tabs as spaces";                run $'\techo\tx\ty';                  has_line "x y"
t "empty line does nothing";              run '' 'echo after';                  has_line "after"
t "whitespace-only line does nothing";    run '   ' 'echo after';               has_line "after"
t "runs a command by absolute path";      run '/bin/echo abs';                  has_line "abs"
t "finds commands through PATH";          run 'ls';                             has_line "in.txt"
t "unknown command reports an error";     run 'nosuchcmd_xyz';                  has_err "Error: Command execution failed."
t "keeps running after unknown command";  run 'nosuchcmd_xyz' 'echo alive';     has_line "alive"
t "bad path with a slash reports error";  run './nosuchfile_xyz';               has_err "Error: Command execution failed."
t "exit quits the shell";                 run 'exit' 'echo after_exit';         lacks_out "after_exit"
t "exit with extra words is not exit";    run 'exit now' 'echo alive';          has_line "alive"
t "end of input (Ctrl+D) exits cleanly";  run 'echo x';                         no_crash

# ---------- environment variables ----------
echo "== Environment variables"
t '$HOME expands';                        run 'echo $HOME';                     has_line "$HOME"
t '$PATH expands';                        run 'echo $PATH';                     has_out "$PATH"
t 'unset variable expands to nothing';    run 'echo start $NO_SUCH_VAR_XYZ end'; has_out "end"
t 'a lone $ stays as $';                  run 'echo cost $';                    has_line 'cost $'
t 'variable used as a cd target';         run 'cd $HOME' 'pwd';                 has_line "$HOME"
t 'very long variable name does not crash'
run "echo \$$(rep A 100)" 'echo alive';                                          has_line "alive"

# ---------- cd ----------
echo "== cd"
t "cd to an absolute path";               run 'cd /usr' 'pwd';                  has_line "/usr"
t "cd with no argument goes to HOME";     run 'cd' 'pwd';                       has_line "$HOME"
t "cd .. goes up a level";                run 'cd ..' 'pwd';                    has_line "$TMP"
t "cd to a missing dir reports an error"; run 'cd /no/such/dir';                has_err "Error: cd failed."
t "shell keeps working after failed cd";  run 'cd /no/such/dir' 'pwd';          has_line "$WORK"

# ---------- redirection ----------
echo "== Redirection"
t "> writes stdout to a file";            run 'echo hi > r1.out';               file_is r1.out "hi"
t "> leaves the screen empty";                                                  lacks_out "hi"
t "> overwrites an existing file";        run 'echo one > r2.out' 'echo two > r2.out'
                                                                                 file_is r2.out "two"
t ">> appends to a file";                 run 'echo one >> r3.out' 'echo two >> r3.out'
                                                                                 file_is r3.out "$(printf 'one\ntwo')"
t ">> creates the file if missing";       run 'echo new >> r4.out';             file_is r4.out "new"
t "< reads stdin from a file";            run 'head -n 1 < in.txt';             has_line "banana"
t "< and > together";                     run 'sort < in.txt > r5.out';         file_is r5.out "$(printf 'apple\nbanana\ncherry')"
t "< and >> together";                    run 'echo top > r6.out' 'sort < in.txt >> r6.out'
                                                                                 file_is r6.out "$(printf 'top\napple\nbanana\ncherry')"
t "missing input file";                   run 'sort < missing.txt';             has_err "Error: cannot open input file."
t "output file that can't be opened";     run 'echo x > /no/such/dir/f.out';    has_err "Error: cannot open output file."
t "> with no file name";                  run 'echo hi >';                      has_err "Error: missing file name after < or >."
t "< with no file name";                  run 'sort <';                         has_err "Error: missing file name after < or >."
t "> followed by an operator";            run 'echo hi > |';                    has_err "Error: missing file name after < or >."
t "> before < is rejected (grammar)";     run 'sort > r7.out < in.txt';         has_err "Error: unexpected symbol."
t "two > redirections are rejected";      run 'echo hi > a.out > b.out';        has_err "Error: unexpected symbol."
t "two < redirections are rejected";      run 'sort < in.txt < in.txt';         has_err "Error: unexpected symbol."

# ---------- pipes ----------
echo "== Pipes"
t "two-stage pipe";                       run 'echo hello | tr a-z A-Z';        has_line "HELLO"
t "three-stage pipe";                     run 'sort -r in.txt | head -n 2 | tail -n 1'
                                                                                 has_line "banana"
t "< feeds the first stage";              run 'sort | head -n 1 < in.txt';      has_line "apple"
t "> takes the last stage";               run 'sort in.txt | head -n 1 > p1.out'
                                                                                 file_is p1.out "apple"
t "pipe with both < and >";               run 'sort | tail -n 1 < in.txt > p2.out'
                                                                                 file_is p2.out "cherry"
t "pipe ends are closed (no hang)";       run 'yes | head -n 3';                has_line "y"
t "pipe doesn't crash or hang";                                                 no_crash
t "pipe starting with |";                 run '| sort';                         has_err "Error: missing command."
t "pipe ending with |";                   run 'echo hi |';                      has_err "Error: missing command."
t "empty stage between pipes";            run 'echo hi | | sort';               has_err "Error: missing command."
t "unknown command inside a pipe";        run 'echo hi | nosuchcmd_xyz' 'echo alive'
                                                                                 has_line "alive"

# ---------- limits ----------
echo "== Limits"
BUF=$(define BUFFER_SIZE); MAXARGS=$(define MAX_ARGS); MAXTOK=$(define MAX_TOKENS)
MAXPIPE=$(define MAX_PIPELINE_LEN); HEAP=$(define HEAP_SIZE)
echo "   (from headers: BUFFER_SIZE=${BUF:-?} MAX_ARGS=${MAXARGS:-?} MAX_TOKENS=${MAXTOK:-?} MAX_PIPELINE_LEN=${MAXPIPE:-?} HEAP_SIZE=${HEAP:-?})"
LINE_MAX=$(( ${BUF:-256} - 2 ))   # longest line that fits, not counting the newline

if [ -n "$BUF" ]; then
  x=$(rep x $((LINE_MAX - 5)))
  t "line of exactly $LINE_MAX chars is accepted";      run "echo $x";      has_line "$x"
  x=$(rep x $((LINE_MAX - 4)))
  t "line of $((LINE_MAX + 1)) chars is rejected";      run "echo $x";      has_err "exceeds maximum length"
  t "too-long line is not executed";                                         lacks_out "xxxxxxxxxx"
  t "shell recovers after a too-long line";             run "echo $(rep x 400)" 'echo alive'
                                                                             has_line "alive"
else
  skip "line length tests (BUFFER_SIZE not found in headers)"
fi

if [ -n "$MAXARGS" ] && [ $((5 + 2 * MAXARGS)) -le "$LINE_MAX" ]; then
  t "exactly MAX_ARGS ($MAXARGS) arguments works";  run "echo$(words $((MAXARGS - 1)))" 'echo alive'
                                                                             has_line "alive"
  t "MAX_ARGS arguments prints no error";                                    no_err
  t "MAX_ARGS + 1 arguments is rejected";           run "echo$(words "$MAXARGS")"
                                                                             has_err "too many"
else
  skip "argument limit tests (MAX_ARGS missing or too large to fit in one line)"
fi

if [ -n "$MAXTOK" ] && [ $((5 + 2 * MAXTOK)) -le "$LINE_MAX" ]; then
  t "MAX_TOKENS + 1 tokens is rejected";            run "echo$(words "$MAXTOK")"
                                                                             has_err "too many"
else
  skip "token limit test (MAX_TOKENS missing or too large to fit in one line)"
fi

if [ -n "$MAXPIPE" ] && [ $((4 + 7 * MAXPIPE)) -le "$LINE_MAX" ]; then
  t "exactly MAX_PIPELINE_LEN ($MAXPIPE) stages works"
  run "echo hi$(rep ' | cat' $((MAXPIPE - 1)))";                             has_line "hi"
  t "MAX_PIPELINE_LEN + 1 stages is rejected"
  run "echo hi$(rep ' | cat' "$MAXPIPE")";                                   has_err "Error:"
else
  skip "pipeline length tests (MAX_PIPELINE_LEN missing or too large to fit in one line)"
fi

if [ -n "$HEAP" ] && [ $((HEAP + 10)) -le "$LINE_MAX" ]; then
  t "token bigger than the heap is rejected";       run "echo $(rep x $((HEAP + 1)))"
                                                                             has_err "Error: Heap limit exceeded."
else
  skip "heap limit test (HEAP_SIZE missing or larger than one input line)"
fi

# ---------- background jobs ----------
echo "== Background jobs"
t "& returns to the prompt right away"
start=$(date +%s); run 'sleep 3 &' 'echo back'; elapsed=$(( $(date +%s) - start ))
if [ "$elapsed" -lt 3 ]; then ok; else bad "took ${elapsed}s, the shell waited for the background job"; fi
t "prompt works while a job runs in the background";                         has_line "back"
t "background job produces its output";   run 'echo bg_output &'; sleep 0.5;  has_line "bg_output"
t "background job with redirection";      run 'echo bgfile > bg1.out &'; sleep 0.5
                                                                             file_is bg1.out "bgfile"
t "background pipeline";                  run 'sort in.txt | head -n 1 > bg2.out &'; sleep 0.5
                                                                             file_is bg2.out "apple"
t "& on its own";                         run '&';                           has_err "Error: missing command."
t "words after & are rejected";           run 'sleep 1 & echo hi';           has_err "Error: unexpected symbol."
t "& before a redirection is rejected";   run 'echo hi & > x.out';           has_err "Error: unexpected symbol."

# ---------- fg / bg builtins ----------
echo "== fg / bg"
t "fg with no job";                       run 'fg';                          has_err "Error: no job to bring to foreground."
t "bg with no job";                       run 'bg';                          has_err "Error: no job to resume in background."
t "fg waits for a running background job"
start=$(date +%s); run 'sleep 2 &' 'fg' 'echo after_fg'; elapsed=$(( $(date +%s) - start ))
if [ "$elapsed" -ge 1 ]; then ok; else bad "fg returned after ${elapsed}s, it didn't wait for the job"; fi
t "shell continues after fg";                                                has_line "after_fg"

# ---------- summary ----------
echo
echo "$pass passed, $fail failed, $skipped skipped"
[ "$fail" -eq 0 ]

# =====================================================================
# MANUAL TESTS - run ./mysh in a real terminal and check each one
# =====================================================================
#  1. sleep 5, then Ctrl+C
#       -> "-->mysh(task ended)" appears, prompt comes back, shell still runs
#  2. Ctrl+C at an empty prompt
#       -> message and a new prompt; the shell does NOT exit
#  3. sleep 30, then Ctrl+Z
#       -> "-->mysh(task suspended)", prompt comes back
#  4. After step 3, type fg
#       -> sleep resumes in the foreground; Ctrl+C now ends it
#  5. sleep 30, Ctrl+Z, then bg
#       -> prompt stays usable while sleep runs; fg later brings it back
#  6. sleep 30 | sleep 30, Ctrl+Z, then fg
#       -> both stages resume (whole process group), Ctrl+C ends both
#  7. cat (no arguments), type a few lines, then Ctrl+D
#       -> cat echoes each line, Ctrl+D ends cat, back at the mysh prompt
#  8. cat, Ctrl+Z, then fg
#       -> cat can read the keyboard again (terminal control handed back)
#  9. sleep 30 &, then Ctrl+C at the prompt
#       -> background sleep keeps running (check with: ps -ef | grep sleep)
# 10. sleep 1 &, wait 2 seconds, press Enter, then in another terminal:
#       ps -o pid,stat,comm -t $(tty of the mysh terminal)
#     -> no process shows state Z (zombie)
# 11. sleep 1 &, wait 2 seconds, then fg
#       -> "Error: no job to bring to foreground." (finished job was cleared)
# 12. Ctrl+D at an empty prompt
#       -> prints a newline and exits
# 13. Run a full-screen program (less in.txt or vim)
#       -> it displays and takes keyboard input normally; q returns to mysh
