/*
 *  makedocs.c
 *
 *  Copyright (C) 2003 by Marc Poirier and Tom Murphy 7
 *
 *  This software is released under the terms of the 
 *  GNU Public License (see COPYING for full license text)
 *
 *  This is a simple program for turning our Destroy FX 
 *  HTML documentation files into single HTML files for release.  
 *  The issue is that our HTML doc files use <link>ed files 
 *  (like our general CSS stylesheet file).  This program 
 *  dumps the contents of any linked files in place of the 
 *  <link> tags.
 *
 *  USAGE:  The first argument is the input file name 
 *  (i.e. the file that has <link> tags) and the second 
 *  argument is the consolidated output file name.  
 *  The second argument is optional (stdout is used by default).
 *
 *  BUGS:  If there is a legitimate (for example, part of a 
 *  attribute value that is in quotes) > character after the 
 *  href value inside of a link tag, the HTML parser will be 
 *  fooled into thinking that that is the end of the link tag.
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>	/* for isspace */
#include <libgen.h>	/* for basename and dirname */
#include <errno.h>	/* for error codes */


#define LINK_TAG	"link"
#define LINK_SOURCE	"href="
#define CVS_ENTRIES_FILE	"CVS/Entries"
#define CVS_ROOT_FILE	"CVS/Root"
#define CVS_REPOSITORY_FILE	"CVS/Repository"

enum {
	kArg_Command = 0,
	kArg_InputFile,
	kArg_OutputFile,

	kNumArgs
};


// hmmm, strcasestr doesn't appear to be included with BSD or Darwin...
char * dfx_strcasestr(const char *big, const char *little)
{
	if ( (big == NULL) || (little == NULL) )
		return NULL;
	int biglen = strlen(big);
	int litlen = strlen(little);
	for (int i=0; i < biglen - litlen; i++)
	{
		if (strncasecmp(big + i, little, litlen) == 0)
			return (char*) (big + i);
	}
	return NULL;
}


