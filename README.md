# Design and Implementation of a PERT Scheduling and Probability Analysis System in C

This C11 command-line program loads an arbitrary-size Activity-on-Node project,
validates its dependencies, explains the forward and backward passes, streams up
to 100 genuine critical paths, and calculates traditional PERT probabilities per
path. The application has no third-party runtime or PERT library dependency.

## Requirements and build

Windows, VS Code (optional editor), and a GCC C11 toolchain on PATH. Development
verification used MinGW-w64 GCC 8.1.0, x86_64 Windows. All source files compiled
with `-std=c11 -Wall -Wextra -Wpedantic -Werror` with no warnings. `erfc` and
`sqrt` are supplied by the C math library; keep `-lm` after the sources.

Open the `PERT_Project` folder in VS Code, open its integrated terminal, and run:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude src/main.c src/model.c src/input.c src/graph.c src/pert.c src/probability.c src/output.c -o pert.exe -lm
.\pert.exe
```

No `make` installation is required. If GNU Make is installed:

```text
make
make test
```

The Makefile recipes use no platform-specific cleanup commands. On some Windows
installations GNU Make is called `mingw32-make`; use that command instead. On a
POSIX host `./pert.exe` runs the same source-built application (the filename is
retained for consistency). The Makefile is supplied but GNU Make was unavailable
in the verification environment; the equivalent GCC commands were executed.

## Project layout

```text
PERT_Project/
  include/          pert_types.h, model.h, input.h, graph.h,
                    pert.h, probability.h, output.h
  src/              main.c, model.c, input.c, graph.c,
                    pert.c, probability.c, output.c
  data/             lecture_example.csv, additional_example_files.csv
  tests/            test_core.c, automated_tests.py,
                    memory_probe.c, memory_probe.h, memory_redirect.h
    results/        actual build/test logs, example transcripts, summary.json
  references/
    assignment_documents/   original assignment PDF and cover DOCX
    sources.md      verified bibliography and provenance
  report/           editable report content and diagram sources
  Makefile
  README.md
```

Executable files are local build products and are excluded from the source ZIP.
The report content and figures are supplementary; Python is never required to run
the C application.

## Interactive use

1. Select `1` to load CSV, `2` for keyboard input, or `0` to exit.
2. For CSV, type the filename without surrounding quotes. Spaces in paths are
   supported. The filename is resolved relative to the current terminal directory.
3. For keyboard input, provide a positive activity count, then each ID,
   description, and a/m/b estimate. Enter predecessors after all IDs exist.
   Invalid fields can be re-entered. Future IDs may be referenced.
4. After graph validation and successful scheduling, enter the deadline in days.
5. Read the detailed report. The menu returns for another analysis.

To reproduce the lecture result, choose CSV, enter `data/lecture_example.csv`,
then deadline `47`. Expected output is 14 activities, 16 dependencies, 44 days,
path `A -> B -> C -> E -> F -> J -> L -> N`, variance 9, sigma 3, Z=1,
and probability 84.1345% (84.13% to two decimals). Every one of the 14 result rows
is checked in the C tests. `tests/results/lecture_output.txt` is an actual run.

## CSV contract

```csv
ID,Description,a,m,b,Predecessors
A,Requirement Analysis,1,2,3,-
B,System Design,2,4,6,A
C,Database Design,1,3,5,A
D,Implementation,4,6,8,B|C
```

The header must contain these six names in this order and case. Surrounding field
whitespace, LF/CRLF, empty lines, and an optional BOM at the beginning of the file
are accepted. Records may be unsorted. Exactly six nonempty fields are required;
empty fields are preserved and rejected, never shifted into another column.

IDs are case-sensitive ASCII letters/digits/underscores, start with a letter, and
have at most 32 bytes. Descriptions have 1-256 bytes and cannot contain commas or
embedded newlines. This is a restricted unquoted CSV dialect, not a full quoted
CSV implementation. Estimates are decimal numbers (scientific notation allowed)
with `0 <= a <= m <= b`. Hexadecimal numbers, NaN, infinity, trailing junk,
overflow, and unrepresentable underflow are rejected. Use `-` for no predecessors,
or `|` between IDs. Unknown, duplicate, self, and cyclic dependencies are errors.

Invalid CSV is rejected without repair. Diagnostics contain filename, physical
line number and activity ID when applicable. A cycle concerns the graph as a
whole and need not have a single line number. The deadline is separate from CSV.

## Output and mathematics

The ten sections show project overview, original data, duration/variance,
topological order, forward pass, backward pass, final table, critical activities
and paths, probability, and limitations. Paths and probabilities stream together
through a synchronous callback; paths are not stored permanently.

`te=(a+4m+b)/6` is evaluated in the equivalent difference form
`a+(m-a)*(2.0/3.0)+(b-a)/6.0` to avoid intermediate `4*m` overflow and preserve
equal estimates. `variance=((b-a)/6)^2`. Stored results are never rounded.
Forward pass uses predecessor maximum EF; backward pass uses successor minimum
LS; every terminal LF equals the maximum project EF. Critical slack and tight
edges use the centralized absolute tolerance `1e-9` days, never an unrestricted
relative tolerance. Very small nonzero slack prints in significant digits.

For each emitted critical path, sum variances, take the square root, and compute
`Z=(deadline-path_mean)/sigma`, `Phi(Z)=0.5*erfc(-Z/sqrt(2))`. Exactly zero variance
uses the deterministic on-or-before comparison, without any duration tolerance.
Positive tiny variances remain probabilistic. Non-finite calculations are errors.

Multiple paths get separate approximate probabilities. They may share activities,
so their probabilities are not multiplied. Even one critical path does not make
this an exact project completion model: near-critical paths can become controlling.
The 101st discovered path is not delivered; enumeration stops, reports 100 emitted
paths and truncation, and makes no claim of an exact total.

## Tests

Core tests require only GCC and the C runtime:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude tests/test_core.c src/model.c src/input.c src/graph.c src/pert.c src/probability.c src/output.c -o tests/test_core.exe -lm
.\tests\test_core.exe
```

