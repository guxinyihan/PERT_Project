# MASTER PROMPT: PERT Scheduling and Probability Analysis System in C

## 1. Role and Objective

You are an experienced C software engineer, software architect, algorithm designer, and technical documentation specialist.

Develop a complete university Software Engineering coursework project entitled:

"Design and Implementation of a PERT Scheduling and Probability Analysis System in C"

This is a complete software engineering project, not a simple demonstration program.

The project must demonstrate:
- Requirements analysis
- Independent architectural design
- Data modeling
- Modular C programming
- PERT algorithm implementation
- Dynamic memory management
- Input validation and error handling
- Testing and verification
- Professional technical documentation

The student has already completed extensive design discussions and approved the specifications below.

Treat this prompt as the frozen design baseline.

Do not change the core architecture, programming language, mathematical model, or agreed implementation methods without obtaining the student's approval.

Your objective is to implement, compile, test, document, and package the complete project.

Do not merely provide instructions or pseudocode. Create the actual deliverables.

--------------------------------------------------
2. ORIGINAL ASSIGNMENT DOCUMENTS
--------------------------------------------------

Two original assignment files will be provided:

1. Assignment for Chapter2-Design and Implementation of PERT model-2026.09.10.pdf

2. Cover Page for homework-2026.09.10.docx

Read both documents before implementation.

The assignment PDF is authoritative for the teacher's requirements.

The DOCX file is authoritative for the required report cover and formatting.

The assignment requires:
- The program must support different numbers of activities.
- It must not work only for a single hardcoded example.
- Input must be independently designed.
- Output must provide sufficient information about the algorithm procedure.
- Appropriate data structures must be designed.
- File names, function names, and variable names must be readable.
- A complete report and source code must be submitted.

Required submission:
1. An English Project Report in PDF format.
2. A compressed archive containing all source code.

Both submission files must use the naming convention:

StudentNo._StudentName

Ask the student for their student number and name before finalizing the submission filenames if necessary.

Never invent student information.

The normal submission deadline is October 1, 2026, at 09:00 AM, with a stated cut-off time of 23:59 on the same day.

If the reference documents cannot be accessed, explicitly identify the missing files.

Do not claim to have read inaccessible files.

If an explicit assignment requirement conflicts with this prompt, explain the conflict and follow the assignment requirement. Obtain approval before making a substantive change to the frozen engineering design.

--------------------------------------------------
3. FROZEN TECHNICAL DECISIONS
--------------------------------------------------

The following decisions are mandatory:

Programming language: C11
Development environment: Windows + VS Code + GCC
Application type: Command-line application
User interaction: Interactive main menu
Network model: Activity-on-Node (AON)
Graph type: Directed Acyclic Graph (DAG)
Graph representation: Dynamic-array adjacency lists
Graph directions: Store predecessors and successors
Memory management: malloc, realloc, free
Input: Keyboard and CSV
Time unit: Days
PERT model: Three-point time estimation
Numerical storage: double
Intermediate rounding: Not allowed
Probability: Traditional single-critical-path normal approximation
Critical path search: Iterative DFS
Path delivery: Callback
Maximum displayed critical paths: 100
Report language: English
Report length target: Approximately 15–20 pages of main content

Do not use C++, Java, Python, or another language to implement the application.

Python may be used for auxiliary testing or document preparation, but the application itself must be implemented in C.

Do not use a third-party PERT calculation library.

Do not add databases, web frameworks, GUI frameworks, or unnecessary architectural complexity.

--------------------------------------------------
4. PROJECT STRUCTURE
--------------------------------------------------

Use the following modular project structure:

PERT_Project/
    include/
        pert_types.h
        model.h
        input.h
        graph.h
        pert.h
        probability.h
        output.h

    src/
        main.c
        model.c
        input.c
        graph.c
        pert.c
        probability.c
        output.c

    data/
        lecture_example.csv
        additional_example_files.csv

    tests/
        automated_tests

    references/
        assignment_documents

    Makefile
    README.md

Additional small internal files may be created when genuinely necessary.

Keep the architecture understandable and appropriate for a university-level project.

