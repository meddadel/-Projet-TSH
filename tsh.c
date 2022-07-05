#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include "commande.h"
#include "shell.h"
int main(int argc, char const *argv[]) {
	//Structure shell qui repertorie la variable pour sortir du shell, le repertoire courante et si on est dans un tarball
	char *cmd_tarballs[] = {"cd","ls","cat","mkdir","rmdir","mv","pwd","cp","rm"};
	char *option[] = {"","-l","","","","","","-r","-r"};
	shell tsh = creation_shell(cmd_tarballs,option,9);
	while (tsh.quit == 0)
	{
		int nb_arg_cmd; //Stocke le nombre d'arguments de la commande
		char **liste_argument =  recuperer_commande(&nb_arg_cmd);
		traitement_commande(liste_argument,nb_arg_cmd,&tsh);
		//Liberation de la memoire
		for(int i = 0; i < nb_arg_cmd;i++)
			free(liste_argument[i]);
		free(liste_argument);
	}
	liberation_shell(&tsh);
	return 0;
}
