#define _POSIX_C_SOURCE 200809L // required for PATH_MAX on cslab

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

/* create a new string dir/file */
char *build_path(char *dir, char *file);

/* create an archive of dir in the archive file */
void create(FILE *archive, char *dir, int verb);

/* read through the archive file and recreate files and directories from it */
void extract(FILE *archive, int verb);

/* standard usage message and exit */
void usage();

/* create a new string dir/file */
char *build_path(char *dir, char *file)
{
	// allocate space for dir + "/" + file + \0 
	int path_length = strlen(dir) + 1 + strlen(file) + 1;
	char *full_path = (char *)malloc(path_length*sizeof(char));

	// fill in full_path with dir then "/" then file
	strcpy(full_path, dir);
	strcat(full_path, "/");
	strcat(full_path, file);

	// be sure to free the returned memory when it's no longer needed
	return full_path;
}

void dir_write(FILE *archive, struct stat finfo, char *dirname, DIR *dir)
{
	fprintf(archive, "%s\n", dirname);
	if ((fwrite(&finfo, sizeof(struct stat), 1, archive)) != 1)
	{
		perror("write");
		exit(1);
	}
}

void file_write(FILE *archive, struct stat finfo, char *filename)
{
	fprintf(archive, "%s\n", filename);
	if ((fwrite(&finfo, sizeof(struct stat), 1, archive)) != 1)
	{
		perror("write");
		exit(1);
	}
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
	{
		perror(filename);
		exit(1);
	}
	unsigned int filesize = finfo.st_size;
	char buf[filesize];
	fread(&buf, filesize, 1, fp);
	if (ferror(fp))
	{
		perror("fread");
		exit(1);
	}

	if (fwrite(buf, finfo.st_size, 1, archive) != 1)
	{
		perror("write");
		exit(1);
	}
	if (fclose(fp) != 0)	
	{
		perror("fclose");
	}
}

/* create an archive of dir in the archive file */
void create(FILE *archive, char *dir, int verb)
{
	// this is int find_size(char *dir); need to transform it into create
	// instead of adding up sizes, somehow copy every file and directory into the archive. you will also need to create this archive.
	struct stat finfo;
	struct dirent *de;
	// make archive
	DIR *d = opendir(dir);
	if (d == NULL)
	{
		perror(dir);
		exit(1);
	}
	// call stat on the directory, make sure its a directory!
	// write the given directory to the archive file 
	// how do I represent the file name?!
	if (stat(dir, &finfo) != 0)
	{
		perror(dir);
	}
	if (S_ISDIR(finfo.st_mode))
	{
		if (verb == 1)
		{
			printf("%s: processing\n", dir);
		}
		dir_write(archive, finfo, dir, d);
	}
	else
	{
		perror(dir);
	}
	for (de = readdir(d); de != NULL; de = readdir(d))
	{
		if (strcmp(de->d_name, "..") == 0)
		{
			continue;
		}
		char *full_path = build_path(dir, de->d_name);
		if (stat(full_path, &finfo) != 0)
		{
			perror(full_path);
			free(full_path);
			continue;
		}
		if (S_ISDIR(finfo.st_mode) && (strcmp(de->d_name, ".") != 0))
		{
			//recursively call create on the directory, which i think is full_path. I think de is the directory
			// and full_path is a pointer to it or something. Need to clarify this.
			create(archive, full_path, verb);
		}
		else if (S_ISREG(finfo.st_mode))
		{
			//add file to archive
			if (verb == 1)
			{
				printf("%s: processing\n", full_path);
			}
			file_write(archive, finfo, full_path);
		}
		free(full_path);
	}
	if (closedir(d) != 0)
	{
		perror("closedir");
	}
}

/* read through the archive file and recreate files and directories from it */
void extract(FILE *archive, int verb)
{
	char name[PATH_MAX];
	struct stat finfo;
	while (fscanf(archive, "%s\n", name) != EOF)
	{
		fread(&finfo, sizeof(struct stat), 1, archive);
		if (ferror(archive))
		{
			perror("fread");
			exit(1);
		}
		if (verb == 1)
		{
			printf("%s: processing\n", name);
		}
		if (S_ISDIR(finfo.st_mode))
		{
			if (mkdir(name, finfo.st_mode) == -1)
			{
				perror("mkdir");
				exit(1);
			}
		}
		if (S_ISREG(finfo.st_mode))
		{
			char buf[finfo.st_size];
			fread(buf, sizeof(char), finfo.st_size, archive);
			if (ferror(archive))
			{
				perror("fread");
				exit(1);
			}
			FILE *fp = fopen(name, "w");
			if (fp == NULL)
			{
				perror(name);
				exit(1);
			}
			if (fwrite(buf, finfo.st_size, 1, fp) != 1)
			{
				perror("write");
				exit(1);
			}
			if (fclose(fp) != 0)
			{
				perror("fclose");
			}
		}		
	}
}

/* standard usage message and exit */
void usage()
{
	printf("usage to create an archive:  tar c[v] ARCHIVE DIRECTORY\n");
	printf("usage to extract an archive: tar x[v] ARCHIVE\n\n");
	printf("  the v parameter is optional, and if given then every file name\n");
	printf("  will be printed as the file is processed\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int verbose;
	FILE *arch;
	// must have exactly 2 or 3 real arguments
	if (argc < 3 || argc > 4)
	{
		usage();
	} 

	// 3 real arguments means we are creating a new archive
	if (argc == 4)
	{ 
		// check for the create flag without the verbose flag
		if (strcmp(argv[1],"c") == 0)
		{
			verbose = 0;
		}
		// check for the create flag with the verbose flag
		else if (strcmp(argv[1],"cv") == 0)
		{
			verbose = 1;
		}
		// anything else is an error
		else
		{
			usage();
		}

		// get the archive name and the directory we are going to archive
		char *archive = argv[2];
		char *dir_name = argv[3];

		// remove any trailing slashes from the directory name
		while (dir_name[strlen(dir_name)-1] == '/')
		{
			dir_name[strlen(dir_name)-1] = '\0';
		}

		// open the archive
		arch = fopen(archive, "w");
		if (arch == NULL)
		{
			perror(archive);
			exit(1);
		}

		// start the recursive process of filling in the archive
		create(arch, dir_name, verbose);
	}

	// 2 real arguments means we are creating a new archive
	else if (argc == 3)
	{ 
		// check for the extract flag without the verbose flag
		if (strcmp(argv[1],"x") == 0)
		{
			verbose = 0;
		}
		// check for the extract flag with the verbose flag
		else if (strcmp(argv[1],"xv") == 0)
		{
			verbose = 1;
		}
		// anything else is an error
		else
		{
			usage();
		}

		// get the archive file name
		char *archive = argv[2];

		// open the archive
		arch = fopen(archive, "r");
		if (arch == NULL)
		{
			perror(archive);
			exit(1);
		}

		// start the recursive processing of this archive
		extract(arch, verbose);
	}
	if (fclose(arch) != 0)
	{
		perror("fclose");
	}

	return 0;
}
