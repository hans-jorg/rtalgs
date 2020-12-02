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
    e   Earliest-Deadline-First (EDF)
    l   Least-Laxity-First (LLF)
    m   Maximum-Urgency-First (MUF)
    r   Rate Monotonic (RM)

****************************************************************************/
#include <stdlib.h> /* for malloc() */
#include <stdio.h>  /* for fopen() */
#include <string.h>
#include <ctype.h>  /* for tolower() */
#include <math.h>   /* for pow() */
#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define SILENT      /* silent operation */

#include "skipl.h"
#include "rtalgs.h"

/* help string */
char *help="\
Usage:\
\trtalgs -[e|l|m|r] <testvect.name>\n\n\
where\
\te\tEarliest-Deadline-First (EDF)\n\
\tl\tLeast-Laxity-First (LLF)\n\
\tm\tMaximum-Urgency-First (MUF)\n\
\tr\tRate Monotonic (RM)\n";

/* algorithm-related definitions */
#define RM     0    /* Rate Monotonic */
#define EDF    1    /* Earliest-Deadline-First */
#define LLF    2    /* Least-Laxity-First */
#define MUF    3    /* Maximum-Urgency-First */
int alg;
char *labels[]={ "Rate Monotonic (RM)",      "Earliest-Deadline-First (EDF)",
                 "Least-Laxity-First (LLF)", "Maximum-Urgency-First (MUF)"};

#ifdef SILENT
char *bell="";
#else
char *bell="\a\a";
#endif

/*
 * emulate non standard strupr
 */
char *strupr(char *s) {
char *p = s;

    while (*p) {
        if( islower(*p) ) *p = toupper(*p);
        p++;
    }
    return s;
}
/*  System-related data structures and definitions */
/***************************************************/
char sys_id='a';  /* 'system name' of the next task in configuration file */
int num_tasks;    /* is limited to 24, for not running out of sys_id letters */
timet sys_time, max_time; /* current clock value, and simulation upper limit */
int context_switches=0;
struct{
    char *history,
         *time_axe_high, *time_high,
             *time_axe_med,  *time_med,
             *time_axe_low,  *time_low;
} timeline;



task_t  *task_set, *idle_task, *current;
list merit_list,  /* list of current task instances, instantiated from
                   * its descriptors in 'task_set', and ordered in decreasing
                   * value of the chosen scheduling algorithm's figure of merit
                   ***********************************************************/
    request_list,  /* list of future task requests */
    deadline_list; /* list of current task instances' deadlines,
                        * ordered by increasing deadlines */

int main( int argc, char *argv[])
{
    node n;
    task_t *task, *new;
    int kn;

    init( argc, argv);

    printf( "\nSelected Scheduling Algorithm: %s,\n", labels[ alg]);
    (sched_alg_init)();

    /* select which task to run next */
    for( sys_time=0;

        /* the first condition is 'merit_list not empty' */
        (merit_list->header->forward[0] !=NIL  ||  request_list->header->forward[0] !=NIL)
            &&  sys_time <= max_time;

        sys_time++){

            /* update current's remaining time: another time unit was executed */
            /* and if the current task emptied its allocated time... */
            if( current!= idle_task  &&  -- current->remaining == 0){
                current->state=DEAD;
                current->cycles++;
                delete_task( deadline_list, current->deadline, current);
                current= idle_task;
            }

            /* Look out for deadline failures */
            while( (n=first_node_of( deadline_list)) && (key_of(n)<=sys_time) ) {
                task = n->v;
                if( task->state != DEAD ) {
                    printf( "At %d: task %c (\"%s\"), instance %d, Deadline Failure%s\n",
                        sys_time, task->sys_id, task->name, task->instance, bell);
                }
                delete( deadline_list, n->key);
            }

            /* if it is time to launch a task... */
            while(  key_of( n=first_node_of( request_list))  <= sys_time){
                task_init( (task= n->v) );
                delete( request_list, n->key);
                insert_task( deadline_list, task->deadline, task);
                insert_task(  request_list, task->deadline, task);
            }

            new = (sched_alg)();

            /* swap and register who's using the processor */
            if( current!=new){
                context_switches++;
                current->state=READY;
                current=new;
                current->state=RUNNING;
            }
            timeline.history[ sys_time]= current->sys_id;
            #ifdef DEBUG
            printf( "%d: %s\n", sys_time, timeline.history);
            #endif
    }
    (sched_alg_end)();

    draw_timeline();

    return 0;
}


