/*
 *   _SKIP LISTS_
 * by Bruce Schneier, DrDobb's Journal, January '94
 * modified by Alberto Ferrari, 02/12/94
 ***************************************************************************/

/*   This file contains source code to implement a dictionary using
 * skip lists and a test driver to test the routines. A couple of comments
 * about this implementation: The routine randomLevel has been hard-coded to
 * generate random levels using p=0.25. It can be easily changed.
 * The insertion routine has been implemented so as to use the dirty hack
 * described in the CACM paper: if a random level is generated that is more
 * than the current maximum level, the current maximum level plus one is used
 * instead. Levels start at zero and go up to MaxLevel (which is equal
 * to (MaxNumberOfLevels-1).
 * The compile flag allowDuplicates determines whether or not duplicates
 * are allowed. If defined, duplicates are allowed and act in a FIFO manner.
 * If not defined, an insertion of a value already in the file updates the
 * previously existing binding. BitsInRandom is defined to be the number
 * of bits returned by a call to random(). For most all machines with 32-bit
 * integers, this is 31 bits as currently set. The routines defined in this
 * file are:
 *      init: defines NIL and initializes the random bit source
 *      newList: returns a new, empty SkipList
 *      freeList(l): deallocates the SkipList l (along with any elements in l)
 *      randomLevel: Returns a random level
 *      insert(l,key,value): inserts the binding (key,value) into l. If
 *           allowDuplicates is undefined, returns true if key was newly
 *           inserted into the SkipList, false if key already existed
 *      delete(l,key): deletes any binding of key from the l. Returns
 *           false if key was not defined.
 *      search(l,key,&value): Searches for key in l and returns true if found.
 *           If found, the value associated with key is stored in the
 *           location pointed to by &value
 ****************************************************************************/

/*
 *      Changes made Alberto - 02/10/1994:
 *           1) random() was #define-d as rand()
 *           2) BitsInRandom was adjusted to 15 (originally 31)
 *           3) sampleSize was adjusted to 1000 (originally 65536),
 *                due to PC unsufficient memory
 *           4) newNodeOfLevel was moved to a function, and the value
 *                returned by malloc() verified
 *           5) randomsLeft and randomBits moved into SkipList structure, to let
 *                the library be used for more than one SkipList at the same time
 *           6) modifications of style (indentation, etc)
 *
 *   I think, when duplicates are allowed, they act in a LIFO manner!
 *
 *
 *      Changes made Hans - 06/14/2011:
 *           1) BitsInRandom changed back to 31
 *           2) Cleared skipl.h
 *           3) Renamed API with Skipl prefix including constants and data
 *              structures to avoid collision
 *           4) Initialization is automatic (by first call to SkiplNew)
 *
 ***************************************************************************/
#define SKIPL_SOURCE
/* #define DEBUG */
#ifdef __MINGW32__
#define MAXINT __INT_MAX__
#define MAXLONG __INT_MAX__
#endif

#include <stdio.h>  /* for stderr */
#include <stdlib.h> /* for rand() and malloc() */

#if defined (SKIPL_TEST)
  #include <string.h> /* for memset */
  #if defined (MSC)
     #include <malloc.h> /* for _heapchk() */
  #endif
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "skipl.h"


struct SkiplNodeStructure{
    SkiplKeyType key;
    SkiplValueType v;
    #ifdef SKIPL_TEST
    int level;
    #endif
    SkiplNode forward[1]; /* variable sized array of forward pointers */
};

struct SkipListStructure {
    int level;    /* Maximum level of the SkipList
             * (1 more than the number of levels in the SkipList) */
    int randomsLeft;
    int randomBits;

    struct SkiplNodeStructure * header; /* pointer to header */
};

static SkiplNode NIL;

/* random function used */
#define random rand
/* number of bits of value returned by random */
#define BitsInRandom     31

/* #define allowDuplicates */ /* NO DUPLICATES allowed */

#define false 0
#define true 1

#define MaxNumberOfLevels 16
#define MaxLevel (MaxNumberOfLevels-1)


/* private functions' prototypes */
static SkiplNode newNodeOfLevel(int level);
static int randomLevel(SkipList l);
#ifdef SKIPL_TEST
static void show_skipl(SkipList l);
#ifdef MSC
static void heapstat(int status);
#endif    /* MSC */
#endif    /* SKIPL_TEST */

