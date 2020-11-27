/*
 *                      RTALGS.H
 * Function Prototypes
 *****************************************************************************/
#define MAX_NAME_LENGTH 50

#define TRUE 1
#define FALSE 0

typedef int time_t;
#define MAX_VALUE 0x7fff

typedef struct task_struct task_t;
struct task_struct{
    char sys_id;
    char *name;
    enum state_e state;
    enum criticality_e criticality;
    time_t period,
        cpu_time,
        remaining,
        deadline, /* relative to instance start time */
        laxity,
        instance, /* current instance number */
        cycles;   /* number of instances executed so far */
    char buffer[ MAX_NAME_LENGTH+1];     /* to hold its name */
    time_t *merit;
};


void main( int argc, char *argv[]);
void init( int argc, char *argv[]);
void draw_timeline( void);
time_t now( void);
void task_init( task_t *task);

task_t *update_laxity_and_get_least( list l);

task_t *first_ready( register list l);
node first_node_of( list l);
int key_of( node n);
void show_task_l( list l);

int get_test_vector( char *fname);
void lookup( char *target, char *seps);
void show_test_vector( void);
/*****************************************************************************/
                    /* Interface to Skipl Library */

long new_key;                  /* let's make a new key for argument to the library:     */
char *id= (char *)&new_key;          /* its two MSBytes will hold the original argument */
int  *high_key= (int * )&new_key+1;  /* and the LSByte the sys_id of the task arguement */
                               /* This way we have a unique key for search and delete   */

void insert_task( list task_l, int key, task_t *task){
    *id= task->sys_id;  /* compose 'new_key' value */
    *high_key= key;
    insert( task_l, new_key, task);
}

void delete_task( list task_l, int key, task_t *task){
    *id= task->sys_id;  /* compose 'new_key' value */
    *high_key= key;
    delete( task_l, new_key);
}
/*****************************************************************************/



/* Scheduling Algoritmhs' function prototypes
 ********************************************/
void    (*sched_alg_init) ( void);
task_t *(*sched_alg)      ( void);
void    (*sched_alg_end)  ( void);
task_t *default_dispatcher( void);

void monotonic_rate_init( void);
task_t *monotonic_rate( void);
void monotonic_rate_end( void);

void earliest_deadline_init( void);
task_t *earliest_deadline( void);
void earliest_deadline_end( void);

void least_laxity_init( void);
task_t *least_laxity( void);
void least_laxity_end( void);

void maximum_urgency_first_init( void);
task_t *maximum_urgency_first( void);
void maximum_urgency_first_end( void);