void init( int argc, char *argv[])
{
    if( argc<3  ||  *argv[1] != '-'){
        fprintf( stderr, "%s", help);
        exit(-1);
    }

    switch( tolower( *(argv[1]+1))){
        case 'e': alg=EDF;
            sched_alg_init = earliest_deadline_init;
            sched_alg = default_dispatcher;
            sched_alg_end = earliest_deadline_end;
            break;
        case 'l': alg=LLF;
            sched_alg_init = least_laxity_init;
            sched_alg = least_laxity;
            sched_alg_end = least_laxity_end;
            break;
        case 'm': alg=MUF;
            sched_alg_init = maximum_urgency_first_init;
            sched_alg = maximum_urgency_first;
            sched_alg_end = maximum_urgency_first_end;
            break;
        case 'r': alg=RM;
            sched_alg_init =monotonic_rate_init;
            sched_alg = default_dispatcher;
            sched_alg_end = monotonic_rate_end;
            break;
        default:
            fprintf( stderr, "No valid algorithm selected: '%c'\n\n%s", *(argv[1]+1), help);
            exit(-1);
    }

    puts( "\n\nInitializing program data structures...");
    if( get_test_vector( argv[2])==FALSE){
        fprintf( stderr, "Couldn't load test set");
        exit(-1);
    }
    if( num_tasks<1){
        fprintf( stderr, "At least one valid task must be specified!\n");
        fprintf( stderr, "Please, check out your configuration file%s\n", bell);
        exit(-1);
    }
    show_test_vector();

    /* allocate and init output timeline */
    /* two chars more: one for zero and the other for the last '\0' */
    if( (timeline.history= malloc( max_time+2))==NULL
        || (timeline.time_axe_high= malloc( max_time+2))==NULL
        || (timeline.time_axe_med= malloc( max_time+2))==NULL
        || (timeline.time_axe_low= malloc( max_time+2))==NULL){
            fprintf( stderr, "Not enough memory available for allocating timeline");
            exit(-1);
    }
    memset( timeline.history, '\0', max_time+2);
    memset( timeline.time_axe_high, '\0', max_time+2); timeline.time_high=timeline.time_axe_high;
    memset( timeline.time_axe_med, '\0', max_time+2); timeline.time_med=timeline.time_axe_med;
    memset( timeline.time_axe_low, '\0', max_time+2); timeline.time_low=timeline.time_axe_low;

    /* idle task initialization */
    idle_task=(task_set+0);
    idle_task->sys_id='.';
    idle_task->name="Idle Task";
    idle_task->state=READY;
    idle_task->deadline= 0;
    idle_task->laxity=MAX_VALUE;   /* maximum value a task can have */
    idle_task->merit= &idle_task->deadline;

    /* init system lists */
    skipl_library_init();
    merit_list= newList(); merit_list->sys_id= 'M';
    request_list = newList(); request_list ->sys_id= 'R';

    current=idle_task;
}

void draw_timeline( void)
{
    #define SCREEN_WIDTH 75
    static char info[ SCREEN_WIDTH+1], c, last, *ptr;
    int i, no_lines, task_axe_length, offset, length;

    /* build output time reference */
    for( sys_time=0; sys_time<=max_time; sys_time++){
        c= *timeline.time_low++ = sys_time%10+'0';
        c= *timeline.time_med++ = (c=='0')? (sys_time/10)%10+'0' : ' ';
        *timeline.time_high++ = (c=='0')? (sys_time/100)%10+'0' : ' ';
    }

    task_axe_length= max_time+1;
    no_lines= task_axe_length/SCREEN_WIDTH;
    if( task_axe_length%SCREEN_WIDTH !=0)
        no_lines++;

    /* finally print timeline */
    printf("\nTIMELINE: [%s]\n", labels[ alg]);
    for( i=1, offset=0;   i<=no_lines;   i++, offset+=SCREEN_WIDTH){
        if( i==no_lines){
            length= ((task_axe_length-1)%SCREEN_WIDTH+1);
            memset( info, '\0', SCREEN_WIDTH);
        }else length=SCREEN_WIDTH;

        strncpy( info, timeline.history       + offset, length); printf( "\n%s\n", info);
        strncpy( info, timeline.time_axe_low  + offset, length); printf( "%s\n",   info);
        strncpy( info, timeline.time_axe_med  + offset, length); printf( "%s\n",   info);
        strncpy( info, timeline.time_axe_high + offset, length); printf( "%s\n",   info);
    }
    printf( "\n%d context switches\n", context_switches);

    puts( "Cross-reference Names:");
    for( i=num_tasks; i>=1; i--)
        printf( "%c\t%s\n", (task_set+i)->sys_id, (task_set+i)->name);
}

