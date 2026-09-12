#ifndef STRING_H
#define STRING_H

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, int n);
void strcpy(char *dst, const char *src);
void strncpy(char *dst, const char *src, int n);
int strlen(const char *s);

#endif
