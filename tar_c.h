#ifndef TAR_C_H
#define TAR_C_H
//Option de supprimer_fichier_tar
#define RM 0 //rm sans option
#define RM_R 1 //rm avec l'option -r
#define RM_DIR 2 //rmdir
#include "tar.h"
char **list_fich(char*tar);
char **affichage_ls_l(char**,char*,int,char**);
int affiche_fichier_tar(char *tar,char*file, int fd_out);
struct posix_header recuperer_entete(char *tar, char *file);
int supprimer_fichier_tar(char *tar,char *file,int option);
int creation_fichier_tar(char *tar,char *repr,struct posix_header entete);
int estRepertoire(char *file,char *tar);
#endif
