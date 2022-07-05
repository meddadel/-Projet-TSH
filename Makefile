FLAGS = -Wall -pedantic -g
FILES = tsh.o commande.o shell.o tar_c.o tar_cmd.o tar.o
all: $(FILES)
	gcc $(FLAGS) $(FILES) -o tsh -lreadline -lm
tsh.o: shell.c shell.h commande.c commande.h tsh.c
	gcc -c $(FLAGS) tsh.c -o tsh.o
commande.o: shell.c commande.c tar_c.c commande.h tar_cmd.c tar_cmd.h
	gcc -c $(FLAGS) commande.c -o commande.o
shell.o: shell.c shell.h commande.c commande.h
	gcc -c $(FLAGS) shell.c -o shell.o
tar_c.o: tar_c.c tar_c.h tar.h tar.c
	gcc -c $(FLAGS) tar_c.c -o tar_c.o
tar_cmd.o: tar_cmd.c tar_cmd.h tar_c.c tar_c.h commande.c commande.h tar.h tar.c
	gcc -c $(FLAGS) tar_cmd.c -o tar_cmd.o
tar.o: tar.c tar.h
	gcc -c $(FLAGS) tar.c -o tar.o
clean:
	rm tsh $(FILES)