timet now( void){ return( sys_time);};

/* set up instance's dynamic parameters */
void task_init( task_t *task)
{
    task->state    = READY;
    task->remaining= task->cpu_time;
    task->deadline = now() + task->period;
    task->instance++;

    /* task->laxity       = task->deadline - now() - task->remaining;
     * but task->deadline = now() + task->period;
     * and task->remaining= task->cpu_time,
     * ==>  task->laxity  = task->period - task->cpu_time;
     *****************************************************************************/
    task->laxity   = task->period - task->cpu_time;
    if( alg==LLF  ||  alg==MUF)   /* The final value must be incremented to cancel */
        task->laxity++;          /* the laxity update of the very first instant */
}

task_t *default_dispatcher( void)
{
    task_t *task;

    if(  (task=first_ready( merit_list))==NULL)
        return( idle_task);
    else if( current== idle_task)
        return( task);
    else /* current task prevails other tasks with same merit */
        return(  (*task->merit == *current->merit)? current : task);
}


/*
 *     Rate Monotonic (RM) algorithm
 ****************************************************************************/
void monotonic_rate_init( void)
{
    int i;
    node n;
    task_t *task;      /* 'task' is the task with 'lesser' period */
    float task_load=0.0, critical_task_load=0.0, schedulability_bound;

    /* in the RM case, 'deadline_list' is different from the 'merit_list' */
    deadline_list= newList(); deadline_list->sys_id= 'D';

    /* calculate n*(2^1/n - 1) */
    schedulability_bound= num_tasks * ( pow( 2.0, 1.0/num_tasks) -1.0);
    printf( "which has a schedulability bound of %.1f%% for %d tasks.\n",
            100.0 * schedulability_bound, num_tasks);

    /* insert tasks in merit_list by increasing periods */
    /* If two tasks with equal period, order them by original sequence */
    for( i=1; i<=num_tasks; i++){
        task->merit= &(task=task_set+i)->period;
        insert_task( merit_list, *task->merit, task);
        insert_task( request_list, 0, task);
    }

    puts( "Critical set is composed of");
    for( task= (n= merit_list->header->forward[0])->v; n!=NIL; task= (n= n->forward[0])->v){
        task_load+= (float )task->cpu_time / (float )task->period;
        if( task_load <schedulability_bound){
            critical_task_load= task_load;
            printf( "\t%s,\n", task->name);
        }
    }
    printf( "which accounts for a critical load of %.1f%%, over a total system load of %.1f%%\n",
            100.0 * critical_task_load, 100.0 * task_load);
    if( task_load<=schedulability_bound) printf( "So, the whole task set IS");
    else if( task_load>1.0) printf( "WARNING: the whole task set IS NOT");
    else printf( "WARNING: the whole task set MAY NOT be");
    printf( " schedulable under RM\n\n");
}

void monotonic_rate_end( void){}
/****************************************************************************/

/*
*
*    Earliest-Deadline-First (EDF) algorithm
*****************************************************************************/

void earliest_deadline_init( void)
{
    task_t *task;
    float task_load=0.0;
    int i;

    printf( "which has a schedulability bound of 100%%\n");

    /* in the EDF case, 'deadline_list' is the same as 'merit_list' */
    deadline_list= merit_list;

    /* insert tasks in merit_list by increasing deadlines */
    for( i=1; i<=num_tasks; i++){
        task->merit= &(task=task_set+i)->deadline;
        task_load+= (float )task->cpu_time / (float )task->period;
        insert_task( request_list, 0, task);
    }

    printf( "Total system task load = %.1f%%\n", 100.0 * task_load);
    if( task_load<=1.0) printf( "So, the whole task set IS");
    else printf( "WARNING: the whole task set IS NOT");
    printf( " schedulable under EDF\n\n");
}

void earliest_deadline_end( void) {}
/****************************************************************************/

