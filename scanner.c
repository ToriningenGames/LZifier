#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <ctype.h>

#define LABEL_MAX (16)

FILE *infile = NULL;
char *filename = NULL;
long line, column;


noreturn void die(int status, char *message, ...)
{
        va_list args;
        va_start(args, message);
        fprintf(stderr, "Error in %s:%li:%li: ", filename, line, column);
        vfprintf(stderr, message, args);
        va_end(args);
        exit(status);
}

void setfile(char *input)
{
        if (filename)
                free(filename);
        filename = malloc(strlen(input) + 1);
        strcpy(filename, input);
        if (infile)
                fclose(infile);
        if (input) {
                infile = fopen(input, "r");
        } else {
                infile = stdin;
        }
        line = 1;
        column = 1;
}

static int unget = 0;

#define ungetnext(v) (unget = (v))

char getnext()
{
        if (unget) {
                int tmp = unget;
                unget = 0;
                return tmp;
        };
        int input = fgetc(infile);
        if (input == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        if (isspace(input)) {   //Collapse spaces
            while (isspace(input = getnext())) {
                if (input == '\n') {
                        line++;
                        column = 1;
                } else {
                        column++;
                }
            }
            unget = input;
            return ' ';
        }
        switch (input) {
                case '(' : ;    //Comment
                    while ((input = getnext()) != ')') {
                        if (!input) {   //misaligned braces
                            fputs("Error: Unexpected end-of-file in comment\n", stderr);
                            exit(-1);
                        }
                    }
                    while (isspace(input = getnext()))  //Collapse spaces
                        ;
                    unget = input;
                    return ' ';
                case '\\' : ;   //Escape
                    switch (fgetc(infile)) {
                            case '\r' :
                            case '\n' : 
                                line++;
                                column = 1;
                                return getnext();
                            case '\\' :
                                column++;
                                return '\\';
                            case '('  :
                                column++;
                                return '(';
                            default   :
                                die(1, "Unknown escape code\n");
                    }
                case EOF : ;    //End of file
                    return 0;
                default : ;     //All others
                    return input;
        }
}

char *getlabel()
{
        //Letter, then alphanum, then colon
        //Only storing the first few
        static char label[LABEL_MAX] = "";
        int i = 0;
        label[i] = getnext();
        while (label[i] == ' ')
                label[i] = getnext();
        if (!label[i])
                return NULL;
        if (!isalpha(label[i]))
                die(-1, "Expected a label, got '%c'\n", label[i]);
        i++;
        while (label[i] = getnext()) {
                if (strchr(": \n", label[i])) {
                        label[i] = '\0';
                        return label;
                };
                if (++i == LABEL_MAX) {
                        label[i] = '\0';
                        break;
                };
        }
        //Label full. Grab rest of it anyways
        while (!strchr(": \n", label[i]))
                ;
        return label;
}

long getnum()
{
        long temp, val = 0;
        temp = getnext();
        while (temp == ' ')
                temp = getnext();
        if (!isdigit(temp)) die(-1, "Expected a number, got '%c'\n", temp);
        do {
                val *= 10;
                val += temp - '0';
        } while (isdigit(temp = getnext()));
        while (temp == ' ')
                temp = getnext();
        ungetnext(temp);
        return val;
}

char getbit()
{
        int val = getnext();
        while (val == ' ')
            val = getnext();
        if (!isalpha(val) && !ispunct(val))
            die(-1, "Expected a bit specifier, got '%c'\n", val);
        return val;
}

char getconst()
{
        int val = getnext();
        while (val == ' ')
            val = getnext();
        if (!isdigit(val))
                die(-1, "Expected a bit constant, got '%c'\n", val);
        val -= '0'; //Within standard for numbers
        return val;
}

long isconstnext()
{
        int val = getnext();
        while (val == ' ')
                val = getnext();
        ungetnext(val);
        if (isdigit(val))
            return val;
        return 0;
}
