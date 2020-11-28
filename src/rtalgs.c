/*
 *               RTALGS.C
 *
 * Algorithms for Real-Time Scheduling
 * Alberto Ferrari, 02/12/94 - alberto@lcmi.ufsc.br
 ******************************************************************/
/*
    This program simulates the execution of a task set in a multitasking
environment, under several real-time scheduling algorithms.  The objective is
to obtain a timeline of the execution, and to show if tasks meet their
deadlines or not.

    Tasks are assumed to be hard real-time, preemptive, periodic, with
deadline equal to the next instance's arrival time, and independent (they do
not need to syncronize with others in order to execute).  They also do not
suspend its execution voluntarily.  All tasks start execution at the same
time in the simulation.

    Available algorithms for scheduling are Rate Monotonic (RM),
Earliest-Deadline-First (EDF), Least-Laxity-First (LLF) and
Maximum-Urgency-First (MUF), selected in the command line.  Also selected in
the command line is the configuration file, containing the task set
description and the system parameters.

Usage:
    rtalgs -[e|E|l|L|m|M|r|R] <testvect.name>
where
    e    Earliest-Deadline-First (EDF)
    l    Least-Laxity-First (LLF)
    m    Maximum-Urgency-First (MUF)
    r    Rate Monotonic (RM)

Ported and expanded by Hans (hans@ele.ufes.br)

****************************************************************************/
#include <stdlib.h> /* for malloc() */
#include <stdio.h>  /* for fopen() */
#include <string.h>
#include <ctype.h>  /* for tolower() */
#include <math.h>   /* for pow() */
#include <values.h> /* for MAXINT */
#include <ctype.h>  /* for toupper */
#include <getopt.h> /* for getopt */
#ifdef DMALLOC
#include "dmalloc.h"
#endif
#ifdef __MINGW32__
#define MAXINT __INT_MAX__
#define MAXLONG __INT_MAX__
#endif
#include "skipl.h"
#include "lcmgcd.h"

typedef int Time; // = SkiplKeyType

/* alias for Skipl Library */
#define Node SkiplNode
#define List SkipList

#define GetValue(N) SkiplGetValue(N)
#define GetKey(N) SkiplGetKey(N)

#define Next(N) SkiplNext(N)
#define Head(L) SkiplHead(L)
#define IsEmpty(L) SkiplIsEmpty(L)
#define Delete(L,K) SkiplDelete(L,K)
#define NewList  SkiplNew
#define FreeList SkiplFree

#define MAXLINESIZE 190

#define TRUE 1
#define FALSE 0


/*  System-related data structures and definitions */
/***************************************************/

int algmask;
int timelineformat = 1;
int screenwidth = 72;
int verbose = 0;

//#define MAX_NAME_LENGTH 50
char *taskset_title = "";   /* title */
int num_tasks = 0;          /* number of tasks in task set */
Time sys_time = 0;          /* current clock value */
Time max_time = 0;          /* simulation upper limit */
int context_switches = 0;   /* context switches counter */
char *timeline_history = 0; /* string with id of scheduled tasks */

/* enum guarantees assignment of values from 0 on
 * IDLE: the task has not started execution yet
 * BLOCKED: the task is not eligible for execution
 * READY: it just needs the CPU to execute
 * RUNNING: the task that is currently running
 * DEAD: the task has finished its current instance's execution
 *****************************************************************************/
enum state_e {DEAD, IDLE, BLOCKED, READY, RUNNING};
enum criticality_e {LOW, HIGH};

struct task_struct {
    char sys_id;
    char *name;
    enum state_e state;
    enum criticality_e criticality;
    Time period;
    Time cpu_time;
    Time remaining;
    Time deadline; /* relative to instance start time */
    Time laxity;
    int  instance; /* current instance number */
    int  cycles;   /* number of instances executed so far */
    Time *merit;
};

typedef struct task_struct *Task;

Task  taskset;
Task  idletask;
Task  current;

/*
 * list of current task instances, instantiated from
 * its descriptors in 'taskset', and ordered in decreasing
 * value of the chosen scheduling algorithm's figure of merit
 */