/*
*
*    least laxity algorithm
*****************************************************************************/
void least_laxity_init( void)
{
    task_t *task;
    float task_load=0.0;
    int i;

    printf( "which has a schedulability bound of 100%%\n");

    /* in the LLF case, 'deadline_list' is not the same as 'merit_list' */
    deadline_list= newList(); deadline_list ->sys_id= 'D';

    for( i=1; i<=num_tasks; i++){
        task->merit= &(task=task_set+i)->laxity;
        task_load+= (float )task->cpu_time / (float )task->period;
        insert_task( merit_list, *task->merit, task);
        insert_task( request_list, 0, task);
    }
    printf( "Total system task load = %.1f%%\n", 100.0 * task_load);

    if( task_load<=1.0) printf( "So, the whole task set IS");
    else printf( "WARNING: the whole task set IS NOT");
    printf( " schedulable under LLF\n\n");
}

task_t *least_laxity( void)
{
    task_t *least;

    /* all tasks (except 'current') now have one less 'laxity' unit */
    if(  (least=update_laxity_and_get_least( merit_list)) ==idle_task)
        return( idle_task);
    else if( current== idle_task)
        return( least);
    else /* current task prevails other tasks with same merit */
        return( (*least->merit == *current->merit)? current : least);
}


void least_laxity_end( void){}
/****************************************************************************/

/*
 *
 *   Maximum-Urgency-First (MUF) Scheduling Algorithm
 ****************************************************************************/
list high_crit_l, low_crit_l;
task_t *first;

void maximum_urgency_first_init( void)
{
    node n;
    list temp_list;
    task_t *task;
    float critical_task_load=0.0, task_load=0.0, temp=0.0, load;
    int i, critical_set=TRUE;

    printf( "which has a schedulability bound of 100%%\n");

    /* in the MUF case, 'deadline_list' is not the same as 'merit_list' */
    deadline_list= newList(); deadline_list->sys_id= 'D';
    temp_list= newList(); temp_list->sys_id= 'T';
    high_crit_l= merit_list; high_crit_l->sys_id= 'H';
    low_crit_l= newList(); low_crit_l->sys_id= 'L';

    for( i=1; i<=num_tasks; i++){
        task=task_set+i;
        task->merit= &task->laxity;
        /* use temp_list to order tasks by increasing periods */
        insert_task( temp_list, task->period, task);
        insert_task( request_list, 0, task);
    }

    /* insert tasks in both (high_crit_l and low_crit_l) lists */
    puts( "Critical set is composed of"); /* the first 'n' tasks in 'high_crit_l'
                                           * with combined load less than 100% */
    for( task= (n= temp_list->header->forward[0])->v; n!=NIL; task= (n=n->forward[0])->v){
        task_load+= (load= (float )task->cpu_time / (float )task->period);

        if( task->criticality ==HIGH){
            if(  (temp+=load)<=1.0  &&  critical_set==TRUE){
                critical_task_load= temp;
                printf( "\t%s,\n", task->name);
                insert_task( high_crit_l, task->period, task);
            }else{
                critical_set= FALSE;
                printf( "WARNING at %d: Highly critical task %c (\"%s\"),\
                    found NOT Schedulable!!%s", now(), task->sys_id, task->name, bell);
                printf( "\nContinue anyway? (y/[N]) ");
                if( (i=getchar()) == '\n' || i=='n' || i=='N') exit(0);
                else insert_task( low_crit_l, task->period, task);
            }

        }else{    /* task->criticality ==LOW */
            insert_task( low_crit_l, task->period, task);
        }
    }
    freeList( temp_list);

    printf( "which accounts for a critical load of %.1f%%, over a total system load of %.1f%%\n",
            100.0 * critical_task_load, 100.0 * task_load);
    if( task_load<=1.0) printf( "So, the whole task set MAY BE");
    else printf( "WARNING: the whole task set IS NOT");
    printf( " schedulable under MUF\n\n");
}

task_t *maximum_urgency_first( void)
{
    task_t *least, *leasth, *leastl;

    /* all tasks (except 'current') now have one less 'laxity' unit */
    leasth= update_laxity_and_get_least( high_crit_l);
    leastl= update_laxity_and_get_least( low_crit_l);
    least= (leasth==idle_task)? leastl : leasth;

    /* all tasks (except 'current') have one less 'laxity' time unit */
    if( least==idle_task)
        return( idle_task);
    else if( current== idle_task)
        return( least);
    else /* current task prevails other tasks with same merit */
        return( (*least->merit == *current->merit)? current : least);
}