// try to use CVS logged data to construct an ultra-informative file header section
void writeinfoheader(FILE *outputfile, const char *inputfilename)
{
	// we're going to look for a CVS directory in the same 
	// directory where the input file is located
	char *sourcedir = dirname(inputfilename);

	// get full path and filename for the CVS Entries file
	char cvsentriesfilefullpath[strlen(sourcedir) + strlen(CVS_ENTRIES_FILE) + 4];
	sprintf(cvsentriesfilefullpath, "%s/%s", sourcedir, CVS_ENTRIES_FILE);
	// open the Entries file
	FILE *cvsentriesf = fopen(cvsentriesfilefullpath, "r");
	if (cvsentriesf != NULL)
	{
		// get the filename of the input file so that we 
		// can look for it in the CVS Entries log
		char *sourcefilebase = basename(inputfilename);
		size_t sourcefilebaselen = strlen(sourcefilebase);

		// loop through reading each line of the Entries file
		while ( !feof(cvsentriesf) )
		{
			// read the input file one line at a time
			size_t linesize;
			char *linestr = fgetln(cvsentriesf, &linesize);
			// try the next line if this one failed
			if ( (linestr == NULL) || (linesize <= (sourcefilebaselen+1)) )
				continue;

			// values in each line are separated by / characters
			// look for the first / to find where the first value begins
			long linepos = 0;
			while (linepos < (linesize-1))
			{
				if (linestr[linepos] == '/')
					break;
				linepos++;
			}
			linepos++;	// advance past the / delimiting character

			// check if the first value, the filename, matches our input filename
			if (strncmp(linestr+linepos, sourcefilebase, sourcefilebaselen) != 0)
				continue;	// move on to the next line if it doesn't match

			// we found a match, so advance past the filename value in the line
			linepos += sourcefilebaselen;
			while ( (linepos < (linesize-1)) && (linestr[linepos] != '/') )
				linepos++;
			linepos++;	// advance past the / delimiting character

			// the next value is the latest version number of the file
			char versionstr[linesize-linepos];
			long versionstrpos = 0;
			// copy the version value into our version string until a / is reached
			while ( (linepos < (linesize-1)) && (linestr[linepos] != '/') )
			{
				versionstr[versionstrpos] = linestr[linepos];
				linepos++;
				versionstrpos++;
			}
			versionstr[versionstrpos] = 0;	// terminate the string
			linepos++;	// advance past the / delimiting character

			// the next value in the line is the last file commit date and time
			char datestr[linesize-linepos];
			long datestrpos = 0;
			// copy the date and time value into our date and time string until a / is reached
			while ( (linepos < (linesize-1)) && (linestr[linepos] != '/') )
			{
				datestr[datestrpos] = linestr[linepos];
				linepos++;
				datestrpos++;
			}
			datestr[datestrpos] = 0;	// terminate the string


		// Root file
			// next we will try to retrive the CVS Root from the Root file
			char cvsrootfilefullpath[strlen(sourcedir) + strlen(CVS_ROOT_FILE) + 4];
			sprintf(cvsrootfilefullpath, "%s/%s", sourcedir, CVS_ROOT_FILE);
			char cvsrootpathstr[1024];	// for storing the Root string
			cvsrootpathstr[0] = 0;
			// see if we can find the CVS Root file
			FILE *cvsrootf = fopen(cvsrootfilefullpath, "r");
			if (cvsrootf != NULL)
			{
				// first find the @ character 
				// (we're not interested in login name and type, 
				// so we want to begin the path after the @)
				while (!feof(cvsrootf))
				{
					int got = fgetc(cvsrootf);
					if ( (got == EOF) || (got == '@') || iscntrl(got) )
						break;
				}

				// now copy up until the : character 
				// (beyond that is partially the repository path)
				long strpos = 0;
				while (!feof(cvsrootf))
				{
					int got = fgetc(cvsrootf);
					if ( (got == EOF) || (got == ':') || iscntrl(got) )
						break;
					cvsrootpathstr[strpos] = got;
					strpos++;
				}
				cvsrootpathstr[strpos] = 0;	// terminate the string
				// if we managed to get anything, then 
				// append the : to the root path string
				if (strlen(cvsrootpathstr) > 0)
					strcat(cvsrootpathstr, ":");

				fclose(cvsrootf);
			}

		// Repository file
			// then we will try to retrive the CVS Repository from the Repository file
			char cvsrepositoryfilefullpath[strlen(sourcedir) + strlen(CVS_REPOSITORY_FILE) + 4];
			sprintf(cvsrepositoryfilefullpath, "%s/%s", sourcedir, CVS_REPOSITORY_FILE);
			char cvsrepositorypathstr[1024];	// for storing the Repository string
			cvsrepositorypathstr[0] = 0;
			// see if we can find the CVS Repository file
			FILE *cvsrepositoryf = fopen(cvsrepositoryfilefullpath, "r");
			if (cvsrepositoryf != NULL)
			{
				// copy the entire line from the Repository file
				long strpos = 0;
				while (!feof(cvsrootf))
				{
					int got = fgetc(cvsrepositoryf);
					if ( (got == EOF) || iscntrl(got) )
						break;
					cvsrepositorypathstr[strpos] = got;
					strpos++;
				}
				cvsrepositorypathstr[strpos] = 0;	// terminate the string
				// if we managed to get anything, then 
				// append the / to the repository path string
				if (strlen(cvsrepositorypathstr) > 0)
					strcat(cvsrepositorypathstr, "/");

				fclose(cvsrepositoryf);
			}

			// print a commented-out info blurb at the beginning of the HTML output stream
			fprintf(outputfile, "<!--\n");
			fprintf(outputfile, "\tthis file generated from:  %s%s%s\n", 
					cvsrootpathstr, cvsrepositorypathstr, sourcefilebase);
			// only print version info if we managed to get any
			if (strlen(versionstr) > 0)
				fprintf(outputfile, "\tfile version:  %s\n", versionstr);
			// only print date and time info if we managed to get any
			if (strlen(datestr) > 0)
				fprintf(outputfile, "\tlast modified:  %s\n", datestr);
			fprintf(outputfile, "-->\n\n");

			// if we made it this far, then we found our CVS entry info 
			// and don't need to loop through the file any more
			break;
		}
		// end of while loop reading CVS Entries file

		fclose(cvsentriesf);
	}
	// end of if CVS Entries file is valid statement
}



