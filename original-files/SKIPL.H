/*
 *                  SKIPL.H
 * Skip list function declarations
 *****************************************************************************/

#ifdef SKIPL_TEST
typedef int keyType;
#define NIL_FOOTPRINT    0x7fff
typedef int valueType;

#else     /* when the library is applied to a project... */
typedef long keyType;
#define NIL_FOOTPRINT    0x7fffffff
typedef void *valueType;
#define init skipl_library_init
#endif

typedef char boolean;

typedef struct nodeStructure *node;
typedef struct nodeStructure{
	keyType key;
	valueType v;
	#ifdef SKIPL_TEST
	int level;
	#endif
	node forward[1]; /* variable sized array of forward pointers */
};

typedef struct listStructure  *list;
typedef struct listStructure{
	char sys_id;
	int level;	/* Maximum level of the list
			 * (1 more than the number of levels in the list) */
	int randomsLeft;
	int randomBits;

	struct nodeStructure * header; /* pointer to header */
};

extern node NIL;

/* Public Interface Library Functions' Prototypes */
void init( void);
list newList( void);
void freeList( list l);

#ifdef allowDuplicates
void
#else
boolean
#endif
insert( register list l, register keyType key, register valueType value);

boolean delete( register list l, register keyType key);
boolean search( register list l, register keyType key, valueType *valuePointer);

#ifndef SKIPL_SOURCE
#undef init
#endif

