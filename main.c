//LZ-ifier
//Compresses data into a configurable LZ format
//Specifiers:
    //LZ bit constants
    //Literal bit constants
    //LZ Backreference bitlength
    //LZ Backreference bit location
    //LZ Length bitlength
    //LZ Length bit location
    //Literal Length bitlength
    //Literal Length bit location
    //Constant offsets
    //Alignment


/*Example:
Literal: l7 1
              0  l6  l5  l4  l3  l2  l1 l0
LZ:  b15 l8 3
              1  b14 b13 b12 b11 b10 b9 b8 
              b7 b6  b5  b4  b3  b2  b1 b0
              l7 l6  l5  l4  l3  l2  l1 l0
Extra:  l11*4+6   2         (Please don't do this)
              1   0   l6  1   1   l2  0  l4
              l10 l8  l9  0   l3  0   l5 0
*/
/* That is,
 *   Entry =: Label ':' Specifiers Bytelength Bytes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"
#include "encoders.h"


int main(int argc, char **argv)
{
        if (argc != 5) {
                puts("Usage: LZifier mode specfile");
                puts("Modes:");
                puts("\tLZ77");
                return -1;
        };
        //Check the mode
        if (strcmp(argv[1], "LZ77")) {
                printf("Unknown mode '%s'\n", argv[1]);
                return -2;
        };
        //Get the specifications
        setfile(argv[2]);
        //Parse the specifications
        char *signature = NULL;
        while (signature = getlabel()) {
                int i = 0;
                for (; strcmp(signature, formatstrs[i]); i++)
                        ;
                inits[i]();
        }
        //Ensure each component is initialized
        //At this point, code is mode specific
                //Compressed information can't get much larger than input-
                //if it does, we don't want to compress it.
                //For each position, query to see which option is best
                        //Make lengths match!
        FILE *infile = fopen(argv[3], "rb");
        long inlen = 0;
        while (fgetc(infile) != EOF)
                inlen++;
        rewind(infile);
        char *indata = malloc(inlen);
        fread(indata, 1, inlen, infile);
        fclose(infile);
        char *outdata = malloc((int)(inlen * 1.25));    //Buffer in case of expansion
        char *outstart = outdata;
        long litspan = 0;
        long outlen = 0;
        //Decide which portions to encode in what formats
        long i = 0;
        for (; i < inlen; i++) {
                long count = inlen;
                long lzsiz = querys[FT_LZ](indata, i, &count);
                long litsiz = querys[FT_LITERAL](indata, i, &count);
                if (lzsiz && lzsiz < litsiz) {
                        outlen += querys[FT_LITERAL](indata, i - litspan, &litspan);
                        encodes[FT_LITERAL](indata, i - litspan, &outdata, litspan);
                        outlen += querys[FT_LZ](indata, i, &count);
                        encodes[FT_LZ](indata, i, &outdata, count);
                        i += count - 1;
                        litspan = -1;
                };
                litspan++;
        }
        //Run literal run, just in case LZ wasn't last
        outlen += querys[FT_LITERAL](indata, i - litspan, &litspan);
        encodes[FT_LITERAL](indata, i - litspan, &outdata, litspan);
        //Write to file
        free(indata);
        FILE *outfile = fopen(argv[4], "wb");
        fwrite(outstart, 1, outlen, outfile);
        free(outstart);
        fclose(outfile);
        return 0;
}