For all integration, randomized-oracle, and allocation-injection tests, install
Python 3.8+ and run from the project root:

```powershell
python tests/automated_tests.py
```

The runner rebuilds the application and tests, uses temporary CSV files, and
rewrites `tests/results/`. It needs no Python packages. Test failures cause a
nonzero exit. Actual verification: 22 groups passed, 62,547 core assertions,
62,745 assertions with allocation instrumentation, and 100 additional randomized
DAG comparisons. Large-network test: 20,000 activities in a chain. Path-limit
test: eight layers of two nodes (256 paths), exactly 100 callbacks and truncation.
The exact 100-path case is also checked and is not truncated.

Memory instrumentation tracks allocations made by project/test source, checks
16-byte tail guards and invalid frees, and injects failure at all 97 allocation
positions of load/calculate/enumerate for the lecture case. Cleanup leaves zero
tracked blocks. It does not detect arbitrary invalid reads, all out-of-bounds
writes, or all use-after-free accesses. MinGW here could not link AddressSanitizer
(`cannot find -lasan`), so no AddressSanitizer pass is claimed. Logs preserve this.
On a supporting toolchain, compile core tests with
`-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer` and run them.

## Ownership and limitations

Project owns activities/strings, graph arrays and sorted ID index. PERTResult owns
results and topological order. Input owns temporary predecessor strings. Iterative
DFS owns path/traversal buffers; the callback only borrows them until return.
Initialize outputs before calling APIs and destroy them before reuse. Public
headers document preconditions and cleanup. Failed loading/calculation does not
transfer a partially constructed object to the caller.

There is no fixed activity limit; address space and available memory apply. Array
growth checks size arithmetic. CSV lookup uses a sorted index with binary search.
Keyboard duplicate-ID validation scans earlier activities, and duplicate-edge
checks scan the shorter adjacency list; these validation steps are not universally
linear. Kahn and the scheduling passes are O(V+E); DFS uses O(V) extra memory and
is output-sensitive. Very large magnitudes can lose representable time increments;
detected loss is rejected, and the absolute tolerance is intentionally conservative.
All dependencies are finish-to-start, zero lag. Calendars, resource leveling,
correlated activity models and Monte Carlo simulation are outside scope.

## Troubleshooting

- `gcc` not found: add the chosen GCC toolchain's `bin` directory to PATH, restart
  the terminal, and check `gcc --version`.
- `make` not found: use the direct GCC command; make is optional.
- Missing `sqrt`/`erfc`: ensure `-lm` follows the source files and use a C11 toolchain.
- CSV cannot open: run from `PERT_Project`, use the correct path without quotes,
  and verify the file exists.
- No deadline prompt: fix the preceding load/graph/scheduling error first.
- Output wraps: widen the terminal or redirect a scripted run to a text file.
- Source ZIP: extract it before building; run tests from the project root.
- Report naming: student name/number must be confirmed before renaming the report
  and source ZIP as `StudentNo._StudentName.pdf` and `StudentNo._StudentName.zip`.
  No student identity is inferred from the computer account or template examples.
