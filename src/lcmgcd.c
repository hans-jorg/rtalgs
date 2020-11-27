
/*
 * Routines to calculate least common multiple (lcm) and 
 *    the greatest common divider (gcd) of two integer and
 *    the lcd of many integers.
 *
 * Routines:
 *   int gcd(a,b) returns the gcd using Euclydean algorithm
 *
 *   int lcd(a,b) returns the lcm using the property a*b=lcm*gcd
 *
 *   void lcm_gcd(int a, int b, int *plcm, *pgcd) calculates both
 *                but avoids to duplicate the gcd calculation
 *
 *   int lcmv1(int v[], int n) calculates the lcm of the n integers 
 *                in v using gcd of pair of numbers (FASTER!!!)
 *
 *   int lcmv2(int v[], int n) calculates the lcm of the n integers
 *                in v using an algorithm that add the original number
 *                to the smallest number in v
 *
 *   lcmv is an alias to lcmv1
 * 
 *   Author: Hans (22/06/2011)
 *
 */

// use non standard alloca
#define USE_ALLOCA 1
#ifdef __MINGW32__
#define MAXINT __INT_MAX__
#define MAXLONG __INT_MAX__
#undef USE_ALLOCA
#endif


#include<stdlib.h>
#ifdef DEBUG
#include<stdio.h>
#endif
#ifdef DMALLOC
#include "dmalloc.h"
//#undef USE_ALLOCA
#endif
#ifdef USE_ALLOCA
#include<alloca.h>
#endif

#include "lcmgcd.h"

int gcd(int a, int b)  {
int c;

	if( a < 0 ) a = -a;
	if( b < 0 ) b = -b;

	if( a < b ) { c=a; a=b; b=c; }

	if( b == 0 ) return 0;

	while( (c=a%b)!=0 ) {
		a=b;
		b=c;
	}
	return b;
}

int lcm(int a, int b) {
int c;

	c = gcd(a,b);
	if( c )
		return a*b/c;
	else
		return 0;
}

void lcm_gcd( int a, int b, int *l, int *g) {
	*l = *g = 0;
	int c = gcd(a,b);
	*g = c;
	if( c )
		*l = a*b/c;
}

int lcmv1(int *v, int n) {
#ifdef USE_ALLOCA
int *t = alloca(n*sizeof(int));
#else
int *t = calloc(n,sizeof(int));
#endif
int i,j,k,x;

	if( t == NULL ) {
		return 0;
	}
	for(j=0;j<n;j++) t[j] = v[j];
	j = 0;
	k = n;
	while(k>1) {
#ifdef DEBUG
        for(i=0;i<n;i++) printf("%d ",t[i]); putchar('\n');
#endif
		j = 0;
		i = 0;
		while(i<k-1) {
			t[j++]=lcm(t[i],t[i+1]);
			i += 2;
		}
		if( i != k )
			t[j++]=t[i];
		k = j;
	}
	x = t[0];
#ifndef USE_ALLOCA
	free(t);
#endif
	return x;
}

int lcmv2(int *v, int n) {
#ifdef USE_ALLOCA
int *t = alloca(n*sizeof(int));
#else
int *t = calloc(n,sizeof(int));
#endif
int j,k,min,cnt;
#ifdef DEBUG
int i;
#endif

	if( t == NULL ) {
		return 0;
	}
	for(j=0;j<n;j++) t[j] = v[j];

	while(1) {
		cnt = 1;
		min = t[0];
		k = 0;
		j = 1;
		/*find minimum value and count how many are equal to min */
		while(j<n) {
			if( t[j] < min ) {
				min = t[j];
				k = j;
				cnt = 1;
			} else if ( t[j] == min ) {
				 cnt++;
			}
			j++;
		}
#ifdef DEBUG        
        for(i=0;i<n;i++) printf("%d ",t[i]);putchar('\n');
#endif

		if( cnt == n ) 
			break;
		t[k] = t[k] + v[k];	
	};
	min = t[0];
#ifndef USE_ALLOCA
	free(t);
#endif
	return min;
}

int lcmv(int *v, int n) {
    return lcmv1(v,n);
}

	
#if defined(TEST) || defined(TESTV)
#include<stdio.h>

#ifdef TEST
int main(int argc,char *argv[]) {
int a,b,l,g;
char line[255];

	while( !feof(stdin) ) {
		fgets(line,254,stdin);
		int rc = sscanf(line,"%d %d",&a,&b);	
		if( rc == 0 )
			break;
		if( rc != 2 )
			continue;
		l = lcm(a,b);
		g = gcd(a,b);
		printf("gdc(%1d,%1d)=%d  lcm(%d,%d)=%d\n",a,b,g,a,b,l);
	}
	return 0;
}
#else
#include <string.h>
#include <time.h>

#ifndef NTIMES
#define NTIMES 1
#endif

int main(int argc,char *argv[]) {
char line[255];
int v[127];
int n,x;
char *p;
clock_t ti,tf;
int i;

	while( !feof(stdin) ) {
		fgets(line,254,stdin);
		p = strtok(line," \t");
        n = 0;
        printf("lcm of ");
		while(p) {
			x = atoi(p);
			if( x ) {
				v[n++] = x;
                printf("%d ",x);
            }
			p = strtok(NULL," \t");
		}
		printf(" is %d",lcmv1(v,n));
		printf(" (%d)\n",lcmv2(v,n));
 
        printf("NTIMES=%d\n",NTIMES);
        ti = clock();
        for(i=0;i<NTIMES;i++) x = lcmv1(v,n);
        tf = clock();
        printf("tempo v1 = %f\n",(((double) (tf-ti))/CLOCKS_PER_SEC)/1000.0);
        ti = clock();
        for(i=0;i<NTIMES;i++) x = lcmv2(v,n);
        tf = clock();
        printf("tempo v2 = %f\n",(((double) (tf-ti))/CLOCKS_PER_SEC)/1000.0);
	}
	return 0;
}

#endif
#endif


