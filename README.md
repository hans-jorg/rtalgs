# Real Time Scheduling Algorithms Simulation

This is a port and upgrade of the simulation tool for real time schedulers presented by
Alberto Ferrari's article [Real Time Scheduling Algorithms](https://www.drdobbs.com/embedded-systems/real-time-scheduling-algorithms/184409363?pgno=10), published in the December, 1994 issue of Dr. Dobb's magazine.

The code originally was developed for DOS.  The code was ported to Linux and in the process expanded. It can now simulates many task sets and
has a verbose option.

This repo has two versions: 

* src: a new (updated) code with a new input format.
* src-original: the original one (running and using the old input format)


# Usage

## new version

    rtalgs: calculate the schedulability of a task set
    Usage:	rtalgs { [-r] [-e] [-l] [-m] } [-a] [-w <size>] [-v] <taskset file>
    where
        r	Rate Monotonic (RM)
        e	Earliest-Deadline-First (EDF)
        l	Least-Laxity-First (LLF)
        m	Maximum-Urgency-First (MUF)
            (At least one of the above algorithms must be specified)
        a	Alternative timeline ouput (1 task per line)
        w	Screen width (for timeline output)
        v	Verbose output

## original version

    rtalgs: calculate the schedulability of a task set
    Usage:  rtalgs { [-r] [-e] [-l] [-m] } <taskset file>
    where
        r   Rate Monotonic (RM)
        e   Earliest-Deadline-First (EDF)
        l   Least-Laxity-First (LLF)
        m   Maximum-Urgency-First (MUF)
            (At least one of the above algorithms must be specified)


# Compilation

Just use make and you get a *rtalgs* binary
    
	make

# Input format

## New version

The input format has changed to a simple one. The lines have a simple keyword to
identify the data.

    title Example 1
    tasks 3
    maxtime 2000
    ;    Name  Cricticality    Period    Execution time
    task name1 HIGH              6              2
    task name2 HIGH              8              2
    task name2 LOW              12              3
    end
    
The task line has the same format as before.

    task <name> <HIGH|LOW> <period> <load>

All lines with an asterisk or a semicolon in the column 1 are ignored.
So are all blank lines.
    

## Original version

    CONFIGURATION FILE:
    START:
    TEST SET: Example 1
    MAXTIME= 27 
    NUMBER OF APPLICATION TASKS=3
    APPLICATION TASKS DESCRIPTION:
    Name            Criticality  Period      Execution_time    
    Task Name1,       HIGH,        6,           2.
    Task Name2,       HIGH,        8,           2.
    Task Name3,       LOW,        12,           3.
    end.

All lines with a semicolon in the column 1 are ignored.
