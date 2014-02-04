
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <process.h>
#include <io.h>

/*
 This program scans the sub-directories in the gsl root directory 
 (except the gsl subdirectory) for headers with names matching 
 gsl_*.h and then processes each such header file to find any
 functions that need to be exported from the DLL versions of
 gsl and cblas. It copies these header files without changes into 
 the gsl sub-directory and writes two definition files that are
 needed for DLL generation.  It employs a simplified function 
 detection method that depends on the format of gsl headers.
*/

void process_file(FILE *inf, FILE *gslf, FILE *blsf)
{   char buf[4096], *bp, *b0, *b1, *p_sym[5], *q_sym[5], *nl0, *nl1;
    int n_sym = 0, in_sym = 0, in_pars = 0, in_fun = 0, in_com = 0,
        in_data = 0, in_inhib = 1, cb = 0, sep, lsep, t, ch;

    nl0 = bp = buf; ch = fgetc(inf);
    while(!feof(inf) && ch != EOF)
    {
        *bp = ch;
        if(in_com)	/* in comment */
        {
            if(bp > buf && ch == '/' && *(bp - 1) == '*')
                in_com = 0;
            ++bp;
        }
        else if(in_pars)	/* in function parameter list	*/
        {
            if(ch == '(')	/* count of open brackets		*/
                ++cb;
            else if(ch == ')' && !cb--)	
            {				/* end of parameter list		*/
                in_pars = 0; cb = 0;
                if(n_sym > 1)	/* a function declaration   */
                    in_fun = 1;
            }
            ++bp;
        }
        else if(in_sym)		/* in a symbol */
        {
            if(!iscsym(ch))	/* not a C identifier character */
            {
                q_sym[n_sym - 1] = bp;
                in_sym = 0;
                lsep = sep; sep = 1;
                if(!strncmp(p_sym[n_sym - 1], "GSL_VAR", 7))
                    in_data = 1;		/* in data */
                if(!strncmp(p_sym[n_sym - 1], "inline", 6))
                    in_inhib = 1;		/* inhibit scanning */
                if(!strncmp(p_sym[n_sym - 1], "typedef", 7))
                    in_inhib = 1;		/* inhibit scanning */
                if(!strncmp(p_sym[n_sym - 1], "__BEGIN_DECLS", 13))
                    if(p_sym[n_sym - 1] == nl0)
                        in_inhib = 0;	/* start scanning */
                continue;
            }
            ++bp;
        }
        else if(ch == '(')	/* start of parameter list */
        {
            in_pars = 1; ++bp;
        }
        else if(iscsym(ch))
        {	/* save the start and end positions of up last 5 identifiers found */
            if(n_sym < 5)
                p_sym[n_sym++] = bp++;
            else
            {
				/* discard buffer to end of first symbol and adjust the	*/
				/* symbol pointers accordingly							*/
                t = q_sym[0] - buf; b0 = q_sym[0];
                p_sym[0] = p_sym[1] - t;
                q_sym[0] = q_sym[1] - t;
                p_sym[1] = p_sym[2] - t;
                q_sym[1] = q_sym[2] - t;
                p_sym[2] = p_sym[3] - t;
                q_sym[2] = q_sym[3] - t;
                p_sym[3] = p_sym[4] - t;
                q_sym[3] = q_sym[4] - t;
                nl0 -= t;	/* adjust new line pointers */
                nl1 -= t;
                b1 = buf;	/* adjust buffer contents   */
                while(b0 < bp)
                    *b1++ = *b0++;
                *b1 = *bp;
                bp = b1;
                p_sym[n_sym - 1] = bp;
                ++bp;
            }
            in_sym = 1;
        }
        else if(ch == ';')	/* end of declaration */
        {
            b0 = buf;
            if(!in_inhib && n_sym > 1 && (in_fun && lsep || in_data))
            {
                if(!strncmp(p_sym[n_sym - 1], "gsl_", 4))
                {
                    if(q_sym[n_sym - 2] - p_sym[n_sym - 2] != 2
                        || strncmp(p_sym[n_sym - 2], "do", 2))
                    {
                        fprintf(gslf, "\n\t");
                        b0 = p_sym[n_sym - 1];
                        b1 = q_sym[n_sym - 1];
                        while(b0 < b1)
                            fputc(*b0++, gslf);
                        if(in_data)
                            fprintf(gslf, " DATA");
                    }
                }
                if(!strncmp(p_sym[n_sym - 1], "cblas_", 6))
                {
                    if(q_sym[n_sym - 2] - p_sym[n_sym - 2] != 2
                        || strncmp(p_sym[n_sym - 2], "do", 2))
                    {
                        fprintf(blsf, "\n\t");
                        b0 = p_sym[n_sym - 1];
                        b1 = q_sym[n_sym - 1];
                        while(b0 < b1)
                            fputc(*b0++, blsf);
                        if(in_data)
                            fprintf(blsf, " DATA");
                    }
                }
            }
            bp = buf; n_sym = 0; in_fun = in_data = in_inhib = 0;
        }
        else if(bp > buf && ch == '*' && *(bp - 1) == '/')
        {
            in_com = 1;
            ++bp;
        }
        else if(!strchr("*& \t\r\n", ch))
        {
            sep = 0; ++bp;
        }
        else if(ch == '\r' || ch == '\n')
        {
            ++bp; nl1 = nl0; nl0 = bp;
        }
        else
            ++bp;
        ch = fgetc(inf);
    }
}

