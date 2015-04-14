/*
 * driver.h - Various definitions for the Performance Lab.
 * 
 * DO NOT MODIFY ANYTHING IN THIS FILE
 */
#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdlib.h>

#define RIDX(i,j,n) ((i)*(n)+(j))

typedef struct {
  char *id;
  char *name, *email;
} student_t;

extern student_t studetn;

typedef struct {
   unsigned short red;
   unsigned short green;
   unsigned short blue;
} pixel;

typedef void (*lab_test_func) (int, pixel*, pixel*);

void rotate(int, pixel *, pixel *);

void register_rotate_functions(void);
void add_rotate_function(lab_test_func, char*);

#endif /* _DEFS_H_ */

