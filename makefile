%.o: %.c
	gcc -c -o $@ $<

ash: shell_main.o shell_prompt.o shell_builtin.o shell_execute.o shell_pinfo.o
	gcc -o ash $^

all: ash

.PHONY: clean

clean:
	rm -f *.o ash