void proc_hdrs(char *dir, char *cdir, FILE *gslf, FILE *blsf)
{   struct _finddata_t cfile;
    intptr_t hfile;
    char nbuf[_MAX_PATH], *p;
	FILE *inf, *outf;

	/* prepare to find all files and folders in directory 'dir' */
	strcpy(nbuf, dir);
	strcat(nbuf, "\\*.*");
    if((hfile = _findfirst( nbuf, &cfile )) == -1L )
		return;		/* none found */

	do
	{
		strcpy(nbuf, dir);	/* compose full path to current file or sub-directory	*/
		strcat(nbuf, "\\");
		strcat(nbuf, cfile.name);

		if((cfile.attrib & _A_SUBDIR) == _A_SUBDIR)	/* if it is a sub-directory		*/
		{
			if(strcmp(cfile.name, ".") && strcmp(cfile.name, "..") && strcmp(cfile.name, "gsl"))
				proc_hdrs(nbuf, cdir, gslf, blsf);	/* and not ".", ".." or "gsl"	*/
		}
		else if((cfile.attrib & 0x1f) == _A_NORMAL && (p = strrchr(cfile.name, '.'))
					&& !strcmp(p, ".h") && !strncmp(cfile.name, "gsl_", 4))
		{	
			size_t len;						/* if a normal file matching "gsl_*.h"	*/

			if(!(inf = fopen(nbuf, "r")))	/* open it for input */
			{
				printf("\nFile open error on: %s for reading", cfile.name); 
				break;
			}
			
	        process_file(inf, gslf, blsf);	/* look for function definitions */
		    fclose(inf);

			if(!(inf = fopen(nbuf, "rb")))	/* open input for binary copying */
			{
				printf("\nFile open error on: %s for reading", cfile.name); 
				break;
			}
			strcpy(nbuf, cdir);
			strcat(nbuf, "\\");
			strcat(nbuf, cfile.name);
			if(!(outf = fopen(nbuf, "wb")))	/* open output for writing */
			{
				printf("\nFile open error on %s for writing", nbuf); 
				break;
			}

			do	/* copy the file */
			{
				len = fread(nbuf, 1, _MAX_PATH, inf);
				len = fwrite(nbuf, 1, len, outf);
			}
			while
				(len);
			fclose(inf);
			fclose(outf);
		}
	}
    while
        ( _findnext( hfile, &cfile ) == 0 );	/* goto next file or sub-directory */

    _findclose( hfile );
}

int main(void)
{	char cdir[_MAX_PATH], *dir, *p;
    FILE *gslf, *blsf;
	int err = 0;

	if(!(dir = _getcwd( NULL, 0 )))
	{
		printf("Cannot obtain current working directory\n"); goto exit4;
	}

	if(((p = strrchr(dir, '\\')) || (p = strrchr(dir, '/'))) && strncmp(p, "\\gsldefs", 8))
	{
		printf("Incorrect working directory: %s\n", dir); goto exit3;
	}
	else
	{
		*p = '\0';
		if(((p = strrchr(dir, '\\')) || (p = strrchr(dir, '/'))) && strncmp(p, "\\build.vc10", 10))
		{
			printf("Incorrect working directory: %s\n", dir); goto exit3;
		}
	}

	*++p = '\0';
	strcpy(cdir, dir);
	strcat(cdir, "\\gsl");

	if(!(gslf = fopen("gsl.def", "w")))		/* create GSL defintion file */ 
	{
        printf("File open error on: gsl.def for writing\n"); goto exit2;
	}

    if(!(blsf = fopen("cblas.def", "w")))	/* create CBLAS definition file */
	{
        printf("File open error on: cblas.def for writing\n"); goto exit1;
	}

    fprintf(gslf, "LIBRARY gsl\nEXPORTS");
    fprintf(blsf, "LIBRARY cblas\nEXPORTS");

	proc_hdrs(dir, cdir, gslf, blsf);

	fprintf(gslf, "\n");
    fprintf(blsf, "\n"); 
	printf("\n\n");
	err = 4;

exit1:
    --err; fclose(blsf);
exit2:
    --err; fclose(gslf);
exit3:
    --err; free(dir);
exit4:
	return --err;
}