#ifdef DEBUG
static void free_list(SkipList l){
    l->header=NULL;
    free(l);
}

static void free_node(SkiplNode q){
    q->key=0;
    #ifdef SKIPL_TEST
    q->v=0;
    #else
    q->v=NULL;
    #endif    /* SKIPL_TEST */
    free(q);
}

#else     /* DEBUG */
#define free_list(l) free(l)
#define free_node(q) free(q)
#endif    /* DEBUG */


static SkiplNode NIL; /* NIL is unique among all lists created */
static int initialized = 0;

void SkiplInit(void)
{
    NIL = newNodeOfLevel(0);
    NIL->key = SKIPL_MAXKEY;
    NIL->forward[0]=NIL;
    initialized = 1;
}

static SkiplNode newNodeOfLevel(int level)
{
    SkiplNode n;

    if((n=malloc(sizeof(struct SkiplNodeStructure) + level*sizeof(SkiplNode ) ))==NULL){
        fprintf(stderr, "Insufficient memory available");
        exit(-1);
    }else{
        #ifdef SKIPL_TEST
        n->level=level;
        #endif
        return(n);
    }
}


SkipList SkiplNew(void)
{
    SkipList l;
    int i;

    if(!initialized ) SkiplInit();
    if((l = (SkipList)malloc(sizeof(struct SkipListStructure)) )==NULL) {
        return NULL;
    }
    l->level = 0;
    l->header = newNodeOfLevel(MaxNumberOfLevels);
    l->randomBits = random();
    l->randomsLeft = BitsInRandom/2;
    for(i=0; i<MaxNumberOfLevels; i++)
        l->header->forward[i] = NIL;
    return(l);
}


void SkiplFree(SkipList l)
{
    SkiplNode p,q;
    p = l->header;
    do{
        q = p->forward[0];
        free_node(p);
        p = q;
    }while (p!=NIL);
    free_list(l);
}


static int randomLevel(SkipList l)
{
    int level = 0;
    int b;

    /* values hardcoded for p=0.25 */
    do {
        b = (l->randomBits)&3;     /* &1 for p=0.5 */
        if (!b) level++;
        l->randomBits >>= 2;     /* >>= 1 for p=0.5 */
        if (-- (l->randomsLeft) == 0) {
            l->randomBits = random();
            l->randomsLeft = BitsInRandom/2;
        };
    } while (!b);

    return(level>MaxLevel ? MaxLevel : level);
}



int SkiplInsert(SkipList l, SkiplKeyType key, SkiplValueType value)
{
    int k;
    SkiplNode update[MaxNumberOfLevels];
    SkiplNode p,q;

    p = l->header;
    k = l->level;
    do{
        while (q = p->forward[k], q->key < key)
            p = q;
        update[k] = p;
    } while(--k>=0);

#ifndef allowDuplicates
    if (q->key == key) {
        q->v = value;
        return(false);
    }
#endif

    k = randomLevel(l);
    if(k> l->level){
        k = ++ (l->level);
        update[k] = l->header;
    }

    q = newNodeOfLevel(k);
    q->key = key;
    q->v = value;
    do{
        p = update[k];
        q->forward[k] = p->forward[k];
        p->forward[k] = q;
    }while(--k>=0);

#ifndef allowDuplicates
    return(true);
#endif
}


int SkiplDelete(SkipList l, SkiplKeyType key)
{
    int k,m;
    SkiplNode update[ MaxNumberOfLevels];
    SkiplNode p, q;

    p = l->header;
    k = m = l->level;
    do {
        /* if the node ahead of p has lower key, advance p */
        while (q = p->forward[k], q->key < key) p = q;
        update[k] = p;
    } while(--k>=0);

    if(q->key == key) {
        for(k=0; k<=m && (p=update[k])->forward[k] == q; k++)
            p->forward[k] = q->forward[k];
        free_node(q);
        while(l->header->forward[m] == NIL && m > 0 )
                 m--;

        l->level = m;
        return(true);

    } else {
        return(false);
    }
}


