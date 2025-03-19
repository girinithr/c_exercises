/*Exercise 7-7. Modify the pattern finding program of Chapter 5 to take its input from a set of
named files or, if no files are named as arguments, from the standard input. Should the file
name be printed when a matching line is found?*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXFILES 10  /* maximum number of files to search in */
#define MAXLINE 1024 /* longest line that can be read at once + 1 for '\0' */

struct file {
	FILE *p;
	char *name;
};

int main(int argc, char *argv[])
{
	struct file files[MAXFILES + 1];
	struct file *fp = files;
	char **argp;
	char *pat;
	char line[MAXLINE];
	int c;
	int found  = 0, except = 0, number = 0;
	long line_num;

	if(argc < 2) {
		fprintf(stderr, "usage: %s -x -n [file1] [file2] ... pattern\n",
				argv[0]);
		exit(-1);
	}

	/* get pattern */
	pat = argv[--argc];

	/* open files and read arguments */
	for(argp = argv + 1; argp - argv < argc; ++argp) {
		/* read arguments */
		if(*argp[0] == '-') {
			while((c = *++argp[0]))
				switch(c) {
				case 'x':
					except = 1;
					break;
				case 'n':
					number = 1;
					break;
				default:
					fprintf(stderr, "%s: illegal option %c\n",
						argv[0], c);
					fprintf(stderr,
						"usage: %s -x -n [file1] [file2] ... pattern\n",
						argv[0]);
					exit(-1);
				}
		}
		/* read filenames */
		else {
			if(fp - files >= MAXFILES) {
				fprintf(stderr, "%s: can only open %d files\n", argv[0],
						MAXFILES);
				exit(-1);
			}
			if((fp->p = fopen(*argp, "r")) == NULL) {
				fprintf(stderr, "%s: error opening %s\n", argv[0], *argp);
				exit(-1);
			}
			else
				fp++->name = *argp;
		}
	}

	/* if there were no filenames, read from stdin */
	if(fp == files) {
		fp++->p = stdin;
	}
	fp->p = NULL;    /* put NULL pointer at end of array */

	/* search for pattern in each file */
	for(fp = files; fp->p != NULL; ++fp) {
		line_num = 0;
		while(fgets(line, MAXLINE, fp->p) != NULL) {
			++line_num;
			if((strstr(line, pat) != NULL) != except) {
				if(fp->p != stdin)
					printf("%s ", fp->name);
				if(number)
					printf("%ld", line_num);
				if(number || fp->p != stdin)
					putchar(':');
				puts(line);
				++found;
			}
		}
	}

	/* clean up */
	for(fp = files; fp->p != NULL; ++fp)
		fclose(fp->p);
	return found;
} /* end of main */
