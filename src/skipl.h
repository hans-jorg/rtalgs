/******************************************************************************
 *                  SKIPL.H
 * Skip list interface (functions and constant declarations)
 *****************************************************************************/
#ifndef SKIPL_H
#define SKIPL_H

#ifndef SKIPL_TEST   /* when the library is used in a project... */
  #include<values.h> /* for MAXLONG */
  typedef long SkiplKeyType;
  #define SKIPL_MAXKEY    MAXLONG
  typedef void *SkiplValueType;
#else   /* for testing */
  #include <values.h> /* for MAXINT */
  typedef int SkiplKeyType;
  #define SKIPL_MAXKEY    MAXINT
  typedef int SkiplValueType;
#endif

typedef struct SkipListStructure  *SkipList;
typedef struct SkiplNodeStructure *SkiplNode;

void SkiplInit(void);
SkipList SkiplNew(void);
void SkiplFree(SkipList l);
int  SkiplInsert(SkipList l, SkiplKeyType key, SkiplValueType value);
int  SkiplDelete(SkipList l, SkiplKeyType key);
int  SkiplSearch(SkipList l, SkiplKeyType key, SkiplValueType *valuePointer);

SkiplNode SkiplHead(SkipList l);
SkiplNode SkiplNext(SkiplNode n);
int SkiplIsEmpty(SkipList l);

SkiplValueType SkiplGetValue(SkiplNode n);
SkiplKeyType SkiplGetKey(SkiplNode n);

#endif /* SKIPL_H */
