# mysh Test Results

Each input line is typed in turn (lines separated by ' / '), then input ends like Ctrl+D. Long text is cut short, with its full length in brackets.

## Automated tests

| Category | Test | Input | Expected output | Actual output | Result |
|---|---|---|---|---|---|
| Basic commands | runs a simple command | `echo hello` | `stdout: hello` | `stdout: hello` | PASS |
| Basic commands | simple command prints no error | `echo hello` | `no error message` | `stderr: ` | PASS |
| Basic commands | collapses extra spaces | `   echo   a    b   ` | `stdout: a b` | `stdout: a b` | PASS |
| Basic commands | treats tabs as spaces | `<TAB>echo<TAB>x<TAB>y` | `stdout: x y` | `stdout: x y` | PASS |
| Basic commands | empty line does nothing | ` / echo after` | `stdout: after` | `stdout: after` | PASS |
| Basic commands | whitespace-only line does nothing | `    / echo after` | `stdout: after` | `stdout: after` | PASS |
| Basic commands | runs a command by absolute path | `/bin/echo abs` | `stdout: abs` | `stdout: abs` | PASS |
| Basic commands | finds commands through PATH | `ls` | `stdout: in.txt` | `stdout: in.txt` | PASS |
| Basic commands | unknown command reports an error | `nosuchcmd_xyz` | `stderr: Error: Command execution failed.` | `stderr: Error: Command execution failed.` | PASS |
| Basic commands | keeps running after unknown command | `nosuchcmd_xyz / echo alive` | `stdout: alive` | `stdout: alive` | PASS |
| Basic commands | bad path with a slash reports error | `./nosuchfile_xyz` | `stderr: Error: Command execution failed.` | `stderr: Error: Command execution failed.` | PASS |
| Basic commands | exit quits the shell | `exit / echo after_exit` | `stdout does not contain: after_exit` | `stdout: ` | PASS |
| Basic commands | exit with extra words is not exit | `exit now / echo alive` | `stdout: alive` | `stdout: alive` | PASS |
| Basic commands | end of input (Ctrl+D) exits cleanly | `echo x` | `shell exits normally` | `exit status 0` | PASS |
| Environment variables | $HOME expands | `echo $HOME` | `stdout: /Users/ebiobuma` | `stdout: /Users/ebiobuma` | PASS |
| Environment variables | $PATH expands | `echo $PATH` | `stdout contains: /Library/Java/JavaVirtualMachines/jdk-23.jd... (674 chars)` | `stdout: /Library/Java/JavaVirtualMachines/jdk-23.jdk/Content... (665 chars)` | PASS |
| Environment variables | unset variable expands to nothing | `echo start $NO_SUCH_VAR_XYZ end` | `stdout contains: end` | `stdout: start  end` | PASS |
| Environment variables | a lone $ stays as $ | `echo cost $` | `stdout: cost $` | `stdout: cost $` | PASS |
| Environment variables | variable used as a cd target | `cd $HOME / pwd` | `stdout: /Users/ebiobuma` | `stdout: /Users/ebiobuma` | PASS |
| Environment variables | very long variable name does not crash | `echo $AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA... (117 chars)` | `stdout: alive` | `stdout: alive` | PASS |
| cd | cd to an absolute path | `cd /usr / pwd` | `stdout: /usr` | `stdout: /usr` | PASS |
| cd | cd with no argument goes to HOME | `cd / pwd` | `stdout: /Users/ebiobuma` | `stdout: /Users/ebiobuma` | PASS |
| cd | cd .. goes up a level | `cd .. / pwd` | `stdout: /private/tmp/mysh_test.qYJiTn` | `stdout: /private/tmp/mysh_test.qYJiTn` | PASS |
| cd | cd to a missing dir reports an error | `cd /no/such/dir` | `stderr: Error: cd failed.` | `stderr: Error: cd failed.` | PASS |
| cd | shell keeps working after failed cd | `cd /no/such/dir / pwd` | `stdout: /private/tmp/mysh_test.qYJiTn/work` | `stdout: /private/tmp/mysh_test.qYJiTn/work` | PASS |
| Redirection | > writes stdout to a file | `echo hi > r1.out` | `r1.out contains: hi` | `r1.out contains: hi` | PASS |
| Redirection | > leaves the screen empty | `echo hi > r1.out` | `stdout does not contain: hi` | `stdout: ` | PASS |
| Redirection | > overwrites an existing file | `echo one > r2.out / echo two > r2.out` | `r2.out contains: two` | `r2.out contains: two` | PASS |
| Redirection | >> appends to a file | `echo one >> r3.out / echo two >> r3.out` | `r3.out contains: one / two` | `r3.out contains: one / two` | PASS |
| Redirection | >> creates the file if missing | `echo new >> r4.out` | `r4.out contains: new` | `r4.out contains: new` | PASS |
| Redirection | < reads stdin from a file | `head -n 1 < in.txt` | `stdout: banana` | `stdout: banana` | PASS |
| Redirection | < and > together | `sort < in.txt > r5.out` | `r5.out contains: apple / banana / cherry` | `r5.out contains: apple / banana / cherry` | PASS |
| Redirection | < and >> together | `echo top > r6.out / sort < in.txt >> r6.out` | `r6.out contains: top / apple / banana / cherry` | `r6.out contains: top / apple / banana / cherry` | PASS |
| Redirection | missing input file | `sort < missing.txt` | `stderr: Error: cannot open input file.` | `stderr: Error: cannot open input file.` | PASS |
| Redirection | output file that can't be opened | `echo x > /no/such/dir/f.out` | `stderr: Error: cannot open output file.` | `stderr: Error: cannot open output file.` | PASS |
| Redirection | > with no file name | `echo hi >` | `stderr: Error: missing file name after < or >.` | `stderr: Error: missing file name after < or >.` | PASS |
| Redirection | < with no file name | `sort <` | `stderr: Error: missing file name after < or >.` | `stderr: Error: missing file name after < or >.` | PASS |
| Redirection | > followed by an operator | `echo hi > \|` | `stderr: Error: missing file name after < or >.` | `stderr: Error: missing file name after < or >.` | PASS |
| Redirection | > before < is rejected (grammar) | `sort > r7.out < in.txt` | `stderr: Error: unexpected symbol.` | `stderr: Error: unexpected symbol.` | PASS |
| Redirection | two > redirections are rejected | `echo hi > a.out > b.out` | `stderr: Error: unexpected symbol.` | `stderr: Error: unexpected symbol.` | PASS |
| Redirection | two < redirections are rejected | `sort < in.txt < in.txt` | `stderr: Error: unexpected symbol.` | `stderr: Error: unexpected symbol.` | PASS |
| Pipes | two-stage pipe | `echo hello \| tr a-z A-Z` | `stdout: HELLO` | `stdout: HELLO` | PASS |
| Pipes | three-stage pipe | `sort -r in.txt \| head -n 2 \| tail -n 1` | `stdout: banana` | `stdout: banana` | PASS |
| Pipes | < feeds the first stage | `sort \| head -n 1 < in.txt` | `stdout: apple` | `stdout: apple` | PASS |
| Pipes | > takes the last stage | `sort in.txt \| head -n 1 > p1.out` | `p1.out contains: apple` | `p1.out contains: apple` | PASS |
| Pipes | pipe with both < and > | `sort \| tail -n 1 < in.txt > p2.out` | `p2.out contains: cherry` | `p2.out contains: cherry` | PASS |
| Pipes | pipe ends are closed (no hang) | `yes \| head -n 3` | `stdout: y` | `stdout: y / y / y` | PASS |
| Pipes | pipe doesn't crash or hang | `yes \| head -n 3` | `shell exits normally` | `exit status 0` | PASS |
| Pipes | pipe starting with | | `\| sort` | `stderr: Error: missing command.` | `stderr: Error: missing command.` | PASS |
| Pipes | pipe ending with | | `echo hi \|` | `stderr: Error: missing command.` | `stderr: Error: missing command.` | PASS |
| Pipes | empty stage between pipes | `echo hi \| \| sort` | `stderr: Error: missing command.` | `stderr: Error: missing command.` | PASS |
| Pipes | unknown command inside a pipe | `echo hi \| nosuchcmd_xyz / echo alive` | `stdout: alive` | `stdout: alive` | PASS |
| Limits | longest allowed line is accepted | `echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... (255 chars)` | `stdout: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... (258 chars)` | `stdout: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... (258 chars)` | PASS |
| Limits | line one char too long is rejected | `echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... (256 chars)` | `stderr: exceeds maximum length` | `stderr: Error: Command line exceeds maximum length (255) all... (65 chars)` | PASS |
| Limits | too-long line is not executed | `echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... (256 chars)` | `stdout does not contain: xxxxxxxxxx` | `stdout: ` | PASS |
| Limits | shell recovers after a too-long line | `echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... (416 chars)` | `stdout: alive` | `stdout: alive` | PASS |
| Limits | MAX_ARGS arguments works | `echo a a a a a a a a a a a a a a a a a a a / echo alive` | `stdout: alive` | `stdout: a a a a a a a a a a a a a a a a a a a / alive` | PASS |
| Limits | MAX_ARGS arguments prints no error | `echo a a a a a a a a a a a a a a a a a a a / echo alive` | `no error message` | `stderr: ` | PASS |
| Limits | MAX_ARGS + 1 arguments is rejected | `echo a a a a a a a a a a a a a a a a a a a a` | `stderr: too many` | `stderr: Error: too many arguments.` | PASS |
| Limits | MAX_TOKENS + 1 tokens is rejected | `echo a a a a a a a a a a a a a a a a a a a a a a a a a a a a... (132 chars)` | `stderr: too many` | `stderr: Error: too many tokens.` | PASS |
| Limits | MAX_PIPELINE_LEN stages works | `echo hi \| cat \| cat \| cat \| cat \| cat \| cat \| cat \| cat \| ca... (61 chars)` | `stdout: hi` | `stdout: hi` | PASS |
| Limits | MAX_PIPELINE_LEN + 1 stages is rejected | `echo hi \| cat \| cat \| cat \| cat \| cat \| cat \| cat \| cat \| ca... (67 chars)` | `stderr: Error:` | `stderr: Error: pipeline too long.` | PASS |
| Background jobs | & returns to the prompt right away | `sleep 3 & / echo back` | `finishes in under 3s` | `took 0.6s` | PASS |
| Background jobs | prompt works while a job runs | `sleep 3 & / echo back` | `stdout: back` | `stdout: back` | PASS |
| Background jobs | background job produces its output | `echo bg_output &` | `stdout: bg_output` | `stdout: bg_output` | PASS |
| Background jobs | background job with redirection | `echo bgfile > bg1.out &` | `bg1.out contains: bgfile` | `bg1.out contains: bgfile` | PASS |
| Background jobs | background pipeline | `sort in.txt \| head -n 1 > bg2.out &` | `bg2.out contains: apple` | `bg2.out contains: apple` | PASS |
| Background jobs | & on its own | `&` | `stderr: Error: missing command.` | `stderr: Error: missing command.` | PASS |
| Background jobs | words after & are rejected | `sleep 1 & echo hi` | `stderr: Error: unexpected symbol.` | `stderr: Error: unexpected symbol.` | PASS |
| Background jobs | & before a redirection is rejected | `echo hi & > x.out` | `stderr: Error: unexpected symbol.` | `stderr: Error: unexpected symbol.` | PASS |
| fg / bg | fg with no job | `fg` | `stderr: Error: no job to bring to foreground.` | `stderr: Error: no job to bring to foreground.` | PASS |
| fg / bg | bg with no job | `bg` | `stderr: Error: no job to resume in background.` | `stderr: Error: no job to resume in background.` | PASS |
| fg / bg | fg waits for a background job | `sleep 2 & / fg / echo after_fg` | `takes at least 1s` | `took 2.0s` | PASS |
| fg / bg | shell continues after fg | `sleep 2 & / fg / echo after_fg` | `stdout: after_fg` | `stdout: after_fg` | PASS |

