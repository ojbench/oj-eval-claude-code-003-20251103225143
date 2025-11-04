# ICPC Management System

**ACMOJ Problem ID**: 1986

## Table of Contents

- [ICPC Management System](#icpc-management-system)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
    - [Background](#background)
  - [Assignment Description](#assignment-description)
    - [Grade Composition](#grade-composition)
  - [Assignment Requirements](#assignment-requirements)
    - [Terminology](#terminology)
    - [Command Descriptions](#command-descriptions)
    - [Input Format](#input-format)
    - [Output Format](#output-format)
    - [Data Constraints](#data-constraints)
  - [Per-Testcase Resource Limits](#per-testcase-resource-limits)
  - [Submission Requirements](#submission-requirements)
    - [OJ Git Repository Compilation Process](#oj-git-repository-compilation-process)
    - [Git Configuration Requirements](#git-configuration-requirements)
    - [Submission Guidelines](#submission-guidelines)
    - [Evaluation Notes](#evaluation-notes)
    - [Academic Integrity](#academic-integrity)

## Introduction

### Background

**ICPC** (International Collegiate Programming Contest) is an annual competition organized by the ICPC Foundation, designed to showcase university students' innovation capabilities, teamwork, and their ability to write programs, analyze and solve problems under pressure. It is the most influential computer science competition for university students. In ICPC competitions, each team attempts to solve the maximum number of problems with the minimum number of incorrect submissions. The winner is the team that correctly solves the most problems with the least total penalty time.

## Assignment Description

### Grade Composition

| Grading Component | Percentage |
| :--: | :--: |
| Pass **1986. ICPC Management System (2024 A)** | 80% |
| Code Review | 20% |

Here are several points that need clarification:

- In the Code Review, we will **strictly examine your code style and repository organization structure, etc.**. 

- This assignment provides some sample data for testing, stored in the `/workspace/data/003/data_test/` directory. Note that these are not the test cases on the Online Judge. Passing all local test cases does not guarantee that you will pass the OJ tests.

- Besides the provided sample data, we also encourage you to design your own test data based on your program logic to assist debugging.

## Assignment Requirements

### Terminology

Since this assignment involves many specialized terms, to better understand the command descriptions below, we will first explain the terminology used in the assignment.

- **Competition Time**: We use `duration_time` to represent the duration of the competition. The competition time range is the closed interval `[1, duration_time]`. Therefore, we can use an integer in this interval to represent a specific time point during the competition. We only guarantee that submission times in the input data are **monotonically non-decreasing**, which means **identical** times may occur.

- **Team**: Each participating team has its own unique team name. Team names consist of combinations of uppercase and lowercase letters, numbers, and underscores, with a maximum length of 20 characters (inclusive).

- **Submission**: A team submits a solution which, after being evaluated by the judge system, provides the backend with basic information about this submission. Submissions before the freeze will update the team's status in real-time (such as the number of solved problems), but **will not update the team's ranking on the scoreboard**.

- **Judge Status**: Each submission has a corresponding judge status, which may include:

  - Accepted
  - Wrong_Answer
  - Runtime_Error
  - Time_Limit_Exceed

  Only Accepted counts as passing; the remaining statuses do not count as passing.

- **Flush Scoreboard**: Update the team rankings on the scoreboard.

- **Scoreboard**: Displays the status of each team in order from highest to lowest ranking.

- **Penalty Time**: A parameter used to compare team rankings. A team's penalty time for a particular problem is defined as $P = 20X + T$, where $X$ is the number of submissions before the first correct submission, and $T$ is the time when the team solved this problem (i.e., the time of the first correct submission). A team's penalty time is defined as the sum of penalty times for all **solved problems**.

- **Ranking**: Competition rankings are determined by multiple parameters:
  - First, teams with more solved problems rank higher;
  - When two teams have solved the same number of problems, we compare their penalty times; the team with less penalty time ranks higher;
  - If still tied, we compare the maximum solve time among solved problems for both teams; the team with the smaller maximum solve time ranks higher. If equal, compare the second largest solve time, then the third largest, and so on;
  - If still tied, compare team names lexicographically; the team with the smaller lexicographic order ranks higher (since team names are unique, one must be lexicographically smaller than the other).
  - **Note**: All of the above factors **do not include frozen problems** (see next item for frozen status). Obviously, after freezing and before scrolling, the rankings on the scoreboard will not change.
  - Before the first scoreboard flush, rankings are based on the lexicographic order of team names.

- **Freeze**: After freezing, for any team, all **problems unsolved by that team before the freeze**, the real-time submission results are not displayed on the scoreboard after freezing. Instead, only the number of submissions to the problem during the freeze period is shown. Problems with at least one submission after freezing will enter a **frozen state** (problems solved before freezing will not be frozen even if submitted again after freezing).

- **Scroll**: During the scrolling session, each time we select the lowest-ranked team on the scoreboard that has frozen problems, and select the problem with the smallest number among that team's frozen problems to unfreeze. We then recalculate rankings and update the ranking status on the scoreboard (the scroll operation first flushes the scoreboard before proceeding). Then, on the updated scoreboard, we again select the lowest-ranked team with frozen problems and repeat the unfreezing operation until no team has any frozen problems remaining. This way, we obtain the current correct scoreboard.
  - **Note**: Unlike actual competitions, in this assignment, multiple freezes and scrolls can occur within a single competition. Each scroll must be executed while in a frozen state; after scrolling ends, the frozen state will be lifted, and freezing can be done again afterward.

### Command Descriptions

All command formats are provided in the code blocks below. The all-uppercase parts represent commands, and the lowercase parts within square brackets `[]` represent corresponding parameters (the brackets will not appear in the input).

```plain
# Add team
ADDTEAM [team_name]

# Start competition
START DURATION [duration_time] PROBLEM [problem_count]

# Submit problem
SUBMIT [problem_name] BY [team_name] WITH [submit_status] AT [time]

# Flush scoreboard
FLUSH

# Freeze scoreboard
FREEZE

# Scroll scoreboard
SCROLL

# Query team ranking
QUERY_RANKING [team_name]

# Query team submission
QUERY_SUBMISSION [team_name] WHERE PROBLEM=[problem_name] AND STATUS=[status]

# End competition
END
```

- Add team
  - `ADDTEAM [team_name]`
  - Add a team to the system.
    - If successfully added, output `[Info]Add successfully.\n`
    - If the competition has started, output `[Error]Add failed: competition has started.\n`
    - If the competition hasn't started but the team name is duplicated, output `[Error]Add failed: duplicated team name.\n`

- Start competition
  - `START DURATION [duration_time] PROBLEM [problem_count]`
  - Start the competition. The competition time range is the closed interval `[1, duration_time]`, and problem IDs range over the first `problem_count` uppercase English letters.
    - If successfully started, output `[Info]Competition starts.\n`
    - If the competition has already started, output `[Error]Start failed: competition has started.\n`

**All subsequent operations are guaranteed to occur after the competition has started.**

- Submit problem
  - `SUBMIT [problem_name] BY [team_name] WITH [submit_status] AT [time]`
  - The input is guaranteed to be valid. Record a submission by `team_name` at time `time` for problem `problem_name` with judge status `submit_status`.
    - `submit_status` may include: Accepted, Wrong_Answer, Runtime_Error, Time_Limit_Exceed. Only Accepted counts as passing; the remaining statuses do not count as passing. Times are guaranteed to increase monotonically (non-strictly) in the order submissions appear.
    - This command has no output.

- Flush scoreboard
  - `FLUSH`
  - Flush the current scoreboard.
    - Output `[Info]Flush scoreboard.\n`

- Freeze scoreboard
  - `FREEZE`
  - Perform the freeze operation.
    - If successful, output `[Info]Freeze scoreboard.\n`
    - If already frozen but not yet scrolled, output `[Error]Freeze failed: scoreboard has been frozen.\n`

- Scroll scoreboard
  - `SCROLL`
    - If not frozen, output `[Error]Scroll failed: scoreboard has not been frozen.`
    - If frozen, scrolling can begin:
      - First output the prompt `[Info]Scroll scoreboard.\n`
      - Then output the scoreboard **before scrolling** (this scoreboard is **after flushing**)
      - Next, output each unfreeze that **causes a ranking change** during scrolling, one per line
      - Finally, output the scoreboard **after scrolling**
    - The output format for ranking changes is as follows:

      ```plain
      [team_name1] [team_name2] [solved_number] [penalty_time]
      ```

      `team_name1` represents the team whose ranking increased due to problem unfreezing, `team_name2` represents the team whose ranking was replaced by `team_name1` (i.e., the team that was at the position before `team_name1`'s ranking increase), `solved_number` and `penalty_time` are `team_name1`'s new number of solved problems and penalty time.
    - The scoreboard output format is as follows:
      Output $N$ lines (where $N$ is the total number of teams), each line in the format:

      ```plain
      team_name ranking solved_count total_penalty A B C ...
      ```

      representing a team's status, where "A B C ..." represents the status of each problem, with three possible cases:

      - Problem is not frozen and has been solved:
        - Display `+x`, where `x` is the number of incorrect attempts before the first successful submission
        - If `x` is 0, display `+` instead of `+0`
      - Problem is not frozen but not solved:
        - Display `-x`, where `x` is the number of incorrect attempts
        - If `x` is 0 (i.e., the team hasn't submitted this problem yet), display `.` instead of `-0`
      - Problem is frozen:
        - Display `-x/y`, where `x` is the number of incorrect attempts before freezing, and `y` is the number of submissions after freezing
        - If `x` is 0, display `0/y` instead of `-0/y`

- Query team ranking
  - `QUERY_RANKING [team_name]`
    - Query the ranking of the corresponding team.
    - If the team doesn't exist, output `[Error]Query ranking failed: cannot find the team.\n`
    - If the team exists, output `[Info]Complete query ranking.\n`. If in a frozen state, output an additional line `[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n`. Regardless of freeze status, output the team's ranking after the last scoreboard flush in the following format:

      ```plain
      [team_name] NOW AT RANKING [ranking]
      ```

- Query team submission
  - `QUERY_SUBMISSION [team_name] WHERE PROBLEM=[problem_name] AND STATUS=[status]`
    - Query the last submission of the corresponding team that satisfies the conditions. **Submissions after freezing can be queried.**

      Here are some valid examples for reference:

      ```plain
      # Query the last submission by Team_Rocket
      QUERY_SUBMISSION Team_Rocket WHERE PROBLEM=ALL AND STATUS=ALL
      
      # Query the last submission with Accepted status by Team_Plasma
      QUERY_SUBMISSION Team_Plasma WHERE PROBLEM=ALL AND STATUS=Accepted

      # Query the last submission to problem A by Pokemon_League
      QUERY_SUBMISSION Pokemon_League WHERE PROBLEM=A AND STATUS=ALL

      # Query the last submission to problem M with Runtime_Error status by Opelucid_Gym
      QUERY_SUBMISSION Opelucid_Gym WHERE PROBLEM=M AND STATUS=Runtime_Error
      ```

    - If the team doesn't exist, output `[Error]Query submission failed: cannot find the team.\n`
    - If the team exists, output `[Info]Complete query submission.\n`
      - If no submission satisfies the conditions, output `Cannot find any submission.\n`
      - If there is a submission satisfying the conditions, output one line representing the last submission that satisfies the conditions in the following format:

        ```plain
        [team_name] [problem_name] [status] [time]
        ```

        `problem_name` is the problem ID of the submission, `status` is the submission status, and `time` is the submission time. The formats of `problem_name` and `status` in the query are guaranteed to be valid.

- End competition
  - `END`
    - End the competition.
      - Output `[Info]Competition ends.\n`. The scoreboard is guaranteed not to be in a frozen state when the competition ends, and there will be no operations afterward.

### Input Format

- After the program starts running, it will read several commands until the `END` command is read.
- Command formats are guaranteed to be valid (but the content executed by commands is not guaranteed to be valid; see the text above for details).

### Output Format

Output according to the format required in the Command Descriptions section.

### Data Constraints

For 60% of the data: total number of teams $N \le 500$, number of operations $\mathit{opt}\le 10^4$.

For 100% of the data: total number of teams $N \le 10^4$, total number of problems $M \le 26$, competition duration $T \le 10^5$, number of operations $\mathit{opt}\le 3\times 10^5$, number of flush operations $\mathit{opt_{flush}} \le 1000$, number of freeze operations $\mathit{opt_{freeze}}\le 10$.

## Per-Testcase Resource Limits

- **Time Limit (per test case)**: 2000 ms  
- **Memory Limit (per test case)**: 512 MiB  
- **Disk Usage**: Disk access is not permitted.

## Submission Requirements

### OJ Git Repository Compilation Process

For Git compilation, we will first clone the repository using a command similar to:
```bash
git clone <repo_url> . --depth 1 --recurse-submodules --shallow-submodules --no-local
```

Then we check if there is a `CMakeLists.txt` file. If it exists, we run (if not, a warning message will be displayed):
```bash
cmake .
```

Finally, we check if there is any of `GNUmakefile`/`makefile`/`Makefile` (if cmake was run previously, this will be the generated Makefile). If it exists, we run (if not, a warning message will be displayed):
```bash
make
```

After this process is complete, we will use the `code` file in the project root directory as the compilation result.

The project does not provide a CMakeLists.txt file, so you need to create and edit it yourself. The local environment has gcc-13 and g++-13 available.

### Git Configuration Requirements

**IMPORTANT**: You must create a `.gitignore` file in your project root directory to avoid OJ evaluation conflicts.

The `.gitignore` file should include at least the following entries:

```gitignore
CMakeFiles/
CMakeCache.txt
```

### Submission Guidelines

- The submitted code must be able to compile successfully through the above compilation process
- The compiled executable file name must be `code`
- The program needs to be able to read data from standard input and write results to standard output
- Please ensure the code runs correctly within the given time and space limits
- **You must use C++ or C language** to implement this assignment

### Evaluation Notes

- The program output must exactly match the expected output (including format)
- Exceeding time or memory limits will be judged as the corresponding error type
- Please pay attention to the overall time performance of your code and the time complexity of each part of your algorithm.

### Academic Integrity

If any violations are found during evaluation or code review (including but not limited to using unconventional methods to pass test cases), your final score may be significantly reduced or become **0 points**.