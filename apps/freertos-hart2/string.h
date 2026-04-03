#ifndef APP2_STRING_H
#define APP2_STRING_H

#include <stddef.h>

void * memcpy( void * dest, const void * src, size_t n );
void * memset( void * dest, int value, size_t n );
int memcmp( const void * lhs, const void * rhs, size_t n );
size_t strlen( const char * s );
char * strcpy( char * dest, const char * src );

#endif