The project must compile using GCC.

Provide both a Makefile and a direct GCC compilation command.

Do not assume that make is installed on every Windows computer.

--------------------------------------------------
5. MODULE RESPONSIBILITIES
--------------------------------------------------

main.c:
- Interactive menu
- Input-method selection
- Complete workflow coordination
- Deadline input
- Error recovery
- Resource management
- Returning to the menu after analysis

model.c / model.h:
- Activity storage
- Project storage
- Dynamic arrays
- ID lookup
- Memory ownership and cleanup

input.c / input.h:
- Keyboard input
- CSV parsing
- Input validation
- Temporary predecessor references
- Resolving IDs to internal indices

graph.c / graph.h:
- Predecessor adjacency lists
- Successor adjacency lists
- Edge insertion
- Topological sorting
- Cycle detection

pert.c / pert.h:
- Expected duration
- Activity variance
- Forward Pass
- Backward Pass
- Slack
- Critical activities
- Critical edges
- Critical path enumeration

probability.c / probability.h:
- Path variance
- Standard deviation
- Z-score
- Standard normal CDF
- Probability calculations
- Zero-variance handling

output.c / output.h:
- Project overview
- Input table
- Duration estimation results
- Topological order
- Forward Pass explanation
- Backward Pass explanation
- Final PERT table
- Critical paths
- Probability report
- Warnings and limitations

Architectural constraints:

The PERT Engine must not parse files or directly print the final report.

The Probability Module must not print formatted reports.

The Output Module must not rerun the scheduling algorithm.

The Main Controller coordinates the modules.

Avoid circular module dependencies.

--------------------------------------------------
6. DATA STRUCTURES
--------------------------------------------------

Use separate structures for original data and calculated results.

Activity:

Fields:
- char *id
- char *description
- double a
- double m
- double b

Activity must contain original input data only.

Do not store ES, EF, LS, LF, or Slack inside Activity.

AdjacencyList:

Fields:
- size_t *indices
- size_t count
- size_t capacity

Store integer activity indices rather than duplicated ID strings.

Project:

Contains:
- Dynamic Activity array
- Activity count
- Activity capacity
- Predecessor adjacency lists
- Successor adjacency lists
- Dependency edge count
- Activity ID lookup index

The ID lookup index should be sorted by ID and support binary search.

Keep activity indices stable.

After input is complete, finalize the activity collection before building the ID index and resolving dependencies.

ActivityResult:

Fields:
- Expected duration
- Variance
- ES
- EF
- LS
- LF
- Slack
- Critical flag

PERTResult:

Contains:
- ActivityResult array
- Topological order
- Activity count
- Project expected duration
- Critical activity count

Do not permanently store all critical paths.

ProbabilityResult:

May contain:
- Expected path duration
- Path variance
- Standard deviation
- Z-score
- Probability
- Deterministic flag

Document the data structures and their relationships.

--------------------------------------------------
7. MEMORY MANAGEMENT
--------------------------------------------------

Use malloc, realloc, and free.

Project owns:
- Activity array
- ID strings
- Description strings
- Predecessor adjacency lists
- Successor adjacency lists
- ID lookup index

PERTResult owns:
- ActivityResult array
- Topological order

The Input Module owns temporary parsing data.

The PERT Engine owns the temporary DFS path buffer.

The Callback only borrows the current path buffer during its invocation.

It must not:
- Modify the borrowed path.
- Free the borrowed path.
- Retain the path pointer after returning.

Check all allocation operations.

Check capacity arithmetic for overflow.

Do not overwrite the original pointer directly with realloc before confirming success.

Correctly release partially initialized objects.

Avoid:
- Memory leaks
- Double frees
- Use-after-free
- Out-of-bounds memory access

No fixed maximum activity count is allowed.

Available memory is the practical limitation.

--------------------------------------------------
8. INPUT SPECIFICATION
--------------------------------------------------

Support both keyboard and CSV input.

Both methods must use consistent validation rules.

CSV format:

ID,Description,a,m,b,Predecessors
A,Requirement Analysis,1,2,3,-
B,System Design,2,4,6,A
C,Database Design,1,3,5,A
D,Implementation,4,6,8,B|C

