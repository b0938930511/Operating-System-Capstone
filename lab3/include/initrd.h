#ifndef __INITRD__
#define __INITRD__



/* cpio hpodc format */
typedef struct cpio_header{
    char magic[6]; //070701
    char ino[8];  //inodes number from disk
    char mode[8]; //specifies both the regular permissions and file type
    char uid[8]; //userid
    char gid[8]; //groupid
    char nlink[8];  //number of links to this file
    char mtime[8];  //modification time
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8]; //always set to 0 by writers and ignored by reader
} cpio_header; // tell compiler don't do the alignment in order to save memory space

char *findFile(char *name);
void initrd_ls();
void initrd_cat(char *filename);
char * cpio_addr;
void load_program();
#endif