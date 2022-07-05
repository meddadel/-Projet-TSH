#ifndef SHELL_C
#define SHELL_C
#define _GNU_SOURCE
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "commande.h"
#include "shell.h"
#include "tar_c.h"
/*
	Fonctions gerant le fontionnement du shell
*/
/*
Renvoie le shell qui gere les commandes et les options en argument sur les
tarballs
*/
shell creation_shell(char **cmd_tarballs,char **option,int nb_cmds)
{
	shell tsh;
	memset(&tsh, 0, sizeof(shell));
	tsh.repertoire_courant = malloc(1024);
	strcpy(tsh.repertoire_courant,getcwd(NULL,1024));
	strcat(tsh.repertoire_courant,"/");
	tsh.quit = 0;
	tsh.tarball = 0;
	tsh.nb_cmds = nb_cmds;
	tsh.cmd_tarballs = malloc(nb_cmds*sizeof(char*));
	tsh.option = malloc(nb_cmds*sizeof(char *));
	int i = 0;
	while (i < nb_cmds)
	{
		tsh.cmd_tarballs[i] = malloc(strlen(cmd_tarballs[i])+3);
		sprintf(tsh.cmd_tarballs[i],"%s",cmd_tarballs[i]);
		tsh.option[i] = malloc(strlen(option[i])+3);
		sprintf(tsh.option[i],"%s",option[i]);
		i++;
	}
	return tsh;
}
/*
Libere la memoire occupee par le tsh dont l'adresse est en argument
*/
void liberation_shell(shell *tsh)
{
	free(tsh->repertoire_courant);
	for (int i = 0; i < tsh->nb_cmds; i++)
	{
		free(tsh->cmd_tarballs[i]);
		free(tsh->option[i]);
	}
	free(tsh->cmd_tarballs);
	free(tsh->option);
}
/*
Initialise la variable globale "chemin_a_explorer" et ses attributs
*/
void init_chemin_explorer(char *path)
{
	chemin_a_explorer = calloc(strlen(path)+3,sizeof(char));
	chemin_length = strlen(path);
	index_chemin_a_explorer = 0;
	sprintf(chemin_a_explorer,"%s",path);
}
/*
Libere la variable "chemin_a_explorer" et ses attributs
*/
void free_chemin_explorer()
{
	//free(chemin_a_explorer);
	memset(chemin_a_explorer,0,chemin_length);
	chemin_length = 0;
	index_chemin_a_explorer = 0;
	//free(chemin_a_explorer);
}
/*
Parcourt la commande present dans la variable globale commande_a_explorer a partir
de l'index en argument et renvoie l'index correspondant a la fin du mot debute
en l'argument index. On definit un mot comme un suite de caractère sans espace.
*/
int decoup_mot(int index)
{
	//On est a la fin de commande_a_explorer, on renvoie directement index
	if (commande_a_explorer[index] == '\n' || commande_a_explorer[index] == '\0')
		return index;
	while (commande_a_explorer[index] == ' ') //Pour ne pas compter les espaces entre les mots
		index++;
	//On parcourt la commande jusqu'a la fin de la ligne ou au prochain espace
	while (commande_a_explorer[index] != ' ' && commande_a_explorer[index] != '\n' && commande_a_explorer[index] != '\0')
	{
		index++;
	}
	return index;
}
/*
Renvoie l'index, dans chemin_a_explorer, de la fin du fichier, qui commence a l'index
index_chemin_a_explorer au moment de l'appel a decoup_fich
*/
int decoup_fich()
{
	while (index_chemin_a_explorer < chemin_length)
	{
		if (chemin_a_explorer[index_chemin_a_explorer]=='/')
		{
			break;
		}
		index_chemin_a_explorer++;
	}

	if (chemin_length <= index_chemin_a_explorer)
	{
		return index_chemin_a_explorer;
	}
	index_chemin_a_explorer += 1;
	return index_chemin_a_explorer;
}
/*
Simplifie le chemin absolu en enlevant les .. et . contenu dans le chemin en argument
*/
char *simplifie_chemin(char *chemin)
{
	if (chemin == NULL)
		return NULL;
	else
	{
		//Initialisation de chemin_a_explorer avec chemin
		init_chemin_explorer(chemin);
		char * simplified_path = malloc(strlen(chemin)+1);
		int index_simple = 0;
		int index_prec = 0;
		decoup_fich();
		//On parcourt le chemin
		while(index_prec < chemin_length)
		{
			char * fich = calloc(index_chemin_a_explorer-index_prec+1, sizeof(char));
			strncpy(fich,&chemin_a_explorer[index_prec],index_chemin_a_explorer-index_prec);
			fich[index_chemin_a_explorer - index_prec] = '\0';
			//Présence .
			if (strcmp(fich,".") == 0)
			{
				index_prec = index_chemin_a_explorer;
				decoup_fich();
				free(fich);
				continue;
			}
			//Présence ..
			if ((strncmp(fich,"..",2) == 0))
			{
				if (index_simple >= index_prec)
					index_simple = index_prec;
				index_simple--;
				index_simple--;
				while (chemin_a_explorer[index_simple] != '/' && index_simple > 0)
				{
					index_simple--;
				}
				index_simple++;
				simplified_path[index_simple] = '\0';
				index_prec = index_chemin_a_explorer;
				decoup_fich();
				free(fich);
				continue;
			}
			//Dossier autre
			sprintf(&simplified_path[index_simple],"%s",fich);
			index_simple += strlen(fich);
			index_prec = index_chemin_a_explorer;
			decoup_fich();
			free(fich);

		}
		free_chemin_explorer();
		simplified_path[index_simple] = '\0';
		//Si on a la chaine vide, on est a la racine
		if (strcmp(simplified_path,"")==0)
		{
			free(simplified_path);
			return "/";
		}
		return simplified_path;
	}
}
/*
Recupere la commande de l'utilisateur, renvoie la liste des arguments (mot != " ")
et Stocke son nombre dans l'adresse en argument
*/
char **recuperer_commande(int * taille_commande)
{
	//Chaine de caractère contenant la commande ecrite par l'utilisateur
	char * commande = NULL;
	commande = readline(">");
	commande[strlen(commande)] = '\0';
	add_history(commande);
	int taille_commande_max = 10;
	char **liste_argument = (char **)malloc(taille_commande_max*sizeof(char*));
	int j = 1;
	commande_a_explorer = malloc(strlen(commande) +3);
	sprintf(commande_a_explorer,"%s",commande);
	int index = decoup_mot(0);
	char * nom_commande = malloc(index +3);
	strncpy(nom_commande,&commande_a_explorer[0],index);
	nom_commande[index] = '\0';
	liste_argument[0] = malloc(strlen(nom_commande)+2);
	sprintf(liste_argument[0],"%s",nom_commande);
	int index_prec = index;
	int lg_cmd = strlen(commande_a_explorer);
	while (index_prec < lg_cmd)
	{
		//Gestion de la memoire
		if(j == taille_commande_max)
		{
			taille_commande_max *= 2;
			liste_argument = (char **)realloc(liste_argument,taille_commande_max*sizeof(char*));
		}
		//Pour ne pas prendre les espaces entre les differents arguments de la commande
		while (commande_a_explorer[index_prec] == ' ' && commande_a_explorer[index_prec]!='\0')
			index_prec++;
		//Arivee a la fin de la commande
		if (commande_a_explorer[index_prec] == '\0')
			break;
		index = decoup_mot(index_prec);
		char * mot = malloc(index - index_prec + 1);
		strncpy(mot,&commande_a_explorer[index_prec],index - index_prec);
		mot[index - index_prec] = '\0';
		liste_argument[j] = malloc(strlen(mot) + 1);
		sprintf(liste_argument[j],"%s",mot);
		j++;
		free(mot);
		index_prec = index;
	}
	free(commande_a_explorer);
	free(commande);
	//On stocke le nombre d'arguments dans l'adresse donne en parametre
	*taille_commande = j;
	for (;j < taille_commande_max;j++)
	{
		liste_argument[j] = NULL;
	}
	return liste_argument;
}
/*
Analyse la ligne de commande et traite la commande
*/
int traitement_commande(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	int cmds=0;//nombre de commande entre pipe
	int redirect = 0;
	char *nom_commande = malloc(strlen(liste_argument[0])+1);
	nom_commande[strlen(liste_argument[0])] = '\0';
	//On verifie si la commande est nulle / vide
	//Si oui on revient a la ligne
	if (liste_argument[0] == NULL)
		write(STDOUT_FILENO,"\n",strlen("\n"));
	//Sinon on traite la commande
	else
	{
		strcpy(nom_commande,liste_argument[0]);
		//Execution de la commande "exit" et depart du shell
		if(strcmp(nom_commande,"exit")==0)
		{
			write(STDOUT_FILENO,"Au revoir\n",strlen("Au revoir\n"));
			tsh->quit = 1;
		}
		else
		{
            //compter le nombre de commande entre pipe
		    for(int i = 0; i <nb_arg_cmd; i++){
               if (strcmp(liste_argument[i], "|") == 0){
                 cmds++;
              }
                if (strcmp(liste_argument[i], ">") == 0){
					redirect = 1;
				}
		  		else if (strcmp(liste_argument[i], "2>") == 0){
					redirect = 2;
				}
            }
            cmds++;//sauvgarde le nombre de commande entre  pipe

		    //s'il notre comamande n'a pas de pipe
		    if(cmds==1)
			{
			    if(redirect > 0){
					redirection(liste_argument, nb_arg_cmd, tsh, redirect);
					redirect = 0;
				}else
				//On verifie que la commande peut s'effectuer sur les tar ou non
				if (estCommandeTar(nom_commande,tsh))
				{
					traitement_commandeTar(liste_argument,nb_arg_cmd,tsh);
				}
				//Sinon execution de la commande voulue si possible
				else
				{
					int pid = fork();
					if (pid == -1)
						perror("Fils non cree");
					if (pid==0)
					{
						if(execvp(nom_commande,liste_argument)==-1) //Si execvp renvoie -1, la commande n'existe pas
						{
							char error[strlen(nom_commande) + strlen("Commande   introuvable") + 1];
							sprintf(error,"Commande %s introuvable\n",nom_commande);
							write(STDERR_FILENO,error,strlen(error));
						}
						exit(0);
					}
					wait(NULL);
				}
			}

			//si notre commande contient des pipe
			else
			{
				  int i = 0;
	              int cmds=0;
	              int fd[2];
	              int fd2[2];

	              //compter le nombre de commande entre pipe
	              for(int i = 0; i < nb_arg_cmd; i++){
	                 if (strcmp(liste_argument[i], "|") == 0){
	                   cmds++;
	                 }
	              }
	              cmds++;


	             int j = 0;
	             char ** commandes = malloc(20*sizeof(char*));

	             int fin= 0;
	             pid_t pid;
	             while(liste_argument[j] != NULL && fin!= 1){

		             int k = 0;
		             while (strcmp(liste_argument[j],"|") != 0){
			              commandes[k] = liste_argument[j];
			              j++;

		                  if (liste_argument[j] == NULL){
		                     //la variable fin va nous indiquer si on a lis tout les commandes
		                     fin = 1;
		                     k++;
		                     break;
		                  }
						  k++;
	            	}
		            commandes[k] = NULL;
		            j++;

	           		if (i % 2 != 0){
					// si i est impaire

	           			pipe(fd);

	            	}

	          		else
					{
						pipe(fd2);
					}

	          		pid=fork();

					if(pid==0){

		           		if (i == 0){ //si on est dans la premier commande

		               		dup2(fd2[1], STDOUT_FILENO);
		           		}


		           		else
							if (i == cmds - 1){ // si on nest dans la dernier commande
			               		if (cmds % 2 != 0){
			                   		dup2(fd[0],STDIN_FILENO);
			               		}

			               		else{
			                   		dup2(fd2[0],STDIN_FILENO);
			                   }
		           			}


	          //si on est dans une commande qui est au millieu on doit utiliser 2 pipe un pour recuper
	          //sa sortie et lautre pour ecrire dans son entrer
		              else
					  {
			               if (i % 2 != 0){
			                   dup2(fd2[0],STDIN_FILENO);
			                   dup2(fd[1],STDOUT_FILENO);
			               }
						   else{
			                   dup2(fd[0],STDIN_FILENO);
			                   dup2(fd2[1],STDOUT_FILENO);
			               }
				   	  }
				  	  traitement_commande(commandes,k,tsh);
				  	  exit(0);
	       		}
		        if(i==0){
		     		close(fd2[1]);
		     	}
		     	else if(i==cmds -1){
		         	if(cmds % 2 !=0){ close(fd[0]); }

		          	else{ close(fd2[0]); }

		            }
		     		else{
		         		if (i%2 != 0){
		          			close(fd2[0]);
		          			close(fd[1]);
		         		}
		        		else{
		         			close(fd[0]);
		         			close(fd2[1]);
		        		}

	       			}

		   			waitpid(pid,NULL,0);
		   			i++;
	   		}
		}
	}
	}
	free(nom_commande);
	return 0;
}
#endif