Rules:

Use exactly six fields.

Use "-" for no predecessors.

Use "|" to separate multiple predecessor IDs.

Activity order is arbitrary.

A predecessor may appear later in the CSV file.

Read all activities first, then resolve dependencies.

Activity ID:
- Nonempty
- Unique
- Case-sensitive
- Must begin with an English letter
- Remaining characters may contain letters, digits, and underscores
- Maximum 32 characters

Description:
- Nonempty
- Maximum 256 bytes
- Spaces allowed
- Commas and embedded newlines not supported

Time estimates must satisfy:

0 <= a <= m <= b

Reject:
- Negative estimates
- NaN
- Infinity
- Missing values
- Invalid numerical strings
- Values causing numerical overflow

Allow decimal estimates.

CSV compatibility:
- CRLF and LF
- Optional UTF-8 BOM
- Empty lines
- Leading and trailing whitespace around fields

Require the exact six-column header.

Reject malformed records.

Preserve empty fields when parsing.

Do not silently truncate valid input lines.

Use dynamically managed line buffers.

Dependency validation:
- Unknown predecessors
- Duplicate dependencies
- Self-dependencies
- Cyclic dependencies

Do not automatically repair invalid input.

Keyboard input errors:
Allow the user to re-enter invalid fields.

If a completed project contains a cycle, explain the error and allow the user to start a new project.

CSV errors:
Display filename, line number when applicable, activity ID when applicable, and an informative message.

Abort the current analysis safely and return to the main menu.

Request the target deadline only after the project has been successfully loaded and validated.

The deadline is entered in days.

It is not part of the CSV activity records.

--------------------------------------------------
9. GRAPH IMPLEMENTATION
--------------------------------------------------

Use dynamic-array adjacency lists.

Store both predecessor and successor relationships.

For each edge u -> v:

Add v to successors[u].

Add u to predecessors[v].

Both sides must remain consistent.

Ensure adequate capacity on both sides before committing the edge.

An allocation failure must not leave a half-added dependency.

Use Kahn's algorithm for topological sorting.

Requirements:
- Independent of input order
- Each activity appears exactly once
- All dependencies respected
- Cycles detected
- Original graph remains unchanged

Support:
- Single starting activity
- Multiple starting activities
- Single ending activity
- Multiple ending activities
- Disconnected DAG components

Dependencies are finish-to-start with zero lag.

Resource constraints and other dependency types are outside scope.

--------------------------------------------------
10. PERT MATHEMATICAL MODEL
--------------------------------------------------

Implement the complete classroom PERT model.

Expected duration:

te = (a + 4m + b) / 6

Variance:

V = ((b - a) / 6)^2

Use double precision.

Never round intermediate results.

Check for numerical overflow and non-finite results.

Forward Pass:

If activity i has no predecessors:

ES(i) = 0

Otherwise:

ES(i) = max(EF(j)) for all predecessors j.

EF(i) = ES(i) + te(i)

Process in topological order.

Project duration:

T = max(EF(i))

Backward Pass:

For terminal activities:

LF(i) = T

Otherwise:

LF(i) = min(LS(j)) for all successors j.

LS(i) = LF(i) - te(i)

Process in reverse topological order.

Slack:

Slack(i) = LS(i) - ES(i)

Identify critical activities using a consistent numerical tolerance policy.

Do not rely on direct floating-point equality.

A critical edge must:
- Exist in the graph
- Connect critical activities
- Satisfy EF(u) approximately equal to ES(v)

Do not construct a critical path by simply concatenating all zero-Slack activities.

Enumerate genuine paths through the critical subgraph.

Verify that emitted paths have expected durations consistent with the project duration.

--------------------------------------------------
11. CRITICAL PATH ENUMERATION
--------------------------------------------------

Use on-demand iterative DFS.

Use dynamically allocated traversal and path buffers.

Do not use recursive DFS that can overflow the program stack on deep networks.

Use a callback mechanism.

Conceptual callback:

PathCallback(pathIndices, pathLength, context, error)