void maximum_urgency_first_end( void){}
/****************************************************************************/

/***************************************************************************/
        /**** scheduling algs related constants and data ****/

/* returns idle_task if 'l' is empty */
task_t *update_laxity_and_get_least( list l)
{
    task_t *task, *least;
    node n;

    least= idle_task;
    for( task= (n= l->header->forward[0])->v; n!=NIL; task= (n= n->forward[0])->v){
        /* task->laxity(t) = task->deadline - t - task->remaining(t);
         * but now(t)= now(t-1)+1,
         * and task->remaining(t)=  task->remaining(t-1), if task!=current,
         * ==> task->laxity(t) = task->laxity(t-1) -1;
         *********************************************************************/
        /* look out! task->laxity is decremented only if its state is READY, because of && */
        if( task->state ==READY  &&  -- task->laxity<0){ /* if it's eligible... */
            printf( "At %d: task %c (\"%s\"), instance %d, will lose its deadline at %d%s\n",
                sys_time, task->sys_id, task->name, task->instance, task->deadline, bell);
            task->state=BLOCKED;
        }
        if( (task->state ==READY  ||  task->state ==RUNNING)  &&  task->laxity < least->laxity)
                least=task;
    }
    return( least);
}

/* return the first READY or RUNNING task in the list */
task_t *first_ready( register list l)
{
    register node p;
    task_t *task;  /* this variable must be used to cast the void *v in 'p' */

    p = l->header->forward[0];
    /* <READY  means it is not IDLE, BLOCKED nor DEAD
     * ==> This sentence fails if the states are re-enum-bered <==
     *************************************************************/
    while( p!= NIL  &&  (task=p->v)->state <READY)
        p = p->forward[0];

    if( p==NIL) return( NULL);
    else return( p->v);
}


node first_node_of( list l){ return( l->header->forward[0]); }

int key_of( node n)
{
    int *key;

    key= (int * ) &(n->key) + 1;
    return( *key);
}


void show_task_l( list l)
{
    char state;
    task_t *task;
    node n;

    printf( "%c |", l->sys_id);
    for( n= l->header->forward[0]; n != NIL; n= n->forward[0]){
        printf( "%d('%c',", key_of( n), (task=(n->v))->sys_id);
        switch ( task->state){
            case DEAD:     state='d'; break;
            case IDLE:     state='i'; break;
            case BLOCKED:  state='b'; break;
            case READY:    state='r'; break;
            case RUNNING:  state='R'; break;
            default:       state='?'; break;
        }
        printf( "%c)--> ", state);
    }
    printf( "NIL\n\n");
}


/***************************************************************************/
        /**** configuration file related constants and data ****/

#define MAX_STRING 190
char tmp[ MAX_STRING+1]; /* buffer to read in lines from configuration file */

FILE *infile;
char test_name[ MAX_STRING+1];     /* its name */

