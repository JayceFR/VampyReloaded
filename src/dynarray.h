#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <stdio.h>

// Macros and Typedefs 
#define DA_ELEMENT void *
typedef void (*dynarray_printf)( FILE *, DA_ELEMENT, int );
typedef void (*dynarray_freef)( DA_ELEMENT );

// ADT
struct dynarray{
  DA_ELEMENT *data;
  int cap; 
  int len; 
  dynarray_freef fef;
  dynarray_printf pef; 
};
typedef struct dynarray *dynarray;

extern dynarray create_dynarray(dynarray_freef, dynarray_printf);
extern void free_dynarray(dynarray);
extern void add_dynarray(dynarray, DA_ELEMENT);
extern void remove_dynarray(dynarray, int);
extern void print_dynarray(dynarray, FILE*);

#endif