int main(int argc, char **argv)
{
	// fail and print usage if there weren't enough arguments
	// (only the last argument is optional)
	if (argc < (kNumArgs-1))
	{
		printf("\t%s inputfile [outputfile]\n", basename(argv[kArg_Command]));
		return EINVAL;
	}

	// try to open the input file
	FILE *sourcef = fopen(argv[kArg_InputFile], "r");
	if (sourcef == NULL)
	{
		printf("could not open input file\n");
		return ENOENT;
	}

	// if an output file was specified, try to open it, otherwise use stdout
	FILE *destf = stdout;
	if (argc > kArg_OutputFile)
	{
		destf = fopen(argv[kArg_OutputFile], "w");
		if (destf == NULL)
		{
			printf("could not create output file %s\n", argv[kArg_OutputFile]);
			return EIO;
		}
	}

	// if possible, start by writing info about the input file (version, date, etc.)
	writeinfoheader(destf, argv[kArg_InputFile]);

	// this is the main input file reading loop
readlineloop:
	while ( !feof(sourcef) )
	{
		// read the input file one line at a time
		size_t linesize;
		char *linestr = fgetln(sourcef, &linesize);
		// try the next line if this one failed
		if (linestr == NULL)
			continue;

		// this is the per-character line reading loop
		for (size_t i=0; i < linesize; i++)
		{
		// begin Marc's sorry-ass HTML parsing...
			// look for a tag
			if (linestr[i] == '<')
			{
				// we need to make a copy the current line and null-terminate it 
				// (most of those handy functions in string.h require null-terminated strings)
				char linestr_copy[linesize];
				for (size_t j=0; j < linesize; j++)
					linestr_copy[j] = linestr[j];
				linestr_copy[linesize-1] = 0;	// terminate over newline character
				// read the first word after the tag-begin bracket (should be the tag name)
				char firstword[linesize];
				firstword[0] = 0;
				if (sscanf(linestr_copy + i + 1, "%s", firstword) > 0)
				{
					// see if it is a <link> tag
					if (strncasecmp(firstword, LINK_TAG, strlen(LINK_TAG)) == 0)
					{
						// find where the source filename tag attribute begins
						char *linksource = dfx_strcasestr(linestr_copy + i, LINK_SOURCE);
						// looks like we found a linked filename attribute
						if (linksource != NULL)
						{
							// move string past the tag attribute label 
							// so that it points to the attribute value
							linksource += strlen(LINK_SOURCE);
							char linkfilename[linesize];
							linkfilename[0] = 0;
							// see if the value is wrapped in quotes (more sorry-ass HTML parsing)
							if (linksource[0] == '"')
							{
								linksource++;
								long k = 0;
								// copy the value until end quotes are found, or the string ends
								while ( (linksource[k] != '"') && (linksource[k] != 0) )
								{
									linkfilename[k] = linksource[k];
									k++;
								}
								linkfilename[k] = 0;	// terminate the filename string
							}
							// value is not wrapped in quotes
							else
							{
								long k = 0;
								// copy the value until end-tag bracket or white space are found, or the string ends
								while ( (linksource[k] != '>') && !isspace(linksource[k]) && (linksource[k] != 0) )
								{
									linkfilename[k] = linksource[k];
									k++;
								}
								linkfilename[k] = 0;	// terminate the filename string
							}
							// assume that the path of the linked file is relative to the source file 
							// XXX I don't think that we need to deal with basehrefs or anything like that
							char *basedir = dirname(argv[kArg_InputFile]);
							// get full path and filename for linked file
							char linkfilefullpath[strlen(basedir) + strlen(linkfilename) + 4];
							sprintf(linkfilefullpath, "%s/%s", basedir, linkfilename);
							// open the linked file
							FILE *linkf = fopen(linkfilefullpath, "r");
							if (linkf != NULL)
							{
								// copy every byte of the linked file to the output stream
								while (!feof(linkf))
								{
									int got = fgetc(linkf);
									if (got != EOF)
										fputc(got, destf);
								}
								fclose(linkf);
								// advance our line character reader position past the <link> tag 
								// to continue copying anything else in this line to the output stream
								i += linksource - linestr_copy + strlen(linkfilename);
								while (1)
								{
									// if a > is found in this line, then advance past it, 
									// break out of this loop, and continue copying from 
									// the input file to the output stream
									if (linestr_copy[i] == '>')
									{
										i++;	// advance past the tag closure bracket
										break;
									}
									// if the end of the line is reached, then keep grabbing 
									// characters from the input stream until > is found, 
									// and then return to the start of the line reading loop
									if (linestr_copy[i] == 0)
									{
										while (!feof(sourcef))
										{
											int got = fgetc(sourcef);
											if ( (got == '>') || (got == EOF) )
												goto readlineloop;
										}
									}
									i++;
								}
							}
						}	// end found linked filename attribute
					}	// end tag is a <link> tag
				}	// end scanning of tag name
			}	// end tag-begin found
			fprintf(destf, "%c", linestr[i]);
		}
		// end of per-character line reading loop
	}
	// end of main input file reading loop

	// cleanup
	fclose(sourcef);
	if (destf != stdout)
		fclose(destf);
}
