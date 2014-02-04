
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <io.h>

/*
 This program scans the sub-directories in the gsl root directory 
 (except the gsl subdirectory) for headers with names matching 
 gsl_*.h and then processes each such header file to find any
 functions that need to be exported from the DLL versions of
 gsl and cblas. It copies these header files into the gsl 
 sub-directory in a modified form in which each such function 
 definition is annotated with the symbol GSL_FUN. It also adds
 a section to each header file to allow this this symbol to be 
 defined for DLL import or DLL export when necessary.  
*/

void process_file(FILE *inf, FILE *outf)
{   char buf[4096], *bp, *b0, *b1, *p_sym[5], *q_sym[5], *nl0, *nl1;
    int n_sym = 0, in_sym = 0, in_pars = 0, in_fun = 0, in_com = 0,
        in_data = 0, in_inhib = 1, cb = 0, sep, lsep, t, ch;

    nl0 = bp = buf; ch = fgetc(inf);
    while(!feof(inf) && ch != EOF)
    {
        *bp = ch;
        if(in_com)
        {
            if(bp > buf && ch == '/' && *(bp - 1) == '*')
                in_com = 0;
            ++bp;
        }
        else if(in_pars)
        {
            if(ch == '(')
                ++cb;
            else if(ch == ')' && !cb--)
            {
                in_pars = 0; cb = 0;
                if(n_sym > 1)
                    in_fun = 1;
            }
            ++bp;
        }
        else if(in_sym)
        {
            if(!iscsym(ch))
            {
                q_sym[n_sym - 1] = bp;
                in_sym = 0;
                lsep = sep; sep = 1;
                if(!strncmp(p_sym[n_sym - 1], "GSL_VAR", 7))
                    in_data = 1;
                if(!strncmp(p_sym[n_sym - 1], "inline", 6))
                    in_inhib = 1;
                if(!strncmp(p_sym[n_sym - 1], "typedef", 7))
                    in_inhib = 1;
                if(!strncmp(p_sym[n_sym - 1], "__BEGIN_DECLS", 13))
                    if(p_sym[n_sym - 1] == nl0)
                        in_inhib = 0;
                continue;
            }
            ++bp;
        }
        else if(ch == '(')
        {
            in_pars = 1; ++bp;
        }
        else if(iscsym(ch))
        {
            if(n_sym < 5)
                p_sym[n_sym++] = bp++;
            else
            {
                t = q_sym[0] - buf; b0 = q_sym[0];
                p_sym[0] = p_sym[1] - t;
                q_sym[0] = q_sym[1] - t;
                p_sym[1] = p_sym[2] - t;
                q_sym[1] = q_sym[2] - t;
                p_sym[2] = p_sym[3] - t;
                q_sym[2] = q_sym[3] - t;
                p_sym[3] = p_sym[4] - t;
                q_sym[3] = q_sym[4] - t;
                nl0 -= t; nl1 -= t;
                b1 = buf;
                while(b1 < b0)
                    fputc(*b1++, outf);
                b1 = buf;
                while(b0 < bp)
                    *b1++ = *b0++;
                *b1 = *bp;
                bp = b1;
                p_sym[n_sym - 1] = bp;
                ++bp;
            }
            in_sym = 1;
        }
        else if(ch == ';')
        {
            b0 = buf;
            if(!in_inhib && n_sym > 1 && in_fun && lsep)
            {
                if(!strncmp(p_sym[n_sym - 1], "gsl_", 4))
                {
                    if(q_sym[n_sym - 2] - p_sym[n_sym - 2] != 2
                        || strncmp(p_sym[n_sym - 2], "do", 2))
                    {
                        b1 = nl0 > p_sym[n_sym - 2] ? nl1 : nl0;
                        while(b0 < b1)
                            fputc(*b0++, outf);
                        fputs("GSL_FUN ", outf);
                    }
                }
                if(!strncmp(p_sym[n_sym - 1], "cblas_", 6))
                {
                    if(q_sym[n_sym - 2] - p_sym[n_sym - 2] != 2
                        || strncmp(p_sym[n_sym - 2], "do", 2))
                    {
                        b1 = nl0 > p_sym[n_sym - 2] ? nl1 : nl0;
                        while(b0 < b1)
                            fputc(*b0++, outf);
                        fputs("CBL_FUN ", outf);
                    }
                }
            }
            while(b0 <= bp)
                fputc(*b0++, outf);
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
    b0 = buf;
    while(b0 < bp)
        fputc(*b0++, outf);
}

char *gsl_insert = 
	"\n"
	"#if !defined( GSL_FUN )\n"
	"#  if !defined( GSL_DLL )\n"
	"#    define GSL_FUN extern\n"
	"#  elif defined( BUILD_GSL_DLL )\n"
	"#    define GSL_FUN extern __declspec(dllexport)\n"
	"#  else\n"
	"#    define GSL_FUN extern __declspec(dllimport)\n"
	"#  endif\n"
	"#endif\n";

char *cbl_insert = 
	"#if !defined( CBL_FUN )\n"
	"#  if !defined( CBLAS_DLL )\n"
	"#    define CBL_FUN extern\n"
	"#  elif defined( BUILD_CBLAS_DLL )\n"
	"#    define CBL_FUN extern __declspec(dllexport)\n"
	"#  else\n"
	"#    define CBL_FUN extern __declspec(dllimport)\n"
	"#  endif\n"
	"#endif\n";

void proc_hdrs(char *dir, char *cdir)
{   struct _finddata_t cfile;
    intptr_t hfile;
    char nbuf[_MAX_PATH], *p;

	strcpy(nbuf, dir);
	strcat(nbuf, "\\*.*");
    if((hfile = _findfirst( nbuf, &cfile )) == -1L )
		return;

	do
	{
		strcpy(nbuf, dir);
		strcat(nbuf, "\\");
		strcat(nbuf, cfile.name);

		if((cfile.attrib & _A_SUBDIR) == _A_SUBDIR)
		{
			if(strcmp(cfile.name, ".") && strcmp(cfile.name, "..") && strcmp(cfile.name, "gsl"))
				proc_hdrs(nbuf, cdir);
		}
		else if((cfile.attrib & 0x1f) == _A_NORMAL && (p = strrchr(cfile.name, '.'))
					&& !strcmp(p, ".h") && !strncmp(cfile.name, "gsl_", 4))
		{
		    FILE *inf, *outf;
		    char iobuf[256], hdr_line[128], *q;
			if((inf = fopen(nbuf, "r")) == 0)
			{
				printf("\nFile open error on: %s for reading", cfile.name); 
				break;
			}

			strcpy(nbuf, cdir);
			strcat(nbuf, "\\");
			strcat(nbuf, cfile.name);
			if((outf = fopen(nbuf, "w")) == 0)
			{
				printf("\nFile open error on %s for writing", nbuf); 
				break;
			}
			q = hdr_line;
			p = cfile.name;
			*q++ = '_'; *q++ = '_';
			while(*p && *p != '.')
				*q++ = toupper(*p++);
			*q = '_'; *q = '\0';

			do
				if(!feof(inf))
				{
					fgets(iobuf, 256, inf);
					p = iobuf;
					while(*p)
						fputc(*p++, outf);
				}
			while
				(!(strstr(iobuf, "#define") && strstr(iobuf, hdr_line)) && !feof(inf));

			if(!feof(inf))
			{
				p = strcmp(cfile.name, "gsl_cblas.h") ? gsl_insert : cbl_insert;
				while(*p)
					fputc(*p++, outf);

				process_file(inf, outf);
			}
			fclose(inf);
			fclose(outf);
		}
	}
    while
        ( _findnext( hfile, &cfile ) == 0 );

    _findclose( hfile );
}

int main(void)
{	char cdir[_MAX_PATH], *dir, *p;
	int err = 0;

	if(!(dir = _getcwd( NULL, 0 )))
	{
		printf("Cannot obtain current working directory\n"); 
		err = 1;
		goto exit2;
	}

	if(((p = strrchr(dir, '\\')) || (p = strrchr(dir, '/'))) && strncmp(p, "\\gslhdrs", 8))
	{
		printf("Incorrect working directory: %s\n", dir); 
		err = 2;
		goto exit1;
	}
	else
	{
		*p = '\0';
		if(((p = strrchr(dir, '\\')) || (p = strrchr(dir, '/'))) && strncmp(p, "\\build.vc10", 10))
		{
			printf("Incorrect working directory: %s\n", dir); 
			err = 3;
			goto exit1;
		}
	}

	*++p = '\0';
	strcpy(cdir, dir);
	strcat(cdir, "\\gsl");
	proc_hdrs(dir, cdir);

    printf("\n\n");
	err = 0;
exit1:
	free(dir);
exit2:
    return -err;
}
