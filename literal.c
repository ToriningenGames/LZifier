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


//Module for literals
static int sigLength = -1;

enum wordtype {WT_LENGTH, WT_END};

static struct word wordlist[] = {
        {
                .constant = 'l',
                .bitcount  = 0,
                .alignment = 0,
                .offset    = 0
        },
        {
                .constant = '\0'
        }
};

static struct bit *bitlist = NULL;

bool lit_Spec()
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

void lit_Init()
{
        //Already identified correctly; specifier awaits us!
        //Set all specifiers
        while (lit_Spec())
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

long lit_Query(char *input, long offset, long *count) {
        (void)input;    //Output size independent of input
        (void)offset;
        if (!*count)
                return 0;
        //Get the maximal amount of bytes in a literal string
        long litStrSize = (1L << (wordlist[WT_LENGTH].bitcount)) - 1 + wordlist[WT_LENGTH].offset;
        if (litStrSize < *count)
                *count = litStrSize;
        //There is a minimum size requirement
        if (*count < wordlist[WT_LENGTH].offset)
                *count = wordlist[WT_LENGTH].offset;
        //Number of bytes that would be output
        long outsize = *count + sigLength;
        //Pad output by offset (since the output may require it)
        outsize += wordlist[WT_LENGTH].offset;
        return outsize;
}

long lit_Header(char **output, long count)
{
        unsigned char outchar = 0;
        int bitcount = sigLength * 8;
        long byteCount = 0;
        //Literal specific values
        long length = 0;
        //Find the value of these... values
        //In order: offset, bitwidth, alignment
        //Get the word data
        count -= wordlist[WT_LENGTH].offset;
        if (count < 0)
            count = 0;
        length = count & ((1L << (wordlist[WT_LENGTH].bitcount)) - 1);
        length &= ~((1L << (wordlist[WT_LENGTH].alignment)) - 1);
        byteCount = length + wordlist[WT_LENGTH].offset;
        for (int i = 0; i < bitcount; i++) {
                outchar <<= 1;
                if (bitlist[i].constantVal != -1) {
                        outchar |= bitlist[i].constantVal;
                } else {
                        //find this word in the wordlist
                        //Literals should just have 'l'
                        if (bitlist[i].wordType == 'l') {
                                outchar |= !!(length & (1L << bitlist[i].bitNo));
                        };
                }
                if (i % 8 == 7) {
                    **output = outchar;
                    (*output)++;
                    outchar = 0;
                };
        }
        return byteCount;
}

long lit_Encode(char *input, long offset, char **output, long count)
{
        input += offset;
        while (count) {
            int runlen = lit_Header(output, count);
            count -= runlen;
            memcpy(*output, input, runlen);
            *output += runlen;
            input += runlen;
        }
        return 0;
}
