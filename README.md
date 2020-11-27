# Real Time Scheduling Algorithms Simulation

This is a port and upgrade of the simulation tool for real time schedulers presented by
Alberto Ferrari's article [Real Time Scheduling Algorithms](https://www.drdobbs.com/embedded-systems/real-time-scheduling-algorithms/184409363?pgno=10), published in the December, 1994 issue of Dr. Dobb's magazine.

The code originally was developed for DOS.

The code was ported to Linux and in the process expanded. It can now simulates many task sets and
has a verbose option.

# Use
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


# Compilation

Just use make and you get a *rtalgs* binary
    
	make