The callback receives:
- Read-only path indices
- Path length
- Opaque context
- Error-output parameter

Return PertStatus.

The callback is synchronous.

The borrowed path pointer is valid only during the callback.

The Main Controller coordinates probability calculation and output through a callback adapter.

The PERT Engine must remain independent of the Probability and Output modules.

Maximum output: 100 critical paths.

For paths 1–100:
Invoke the callback.

When the 101st path is discovered:
- Do not invoke the callback.
- Mark enumeration as truncated.
- Stop enumeration.

Do not enumerate all possible paths before applying the limit.

Report:
- Number of emitted paths
- Whether enumeration was truncated

Do not claim an exact total path count when enumeration is truncated.

--------------------------------------------------
12. NUMERICAL PRECISION
--------------------------------------------------

Use double for all mathematical calculations.

Do not round intermediate durations or variances.

Centralize floating-point comparisons.

The approved starting tolerance is 1e-9.

Avoid unrestricted relative tolerance that may incorrectly merge different paths at large numerical scales.

Use appropriate local scales.

Do not use an inappropriate duration tolerance to determine whether variance is zero.

Reject non-finite results.

Generally display 2–4 decimal places.

Show additional precision when necessary to distinguish small nonzero Slack values.

Never label a noncritical activity as critical solely because its displayed Slack rounds to zero.

--------------------------------------------------
13. PROBABILITY ANALYSIS
--------------------------------------------------

Use traditional PERT single-critical-path normal approximation.

Assume independent activity durations when summing path variances.

Path variance:

Vpath = sum of activity variances along the path.

Standard deviation:

sigma = sqrt(Vpath)

Z-score:

Z = (D - Tpath) / sigma

Probability:

P = Phi(Z)

Implement the standard normal CDF using the C mathematics library, such as erfc().

Link the mathematics library correctly when GCC requires it.

One critical path:

Output its approximate completion probability.

Clearly label it as a traditional PERT approximation.

Do not claim it is an exact project completion probability.

Multiple critical paths:

Calculate and display approximate probabilities separately for enumerated paths.

Do not select the first path and present its probability as the exact project probability.

Do not simply multiply path probabilities.

Explain that paths may share activities and therefore have dependent durations.

When enumeration is truncated, explain that only a subset of paths was analyzed.

Zero variance:

Avoid division by zero.

If D >= Tpath:
Probability = 1.

Otherwise:
Probability = 0.

Use completion on or before the deadline as the convention.

Reject negative or non-finite deadlines.

--------------------------------------------------
14. FUNCTION CONTRACTS AND ERROR HANDLING
--------------------------------------------------

Define PertStatus in pert_types.h.

Include at least:

PERT_OK
PERT_ERR_INPUT
PERT_ERR_FILE
PERT_ERR_MEMORY
PERT_ERR_DUPLICATE
PERT_ERR_REFERENCE
PERT_ERR_CYCLE
PERT_ERR_NUMERIC
PERT_ERR_INTERNAL

Define PertError containing:
- Status
- Source line number, when applicable
- Activity ID, when applicable
- Human-readable error message

Use fixed diagnostic buffers where appropriate.

Do not require new memory allocation merely to report an allocation failure.

Public functions should generally:
- Return PertStatus
- Return computed data via output parameters
- Document preconditions
- Document postconditions
- Preserve safe cleanup after failure

The main calculation entry point must be:

pert_calculate()

Forward Pass, Backward Pass, and other helper functions may remain private to pert.c.

Avoid exposing internal helpers unnecessarily.

Clearly define memory ownership for every public function.

--------------------------------------------------
15. DETAILED COMMAND-LINE REPORT
--------------------------------------------------

The output must demonstrate the algorithm, not merely the final answers.

Report sections:

1. Project Overview
2. Input Activity Table
3. Activity Duration Estimation
4. Topological Order
5. Forward Pass
6. Backward Pass
7. Final PERT Results
8. Critical Activities and Paths
9. Probability Analysis
10. Warnings and Limitations

The final activity table must include:

Activity ID
Expected Duration
Variance
ES
EF
LS
LF
Slack
Critical Flag

Forward Pass:
Show how ES and EF were calculated.

Backward Pass:
Show how LF and LS were calculated.

Use stored results and graph relationships to reconstruct explanations.

Do not rerun the scheduling algorithm inside the Output Module.

Ensure output is readable in a Windows terminal.

--------------------------------------------------
16. MANDATORY CLASSROOM REGRESSION TEST
--------------------------------------------------

Create data/lecture_example.csv using the following original three-point estimates and dependency relationships:

ID,Description,a,m,b,Predecessors
A,Site clearing,1,2,3,-
B,Foundation,2,3.5,8,A
C,Block Laying,6,9,18,B
D,Roofing,4,5.5,10,C
E,Plumbing,1,4.5,5,C
F,Electrical work,4,4,10,E
G,Plastering,5,6.5,11,D
H,Fixing up of doors and windows,5,8,17,E|G
I,Ceiling,3,7.5,9,C
J,Flooring,3,9,9,F|I
K,Interior Fixtures,4,4,4,J
L,Exterior fixtures,1,5.5,7,J
M,Painting,1,2,3,H
N,Landscaping,5,5.5,9,K|L

If the original lecture screenshots are available, cross-check the input values and descriptions.

Use deadline D = 47 days.

Expected results:

Activity count: 14

Project duration: 44 days

Critical path:

A -> B -> C -> E -> F -> J -> L -> N

Critical path variance: 9

Standard deviation: 3 days

Z-score: 1

Approximate completion probability: 84.13%

Selected scheduling results:

Activity A:
ES=0 EF=2 LS=0 LF=2 Slack=0

Activity D:
ES=16 EF=22 LS=20 LF=26 Slack=4

Activity H:
ES=29 EF=38 LS=33 LF=42 Slack=4

Activity I:
ES=16 EF=23 LS=18 LF=25 Slack=2

Activity J:
ES=25 EF=33 LS=25 LF=33 Slack=0

Activity K:
ES=33 EF=37 LS=34 LF=38 Slack=1

Activity N:
ES=38 EF=44 LS=38 LF=44 Slack=0

Verify all 14 rows, not merely the selected values.

The classroom example uses Activity-on-Arrow diagrams.

This project intentionally uses Activity-on-Node.

Explain this architectural choice in the report.

Do not hardcode calculated durations, critical paths, or probability results.

Do not alter input data or round intermediate values to force expected results.

--------------------------------------------------
17. TESTING REQUIREMENTS
--------------------------------------------------

Create automated tests covering:

1. Classroom example
2. Single activity
3. Multiple starting activities
4. Multiple ending activities
5. Disconnected DAG components
6. Unsorted CSV records
7. Multiple critical paths
8. More than 100 critical paths
9. Cyclic dependencies
10. Duplicate activity IDs
11. Duplicate dependencies
12. Unknown predecessors
13. Self-dependencies
14. Invalid time estimates
15. Malformed CSV records
16. Empty fields
17. Zero variance
18. Fractional expected durations
19. Numerical overflow
20. Large dynamically allocated networks
21. Repeated analyses using the interactive menu

For the path-limit test:

Construct a graph containing more than 100 distinct critical paths.

Verify:
- Exactly 100 paths are emitted.
- Truncation is reported.
- Enumeration terminates safely.

Use appropriate floating-point tolerance comparisons in numerical tests.

Compile using GCC and C11.

Use warning flags including:

-std=c11 -Wall -Wextra -Wpedantic

Fix compiler warnings.

Actually execute the tests.

Where supported, use AddressSanitizer or another memory-safety checking tool.

If the environment does not support a particular checker, explicitly report the limitation.

Do not fabricate test results.

--------------------------------------------------
18. ENGLISH PROJECT REPORT
--------------------------------------------------

Prepare an English report.

Use the provided cover-page DOCX template.

Preserve the required institutional formatting.

Replace all placeholders with confirmed information.

Remove irrelevant example references and broken cross-references from the template.

Target approximately 15–20 pages of meaningful main content.

Suggested structure:

Abstract

