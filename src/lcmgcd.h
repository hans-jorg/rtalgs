#ifndef LCM_GCD_H
#define LCM_GCD_H
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


int gcd(int a, int b);
int lcm(int a, int b);
void lcm_gcd(int a, int b, int *l, int *g);
int lcmv1(int *v, int n);
int lcmv2(int *v, int n);
int lcmv(int *v, int n);


#endif

