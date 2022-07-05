#ifndef TAR_C_C
#define TAR_C_C
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "shell.h"
#include "commande.h"
#include "tar_c.h"
#include "tar.h"
/*
Fichier qui fait les actions demandé sur les tar
*/
/*
Renvoie le nom des fichiers contenus dans un tar
Si le tar n'existe pas, on renvoie NULL
*/
char **list_fich(char *tar)
{
	int taille_archive_max = 20;
	int taille_archive = 0;
	//Liste des fichiers presents dans le tar
	char **liste_fichier = malloc(taille_archive_max*sizeof(char*));
	int fd,lus;
	struct posix_header entete;
	fd = open(tar,O_RDONLY);
	if (fd==-1)
	{
		perror(tar);
		free(liste_fichier);
		return NULL;
	}
	//Tant qu'on peut lire dans le fichier tar, on le fait et on le stocke dans une variable posix_header
	while ((lus=read(fd,&entete,BLOCKSIZE)) > 0)
	{
		//Gestion de la memoire dynamique
		if (taille_archive == taille_archive_max)
		{
			taille_archive_max *= 2;
			liste_fichier = realloc(liste_fichier,taille_archive_max*sizeof(char*));
		}
		//Si nous sommes dans une entete et que nous sommes pas dans un entete de fin de fichier
		if (entete.name[0] != '\0' && check_checksum(&entete))
		{
			//On rajoute le nom dans la liste
			liste_fichier[taille_archive] = calloc(strlen(entete.name)+3,sizeof(char));
			sprintf(liste_fichier[taille_archive],"%s",entete.name);
			liste_fichier[taille_archive][strlen(entete.name)] = '\0';
			if (entete.typeflag != '5')
			{
				unsigned long taille;
				sscanf(entete.size,"%lo",&taille);
				long nb_blocs = ceil(taille / 512.0);
				lseek(fd,nb_blocs*512,SEEK_CUR);
			}
			taille_archive++;
		}
		//Sinon, on arrive a la fin du fichier
		else
		{
			break;
		}
	}
	close(fd);
	for(int i = taille_archive;i < taille_archive_max;i++)
		liste_fichier[i] = NULL;
	return liste_fichier;

}
/*
Renvoie l'entete du fichier file present dans tar
*/
struct posix_header recuperer_entete(char *tar,char *file)
{
	struct posix_header entete;
	memset(&entete,0,BLOCKSIZE);
	int fd = open(tar,O_RDONLY);
	if (fd == -1)
	{
		char *erreur = malloc(strlen(tar) + strlen(file) + 50);
		sprintf(erreur,"recuperer_entete %s %s erreur :",tar,file);
		perror(erreur);
		free(erreur);
		return entete;
	}
	//On verifie le cas ou l'utilisateur a mis le nom du dossier sans le / final
	char * file2 = malloc(strlen(file)+2);
	sprintf(file2,"%s/",file);
	while(read(fd,&entete,BLOCKSIZE) > 0)
	{
		if (strcmp(entete.name,file) == 0 || strcmp(entete.name,file2)==0)
		{
			free(file2);
			close(fd);
			return entete;
		}
		unsigned long taille;
		sscanf(entete.size,"%lo",&taille);
		long nb_blocs = ceil(taille / 512.0);
		lseek(fd,nb_blocs*512,SEEK_CUR);
	}
	memset(&entete,0,BLOCKSIZE);
	close(fd);
	free(file2);
	return entete;
}
/*
REnvoie si le fichier dont le nom est file est bien un dossier dans tar. On suppose
qu'il est bien présent dans le fichier tar
1 -> repertoire
0 -> different
*/
int estRepertoire(char *file, char *tar)
{
	struct posix_header entete = recuperer_entete(tar,file);
	//Si le fichier n'a pas d'entete, on sait que c'est un dossier
	if (entete.name[0]=='\0')
	{
		return 1;
	}
	return entete.typeflag == '5';
}
/*
Prend en argument les droits sur un fichier, sous la forme octale, et renvoie la
chaine de caractère correspondant.
*/
char * from_mode_to_str_ls_l(char *mode)
{
	char * mode_ls = malloc(10);
	int perm[3];
	//On recupere la valeur des permissions
	//permissions proprietaire
	perm[0] = mode[4] - '0';
	//permissions groupe
	perm[1] = mode[5] - '0';
	//permissions autres
	perm[2] = mode[6] - '0';
	for (int i = 0; i < 3; i++)
	{
		//Droit de lecture
		if ((perm[i] - 4) >= 0)
		{
			mode_ls[0 + i*3] = 'r';
			perm[i] -= 4;
		}
		else
			mode_ls[0 + i*3] = '-';

		//Droit d'ecriture
		if ((perm[i] - 2) >= 0)
		{
			mode_ls[1 + i*3] = 'w';
			perm[i] -= 2;
		}
		else
			mode_ls[1 + i*3] = '-';

		//Droit d'execution
		if ((perm[i] - 1) >= 0)
		{
			mode_ls[2 + i*3] = 'x';
		}
		else
			mode_ls[2 + i*3] = '-';
	}
	mode_ls[9] = '\0';
	return mode_ls;
}
/*
REnvoie le nombre de sous répertoire directs.
*/
int nombre_sous_dossier(char *repr,char *tar,char **list)
{
	int i = 0;
	int nb_ss_dossier = 0;
	while (list[i] != NULL)
	{
		if (strncmp(repr,list[i],strlen(repr))==0)
		{
			init_chemin_explorer(&list[i][strlen(repr)]);
			int index = decoup_fich();
			if (index != 0 && chemin_a_explorer[index-1] == '/')
			{
				nb_ss_dossier++;
			}
			free_chemin_explorer();
		}
		i++;
	}
	return nb_ss_dossier;
}
/*
Recherche la date de la derniere modification pour un fichier n'ayant pas d'entete
dans tar
*/
time_t recherche_date_modif(char *tar,char *repr)
{
	int fd = open(tar,O_RDONLY);
	if (fd == -1)
	{
		char *erreur = malloc(strlen(tar) + strlen(repr) + 50);
		sprintf(erreur,"recherche_date_modif %s %s erreur :\n",tar,repr);
		perror(erreur);
		free(erreur);
		return 0;
	}
	lseek(fd,0,SEEK_SET);
	time_t last_modif = 0;
	struct posix_header entete;
	memset(&entete,0,BLOCKSIZE);
	while (read(fd,&entete,BLOCKSIZE)>0)
	{
		if (strncmp(entete.name,repr,strlen(repr))==0)
		{
			time_t last_modif_file = 0;
			sscanf(entete.mtime,"%011lo",&last_modif_file);
			if (last_modif < last_modif_file)
			{
				last_modif = last_modif_file;
			}
		}
		unsigned long taille;
		sscanf(entete.size,"%lo",&taille);
		long nb_blocs = ceil(taille / 512.0);
		lseek(fd,nb_blocs*512,SEEK_CUR);
	}
	if (last_modif==0)
	{
		struct stat st;
		fstat(fd,&st);
		struct timespec tv;
		tv = st.st_mtim;
		last_modif = tv.tv_sec;
	}
	close(fd);
	return last_modif;
}
/*
Renvoie les chaines de caracteres, correspondant aux informations longues sur les
fichiers, dont les noms sont dans to_print, et présent dans tar.
*/
char **affichage_ls_l(char ** to_print,char * argument,int nb_files,char **list)
{
	char ** ls_l = malloc(nb_files*sizeof(char *));
	int index = recherche_fich_tar(argument);
	char *tar = malloc(strlen(argument)+2);
	strncpy(tar,argument,index);
	tar[index] = '\0';
	//Retrait du /
	if (tar[index-1]=='/')
	{
		tar[index-1]= '\0';
	}
	//Calcul du nombre de blocs de 1024 o
	int nb_ln[nb_files];
	long nb_blocs_total = 0;
	for (int i = 0; i < nb_files;i++)
	{
		nb_ln[i] = 1;
		//On passe en argument le chemin absolu depuis la racine du tar
		char *chemin_absolu = calloc(strlen(argument)+strlen(to_print[i])+3,sizeof(char));
		//Racine en tar
		if (argument[index] == '\0')
		{
			sprintf(chemin_absolu,"%s",to_print[i]);
		}
		else
		{
			//Si le fichier
			if (strcmp(&argument[index],to_print[0])==0 && nb_files == 1)
			{
				sprintf(chemin_absolu,"%s",to_print[0]);
			}
			else
			{
				sprintf(chemin_absolu,"%s/%s",&argument[index],to_print[i]);
			}
		}
		struct posix_header entete = recuperer_entete(tar,chemin_absolu);
		//Une entete vide signifie que le fichier n'a pas d'entete -> c'est un dossier
		if (entete.name[0] == '\0')
		{
			entete.typeflag = '5';
			sprintf(entete.size,"%lo",(long unsigned int)0);
			sprintf(entete.mode,"0000777");
			sprintf(entete.mtime,"%011lo",recherche_date_modif(tar,chemin_absolu));
			sprintf(entete.uid,"%d",getuid());
			sprintf(entete.gid,"%d",getgid());
			sprintf(entete.uname,"%s",getpwuid(getuid())->pw_name);
			sprintf(entete.gname,"%s",getgrgid(getgid())->gr_name);
		}
		if (entete.typeflag == '5')
		{
			nb_ln[i] += nombre_sous_dossier(to_print[i],tar,list) + 1;
		}
		ls_l[i] = malloc(1024);
		unsigned long taille;
		char * time_fich = malloc(1024);
		time_t date;
		sscanf(entete.size,"%lo",&taille);
		sscanf(entete.mtime,"%011lo",&date);
		//Type de fichier
		char type_file;
		switch (entete.typeflag - '0') {
			//FIchier ordinaire
			case 0:
				type_file = '-';
				break;
			//Lien
			case 1:
				type_file = 'h';
				break;
			case 2:
				type_file = 'l';
				break;
			//Caractere special
			case 3:
				type_file = 'c';
				break;
			//Bloc
			case 4:
				type_file = 'b';
				break;
			//Repertoire
			case 5:
				type_file = 'd';
				break;
			//FIFO
			case 6:
				type_file = 'f';
				break;
		}
		//Calcul date
		struct tm * tm_t = localtime(&date);
		int hour = tm_t->tm_hour;
		int min = tm_t->tm_min;
		int day = tm_t->tm_mday;
		int mois = tm_t->tm_mon;
		char month[5];
		switch(mois)
		{
			case 0:
				strcpy(month,"jan.");
				break;
			case 1:
				strcpy(month,"fev.");
				break;
			case 2:
				strcpy(month,"mar.");
				break;
			case 3:
				strcpy(month,"avr.");
				break;
			case 4:
				strcpy(month,"mai.");
				break;
			case 5:
				strcpy(month,"jui.");
				break;
			case 6:
				strcpy(month,"jul.");
				break;
			case 7:
				strcpy(month,"aou.");
				break;
			case 8:
				strcpy(month,"sep.");
				break;
			case 9:
				strcpy(month,"oct.");
				break;
			case 10:
				strcpy(month,"nov.");
				break;
			case 11:
				strcpy(month,"dec.");
				break;
		}
		month[4] = '\0';
		if (hour < 10)
			sprintf(time_fich,"%s %d 0%d",month, day, hour);
		else
			sprintf(time_fich,"%s %d %d",month, day, hour);
		if (min < 10)
			sprintf(time_fich,"%s:0%d",time_fich,min);
		else
			sprintf(time_fich,"%s:%d",time_fich,min);
		sprintf(ls_l[i],"%c%s %d %s %s %ld %s %s\n",
		type_file,from_mode_to_str_ls_l(entete.mode),nb_ln[i],entete.uname,entete.gname,taille,time_fich,to_print[i]);
		//Calcul du poids total en bloc de 512 o pour commencer
		nb_blocs_total += ceil(((long double)taille + 512.0) / (long double) 512.0);
	}
	//On affiche la taille quand c'est un repertoire
	if (nb_files != 1 || strcmp(to_print[0],&argument[index]))
	{
		char * chaine_taille = malloc(sizeof(long) + strlen("\n")+3);
		//Le tar etant sous la forme de 20 blocs de 512o, on verifie s'il faut des blocs vides pour combler
		int reste = nb_blocs_total % 20;
		if (reste != 0)
			nb_blocs_total +=  (20 - reste) ;
		/*Maintenant que nous savons le nombre de blocs de 512o qu'on a, il suffit de
		renvoyer la partie entiere de la division du nombre de blocs par 2*/
		if (nb_blocs_total % 2 == 0)
			nb_blocs_total = (nb_blocs_total / 2);
		else
			nb_blocs_total = (nb_blocs_total / 2) + 1;
		sprintf(chaine_taille,"total %ld\n",nb_blocs_total);
		write(STDOUT_FILENO,chaine_taille,strlen(chaine_taille));
		free(chaine_taille);
	}
	return ls_l;

}
/*
Affiche le contenu du fichier file dans le fichier dont le descripteur est en argument
et retourne 1 si file est dans tar. Sinon,renvoie
0 et affiche une erreur
*/
int affiche_fichier_tar(char *tar,char*file, int fd_out)
{
	//Si le fichier est un repertoire, on ne l'affiche et on renvoie une erreur
	if (estRepertoire(file,tar))
	{
		char *error = malloc(strlen(file)+strlen("cat  : est un dossier\n")+2);
		sprintf(error,"cat %s : est un dossier\n", file);
		write(STDERR_FILENO,error,strlen(error));
		free(error);
		return 0;
	}
	int fd,lus;
	struct posix_header entete;
	fd = open(tar,O_RDONLY);
	if (fd==-1)
	{
		char *error = malloc(1024);
		sprintf(error,"Erreur affiche_fichier_tar %s",tar);
		perror(error);
		free(error);
		return 0;
	}
	while ((lus=read(fd,&entete,BLOCKSIZE)) > 0)
	{

		//Si nous sommes dans une entete et que nous sommes pas dans un entete de fin de fichier
		if (entete.name[0] != '\0' && check_checksum(&entete))
		{
			//On affiche le fichier
			if(strcmp(file,entete.name)==0)
			{
				char buffer[BLOCKSIZE];
				unsigned long taille;
				sscanf(entete.size,"%lo",&taille);
				int i = 0;
				while (taille > i)
				{
					lus=read(fd,buffer,BLOCKSIZE);
					write(fd_out,buffer,lus);
					i += lus;
				}
				return 1;
			}
			unsigned long taille;
			sscanf(entete.size,"%lo",&taille);
			long nb_blocs = ceil(taille / 512.0);
			lseek(fd,nb_blocs*512,SEEK_CUR);


		}

	}
	return 0;
}
/*
Supprime le fichier en argument du fichier .tar en argument en fonction de l'option indiquée :
RM -> supprime le fichier file de tar si ce n'est pas un dossier
RM_R -> supprime file et tout son contenu si file est un dossier
RM_DIR -> supprime file si file est un dossier vide
REnvoie 0 en cas d'echec, 1 sinon
*/
int supprimer_fichier_tar(char *tar,char *file,int option)
{
	int fd,fd_copie,lus;
	char *file2 = malloc(strlen(file)+3);
	strcpy(file2,file);
	strcat(file2,"/");
	file2[strlen(file2) + 1] = '\0';
	struct posix_header entete;
	memset(&entete,0,BLOCKSIZE);
	fd = open(tar,O_RDWR);
	//Utilisation d'un fichier auxiliaire
	fd_copie = open(".supprimer_fichier_tar",O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd==-1)
	{
		char *error = malloc(1024);
		sprintf(error,"Erreur supprimer_fichier_tar %s",tar);
		perror(error);
		free(error);
		free(file2);
		return 0;
	}
	if (fd_copie == -1)
	{
		write(STDERR_FILENO,"Erreur supprimer_fichier_tar\n",strlen("Erreur supprimer_fichier_tar\n"));
		free(file2);
		return 0;
	}
	//Compteur pour savoir si le dossier est bien vide pour l'option RM_DIR
	int contenu_dossier = 0;
	unsigned long taille;
	while ((lus=read(fd,&entete,BLOCKSIZE)) > 0)
	{
		//Si nous sommes dans une entete et que nous sommes pas dans un entete de fin de fichier
		if (entete.name[0] != '\0' && check_checksum(&entete))
		{
			//Si c'est le nom du fichier que l'on veut supprimer, on ne l'écrit pas le fichier auxiliaire
			if(strcmp(file,entete.name)==0||strncmp(file2,entete.name,strlen(file2))==0)
			{
				//Option -r absente
				if (entete.typeflag == '5' && option == RM)
				{
					char *error = malloc(strlen(file)+strlen("rm  : est un dossier\n")+1);
					sprintf(error,"rm %s : est un dossier\n",file);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					free(file2);
					return 0;
				}
				//rmdir sur un fichier autre qu'un dossier
				if (entete.typeflag != '5' && option == RM_DIR)
				{
					char *error = malloc(strlen(file)+strlen("rmdir  : n'est pas un dossier\n")+1);
					sprintf(error,"rmdir %s : n'est pas un dossier\n",file);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					free(file2);
					return 0;
				}
				sscanf(entete.size,"%lo",&taille);
				long nb_blocs = ceil(taille / 512.0);
				lseek(fd,nb_blocs*512,SEEK_CUR);
				contenu_dossier++;
			}
			//Sinon on continue de l'ecrire
			else
			{
				write(fd_copie,&entete,lus);
			}
		}
		else
		{
			write(fd_copie,&entete,lus);
		}
	}
	//Le dossier n'est pas vide et l'option est RM_DIR => erreur
	if (contenu_dossier!=1 && option == RM_DIR)
	{
		char *error = malloc(strlen(file)+strlen("rmdir  : n'est pas vide\n")+1);
		sprintf(error,"rmdir %s : n'est pas vide\n",file);
		write(STDERR_FILENO,error,strlen(error));
		free(error);
		free(file2);
		return 0;
	}
	//Copie du fichier auxiliaire dans le fichier en argument
	close(fd_copie);
	fd_copie = open(".supprimer_fichier_tar",O_RDONLY);
	lseek(fd,0,SEEK_SET);
	while ((lus=read(fd_copie,&entete,BLOCKSIZE)) > 0)
	{
		write(fd,&entete,lus);
	}
	close(fd);
	close(fd_copie);
	return 0;
}
/*
Modifie l'entete du fichier file dans le fichier tarball tar, en mettant la date
de la derniere modification a celle renseigne en date
*/
int modification_date_modif(char *tar,char *file,time_t date)
{
	int fd = open(tar,O_RDONLY);
	if (fd == -1)
	{
		perror("Modification Date Acces");
		return 0;
	}
	int fd_copie = open(".modification_date_modif",O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd_copie == -1)
	{
		perror("Modification Date Acces .modification_date_modif");
		return 0;
	}
	struct posix_header entete;
	int lus = 0;
	while ((lus = read(fd,&entete,BLOCKSIZE)) > 0)
	{
		//On verifie si le bloc lu est une entete
		if (entete.name[0] != '\0' && check_checksum(&entete))
		{
			//Entete du fichier voulu -> on modifie la date avant de l'ecrire dans la copie
			if (strcmp(entete.name,file) == 0)
			{
				sprintf(entete.mtime,"%011lo",date);
				write(fd_copie,&entete,BLOCKSIZE);
			}
			//Autre entete -> on l'ecrit tel quel
			else
			{
				write(fd_copie,&entete,BLOCKSIZE);
			}
		}
		//Le bloc n'est pas un entete -> on l'écrit tel quel
		else
		{
			write(fd_copie,&entete,lus);
		}
	}
	close(fd_copie);
	close(fd);
	fd = open(tar,O_WRONLY);
	lseek(fd,0,SEEK_SET);
	memset(&entete,0,sizeof(struct posix_header));
	fd_copie = open(".modification_date_modif",O_RDONLY);
	lseek(fd_copie,0,SEEK_SET);
	while ((lus = read(fd_copie,&entete,BLOCKSIZE)) > 0)
	{
		write(fd,&entete,lus);
	}
	close(fd_copie);
	close(fd);
	return 0;
}
/*
Crée ou écrase le fichier src dans le tar dont le futur entete est en argument.
*/
int creation_fichier_tar(char*tar,char*src,struct posix_header entete)
{

	int fd = open(tar,O_RDONLY);
	if(fd == -1)
	{
		char *error = malloc(strlen(tar)+strlen("Erreur creation_fichier_tar \n")+2);
		sprintf(error,"Erreur creation_fichier_tar %s\n",tar);
		perror(error);
		free(error);
		return 0;
	}
	int fd_copie = open(".creation_fichier_tar",O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR);
	if(fd_copie == -1)
	{
		char *error = malloc(strlen(tar)+strlen("Erreur .creation_fichier_tar \n")+2);
		sprintf(error,"Erreur .creation_fichier_tar %s\n",tar);
		perror(error);
		free(error);
		return 0;
	}
	struct posix_header hd2;
	memset(&hd2,0,BLOCKSIZE);
	long nb_blocs = 0;
	int trouve = 0;
	while ((read(fd,&hd2,512))>0)
	{

		if (hd2.name[0] != '\0' && check_checksum(&hd2))
		{
			if (strcmp(hd2.name,entete.name)==0)
			{
				write(fd_copie,&entete,BLOCKSIZE);
				int fd_src = open(src,O_RDONLY);
				if (fd_src == -1)
				{
					perror("");
					return 1;
				}
				char buffer[BLOCKSIZE];
				int taille_src = 0;
				sscanf(entete.size,"%o",&taille_src);
				if (taille_src != 0)
				{
					int lus = 0;
					while ((lus = read(fd_src, buffer, BLOCKSIZE)) > 0)
					{
						write(fd_copie, buffer, lus);
					}
				}
				long taille_dest = 0;
				sscanf(hd2.size,"%lo",&taille_dest);
				lseek(fd,512 * ceil((double) taille_dest / (double) 512),SEEK_CUR);
				trouve = 1;
				close(fd_src);
			}
			else
			{
				write(fd_copie,&hd2,BLOCKSIZE);
				unsigned long taille = 0;
				sscanf(hd2.size,"%lo",&taille);
				long nb_blocs_fich = ceil((double) taille / (double) 512.0);
				nb_blocs += 1 + nb_blocs_fich;
				while(nb_blocs_fich != 0)
				{
					char buffer[BLOCKSIZE];
					int lus = read(fd,buffer,BLOCKSIZE);
					write(fd_copie,buffer,lus);
					nb_blocs_fich--;
				}
			}
		}
		else
		{
			break;
		}
	}
	close(fd);
	close(fd_copie);
	//lseek(fd,0,SEEK_SET);
	//Ajout du fichier a la fin du tarball s'il n'existe pas
	if (trouve == 0)
	{
		fd = open(tar,O_WRONLY);
		lseek(fd,(nb_blocs)*512,SEEK_CUR);
		write(fd,&entete,512);
		if(entete.typeflag == '0')
		{
			int fd_src = open(src,O_RDONLY);
			if (fd_src == -1)
			{
				perror(tar);
				return 1;
			}
			char buffer[BLOCKSIZE];
			int lus_src = 0;
			while ((lus_src = read(fd_src,buffer,BLOCKSIZE)) > 0)
			{
				write(fd,buffer,lus_src);
				nb_blocs++;
			}
			close(fd_src);
		}
		nb_blocs++;
	}
	//On remplace le fichier de base par sa copie avec le nouveau fichier
	else
	{
		fd = open(tar,O_WRONLY);
		nb_blocs = 0;
		char buffer[BLOCKSIZE];
		int lus = 0;
		fd_copie = open(".creation_fichier_tar", O_RDONLY);
		lseek(fd_copie,0,SEEK_SET);
		while ((lus=read(fd_copie,buffer,BLOCKSIZE)) > 0)
		{
			write(fd,buffer,lus);
			nb_blocs++;
		}
	}
	close(fd);
	//Modification derniere date de modification pour les repertoires "antecedants" du nouveau fichier
	/*init_chemin_explorer(entete.name);
	int index_name = 0;
	time_t date;
	sscanf(entete.mtime,"%o",&date);
	decoup_fich();
	char * name_bis = calloc(strlen(entete.name)+1,sizeof(char));
	while (index_chemin_a_explorer < chemin_length)
	{
		char *file = malloc(strlen(entete.name)+3);
		strncpy(file,&chemin_a_explorer[index_name],index_chemin_a_explorer-index_name);
		file[index_chemin_a_explorer-index_name] = '\0';
		strcat(name_bis,file);
		name_bis[index_chemin_a_explorer] = '\0';
		modification_date_modif(tar,name_bis,date);
		index_name = index_chemin_a_explorer;
		decoup_fich();
		free(file);
	}
	free_chemin_explorer();
	free(name_bis);*/
	return 0;
}
#endif
