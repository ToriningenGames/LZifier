#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "scanner.h"

struct word {
        char constant;
        int bitcount;
        int alignment;
        int offset;
};
struct bit {
        int constantVal;
        char wordType;
        int bitNo;
};


//Module for lz
static int sigLength = -1;

enum wordtype {WT_LENGTH, WT_BACKREF, WT_END};

static struct word wordlist[] = {
        {
                .constant = 'l',
                .bitcount  = 0,
                .alignment = 0,
                .offset    = 0
        },
        {
                .constant = 'b',
                .bitcount  = 0,
                .alignment = 0,
                .offset    = 0
        },
        {
                .constant = '\0'
        }
};

static struct bit *bitlist = NULL;

bool lz_Spec()
{
        if (isconstnext())
                return false;
        char signature = getbit();
 match : ;
        //Find this in the wordlist
        int i = 0;
        for (; wordlist[i].constant != signature; i++) {
                if (wordlist[i].constant == '\0') {
                        die(-1, "No specifier for literal matching '%c'\n", signature);
                };
        }
        wordlist[i].bitcount = getnum();
        while (!isconstnext()) {
                switch (signature = getbit()) {
                    case '-' :
                        wordlist[i].offset = - getnum();
                        break;
                    case '+' :
                        wordlist[i].offset = getnum();
                        break;
                    case '*' :
                        wordlist[i].alignment = getnum() - 1;
                        break;
                    default  :
         goto match;
                }
        }
        return true;
}

void lz_Init()
{
        //Already identified correctly; specifier awaits us!
        //Set all specifiers
        while (lz_Spec())
                ;
        //Bytecount is next
        sigLength = getnum();
        //A check would go here to make sure the sig is long enough
        //to hold all the bits with the given alignment
        
        //Fill in what every bit needs to be
        int bitcount = sigLength * 8;
        bitlist = malloc(sizeof(*bitlist) * bitcount);
        for (int i = 0; i < bitcount; i++) {
                if (isconstnext()) {
                        //Constant
                        bitlist[i].constantVal = getconst();
                } else {
                        //Bit specifier
                        bitlist[i].constantVal = -1;
                        bitlist[i].wordType = getbit();
                        bitlist[i].bitNo = getnum();
                }
        }
}

//Hack to use Query to get the data to encode
static long queryDistance;

long lz_Query(char *input, long offset, long *count) {
        //Perform a maximal search, and edit count appropriately
        if (!*count)
                return 0;
        char *source = input + offset;
        if (*count < wordlist[WT_LENGTH].offset)
                return 0;       //We can't encode this little
        if (offset < wordlist[WT_BACKREF].offset)
                return 0;       //We can't encode this soon
        //Get a pointer the farthest possible backreference can make in the given space
        long distance = wordlist[WT_BACKREF].offset;
        distance += ((1L << (wordlist[WT_BACKREF].bitcount)) - 1);
        distance &= ~((1L << (wordlist[WT_BACKREF].alignment)) - 1);
        if (offset - distance > 0)
                input += offset - distance;
        //Keep running track of how long the best match is
        //Compare at each position, to fill its spot
                //Don't use strcmp or strncmp. Zeros are valid input
        //Modify count to contain length of longest match
                //Consider alignment and offset
        long length = 0;
        long max_length = (1L << wordlist[WT_LENGTH].bitcount) - 1;
        while (input < source) {
                for (long i = 0; source[i] == input[i]; i++) {
                        if (i > length) {
                                length = i & ~((1L << (wordlist[WT_LENGTH].alignment)) - 1);
                                queryDistance = source - input;
                        };
                        //Don't exceed maximum run length
                        if (length > max_length) {
                                length = max_length;
                                break;
                        }
                }
                input += (1L << (wordlist[WT_BACKREF].alignment));
        }
        length++;       //Indicies are zero-based
        *count = length;
        //Since LZ is only the size of its headers, we simply return bytes of headers
        //We still report valid if count becomes something stupidly small
        return sigLength;
}

long lz_Header(char **output, long count, long distance)
{
        unsigned char outchar = 0;
        int bitcount = sigLength * 8;
        long byteCount = 0;
        //Lz specific values
        long length = 0;
        long backref = 0;
        //Find the value of these... values
        //In order: offset, bitwidth, alignment
        //Get the word data
        count -= wordlist[WT_LENGTH].offset;
        if (count < 0)
                count = 0;
        length = count & ((1L << (wordlist[WT_LENGTH].bitcount)) - 1);
        length &= ~((1L << (wordlist[WT_LENGTH].alignment)) - 1);
        distance -= wordlist[WT_BACKREF].offset;
        if (distance < 0)
                distance = 0;
        backref = distance & ((1L << (wordlist[WT_BACKREF].bitcount)) - 1);
        backref &= ~((1L << (wordlist[WT_BACKREF].alignment)) - 1);
        byteCount = length + wordlist[WT_LENGTH].offset;
        for (int i = 0; i < bitcount; i++) {
                outchar <<= 1;
                if (bitlist[i].constantVal != -1) {
                        outchar |= bitlist[i].constantVal;
                } else {
                        //find this word in the wordlist
                        //lz has 'l' and 'b'
                        switch (bitlist[i].wordType) {
                            case 'l' :
                                outchar |= !!(length & (1L << bitlist[i].bitNo));
                                break;
                            case 'b' :
                                outchar |= !!(backref & (1L << bitlist[i].bitNo));
                                break;
                        }
                }
                if (i % 8 == 7) {
                        **output = outchar;
                        (*output)++;
                        outchar = 0;
                };
        }
        return byteCount;
}

long lz_Encode(char *input, long offset, char **output, long count)
{
        //This one assumes a lot.
        if (count < wordlist[WT_LENGTH].offset)
                return 0;       //We can't encode this little
        if (offset < wordlist[WT_BACKREF].offset)
                return 0;       //We can't encode this soon
        lz_Query(input, offset, &count);
        return count - lz_Header(output, count, queryDistance);
}