int SkiplSearch(SkipList l, SkiplKeyType key, SkiplValueType *valuePointer)
{
    int k;
    SkiplNode p,q;

    p = l->header;
    k = l->level;
    do {
        while (q = p->forward[k], q->key < key) {
            p = q;
        }
    } while (--k>=0);

    if (q->key != key) {
        return(false);
    } else {
        *valuePointer = q->v;
        return(true);
    }
}

SkiplNode SkiplHead(SkipList l) {
    SkiplNode n = l->header->forward[0];
    if( n == NIL ) return NULL;
    return n;
}

SkiplNode SkiplNext(SkiplNode n) {
    n = n->forward[0];
    if( n == NIL ) return NULL;
    return n;
}

SkiplValueType SkiplValue(SkiplNode n) {
    return n->v;
}

int SkiplIsEmpty(SkipList l) {
    return l->header->forward[0] == NIL;
}


SkiplValueType SkiplGetValue(SkiplNode n) {
    return n->v;
}

SkiplKeyType SkiplGetKey(SkiplNode n) {
    return n->key;
}


#ifdef SKIPL_TEST
#define sampleSize 1000
SkiplKeyType keys[sampleSize];

static void show_skipl(SkipList l)
{
    SkiplNode n;
    #ifdef SKIPL_TEST
    int count[ MaxNumberOfLevels];
    int i, total=0;

    memset(&count, 0, MaxNumberOfLevels* sizeof(int));
    for(n= l->header->forward[0]; n != NIL; n= n->forward[0])
        count[ n->level]++;
    puts("Node Level Count:");
    for(i=0; i< MaxNumberOfLevels; i++){
        printf("\t% 3d\t% 4d\t(% 7.2f%%)\n",
            i, count[i], 100.0 * (float )count[i] / (float )sampleSize);
        total+=count[i];
    }
    printf("\n");
    printf("Total: %d/% 7.2f%%\t\n",
            total, 100.0 * (float )total / (float )sampleSize);
    #endif

    for(n= l->header->forward[0]; n != NIL; n= n->forward[0])
        printf("%d('%d')--> ", n->key, n->v);
    printf("NIL\n\n");
}

int main(void) {
    SkipList l;
    int i,k;
    SkiplValueType v;

    SkiplInit();
    l = SkiplNew();

    #ifdef MSC
    heapstat(_heapchk());
    #endif

    printf("Building enter #\n");
    for(k=0; k<sampleSize; k++){
        keys[ k]=random();
        SkiplInsert(l, keys[k], keys[k]);
        printf("\r% 3d", k);
    }
    printf("\n\n");

    show_skipl(l);
    puts("Computing tests...");
    for(i=0; i<4; i++) {
        printf("Pass #%d\n", i);
        #ifdef MSC
        heapstat(_heapchk() );
        #endif

        for(k=0; k<sampleSize; k++){
            if(!SkiplSearch(l, keys[k], &v))
                printf("error in search #%d,#%d\n",i,k);
            if (v != keys[k])
                printf("search returned wrong value\n");
        }

        for(k=0; k<sampleSize; k++){
            if(!SkiplDelete(l, keys[k]))
                printf("error in delete\n");
            else{
                keys[k] = random();
                SkiplInsert(l,keys[k],keys[k]);
            }
        }
    }

    #ifdef MSC
    heapstat(_heapchk());
    #endif

    puts("Exiting...");
    SkiplFree(l);

    #ifdef MSC
    if(_heapchk() == _HEAPOK)
    #endif

     puts("Tests completed successfully");
    return 0;
}

#ifdef MSC
/* Reports on the status returned by _heapwalk, _heapset, or _heapchk */
static void heapstat(int status )
{
    printf("\nHeap status: " );
    switch(status )
    {
       case _HEAPOK:
          printf("OK - heap is fine" );
          break;
       case _HEAPEMPTY:
          printf("OK - empty heap" );
          break;
       case _HEAPEND:
          printf("OK - end of heap" );
          break;
       case _HEAPBADPTR:
          fprintf(stderr, "ERROR - bad pointer to heap" );
          break;
       case _HEAPBADBEGIN:
          fprintf(stderr, "ERROR - bad start of heap" );
          break;
       case _HEAPBADNODE:
          fprintf(stderr, "ERROR - bad node in heap" );
          break;
    }
    printf("\n\n" );
}
#endif /* MSC */

#endif /* SKIPL_TEST */