List merit_list;
/*
 * list of future task requests
 */
List request_list;
/*
 * list of current task instances' deadlines, ordered by increasing
 * deadlines
 */
List deadline_list;


/* Scheduling Algorithms' function prototypes */

Task default_dispatcher(void);

void monotonic_rate_init(void);
Task monotonic_rate(void);
void monotonic_rate_end(void);

void earliest_deadline_init(void);
Task earliest_deadline(void);
void earliest_deadline_end(void);

void least_laxity_init(void);
Task least_laxity(void);
void least_laxity_end(void);

void maximum_urgency_first_init(void);
Task maximum_urgency_first(void);
void maximum_urgency_first_end(void);

/* ids used to identify tasks */
char idtable[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
int  idindex = 0;

struct alg_s {
    char id;
    char *label;
    void (*initialize)(void);
    Task (*scheduler)(void);
    void (*finish)(void);
};

struct alg_s algtable[] = {
    {   'r',
        "Rate Monotonic (RM)",
        monotonic_rate_init,
        default_dispatcher,
        monotonic_rate_end
    },
    {   'e',
        "Earliest-Deadline-First (EDF)",
        earliest_deadline_init,
        default_dispatcher,
        earliest_deadline_end
    },
    {   'l',
        "Least-Laxity-First (LLF)",
        least_laxity_init,
        least_laxity,
        least_laxity_end
    },
    {   'm',
        "Maximum-Urgency-First (MUF)",
        maximum_urgency_first_init,
        maximum_urgency_first,
        maximum_urgency_first_end
    },
    {   '\0',
        NULL,
        NULL,
        NULL,
        NULL
     }
};

typedef struct alg_s *Alg;
/*
 * Interface to Skipl Library
 */

/* let's make a new key for argument to the library:     */
/* its two MSBytes will hold the original argument       */
/* and the LSByte the sys_id of the task arguement       */
/* This way we have a unique key for search and delete   */

int build_keyid(int key, int id) {
    return (key<<8) + (id&0xFF);
}

int get_key(int keyid) {
    return keyid>>8;
}


void insert_task(List task_l, int key, Task task) {
    int new_key = build_keyid(key,task->sys_id);
    SkiplInsert(task_l, new_key, task);
}

void delete_task(List task_l, int key, Task task){
    int new_key = build_keyid(key,task->sys_id);
    Delete(task_l, new_key);
}

/*
 * Prototypes
 */

void init(int argc, char *argv[]);
void drawtimeline(char *);
Time now(void);
void taskinit(Task task, char id);

Task getleastlaxityandupdate(List l);

Task getfirstready(List l);
void showtasklist(List l, char sysid);

int readtaskset(char *fname);
void showtaskset(void);

/*
 * simulation routine
 */

void simulate(Alg palg) {
    Node n;
    Task task, new;

    context_switches=0;
    current = idletask;
    /* select which task to run next */
    for (sys_time = 0;
        /* the first condition is 'merit_list not empty' */
        (!IsEmpty(merit_list) || !IsEmpty(request_list))
            &&  sys_time <= max_time;
        sys_time++) {

            /* update current's remaining time: another time unit was executed */
            /* and if the current task emptied its allocated time... */
            if (current!=idletask  &&  -- current->remaining == 0) {
                current->state = DEAD;
                current->cycles++;
                delete_task(deadline_list, current->deadline, current);
                current = idletask;
            }

            /* Look out for deadline failures */
            while ((n=Head(deadline_list)) && (get_key(GetKey(n))<=sys_time)) {
                task=GetValue(n);
                if (task->state != DEAD) {
                    printf("At %d: task %c (\"%s\"), instance %d, Deadline Failure\n",
                        sys_time, task->sys_id, task->name, task->instance);
                }
                Delete(deadline_list, GetKey(n));
            }

            /* if it is time to launch a task... */
            while (get_key(GetKey(n=Head(request_list))) <= sys_time) {
                taskinit((task=GetValue(n)),palg->id);
                Delete(request_list, GetKey(n));
                insert_task(deadline_list, task->deadline, task);
                insert_task(request_list, task->deadline, task);
            }

            new = (palg->scheduler)();

            /* swap and register who's using the processor */
            if (current!=new){
                context_switches++;
                current->state=READY;
                current=new;
                current->state=RUNNING;
            }
            timeline_history[sys_time]= current->sys_id;
            #ifdef DEBUG
            printf("%d: %s\n", sys_time, timeline_history);
            #endif
    }


}


/* help string */
char *help1="\
rtalgs: calculate the schedulability of a task set\n\
Usage:\
\trtalgs {";
char *help2="\
 } [-a] [-w <size>] [-v] <taskset file>\n\
where\n";
char*help3="\
\t\t(At least one of the above algorithms must be specified)\n\
\ta\tAlternative timeline ouput (1 task per line)\n\
\tw\tScreen width (for timeline output)\n\
\tv\tVerbose output\n";

void usage(void) {
struct alg_s *p;

    fputs(help1,stderr);
    p = algtable;
    while (p->id) {
        fprintf(stderr," [-%c]",p->id);
        p++;
    }
    fputs(help2,stderr);
    p = algtable;
    while (p->id) {
        fprintf(stderr,"\t%c\t%s\n",p->id,p->label);
        p++;
    }
    fputs(help3,stderr);
}

void drawtimeline(char *label) {
    char *time_axe_high;
    char *time_axe_med;
    char *time_axe_low;
    char *p;
    char c;
    int i, j, no_lines, task_axe_length, offset, length;
    static char *info = NULL;
    static int infosize = 0;

    if( info == NULL || infosize != screenwidth ) { /* alloc only once */
        if( screenwidth == 0 )
            screenwidth = max_time;
        infosize = screenwidth;
        if( info )
            free(info);
        info = malloc(screenwidth+2);
        if( info == NULL ) {
            fprintf(stderr,"Not enough memory for timeline\n");
            exit(-1);
        }
    }

    time_axe_high = malloc(max_time+2);
    time_axe_med = malloc(max_time+2);
    time_axe_low = malloc(max_time+2);
    if( !time_axe_high || !time_axe_med || !time_axe_low ) {
        fprintf(stderr,"Not enough memory for timeline\n");
        exit(-1);
    }

    /* build output time reference */
    for (i=0; i<=max_time; i++) {
        c = time_axe_low[i] = i%10+'0';
        c = time_axe_med[i] = (c=='0')? (i/10)%10+'0' : ' ';
        time_axe_high[i] = (c=='0')? (i/100)%10+'0' : ' ';
    }
    time_axe_med[max_time] = (max_time/10)%10+'0';
    time_axe_high[max_time] = (max_time/100)%10+'0';
    time_axe_low[max_time+1] = '\0';
    time_axe_med[max_time+1] = '\0';
    time_axe_high[max_time+1] = '\0';

    task_axe_length = max_time+1;
    no_lines = task_axe_length/screenwidth;
    if (task_axe_length%screenwidth !=0)
        no_lines++;

    /* finally print timeline */
    printf("\nTimeline for %s algorithm\n\n", label);
    for (i=1, offset=0; i<=no_lines; i++, offset+=screenwidth) {
        if (i==no_lines) {
            length = ((task_axe_length-1)%screenwidth+1);
            memset(info, '\0', screenwidth);
        } else {
            length = screenwidth;
        }
        if( max_time >= 100 ) {
            strncpy(info, time_axe_high + offset, length);
            printf("%s\n", info);
        }
        /* axes */
        strncpy(info, time_axe_med  + offset, length);
        printf("%s\n", info);
        strncpy(info, time_axe_low  + offset, length);
        printf("%s\n", info);
        /* time line */
        if( timelineformat == 1) {
            strncpy(info, timeline_history + offset, length);
            printf("%s\n", info);
        } else {
            for(j=num_tasks;j>=0;j--) {
                strncpy(info, timeline_history + offset, length);
                p = info;
                while ( *p ) {
                    if( *p != taskset[j].sys_id ) *p = ' ';
                    p++;
                }
                printf("%s\n", info);
            }
        }
        /* axes */
        strncpy(info, time_axe_low  + offset, length);
        printf("%s\n", info);
        strncpy(info, time_axe_med  + offset, length);
        printf("%s\n", info);
        if( max_time >= 100 ) {
            strncpy(info, time_axe_high + offset, length);
            printf("%s\n", info);
        }
    }
    printf("\n%d context switches\n", context_switches);

    puts("Cross-reference Names:");
    for(i=num_tasks; i>=0; i--)
        printf("%c\t%s\n", (taskset+i)->sys_id, (taskset+i)->name);

    free(time_axe_low);
    free(time_axe_med);
    free(time_axe_high);
}

/* set up instance's dynamic parameters */
void taskinit(Task task, char id) {
    task->state    = READY;
    task->remaining= task->cpu_time;
    task->deadline = sys_time + task->period;
    task->instance++;

    /* task->laxity       = task->deadline - now() - task->remaining;
     * but task->deadline = now() + task->period;
     * and task->remaining= task->cpu_time,
     * ==>  task->laxity  = task->period - task->cpu_time;
     *****************************************************************************/
    task->laxity = task->period - task->cpu_time;
    if ( id == 'l'  ||  id == 'm' )     /* The final value must be incremented to cancel */
        task->laxity++;                 /* the laxity update of the very first instant */
}

Task default_dispatcher(void) {
    Task task;

    if ((task=getfirstready(merit_list))==NULL)
        return idletask;
    else if (current == idletask)
        return task;
    else /* current task prevails other tasks with same merit */
        return (*task->merit == *current->merit)? current : task;
}


/*
 *     Rate Monotonic (RM) algorithm
 ****************************************************************************/
void monotonic_rate_init(void)
{
    int i;
    Node n;
    Task task;      /* 'task' is the task with 'lesser' period */
    float task_load = 0.0, critical_task_load=0.0, schedulability_bound;

    /* in the RM case, 'deadline_list' is different from the 'merit_list' */
    deadline_list = NewList();

    /* calculate n*(2^1/n - 1) */
    schedulability_bound= num_tasks * (pow(2.0, 1.0/num_tasks) -1.0);
    printf("which has a schedulability bound of %.1f%% for %d tasks.\n",
            100.0 * schedulability_bound, num_tasks);

    /* insert tasks in merit_list by increasing periods */
    /* If two tasks with equal period, order them by original sequence */
    for (i=1; i<=num_tasks; i++) {
        task = taskset+i;
        task->merit = &(task->period);
        insert_task(merit_list, *(task->merit), task);
        insert_task(request_list, 0, task);
    }

    puts("Critical set is composed of");
    for (n=Head(merit_list); n!=NULL; n=Next(n)) {
        task = GetValue(n);
        task_load += (float )task->cpu_time / (float )task->period;
        if (task_load <schedulability_bound) {
            critical_task_load= task_load;
            printf("\t%s,\n", task->name);
        }
    }
    printf("which accounts for a critical load of %.1f%%, over a total system load of %.1f%%\n",
            100.0 * critical_task_load, 100.0 * task_load);
    if (task_load<=schedulability_bound) {
        printf("So, the whole task set IS");
    } else {
        if (task_load>1.0)
            printf("WARNING: the whole task set IS NOT");
        else
            printf("WARNING: the whole task set MAY NOT be");
    }
    printf(" schedulable under RM\n\n");
}

void monotonic_rate_end(void) {

    FreeList(deadline_list); deadline_list = NULL;

}
/****************************************************************************/

/*
*
*    Earliest-Deadline-First (EDF) algorithm
*****************************************************************************/

void earliest_deadline_init(void)
{
    Task task;
    float task_load = 0.0;
    int i;

    printf("which has a schedulability bound of 100%%\n");

    /* in the EDF case, 'deadline_list' is the same as 'merit_list' */
    deadline_list = merit_list;

    /* insert tasks in merit_list by increasing deadlines */
    for (i=1; i<=num_tasks; i++) {
        task=taskset+i;
        task->merit = &(task->deadline);
        task_load += (float )task->cpu_time / (float )task->period;
        insert_task(request_list, 0, task);
    }

    printf("Total system task load = %.1f%%\n", 100.0 * task_load);
    if(task_load<=1.0)
        printf("So, the whole task set IS");
    else
        printf("WARNING: the whole task set IS NOT");
    printf(" schedulable under EDF\n\n");
}

void earliest_deadline_end(void) {
    deadline_list = NULL;
}

/****************************************************************************/

/*
*
*    least laxity algorithm
*****************************************************************************/
void least_laxity_init(void) {
    Task task;
    float task_load=0.0;
    int i;

    printf("which has a schedulability bound of 100%%\n");

    /* in the LLF case, 'deadline_list' is not the same as 'merit_list' */
    deadline_list = NewList();
    //deadline_id= 'D';

    for(i=1; i<=num_tasks; i++) {
        task = taskset+i;
        task->merit = &(task->laxity);
        task_load += (float )task->cpu_time / (float )task->period;
        insert_task(merit_list, *task->merit, task);
        insert_task(request_list, 0, task);
    }
    printf("Total system task load = %.1f%%\n", 100.0 * task_load);

    if (task_load<=1.0)
        printf("So, the whole task set IS");
    else
        printf("WARNING: the whole task set IS NOT");
    printf(" schedulable under LLF\n\n");
}

Task least_laxity(void) {
    Task least;

    /* all tasks (except 'current') now have one less 'laxity' unit */
    if ((least=getleastlaxityandupdate(merit_list)) ==idletask)
        return idletask;
    else if (current== idletask)
        return least;
    else /* current task prevails other tasks with same merit */
        return (*least->merit == *current->merit)? current : least;
}


void least_laxity_end(void) {
    FreeList(deadline_list); deadline_list = NULL;
}

/*
 *
 *   Maximum-Urgency-First (MUF) Scheduling Algorithm
 ****************************************************************************/
List high_crit_l, low_crit_l;
//char high_crit_id,low_crit_id;
Task first;

void maximum_urgency_first_init(void) {
    Node n;
    List temp_list;
//    char temp_id;
    Task task;
    float critical_task_load = 0.0, task_load = 0.0, temp = 0.0, load;
    int i, critical_set = TRUE;

    printf("which has a schedulability bound of 100%%\n");

    /* in the MUF case, 'deadline_list' is not the same as 'merit_list' */
    deadline_list = NewList();// deadline_id= 'D';
    temp_list = NewList();// temp_id= 'T';
    high_crit_l = merit_list;// high_crit_id= 'H';
    low_crit_l = NewList(); //low_crit_id= 'L';

    for (i=1; i<=num_tasks; i++) {
        task = taskset+i;
        task->merit = &task->laxity;
        /* use temp_list to order tasks by increasing periods */
        insert_task(temp_list, task->period, task);
        insert_task(request_list, 0, task);
    }

    /* insert tasks in both (high_crit_l and low_crit_l) lists */
    puts("Critical set is composed of"); /* the first 'n' tasks in 'high_crit_l'
                                           * with combined load less than 100% */
    for (n=Head(temp_list); n!=NULL; n=Next(n)) {
        task=GetValue(n);
        task_load+=(load= (float )task->cpu_time / (float )task->period);

        if (task->criticality ==HIGH){
            if((temp+=load)<=1.0  &&  critical_set==TRUE){
                critical_task_load = temp;
                printf("\t%s,\n", task->name);
                insert_task(high_crit_l, task->period, task);
            } else {
                critical_set = FALSE;
                printf("WARNING at %d: Highly critical task %c (\"%s\"),\
                    found NOT Schedulable!!", sys_time, task->sys_id, task->name);
                insert_task(low_crit_l, task->period, task);
            }

        } else {    /* task->criticality ==LOW */
            insert_task(low_crit_l, task->period, task);
        }
    }
    FreeList(temp_list);

    printf("which accounts for a critical load of %.1f%%, over a total system load of %.1f%%\n",
            100.0 * critical_task_load, 100.0 * task_load);
    if (task_load<=1.0)
        printf("So, the whole task set MAY BE");
    else
        printf("WARNING: the whole task set IS NOT");
    printf(" schedulable under MUF\n\n");
}

Task maximum_urgency_first(void) {
    Task least, leasth, leastl;

    /* all tasks (except 'current') now have one less 'laxity' unit */
    leasth = getleastlaxityandupdate(high_crit_l);
    leastl = getleastlaxityandupdate(low_crit_l);
    least = (leasth==idletask)? leastl : leasth;

    /* all tasks (except 'current') have one less 'laxity' time unit */
    if (least==idletask)
        return idletask;
    else if (current== idletask)
        return least;
    else /* current task prevails other tasks with same merit */
        return (*least->merit == *current->merit)? current : least;
}

void maximum_urgency_first_end(void) {
    FreeList(deadline_list); deadline_list = NULL;
    high_crit_l = NULL;
    FreeList(low_crit_l); low_crit_l = NULL;

}

/* returns idletask if 'l' is empty */
Task getleastlaxityandupdate(List l) {
    Task task, least;
    Node n;

    least= idletask;
    for (n=Head(l); n!=NULL; n=Next(n)) {
        task = GetValue(n);
        /* task->laxity(t) = task->deadline - t - task->remaining(t);
         * but now(t)= now(t-1)+1,
         * and task->remaining(t)=  task->remaining(t-1), if task!=current,
         * ==> task->laxity(t) = task->laxity(t-1) -1;
         *********************************************************************/
        /* look out! task->laxity is decremented only if its state is READY, because of && */
        if (task->state ==READY  &&  -- task->laxity<0) { /* if it's eligible... */
            printf("At %d: task %c (\"%s\"), instance %d, will lose its deadline at %d\n",
                sys_time, task->sys_id, task->name, task->instance, task->deadline);
            task->state=BLOCKED;
        }
        if ((task->state ==READY || task->state ==RUNNING)  &&  task->laxity < least->laxity)
                least=task;
    }
    return least;
}

/* return the first READY or RUNNING task in the list */
Task getfirstready(List l) {
    Node p;
    Task task;  /* this variable must be used to cast the void *v in 'p' */

    p = Head(l);
    /* <READY  means it is not IDLE, BLOCKED nor DEAD
     * ==> This sentence fails if the states are re-enum-bered <==
     *************************************************************/
    while (p!= NULL  &&  (task=GetValue(p))->state < READY)
        p = Next(p);

    if (p==NULL)
        return NULL;
    else
        return GetValue(p);
}

void showtasklist(List l, char sys_id) {
    char state;
    Task task;
    Node n;

    printf("%c |",sys_id);
    for (n = Head(l); n != NULL; n = Next(n)) {
        printf("%d('%c',", (int) GetKey(n), (task=GetValue(n))->sys_id);
        switch (task->state){
            case DEAD:     state='d'; break;
            case IDLE:     state='i'; break;
            case BLOCKED:  state='b'; break;
            case READY:    state='r'; break;
            case RUNNING:  state='R'; break;
            default:       state='?'; break;
        }
        printf("%c)--> ", state);
    }
    printf("NIL\n\n");
}

void showtaskset(void) {
    char tmp[MAXLINESIZE+1];
    Task task;
    int i, length;

    printf("Task Set: %s\n", taskset_title);
    printf("Number of tasks in the set: %d\n", num_tasks);

    puts("\n");
    puts("Task Set Description");
    puts("--------------------");
    puts("Name                  Criticality  Period  ExecTime  Task Load");
    for (i=num_tasks; i>=1; i--) {
        length = strlen((task=taskset+i)->name);
        if (length>22) length=22;
        strncpy(tmp, task->name, 22);
        memset(tmp+length, ' ', 22-length);
        tmp[22]='\0';

        printf("%s   %6s    ", tmp, task->criticality==HIGH? "high": "low");
        printf("%5d   %6d    ", task->period, task->cpu_time);
        printf("%6.1f%%\n", 100.0 * (float )task->cpu_time / (float )task->period);
    }
}

/*
 * Read case info file
 */

const char *keywordtable[] = {
    "title",    // 0
    "tasks",    // 1
    "maxtime",  // 2
    "task",     // 3
    "end",      // 4
    NULL
};

int readtaskset(char *fname) {
    FILE *infile;
    int  ikey,itask;
    char *token;
    int i,tm,*t;
    static char tmp[MAXLINESIZE+1]; /* buffer to read in lines from configuration file */


    printf("Reading %s\n",fname);
    if ((infile = fopen(fname, "rt" )) == NULL ) {
        fprintf(stderr, "Can't open configuration file %s\n", fname);
        exit(-1);
    }

    itask = 0;
    num_tasks = 0;
    max_time = 0;
    idindex = 0;
    while ( !feof(infile) ) {
        fgets(tmp, MAXLINESIZE-1, infile);
        if (tmp[0]==';' || tmp[0]=='*' || tmp[0] == '\n' || tmp[0] == '\r' )
            continue;
        token=strtok(tmp," \t\n");
         if( token == NULL )
             continue;
        for(ikey=0;keywordtable[ikey];ikey++) {
            if( strcasecmp(token,keywordtable[ikey]) == 0 )
                break;
        }
        if( keywordtable[ikey] == 0 )  {
            fprintf(stderr,"Invalid keyword %s in file %s\n",token,fname);
            exit(-1);
        }
        switch(ikey) {
        case 0: /* title */
            token = strtok(NULL,"\n");
            taskset_title = strdup(token);
            break;
        case 1: /* tasks */
            if( num_tasks ) {
                fprintf(stderr,"Number of tasks already specified\n");
                exit(-1);
            }
               token = strtok(NULL," \t\n");
            num_tasks = atoi(token);
            if ( num_tasks <= 0 ){
                fprintf(stderr, "Invalid number of tasks\n");
                exit(-1);
            }
            if ( num_tasks > 24){
                fprintf(stderr, "Not enough id letters for all tasks\n");
                exit(-1);
            }
             if((taskset = malloc((num_tasks+1)*sizeof(struct task_struct))) == NULL) {
                fprintf(stderr, "Not enough memory available\n");
                exit(-1);
            }
            idletask = (taskset+0);
            idletask->sys_id = '.';
            idletask->name = "Idle Task";
            idletask->state = READY;
            idletask->deadline = 0;
            idletask->laxity = MAXINT;   /* maximum value a task can have */
            idletask->merit = &(idletask->deadline);
            break;
        case 2: /* maxtime */
            token = strtok(NULL," \t\n");
            max_time=atoi(token);
            break;
        case 3: /* task */
            if( num_tasks == 0 ) {
                fprintf(stderr,"Number of tasks must be specified before tasks\n");
                exit(-1);
            }
            if( itask > num_tasks ) {
                   fprintf(stderr,"More tasks than specified\n");
                exit(-1);
            }
            i = num_tasks-itask; /* fill backwards (why?) */
            if( idtable[idindex] == '\0' ) {
                fprintf(stderr,"Run out of id chars\n");
                exit(-1);
            }
            (taskset+i)->sys_id = idtable[idindex++];
            (taskset+i)->state  =IDLE;
            (taskset+i)->instance = 0;
            (taskset+i)->cycles = 0;
            token=strtok(NULL, " \t,");
            (taskset+i)->name = strdup(token);
            token=strtok(NULL, " \t,");
            (taskset+i)->criticality= strcasecmp(token, "HIGH")?  LOW: HIGH;
            (taskset+i)->period = atoi(strtok(NULL, " \t,"));
            (taskset+i)->cpu_time = (taskset+i)->remaining= atoi(strtok(NULL, "."));
            if((taskset+i)->period <0 || (taskset+i)->cpu_time < 1
                || (taskset+i)->cpu_time > (taskset+i)->period ) {
                    fprintf(stderr,"Number of tasks must be specified before tasks\n");
                    exit(-1);
            }
            itask++;
            break;
        case 4: /* end */
            goto endfile;
            break;
        }
    };

endfile:

    if( itask != num_tasks ) {
        fprintf(stderr, "Not enough tasks specified\n");
        exit(-1);
    }

    t = malloc(num_tasks*sizeof(int));
    if( t == NULL ) {
            fprintf(stderr, "Not enough memory available\n");
            exit(-1);
    }
    for(i=1;i<=num_tasks;i++) t[i-1] = taskset[i].period;
    tm = lcmv(t,num_tasks);
    free(t);
    if( max_time == 0 ) {
        max_time = tm;
    } else if ( max_time < tm ) {
        fprintf(stderr,"Time range is %d but least common multiple is %d\n",
              max_time,tm);
    }
    fclose(infile);

    /* allocate and init output timeline */
    /* two chars more: one for zero and the other for the last '\0' */

    timeline_history = malloc(max_time+2);

    if ( !timeline_history ) {
        fprintf(stderr, "Not enough memory available for allocating timeline");
        exit(-1);
    }
    memset(timeline_history, '\0', max_time+2);

    if( verbose ) printf("Done.\n");
    return TRUE; /* if it could get this point, then all was OK */
}


void cleartaskset(Task taskset, int n) {
    int i;

    if( taskset == NULL )
        return;
    if( taskset_title ) {
        free(taskset_title);
        taskset_title = NULL;
    }
    for(i=1;i<=n;i++) {
        free((taskset+i)->name);
    }
    free(taskset);

    /* release timeline */
    if( timeline_history ) {
        free(timeline_history);
        timeline_history = NULL;
    }

}
/*
 * interpret command line and initialize data structures
 */

void init(int argc, char *argv[]) {
int ch;
int mask;
struct alg_s *p;

    algmask = 0;
    while( (ch=getopt(argc,argv,"velmraw:")) != -1 ) {
        switch (ch){
            case 'a': /* alternate timeline output */
                timelineformat = 2;
                break;
            case 'w':
                screenwidth = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                p = algtable;
                mask = 0;
                while(p->id) {
                    if( ch == p->id ) {
                        mask = 1<<(p-algtable);
                    }
                    p++;
                }
                if( mask == 0 ) {
                    fprintf(stderr,"Invalid parameter\n");
                    usage();
                    exit(-1);
                } else {
                    algmask |= mask;
                }
        }
    }

    if( algmask == 0 ) {
        fprintf(stderr, "No algorithm selected\n");
        usage();
        exit(-1);
    }

}


/*
 * MAIN PROCEDURE
 *
 */

int main(int argc, char *argv[]) {
Alg palg = algtable;
int alg;
int iarg;

    SkiplInit();

    init(argc, argv);

    for(iarg=optind;iarg<argc;iarg++) {

        /* release previous allocated memory */

        if( taskset ) {
            if( verbose ) printf("Releasing memory from last case info\n");
            cleartaskset(taskset,num_tasks);
            taskset = NULL;
        }

        if( verbose ) printf("Loading case info from file %s\n",argv[iarg]);

        if (readtaskset(argv[iarg])==FALSE) {
            fprintf(stderr, "Couldn't read case info file %s",argv[iarg]);
            exit(-1);
        }
        showtaskset();

        if( verbose ) printf("Algorithms to be analyzed%X\n",algmask);

        palg = algtable;
        while(palg->id) {

            alg = palg-algtable;

            if( algmask & (1<<alg) ) {    /* if selected */

                printf("\nSelected Scheduling Algorithm: %s,\n", palg->label);

                /* init system lists */
                merit_list = NewList();
                request_list = NewList();

                if( verbose ) printf("Initialization\n");
                (palg->initialize)();

                if( verbose ) printf("Simulation\n");
                simulate(palg);

                if( verbose ) printf("Finishing\n");
                (palg->finish)();

                if( verbose ) printf("Showing timeline\n");
                drawtimeline(palg->label);

                if( verbose ) printf("Releasing memory from last algorithm\n");
                FreeList(merit_list); merit_list = NULL;
                FreeList(request_list); request_list = NULL;

            }
            palg++;
            }
        printf("\nFinished processig of task set %s\n",taskset_title);
    }
#ifdef DMALLOC
    free(taskset);
    free(timeline_history);
    dmalloc_shutdown();
#endif
    return 0;
}