**Summary:** 74 passed, 0 failed

## Manual tests

These need a real keyboard. Run ./mysh in a terminal and fill in the last two columns.

| Category | Test | Input | Expected output | Actual output | Result |
|---|---|---|---|---|---|
| Signals | Ctrl+C ends a foreground job | sleep 5, then Ctrl+C | -->mysh(task ended), prompt returns | | |
| Signals | Ctrl+C at an empty prompt | Ctrl+C | new prompt; shell does not exit | | |
| Signals | Ctrl+Z suspends a job | sleep 30, then Ctrl+Z | -->mysh(task suspended), prompt returns | | |
| Job control | fg resumes a suspended job | sleep 30, Ctrl+Z, fg | sleep resumes; Ctrl+C now ends it | | |
| Job control | bg resumes in the background | sleep 30, Ctrl+Z, bg | prompt stays usable while sleep runs | | |
| Job control | fg resumes a whole pipeline | sleep 30 \| sleep 30, Ctrl+Z, fg | both stages resume; Ctrl+C ends both | | |
| Terminal | Program reads the keyboard | cat, type lines, Ctrl+D | cat echoes each line, then back to mysh | | |
| Terminal | Terminal handed back after fg | cat, Ctrl+Z, fg | cat can read the keyboard again | | |
| Signals | Ctrl+C doesn't hit background jobs | sleep 30 &, then Ctrl+C | background sleep keeps running | | |
| Zombies | Finished background jobs are reaped | sleep 1 &, wait 2s, Enter | no process in state Z (ps -o pid,stat,comm) | | |
| Job control | Finished job is forgotten | sleep 1 &, wait 2s, fg | Error: no job to bring to foreground. | | |
| Basic | Ctrl+D at an empty prompt | Ctrl+D | prints a newline and exits | | |
| Terminal | Full-screen program works | less in.txt | displays normally; q returns to mysh | | |
