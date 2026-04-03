#include <stddef.h>
#include <string.h>

void * memcpy( void * dest, const void * src, size_t n )
{
    unsigned char * d = ( unsigned char * ) dest;
    const unsigned char * s = ( const unsigned char * ) src;

    while( n-- != 0U )
    {
        *d++ = *s++;
    }

    return dest;
}

void * memset( void * dest, int value, size_t n )
{
    unsigned char * d = ( unsigned char * ) dest;

    while( n-- != 0U )
    {
        *d++ = ( unsigned char ) value;
    }

    return dest;
}

int memcmp( const void * lhs, const void * rhs, size_t n )
{
    const unsigned char * a = ( const unsigned char * ) lhs;
    const unsigned char * b = ( const unsigned char * ) rhs;

    while( n-- != 0U )
    {
        if( *a != *b )
        {
            return ( int ) *a - ( int ) *b;
        }

        a++;
        b++;
    }

    return 0;
}

size_t strlen( const char * s )
{
    size_t len = 0U;

    while( s[ len ] != '\0' )
    {
        len++;
    }

    return len;
}

char * strcpy( char * dest, const char * src )
{
    char * out = dest;

    while( *src != '\0' )
    {
        *dest++ = *src++;
    }

    *dest = '\0';
    return out;
}
