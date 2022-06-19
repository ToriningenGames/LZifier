//Sets the file scanner reads from. Call first
//Null reads from standard input
void setfile(char *input);
//Upon failure, call to tell user
_Noreturn void die(int code, char *message, ...);
//Gets a word from input, optionally terminated with a colon
char *getlabel();
//Gets a number from input
long getnum();
//Gets the symbol part of a bit specifier
char getbit();
//Gets a single constant numeral
char getconst();
//Returns nonzero if the next token is a numeral
long isconstnext();
