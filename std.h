#ifndef INCLUDE_std_h
#define INCLUDE_std_h

#ifdef USE_STANDARD_HEADERS

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#else

/* <assert.h> */
void assert(int cond);

/* <ctype.h> */
int isspace(int c);
int isdigit(int c);
int isalpha(int c);
int isalnum(int c);

/* <errno.h> */
#define ERANGE 34

#ifdef __APPLE__
#define errno (*__error())
int *__error(void);
#endif

#ifdef __MINGW64__
#define errno (*_errno())
int *_errno(void);
#endif

/* <limits.h> */
#define INT_MAX 2147483647

/* <stdbool.h> */
#define true 1
#define false 0

typedef int bool;

/* <stddef.h> */
#define NULL ((void *)0)

typedef unsigned long size_t;
typedef long intptr_t;

/* <stdio.h> */
typedef struct FILE FILE;

#ifdef __APPLE__
#define stderr __stderrp
extern FILE *__stderrp;
#endif

#ifdef __MINGW64__
#define stderr (__acrt_iob_func(2))
FILE *__acrt_iob_func(int a);
#endif

#define SEEK_SET 0
#define SEEK_END 2

int printf(const char *format, ...);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);
int fseek(FILE *fp, long offset, int whence);
long ftell(FILE *fp);
size_t fread(void *ptr, size_t size, size_t nitems, FILE *fp);
int fprintf(FILE *fp, const char *format, ...);

/* <stdlib.h> */
void exit(int code);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
long strtol(const char *str, char **end, int base);

/* <string.h> */
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
char *strncpy(char *dest, const char *src, size_t size);

#endif

#endif
