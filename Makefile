all: my_zsh

my_zsh: my_zsh.o
	gcc -o my_zsh my_zsh.o -Wall -Wextra -Werror

my_zsh.o: my_zsh.c
	gcc -c my_zsh.c -Wall -Wextra -Werror

clean: 
	rm -f *.o

fclean: clean
	rm -f my_zsh

re: fclean all