#include <stdlib.h>
#include <stdio.h>


int main(int argc, char **argv)
{
        FILE *infile = fopen(argv[1], "rb");
        char *backbuf = malloc(16384);
        char *backref = backbuf;
        int backind = 0;
        while (1) {
                int inchar = fgetc(infile);
                if (inchar == EOF) {
                        fclose(infile);
                        free(backbuf);
                        return 0;
                }
                if (inchar < 0x80) {
                        int count = inchar;
                        if (!count)
                                count = 128;
                        while (count--) {
                                backref[backind] = fgetc(infile);
                                putc(backref[backind++], stdout);
                        }
                };
                if (inchar >= 0x80) {
                        int index = (inchar & 0x7F) << 8;
                        index |= fgetc(infile);
                        index = backind - index;
                        int length = fgetc(infile);
                        if (!length)
                                length = 256;
                        while (length--) {
                                backref[backind] = backref[index++];
                                putc(backref[backind++], stdout);
                        }
                };
        }
}