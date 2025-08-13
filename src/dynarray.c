#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dynarray.h"

dynarray create_dynarray(dynarray_freef fef, dynarray_printf pef){
  dynarray new = malloc(sizeof(struct dynarray));
  assert(new != NULL);
  new->cap  = 10;
  new->len  = 0;
  new->fef  = fef;
  new->pef  = pef; 
  new->data = malloc(new->cap * sizeof(DA_ELEMENT));
  assert(new->data != NULL);
  return new; 
}

void free_dynarray(dynarray d){
  if (d->fef != NULL){
    for (int i = 0; i < d->len; i++){
      d->fef(d->data[i]);
    }
  }
  free(d->data);
  free(d);
}

static void grow(dynarray d){
  if (d->len + 1 >= d->cap){
    d->cap  = d->cap * 2; 
    d->data = realloc(d->data, d->cap * sizeof(DA_ELEMENT));
    assert(d->data != NULL);
  }
}

void add_dynarray(dynarray d, DA_ELEMENT element){
  grow(d);
  d->data[d->len++] = element;
}

// usage -> remove_dynarray(d, 0) # removes first element from d->data
void remove_dynarray(dynarray d, int pos){
  assert(pos < d->len);

  if (d->fef != NULL){
    d->fef(d->data[pos]);
  }

  for (int i = pos; i < d->len - 1; i++){
    d->data[i] = d->data[i+1];
  }  
  d->len--;
  d->data[d->len] = NULL;
}

void print_dynarray(dynarray d, FILE *out){
  assert(d->pef != NULL);
  fputs("[ ", out);
  for(int i = 0; i < d->len; i++){
    if (i > 0){
      fputc(',', out);
    }
    d->pef(out, d->data[i], i);
  }
  fputs(" ]", out);
}