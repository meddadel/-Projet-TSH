#include "tar_cmd.h"
/*
Renvoie les noms des fichiers contenus dans repr dans tar
*/
char ** sous_dossiers_directs(char *repr, char *tar)
{
	if (repr[strlen(repr) - 1]=='/')
		repr[strlen(repr) - 1] = '\0';
	int index = recherche_fich_tar(repr);
	char **list = list_fich(tar);
	char *file_to_find = malloc(strlen(repr)+5);
	strncpy(file_to_find,&repr[index],strlen(repr)-index+1);
	file_to_find[strlen(repr) -index+1] = '\0';
	int nb_fich_list = 0;
	while (list[nb_fich_list]!=NULL)
		nb_fich_list++;
	//Recherche du fichier dans le fichier .tar
	int k = 0;
	char **to_print = calloc((nb_fich_list + 4),sizeof(char*));
	int index_to_print = 0;
	//Parcours des fichiers du tar
	while (list[k] != NULL)
	{
		//On verifie si le fichier courant commence de la meme facon
		if (strncmp(list[k],file_to_find,strlen(file_to_find))==0)
		{
			/*On verifie si le fichier courant est bien le fichier sur lequel on appelle
			ls ou encore s'il appartient bien a ce fichier.
			On fait ca pour eviter de compter un fichier qui commence de la meme
			facon mais qui reste different*/
			int file_in_tar_len = strlen(repr) - index;
			if (list[k][file_in_tar_len] == '\0'||list[k][file_in_tar_len] == '/')
			{
				//On recupere alors soit un fichier soit un repertoire
				int jpp = strlen(file_to_find) + 1;
				init_chemin_explorer(&list[k][jpp]);
				decoup_fich("");
				int index_path = 0;
				char * fich_to_print = calloc (index_chemin_a_explorer - index_path +10,sizeof(char));
				strncpy(fich_to_print,&chemin_a_explorer[index_path], index_chemin_a_explorer - index_path);
				fich_to_print[index_chemin_a_explorer-index_path] = '\0';
				int d = 0;
				//On verifie si le fichier est deja present dans la liste des elements a afficher
				for (; d < index_to_print;d++)
				{
					if (index_chemin_a_explorer==index_path) continue;
					if (strcmp(fich_to_print,to_print[d])==0)
						break;
				}
				//On est donc sur que le fichier n'est pas deja dans la iste
				if (d == index_to_print)
				{
					//On n'affiche pas le nom du dossier sur lequel on appelle ls
					if (strncmp(list[k],file_to_find,strlen(file_to_find))==0 && list[k][strlen(list[k])-1] == '/')
					{
						//Dossier
						if(index_chemin_a_explorer==index_path)
						{
							k++;
							free(fich_to_print);
							free_chemin_explorer();
							continue;
						}
					}
					//ls sur un fichier != repertoire
					if (index_chemin_a_explorer==index_path)
					{
						sprintf(fich_to_print,"%s",file_to_find);
					}
					to_print[index_to_print] = calloc(strlen(fich_to_print)+4,sizeof(char));
					sprintf(to_print[index_to_print],"%s",fich_to_print);
					index_to_print++;
				}
				free(fich_to_print);
				free_chemin_explorer();
			}
		}
		k++;
	}
	to_print[index_to_print] = NULL;
	free(file_to_find);
	return to_print;
}
/*
Effectue la commande ls sur le fichier present dans un tarball
*/
int ls(char *file, char **options,shell *tsh)
{
	char * simplified_file = malloc(strlen(file) + strlen(tsh->repertoire_courant)+3);
	//Si file est le repertoire courant, pas besoin de simplifier le chemin
	if (strcmp(file,tsh->repertoire_courant)==0)
	{
		sprintf(simplified_file,"%s",tsh->repertoire_courant);
		if (simplified_file[strlen(file)-1]=='/')
			simplified_file[strlen(file)-1] = '\0';
	}
	//SInon, on le simplifie
	else
	{
		sprintf(simplified_file,"%s%s",tsh->repertoire_courant,file);
		sprintf(simplified_file,"%s",simplifie_chemin(simplified_file));
	}
	if (simplified_file[strlen(simplified_file)-1] == '/')
	{
		simplified_file[(strlen(simplified_file))-1] = '\0';
	}
	int s = recherche_fich_tar(simplified_file);
	//Tableau qui stocke les noms des fichiers a afficher
	char ** to_print;
	//Son index
	int index_to_print = 0;
	//fichier tar
	char * tar = calloc(strlen(simplified_file)+2,sizeof(char));
	char **list;
	//ls sur un Fichier .tar
	if (s==strlen(simplified_file))
	{
		list = list_fich(simplified_file);
		sprintf(tar,"%s",simplified_file);
		if (list == NULL)
		{
			char *error = malloc(strlen(file)+strlen("ls \n"));
			sprintf(error,"ls %s\n",file);
			write(STDERR_FILENO,error,strlen(error));
			free(error);
			free(simplified_file);
			return 1;
		}
		else
		{
			//Calcul du nombre de fichier dans le tar
			int nb_fich_list = 0;
			while (list[nb_fich_list]!=NULL)
				nb_fich_list++;
			to_print = calloc((nb_fich_list + 1),sizeof(char *));
			int k = 0;
			while (list[k]!=NULL)
			{
				int d = 0;
				//On n'affiche pas les fichiers non presents "a la racine" du tar
				for(;d < index_to_print;d++)
				{
					if (strncmp(to_print[d],list[k],strlen(to_print[d])) == 0)
					{
						break;
					}
				}
				if (d == index_to_print)
				{
					init_chemin_explorer(list[k]);
					decoup_fich("");
					char * mot = malloc(strlen(list[k])+1);
					strncpy(mot,chemin_a_explorer,index_chemin_a_explorer);
					mot[index_chemin_a_explorer] = '\0';

					to_print[index_to_print] = malloc(strlen(mot)+1);
					sprintf(to_print[index_to_print],"%s",mot);
					index_to_print++;
					free(mot);
					free_chemin_explorer();
				}
				k++;
			}
		}
	}
		//ls sur un fichier dans un fichier .tar
	else
	{
		//Recherche fichier .tar contenant le fichier
		int index = recherche_fich_tar(simplified_file);
		strncpy(tar,simplified_file,index);
		tar[index - 1] = '\0';
		list = list_fich(tar);
		char *file_to_find = malloc(strlen(simplified_file)+1);
		strncpy(file_to_find,&simplified_file[index],strlen(simplified_file)-index + 1);
		if (list == NULL)
		{
			char *error = malloc(strlen(tar)+strlen("ls \n"));
			sprintf(error,"ls %s\n",tar);
			perror(error);
			free(error);
			free(tar);
			free(simplified_file);
			return 1;
		}
		else
		{
			int nb_fich_list = 0;
			while (list[nb_fich_list]!=NULL)
				nb_fich_list++;
			//Recherche du fichier dans le fichier .tar
			int k = 0;
			to_print = calloc((nb_fich_list + 1),sizeof(char*));
			//Parcours des fichiers du tar
			while (list[k]!=NULL)
			{
				//On verifie si le fichier courant commence de la meme facon
				if (strncmp(list[k],file_to_find,strlen(file_to_find))==0)
				{
					/*On verifie si le fichier courant est bien le fichier sur lequel on appelle
					ls ou encore s'il appartient bien a ce fichier.
					On fait ca pour eviter de compter un fichier qui commence de la meme
					facon mais qui reste different*/
					int file_in_tar_len = strlen(simplified_file) - index;
					if (list[k][file_in_tar_len] == '\0'||list[k][file_in_tar_len] == '/')
					{
						//On recupere alors soit un fichier soit un repertoire
						int jpp = strlen(file_to_find) + 1;
						init_chemin_explorer(&list[k][jpp]);
						decoup_fich("");
						int index_path = 0;
						char * fich_to_print = calloc (index_chemin_a_explorer - index_path +4,sizeof(char));
						strncpy(fich_to_print,&chemin_a_explorer[index_path], index_chemin_a_explorer - index_path);
						fich_to_print[index_chemin_a_explorer-index_path + 1] = '\0';
						int d = 0;
						//On verifie si le fichier est deja present dans la liste des elements a afficher
						for (; d < index_to_print;d++)
						{
							if (index_chemin_a_explorer==index_path) continue;
							if (strcmp(fich_to_print,to_print[d])==0)
								break;
						}
						//On est donc sur que le fichier n'est pas deja dans la iste
						if (d == index_to_print)
						{
							//On n'affiche pas le nom du dossier sur lequel on appelle ls
							if (strncmp(list[k],file_to_find,strlen(file_to_find))==0 && list[k][strlen(list[k])-1] == '/')
							{
								//Dossier
								if(index_chemin_a_explorer==index_path)
								{
									k++;
									free(fich_to_print);
									free_chemin_explorer();
									continue;
								}
							}
							//ls sur un fichier != repertoire
							if (index_chemin_a_explorer==index_path)
							{
								sprintf(fich_to_print,"%s",file_to_find);
							}
							to_print[index_to_print] = malloc(strlen(fich_to_print)+2);

							sprintf(to_print[index_to_print],"%s",fich_to_print);

							index_to_print++;
						}
						free(fich_to_print);
						free_chemin_explorer();
					}
				}
				k++;
			}
		}

	}
	//Option -l presente
	if (options)
	{
		char **list_ls = affichage_ls_l(to_print,simplified_file,index_to_print,list);
		//Si ls est sur un seul fichier, on modifie la ligne avant de l'afficher pour y mettre le chemin
		int index_tar = strlen(tar)+1;
		//S'il y a un seul fichier et qu'il a le meme nom, on a donc appele ls sur ce fichier
		if (index_to_print == 1 && strcmp(&simplified_file[index_tar],to_print[0])==0)
		{
		 char * true_line = malloc(strlen(list_ls[0])+strlen(file)+3);
		 sprintf(true_line, "%s", list_ls[0]);
		 int i = strlen(list_ls[0]) - 1;
		 while (true_line[i] != ' ')
		 {
			 i--;
		 }
		 i++;
		 strcpy(&true_line[i],file);
		 strcat(true_line,"\n");
		 write(STDOUT_FILENO,true_line,strlen(true_line));
		 free(true_line);
		}
		else
		{
			for (int i = 0; i < index_to_print; i++)
			{
				write(STDOUT_FILENO,list_ls[i],strlen(list_ls[i]));
			}
			for(int i = 0; i < index_to_print; i++)
			{
				free(list_ls[i]);
			}
			free(list_ls);
		}
	}
	//Sans option -l
	else
	{
		//ls sur un fichier ordinaire
		if (index_to_print == 1)
		{
			if (to_print[0][strlen(to_print[0])-1] != '/')
			{
				char *full_name = malloc(strlen(file)+3);
				sprintf(full_name,"%s\n",file);
				write(STDOUT_FILENO,full_name,strlen(full_name)+1);
				free(full_name);
			}
		}
		//ls sur un repertoire dans un tar
		write(STDOUT_FILENO,file,strlen(file));
		write(STDOUT_FILENO,":\n\n",strlen(":\n\n")+1);
		for (int i = 0; i < index_to_print; i++)
		{
			char * full_name = malloc(strlen(to_print[i]) + 4);
			sprintf(full_name,"%s\n", to_print[i]);
			write(STDOUT_FILENO,full_name,strlen(full_name));
			free(full_name);
		}
	}
	for (int i = 0; i < index_to_print; i++)
		free(to_print[i]);
	free(to_print);
	return 0;
}
/*

*/
int cd(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	if (nb_arg_cmd == 2)
	{
		char * nv_repr_courant = malloc(strlen(liste_argument[1])+
		strlen(tsh->repertoire_courant)+4);
		//Prise en charge des chemins absolus depuis la racine
		if (liste_argument[1][0] == '/')
		{
			strcpy(nv_repr_courant,liste_argument[1]);
		}
		else
		{
			sprintf(nv_repr_courant,"%s",tsh->repertoire_courant);
			if(tsh->repertoire_courant[strlen(tsh->repertoire_courant)-1] != '/')
				strcat(nv_repr_courant,"/");
			strcat(nv_repr_courant,liste_argument[1]);
		}
		//Ajout du / au chemin s'il n'en y a pas
		if (nv_repr_courant[strlen(nv_repr_courant)-1] != '/')
		{
			strcat(nv_repr_courant,"/");
			nv_repr_courant[strlen(nv_repr_courant)] = '\0';
		}
		//On verifie si le chemin est valide (c-a-d si le repertoire existe)
		int valide = cheminValide(nv_repr_courant,"cd");
		if (valide==0)
		{
			erreur_chemin_non_valide(liste_argument[1],"cd");
			free(nv_repr_courant);
			return 0;
		}
		else
		{
			//L'argument n'est pas repertoire -> on affiche une erreur
			if (valide == -1 && errno == ENOTDIR)
			{
				char *error = malloc(strlen(liste_argument[1])+30);
				sprintf(error,"cd %s : n'est pas un dossier\n",liste_argument[1]);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
				free(nv_repr_courant);
				return 1;
			}
			sprintf(nv_repr_courant,"%s",simplifie_chemin(nv_repr_courant));
			//Apres la simplication, il faut bien verifier si le nouveau repertoire est dans un tarball ou non
			if (contexteTarball(nv_repr_courant))
			{
				int index_tar = recherche_fich_tar(nv_repr_courant);
				char *tar = calloc(strlen(nv_repr_courant)+1,sizeof(char));
				strncpy(tar,nv_repr_courant,index_tar);
				//
				if (tar[strlen(tar)-1]=='/')
				{
					tar[strlen(tar)-1] = '\0';
				}
				char *file = malloc(strlen(nv_repr_courant)+5);
				sprintf(file,"%s",&nv_repr_courant[index_tar]);
				//
				if (file[strlen(file)-1]=='/')
				{
					file[strlen(file)-1] = '\0';
				}
				if (index_tar==strlen(nv_repr_courant) || estRepertoire(file,tar)==1)
				{
					sprintf(tsh->repertoire_courant,"%s",nv_repr_courant);
					tsh->tarball = 1;
					free(nv_repr_courant);
					return 1;
				}
				else
				{
					char *error = malloc(strlen(liste_argument[1])+30);
					sprintf(error,"cd %s : n'est pas un dossier\n",liste_argument[1]);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					free(nv_repr_courant);
					return 1;
				}
			}
			else
			{
				if (chdir(nv_repr_courant)==-1)
				{
					char error[strlen(liste_argument[1])+strlen("cd impossible") + 3];
					sprintf(error,"cd %s impossible",liste_argument[1]);
					perror(error);
					free(nv_repr_courant);
				}
				else
				{
					strcpy(nv_repr_courant,simplifie_chemin(nv_repr_courant));
					strcpy(tsh->repertoire_courant,nv_repr_courant);
					tsh->tarball = 0;
					free(nv_repr_courant);
				}
			}
		}
	}
	else
	{
		if (nb_arg_cmd > 2)
			write(STDERR_FILENO,"cd : Trop d'arguments\n",strlen("cd : Trop d'arguments\n"));
		else
			write(STDERR_FILENO,"cd : Pas assez d'arguments\n",strlen("cd : Pas assez d'arguments\n"));
	}
	return 0;
}
int pwd(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	char * repr = malloc(strlen(tsh->repertoire_courant)+ 2);
	sprintf(repr, "%s\n",tsh->repertoire_courant);
	write(STDOUT_FILENO,repr,strlen(repr));
	free(repr);
	return 0;
}
/*
Convertit un mode_t mode sous forme octale
*/
char * perm_str(mode_t mode)
{
	char * perm = malloc(4);
	perm[3] = '\0';
	perm[0] = '0';perm[1] = '0';perm[2] = '0';
	//Droits de l'utilisateur
	if ((mode & S_IRUSR) != 0)
	{
		perm[0] += 4;
	}
	if ((mode & S_IWUSR) != 0)
	{
		perm[0] += 2;
	}
	if ((mode & S_IXUSR) != 0)
	{
		perm[0] += 1;
	}
	//Droits de groupe
	if ((mode & S_IRGRP) != 0)
	{
		perm[1] += 4;
	}
	if ((mode & S_IWGRP) != 0)
	{
		perm[1] += 2;
	}
	if ((mode & S_IXGRP) != 0)
	{
		perm[1] += 1;
	}
	//Droits des autres
	if ((mode & S_IROTH) != 0)
	{
		perm[2] += 4;
	}
	if ((mode & S_IWOTH) != 0)
	{
		perm[2] += 2;
	}
	if ((mode & S_IXOTH) != 0)
	{
		perm[2] += 1;
	}
	return perm;
}
/*
Renvoie le mode en mode_t correspondant a la representation octal en argument
*/
mode_t perm_mode(char mode[8])
{
	mode_t m = 0;
	//Droit proprietaire
	int perm_uti = mode[4] - '0';
	//Droit groupe
	int perm_grp = mode[5] - '0';
	//Droit autres
	int perm_oth = mode[6] - '0';
	//proprietaire
	if (perm_uti - 4 >= 0)
	{
		perm_uti -= 4;
		m = m | S_IRUSR;
	}
	if (perm_uti - 2 >= 0)
	{
		perm_uti -= 2;
		m = m | S_IWUSR;
	}
	if (perm_uti - 1 >= 0)
	{
		perm_uti -= 1;
		m = m | S_IXUSR;
	}
	//Groupe
	if (perm_grp - 4 >= 0)
	{
		perm_grp -= 4;
		m = m | S_IRGRP;
	}
	if (perm_grp - 2 >= 0)
	{
		perm_grp -= 2;
		m = m | S_IWGRP;
	}
	if (perm_grp - 1 >= 0)
	{
		perm_grp -= 1;
		m = m | S_IXGRP;
	}
	//Autres
	if (perm_oth - 4 >= 0)
	{
		perm_oth -= 4;
		m = m | S_IROTH;
	}
	if (perm_oth - 2 >= 0)
	{
		perm_oth -= 2;
		m = m | S_IWOTH;
	}
	if (perm_oth - 1 >= 0)
	{
		perm_oth -= 1;
		m = m | S_IXOTH;
	}
	return m;
}
/*
Renvoie le typeflag de posix_header correspondant au mode en mode_t indique
*/
char type_fich(mode_t mode)
{
	switch(S_IFMT & mode)
	{
		case S_IFREG:
			return '0';
		case S_IFLNK:
			return '2';
		case S_IFCHR:
			return '3';
		case S_IFBLK:
			return '4';
		case S_IFDIR:
			return '5';
		case S_IFIFO:
			return '6';
		default :
			return '\0';

	}
}
/*
Renvoie l'entete correspondant a un dossier avec son nom et sa struct st
*/
struct posix_header entete_dossier(char * name, struct stat st)
{
	struct posix_header hd;
	memset(&hd,0,BLOCKSIZE);
	if (name[strlen(name)-1] == '/')
		sprintf(hd.name,"%s",name);
	else
		sprintf(hd.name,"%s/",name);
	sprintf(hd.mode,"0000%s",perm_str(st.st_mode));
	hd.typeflag = '5';
	time_t date = time(NULL);
	sprintf(hd.mtime,"%011lo",date);
	sprintf(hd.uid,"%d",st.st_uid);
	sprintf(hd.gid,"%d",st.st_gid);
	sprintf(hd.uname,"%s",getpwuid(st.st_uid)->pw_name);
	sprintf(hd.gname,"%s",getgrgid(st.st_gid)->gr_name);
	sprintf(hd.size,"%011lo",(long unsigned int)0);
	strcpy(hd.magic,"ustar");
	set_checksum(&hd);
	if (!check_checksum(&hd))
	{
		perror("Checksum impossible");
		memset(&hd,0,BLOCKSIZE);
		return hd;
	}
	return hd;
}
/*
Fonction auxiliaire de cp_file_to_tar, traitant les fichiers qui ne sont pas
des repertoires
*/
int cp_file_to_tar_aux(char *src, char *destination,struct stat st)
{
	int index_tar = recherche_fich_tar(destination);
	char *tar = malloc(index_tar + 2);
	strncpy(tar,destination,index_tar);
	tar[index_tar] = '\0';
	if (tar[index_tar -1] == '/')
		tar[index_tar - 1] = '\0';
	int index_src = strlen(src) - 1;
	while (index_src != 0 && src[index_src] != '/')
	{
		index_src--;
	}
	if (index_src != 0)
		index_src++;
	struct posix_header entete;
	memset(&entete, 0, BLOCKSIZE);
	if (strlen(destination)==index_tar)
		sprintf(entete.name,"%s",&src[index_src]);
	else
	{
		if (destination[strlen(destination) - 1] != '/')
			sprintf(entete.name,"%s/%s",&destination[index_tar],&src[index_src]);
		else
			sprintf(entete.name,"%s%s",&destination[index_tar],&src[index_src]);
	}
	time_t date = time(NULL);
	sprintf(entete.mtime,"%011lo",date);
	sprintf(entete.uid,"%d",st.st_uid);
	sprintf(entete.gid,"%d",st.st_gid);
	sprintf(entete.uname,"%s",getpwuid(st.st_uid)->pw_name);
	sprintf(entete.gname,"%s",getgrgid(st.st_gid)->gr_name);
  	strcpy(entete.magic,"ustar");
	sprintf(entete.mode,"0000%s",perm_str(st.st_mode));
	entete.typeflag = type_fich(st.st_mode);
	if (entete.typeflag != '0')
	{
		sprintf(entete.size,"%011lo",(long unsigned int) 0);
	}
	else
	{
		sprintf(entete.size,"%011lo",st.st_size);
	}
	set_checksum(&entete);
	if (!check_checksum(&entete))
	{
		perror("Checksum impossible");
		return 1;
	}
	creation_fichier_tar(tar,src,entete);
	free(tar);
	return 0;

}
/*
Copie le fichier src, qui est en dehors d'un tarball, vers destination qui est
dans un tarball
*/
int cp_file_to_tar(char *src, char *destination,int option)
{
	struct stat st_src;
	if (stat(src,&st_src)==-1)
	{
		perror("src cp_file_to_tar");
		return 1;
	}
	//On verifie si la commande est mv ou cp avec -r si on a un dossier
	if (S_ISDIR(st_src.st_mode) && option == 0)
	{
		char * error = malloc(strlen(src) + 50);
		sprintf(error,"cp : -r non specifie : omission du repertoire %s\n", src);
		write(STDERR_FILENO,error,strlen(error));
		free(error);
		return 1;
	}
	int index_tar = recherche_fich_tar(destination);
	char *tar = malloc(index_tar + 2);
	strncpy(tar,destination,index_tar);
	tar[index_tar] = '\0';
	if (tar[index_tar -1] == '/')
	{
		tar[index_tar - 1] = '\0';
	}
	//Le fichier a copier est un dossier -> on le parcourt en creant son entete au prealable
	if (S_ISDIR(st_src.st_mode))
	{
		DIR * repr = opendir(src);
		if (repr == NULL)
		{
			perror("");
			return 0;
		}
		int index_re = strlen(src) - 1;
		if (src[index_re]=='/')
			index_re--;
		while (index_re > 0 && src[index_re] != '/')
		{
			index_re--;
		}
		index_re++;
		char * new_repr = malloc(strlen(src) + strlen(destination) + 4);
		if (index_tar != strlen(destination))
			sprintf(new_repr, "%s/%s",&destination[index_tar],&src[index_re]);
		else
			sprintf(new_repr, "%s",&src[index_re]);
		creation_fichier_tar(tar,new_repr,entete_dossier(new_repr,st_src));
		free(new_repr);
		char * new_destination = malloc(strlen(destination) + strlen(src)+6);
		if (destination[strlen(destination)-1]!= '/')
			sprintf(new_destination,"%s/%s/", destination, &src[index_re]);
		else
			sprintf(new_destination,"%s%s/", destination, &src[index_re]);
		struct dirent * sleep = readdir(repr);
	  	while (sleep)
	  	{
	    	if (strcmp(sleep->d_name,".") && strcmp(sleep->d_name,".."))
	    	{
		      	struct stat st;
		      	char * absolute_name = malloc(strlen(src)+strlen(sleep->d_name)+4);
				if (src[strlen(src)-1] == '/')
					sprintf(absolute_name,"%s%s",src,sleep->d_name);
				else
					sprintf(absolute_name,"%s/%s",src,sleep->d_name);
		      	if(stat(absolute_name,&st)==-1)
	      		{
					char * error = malloc(strlen(absolute_name)+50);
					sprintf(error,"stat cp_file_to_tar %s",absolute_name);
		        	perror("stat cp_file_to_tar");
		        	free(absolute_name);
		        	return -1;
	      		}
				//Si c'est un dossier, on rappelle cp_file_to_tar sur le dossier
		      	if (S_ISDIR(st.st_mode))
		      	{
					cp_file_to_tar(absolute_name, new_destination, option);
		      	}
				//Sinon on appelle la fonction auxiliaire
		      	else
		      	{
					cp_file_to_tar_aux(absolute_name,new_destination,st);
		      	}
	      		free(absolute_name);
	    	}
	    	sleep = readdir(repr);
	  	}
	  	closedir(repr);
	  	free(sleep);
	  	return 0;
	}
	//Src != dossier => appel a cp_file_to_tar_aux
	else
	{
		cp_file_to_tar_aux(src,destination,st_src);
		return 0;
	}
}
/*
Copie le fichier src, present dans un tarball, vers destination, qui est en dehors
d'un tarball
*/
int cp_tar_to_file(char *src, char *destination,int option)
{
	int index_tar_src = recherche_fich_tar(src);
	//Copie d'un tar sans option -r -> erreur
	if (index_tar_src == strlen(src) && option == 0)
	{
		char * error = malloc(strlen(src) + 50);
		sprintf(error,"cp : -r non specifie : omission du repertoire %s\n", src);
		write(STDERR_FILENO,error,strlen(error));
		free(error);
		return 1;
	}
	char *tar = malloc(strlen(src) + 2);
	strncpy(tar,src,index_tar_src);
	tar[index_tar_src] = '\0';
	if (tar[index_tar_src - 1] == '/')
	{
		tar[index_tar_src - 1] = '\0';
	}
	//Copie de fichier dans tar a file
	if (index_tar_src != strlen(src))
	{
		struct stat st_dest;
		if (stat(destination,&st_dest)==-1)
		{
			perror("Destination cp_tar_to_file");
			return 1;
		}
		if (S_ISDIR(st_dest.st_mode))
		{
			//On trouve le nom du nouveau fichier
			int index_src = strlen(src) - 1;
			if (src[index_src - 1] == '/')
				index_src--;
			while (index_src != 0 && src[index_src] != '/')
			{
				index_src--;
			}
			index_src++;
			char *file_name = malloc(strlen(destination) + strlen(src) + 10);
			if (destination[strlen(destination)-1] != '/')
				sprintf(file_name,"%s/%s",destination, &src[index_src]);
			else
				sprintf(file_name,"%s%s",destination, &src[index_src]);
			struct posix_header entete = recuperer_entete(tar,&src[index_tar_src]);
			//Si le fichier n'a pas d'entete ou que son typeflag == 5, c'est un repertoire
			if (entete.name[0] == '\0' || entete.typeflag == '5')
			{
				mode_t mode_dos;
				//Le repertoire n'a pas d'entete -> on lui attribue les droits max
				if(entete.name[0] == '\0')
					mode_dos = S_IRWXU | S_IRWXG | S_IRWXO;
				else
					mode_dos = perm_mode(entete.mode);
				//Creation ou verification de l'existence du repertoire
				if (mkdir(file_name, mode_dos) == -1)
				{
					//
					if (errno != EEXIST)
					{
						perror("cp ");
						return 1;
					}
					//On verifie que le fichier existant est bien un dossier
					struct stat st;
					if (stat(file_name,&st) != -1)
					{
						if (!S_ISDIR(st.st_mode))
						{
							write(STDERR_FILENO,"Fichier != repertoire deja present\n",36);
							return 1;
						}
					}
					else
					{
						perror("Stat file name ");
						return 1;
					}
				}
				/*
				On parcourt les fichiers presents dans le repertoire et appelle
				recursivement cp_tar_to_file sur le fichier
				*/
				char **ss_dossier = sous_dossiers_directs(src,tar);
				char *new_destination = malloc(strlen(destination) + strlen(src)+6);
				if (destination[strlen(destination)-1] != '/')
					sprintf(new_destination,"%s/%s",destination,&src[index_src]);
				else
					sprintf(new_destination,"%s%s",destination,&src[index_src]);
				int i = 0;
				while (ss_dossier[i] != NULL)
				{
					//Retrouver le nom de l'entete
					char *file_in_repr = malloc(strlen(src) + strlen(ss_dossier[i])+8);
					if (src[strlen(src) - 1] != '/')
						sprintf(file_in_repr,"%s/%s", &src[index_tar_src],ss_dossier[i]);
					else
					{
						sprintf(file_in_repr,"%s%s", &src[index_tar_src],ss_dossier[i]);
					}
					struct posix_header entete = recuperer_entete(tar,file_in_repr);
					char *new_src = calloc(strlen(src) + strlen(ss_dossier[i])+10,sizeof(char));
					if (src[strlen(src)-1] == '/')
						sprintf(new_src,"%s%s",src,ss_dossier[i]);
					else
						sprintf(new_src,"%s/%s",src,ss_dossier[i]);
					if (entete.name[0] == '\0' || entete.typeflag == '5')
					{
						char * newest_dest = malloc(strlen(new_destination)+strlen(src)+3);
						if (new_destination[strlen(new_destination) -1] == '/')
							sprintf(newest_dest, "%s%s", new_destination,ss_dossier[i]);
						else
							sprintf(newest_dest, "%s/%s", new_destination,ss_dossier[i]);
						mode_t m_ssd;
						if(entete.name[0] == '\0')
							m_ssd = S_IRWXU | S_IRWXG | S_IRWXO;
						else
							m_ssd = perm_mode(entete.mode);
						mkdir(newest_dest,m_ssd);
						cp_tar_to_file(new_src,newest_dest,option);
					}
					else
					{
						cp_tar_to_file(new_src,new_destination,option);
					}
					free(file_in_repr);
					free(new_src);
					i++;
				}
				free(new_destination);
				for (int j = 0; j < i; j++)
				{
					free(ss_dossier[i]);
				}
				free(ss_dossier);
			}
			else
			{
				mode_t mode = perm_mode(entete.mode);
				int fd_dest = open(file_name,O_WRONLY | O_TRUNC | O_CREAT ,mode);
				if (fd_dest == -1)
				{
					perror("Erreur cp_tar_to_file");
					return 1;
				}
				affiche_fichier_tar(tar,&src[index_tar_src],fd_dest);
				close(fd_dest);
				free(tar);
				free(file_name);
			}
		}
		//Copie vers fichier
		else
		{

		}
	}
	//Copie tarball en entier
	else
	{
		char **list = list_fich(src);
		int i = 0;
		while (list[i] != NULL)
		{
			char * new_src = malloc(strlen(list[i])+strlen(src) + 6);
			if (src[strlen(src) -1] == '/')
				sprintf(new_src,"%s%s",src, list[i]);
			else
				sprintf(new_src,"%s/%s",src, list[i]);
			cp_tar_to_file(new_src,destination,option);
			i++;
		}
		for (int j = 0; j < i; j++)
		{
			free(list[j]);
		}
		free(list);
	}
	return 0;
}
/*
Copie des fichiers d'un contexte tar vers un autre contexte tar
*/
int cp_tar_to_tar(char *src, char *destination,int option)
{
	//Utilisation d'un repertoire auxiliaire
	if (mkdir(".cp_tar_to_tar",S_IRWXO | S_IRWXG | S_IRWXU) == -1)
	{
		perror("cp_tar_to_tar mkdir");
		return 1;
	}
	cp_tar_to_file(src,".cp_tar_to_tar", option);
	DIR * repr = opendir(".cp_tar_to_tar");
	struct dirent * encore = readdir(repr);
	while (encore)
	{
		if (strcmp(encore->d_name,".") && strcmp(encore->d_name,".."))
		{
			char * new_file = malloc(30 + strlen(encore->d_name));
			sprintf(new_file,".cp_tar_to_tar/%s",encore->d_name);
			cp_file_to_tar(new_file,destination,1);
			free(new_file);
		}
		encore = readdir(repr);
	}
	free(encore);
	closedir(repr);
	int fils = fork();
	if (fils == 0)
	{
		execlp("rm","rm",".cp_tar_to_tar", "-rf", NULL);
		exit(0);
	}
	wait(NULL);
	return 0;
}
/*
Supprime le fichier file contenu dans un tarball si l'option le permet
*/
int supprimer_fichier(char *file, int option, shell *tsh)
{

	int index = recherche_fich_tar(file);
	char *tar = malloc(strlen(file));
	strncpy(tar,file,index);
	//On enleve le / du tar
	if (tar[index-1]=='/')
	{
		tar[index-1] = '\0';
	}
	//Si on doit supprimer un fichier  .tar
	if (index == strlen(file))
	{
		//Considerant les .tar comme des dossiers, on attend l'option pour le supprimer
		if (option == RM_R)
		{
			file[strlen(file)] = '\0';
			if(unlink(file)==-1)
			{
				char *error = malloc(strlen(file)+strlen("rm  :"));
				sprintf(error,"rm %s :",file);
				perror(error);
				free(error);
			}
		}
		else
		{
			if (option == RM_DIR)
			{
				char ** list = list_fich(tar);
				if (list[0]==NULL)
				{
					supprimer_fichier(file, RM_R,tsh);
				}
				else
				{
					char *error= malloc(strlen("rmdir  : Tar non vide\n")
					+ strlen(file)+1);
					sprintf(error,"rmdir %s: Tar non vide\n",file);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
				}
			}
			else
			{

				char *error= malloc(strlen("rm  : Veuillez utiliser l'option -r pour supprimer les .tar\n")
				+ strlen(file)+1);
				sprintf(error,"rm %s: Veuillez utiliser l'option -r pour supprimer les .tar\n",file);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
			}
		}
	}
		//Suppression dans un .tar
	else
	{
		char *file_to_rm = malloc(strlen(file)-index + 3);
		strcpy(file_to_rm,&file[index]);
		if(tar[strlen(tar)-1]=='/')
			tar[strlen(tar)-1] = '\0';
		supprimer_fichier_tar(tar,file_to_rm,option);
		free(file_to_rm);
	}
	free(tar);
	return 0;
}
int mkdir_tar(char *file, char **options,shell *tsh)
{
	char * fichier= malloc(strlen(file)+3+strlen(tsh->repertoire_courant));
	sprintf(fichier,"%s%s",tsh->repertoire_courant,file);
	if (file[strlen(file)-1]!='/')
	{
		strcat(fichier,"/");
		file[strlen(file)-1] = '\0';
	}
	if (cheminValide(fichier,"mkdir")==1)
	{
		char error[strlen(file)+strlen("mkdir  impossible : deja existant\n") + 6];
		sprintf(error,"mkdir %s impossible : deja existant\n",file);
		write(STDERR_FILENO,error,strlen(error));
		free(fichier);
		return 1;
	}
	int index1 = strlen(fichier) - 1;
	if (fichier[index1] == '/')
	{
		index1--;
	}
	while (fichier[index1] != '/')
	{
		index1--;
	}
	index1++;
	char * name_repr = malloc(strlen(fichier));
	sprintf(name_repr,"%s",&fichier[index1]);
	//Dossier parent du repertoire que l'on veut creer
	char *parent = malloc(strlen(fichier)+1);
	strncpy(parent,fichier,index1);
	parent[index1-1] = '\0';
	if (cheminValide(parent,"mkdir")==0)
	{
		erreur_chemin_non_valide(file,"mkdir");
		free(parent);
		free(name_repr);
		free(fichier);
		return 1;
	}
	sprintf(fichier,"%s",simplifie_chemin(fichier));
	int index = recherche_fich_tar(fichier);
	//-1 signifie que le chemin ne contient pas de tarball
	if (index == -1)
	{
		if (mkdir(fichier, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		{
			char * error = malloc(strlen(file) + 20);
			sprintf(error,"mkdir %s:",file);
			perror(error);
			free(error);
			free(fichier);
			free(parent);
			free(name_repr);
			return 1;
		}
		return 0;
	}
	//Creation d'un tarball
	if (index == strlen(fichier))
	{
		fichier[index-1] = '\0';
		int fd = creat(fichier, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (fd == -1)
		{
			char * error = malloc(strlen(file) + 15);
			sprintf(error,"mkdir %s", file);
			perror(error);
			free(error);
			return 1;
		}
		char buffer[512];
		memset(buffer,0,512);
		for(int i = 0; i < 20; i++)
		{
			read(fd,buffer,512);
		}
		close(fd);
	}
	else
	{
		char *tar = malloc(strlen(fichier));
		strncpy(tar,fichier,index);
		tar[index-1] = '\0';
		char *repr_to_create = malloc (strlen(fichier)-index+3);
		strncpy(repr_to_create,&fichier[index],strlen(fichier)-index+1);
		repr_to_create[strlen(fichier)-index+1] = '\0';
		if (repr_to_create[strlen(repr_to_create)-1] != '/')
		{
			strcat(repr_to_create,"/");
			repr_to_create[strlen(fichier)-index] = '\0';
		}
		//Creation de l'entete
		struct posix_header hd;
		memset(&hd,0,sizeof(struct posix_header));
		sprintf(hd.name,"%s",repr_to_create);
		sprintf(hd.mode,"0000777");
	  	hd.typeflag = '5';
		time_t date = time(NULL);
		sprintf(hd.mtime,"%011lo",date);
		sprintf(hd.uid,"%d",getuid());
		sprintf(hd.gid,"%d",getgid());
		sprintf(hd.uname,"%s",getpwuid(getuid())->pw_name);
		sprintf(hd.gname,"%s",getgrgid(getgid())->gr_name);
		sprintf(hd.size,"%011o",0);
	  	strcpy(hd.magic,"ustar");
		set_checksum(&hd);
		if (!check_checksum(&hd))
			perror("Checksum impossible");
		creation_fichier_tar(tar,repr_to_create,hd);
		free(fichier);
		free(tar);
		free(repr_to_create);

	}
	return 0;
}
/*
Affiche le contenu du fichier file en argument si cela est possible et renvoie 0.
Sinon, il renvoie 1 quand l'affichage n'est pas possible
*/
int cat(char *file, char **options,shell *tsh)
{
	char * fich = malloc(strlen(file)+strlen(tsh->repertoire_courant)+3);
	sprintf(fich,"%s%s",tsh->repertoire_courant,file);
	sprintf(fich,"%s",simplifie_chemin(fich));
	if (estTarball(fich))
	{
		char *error = malloc(strlen(file)+
						strlen("cat %s : Pas de cat sur un .tar\n"));
		sprintf(error,"cat %s : Pas de cat sur un .tar\n",file);
		write(STDERR_FILENO,error,strlen(error));
		free(fich);
		return 1;
	}
	//Fichier dans un .tar
	else
	{
		//Recherche du .tar contenant l'argument
		char *tar = calloc(strlen(fich)+3,sizeof(char));
		int index = recherche_fich_tar(fich);
		strncpy(tar,fich,index);
		tar[strlen(tar)-1] = '\0';
		char *file_to_find = malloc(strlen(fich) + 2);
		strcpy(file_to_find,&fich[index]);
		if (file[strlen(file)-1]=='/')
			file_to_find[strlen(file_to_find)-1] = '\0';
		else
			file_to_find[strlen(file_to_find)] = '\0';
		affiche_fichier_tar(tar,file_to_find, STDOUT_FILENO);
		free(tar);
		free(file_to_find);
	}

	return 0;
}