Table of Contents

List of Figures

List of Tables

Chapter 1: Introduction

Chapter 2: Requirements Analysis

Chapter 3: System Architecture and Design

Chapter 4: PERT Algorithm Design

Chapter 5: System Implementation

Chapter 6: Testing and Results

Chapter 7: Conclusion

References

Optional Appendices

The report must discuss:

- Background and objectives
- Functional requirements
- Non-functional requirements
- Independent architecture decisions
- AON versus AOA
- Data structures
- Module responsibilities
- Function contracts
- Graph representation
- PERT mathematical formulas
- Algorithm pseudocode
- Algorithm complexity
- Memory ownership
- Error handling
- Test design
- Actual test results
- Classroom example verification
- Probability assumptions
- Limitations
- Future improvements

Include meaningful figures:

- System architecture diagram
- AON network diagram
- PERT algorithm flowchart
- Module dependency diagram or data model diagram

The figures must represent the actual implemented project.

Use real references only.

Do not invent bibliographic information.

Write the report based on the actual implementation and verified tests.

Do not fabricate screenshots, benchmarks, or successful test results.

Generate the final PDF.

If PDF generation or faithful preservation of the template is impossible in the current environment, explain the limitation and provide the best available editable report instead of falsely claiming completion.

--------------------------------------------------
19. README
--------------------------------------------------

Create an English README containing:

- Project overview
- Environment requirements
- Folder structure
- GCC compilation command
- Makefile usage
- Program execution instructions
- Interactive menu usage
- CSV format
- Example input
- Output explanation
- Test instructions
- Mathematical assumptions
- Known limitations
- Troubleshooting

A student using Windows, VS Code, and GCC must be able to follow the README to build and run the application.

--------------------------------------------------
20. REQUIRED IMPLEMENTATION WORKFLOW
--------------------------------------------------

Follow the phases below.

PHASE 1: INSPECT AND PLAN

Read the assignment PDF and cover DOCX.

Inspect the working directory.

Identify available development tools.

Check for genuine specification conflicts.

If a substantive design conflict exists, explain it and obtain approval.

Otherwise, proceed.

PHASE 2: IMPLEMENT CORE MODULES

Implement:
- Common types
- Project data model
- Dynamic adjacency lists
- Input parsing
- Dependency validation
- Topological sorting
- PERT calculations

Keep the computational engine independent from the CLI.

PHASE 3: IMPLEMENT ANALYSIS AND OUTPUT

Implement:
- Critical activities
- Critical edges
- Callback-based path enumeration
- Probability calculations
- Interactive menu
- Detailed CLI report

PHASE 4: COMPILE AND TEST

Compile the complete application.

Run the classroom example.

Run all normal and boundary tests.

Run invalid-input tests.

Run scalability tests.

Fix discovered defects.

Run available memory-safety checks.

PHASE 5: DOCUMENTATION

Write the README based on the actual implementation.

Generate diagrams corresponding to the real architecture.

Write the English report using actual implementation details and test results.

Generate the final PDF using the supplied cover template.

PHASE 6: PACKAGE

Verify:
- All source files exist.
- The project compiles.
- Tests are included.
- README instructions work.
- The report is complete.
- The PDF opens correctly.
- The compressed archive contains the complete source project.

Use the required student-number and student-name naming convention.

Do not include unnecessary temporary files.

--------------------------------------------------
21. FINAL RESPONSE
--------------------------------------------------

When finished, provide:

1. Implementation summary.
2. Final project directory structure.
3. Compilation instructions.
4. Execution instructions.
5. Actual test results.
6. Classroom example verification.
7. Memory-safety verification status.
8. Complete source-code archive.
9. English PDF report.
10. Remaining limitations, if any.

Clearly distinguish completed tasks from incomplete tasks.

Never claim a file exists without verifying it.

Never claim tests passed unless they were executed successfully.

Do not change the frozen design merely for convenience.

If a substantive change becomes necessary, explain the reason and obtain the student's approval.

Begin by inspecting the supplied assignment documents and working environment.

Then systematically implement, compile, test, document, and package the complete project.