FLAGS = -c --std=c11 -O3
DFLAGS = -Wall -Wextra -Wpedantic -g -O0
OBJ = $(addprefix obj\,main.o scanner.o literal.o lz.o)

debug: FLAGS += $(DFLAGS)
.PHONY: release debug clean

release: $(OBJ)
	gcc $(OBJ) -o LZifier.exe

debug: $(OBJ)
	gcc $(DFLAGS) $(OBJ) -o LZifier-debug.exe

clean:
	del $(OBJ)

obj\\%.o : %.c
	gcc $(FLAGS) $< -o $@