/* Get input data from configuration file, and make some consistency checks on it */
char BLANKS[]= " \t\n";
int get_test_vector( char *fname)
{
    int i, ill_task;
    char *token;

    if( (infile = fopen( fname, "rt" )) == NULL ){
        fprintf( stderr, "Can't open configuration file %s\n", fname);
        exit(-1);
    }

        /* try to decide if it's the rigth file */
    if(  fgets( tmp, MAX_STRING-1, infile) != NULL  &&  strcmp( strupr(tmp), "CONFIGURATION FILE:\n") ){
        fprintf( stderr, "Please check input file '%s': it's probably wrong\n", fname);
        exit(-1);
    }

    /* from now on, I will assume input file is complete and has no structural errors */

    /* skip everything until 'start' keyword */
    lookup( "START", ":=");
    lookup( "TEST SET", ":=");
    token= strtok( NULL, "\n");
    while( *token==' ') token++;
    if( strlen( token) >MAX_NAME_LENGTH){
        fprintf( stderr, "Sorry: name[%s] longer than allowed (%d chars)\n", token, MAX_NAME_LENGTH);
        exit(-1);
    }
    strcpy( test_name, token);

    lookup( "MAXTIME", ":=");
    max_time= atoi( strtok( NULL, BLANKS));

    /* Unfortunately, we must explicity put the number of tasks, so
     * we can allocate them in a block.  This could change, as we
     * can allocate them on a per-task basis.
     *****************************************************************/
    lookup( "NUMBER OF APPLICATION TASKS", ":=");
    if( (num_tasks= atoi( strtok( NULL, BLANKS))) >24){
        fprintf( stderr, "Sorry: Not enough sys_id letters for all tasks");
        exit(-1);
    }else if( (task_set= malloc( (num_tasks+1) * sizeof( task_t))) ==NULL){
        fprintf( stderr, "Not enough memory available");
        exit(-1);
    }

        /* look out for the rigth order in the information */
    lookup( "APPLICATION TASKS DESCRIPTION", ":");
    /* Name Criticality Period Execution_time */
    lookup( "NAME", BLANKS);
    lookup( "CRITICALITY", BLANKS);
    lookup( "PERIOD", BLANKS);
    lookup( "EXECUTION_TIME", BLANKS);

        /* now fill in the user data structure from file information */
    ill_task=FALSE;
    /* the loop must be inverse in order to preserve the original task sequence */
    for( i=num_tasks; ill_task!=TRUE  &&  i>=1; i--){ /* our list library is LIFO */
        (task_set+i)->sys_id= sys_id++;
        (task_set+i)->state=IDLE;
        (task_set+i)->instance=0;
        (task_set+i)->cycles=0;

            /* get task's name */
        fgets( tmp, MAX_STRING-1, infile);
        token=strtok( tmp, ",");
        if( strlen( token) >MAX_NAME_LENGTH){
            fprintf( stderr, "Sorry: name[%s] longer than allowed (%d chars)\n", token, MAX_NAME_LENGTH);
            exit(-1);
        }
        strcpy( (task_set+i)->buffer, token);
        (task_set+i)->name = (task_set+i)->buffer;

            /* get task's criticality */
        token=strtok( NULL, " \t,");
        (task_set+i)->criticality= strcmp( strupr( token), "HIGH")?  LOW: HIGH;

        (task_set+i)->period= atoi( strtok( NULL, ","));      /* every line ends in '.' */
        (task_set+i)->cpu_time= (task_set+i)->remaining= atoi( strtok( NULL, "."));
        if( (task_set+i)->period <0
            || (task_set+i)->cpu_time <1
            || (task_set+i)->cpu_time > (task_set+i)->period)
                ill_task=TRUE;
    }
    if( ill_task==TRUE){
        fprintf( stderr, "Ill input task (number %d) in configuration file%s\n\n", i, bell);
        exit(-1);
    }

        /* skip everything until 'end.' keyword */
    lookup( "END", ".");
    fclose( infile);
    printf( "\nCONFIGURATION FILE: %s\n", fname);
    return( TRUE); /* if it could get this point, then all was OK */
}

/* returns everything ready for a call to strtok with NULL argument */
/* It's *case-insensitive*.  It could be coded more general, for reusability purposes */
void lookup( char *target, char *seps)
{
static char *token = NULL;

    do{
        if( (token==NULL)||((token=strtok( NULL, seps))==NULL) ){
            do
                fgets( tmp, MAX_STRING-1, infile);
            while( *tmp==';');
            token=strtok( tmp, seps);
        }
        token= strupr( token);
    }while( strcmp(token,target) && !feof( infile));
    if( feof( infile)){
        fprintf( stderr, "Unexpected EOF while looking for '%s'\n", target);
        exit(-1);
    }
}

void show_test_vector( void)
{
    task_t *task;
    int i, length;

    printf( "Task Set: %s\n", test_name);
    printf( "Number of tasks in the set: %d\n", num_tasks);

    puts( "\nTask Set Description:");
    puts(   "--------------------");

/*Name                  Criticality  Period  ExecTime  Task Load
 *Task A                     high        6        2      33.3%   */
    puts( "Name                  Criticality  Period  ExecTime  Task Load");
    for( i=num_tasks; i>=1; i--){
        length= strlen( (task=task_set+i)->name);
        if( length>22) length=22;
        strncpy( tmp, task->name, 22);
        memset( tmp+length, ' ', 22-length); tmp[ 22]='\0';

        printf( "%s   %6s    ", tmp, task->criticality ==HIGH? "high": "low");
        printf( "%5d   %6d    ", task->period, task->cpu_time);
        printf( "%6.1f%%\n", 100.0 * (float )task->cpu_time / (float )task->period);
    }
}
