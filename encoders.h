#ifndef ENCODERS
#define ENCODERS

#include "literal.h"
#include "lz.h"

//Convenient enum
enum formats {
        FT_LITERAL,
        FT_LZ
};
//Text of the label specifying format
char *formatstrs[] =
    { "Literal", "LZ" };
//Function that initializes format based on specfile
void (*inits[])() =
    { lit_Init, lz_Init };
//Returns how many bytes encoding count bytes would produce,
//at offset within input,
//but edits count to be amount of bytes actually encoded,
//if less than the number passed to function
long (*querys[])(char *input, long offset, long *count) =
    { lit_Query, lz_Query };
//Encodes count bytes at output from input,
//and advances both streams
//Returns however many bytes couldn't be encoded
long (*encodes[])(char *input, long offset, char **output, long count) =
    { lit_Encode, lz_Encode };

#endif
