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
 *  usage:  The first argument is the input file name 
 *  (i.e. the file that has <link> tags) and the second 
 *  argument is the consolidated output file name.  
 *  The second argument is optional (stdout is used by default).
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>	/* for isspace */
#include <libgen.h>	/* for basename and dirname */


#define LINK_TAG	"link"
#define LINK_SOURCE	"href="


enum {
	kArg_Command = 0,
	kArg_InputFile,
	kArg_OutputFile,

	kNumArgs
};

// strcasestr doesn't appear to be included with BSD or Darwin
// XXX this is not a valid way to check if strcasestr is available
#ifndef strcasestr
char * strcasestr(const char *big, const char *little)
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
#endif



int main(int argc, char **argv)
{
	// fail and print usage if there weren't enough arguments
	// (only the last argument is optional)
	if (argc < (kNumArgs-1))
	{
		printf("\t%s inputfile [outputfile]\n", basename(argv[kArg_Command]));
		return 1;
	}

	// try to open the input file
	FILE *sourcef = fopen(argv[kArg_InputFile], "r");
	if (sourcef == NULL)
	{
		printf("could not open input file\n");
		return 2;
	}

	// if an output file was specified, try to open it, otherwise use stdout
	FILE *destf = stdout;
	if (argc > kArg_OutputFile)
	{
		destf = fopen(argv[kArg_OutputFile], "w");
		if (destf == NULL)
		{
			printf("could not create output file %s\n", argv[kArg_OutputFile]);
			return 3;
		}
	}

	// this is the main input file reading loop
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
						char *linksource = strcasestr(linestr_copy + i, LINK_SOURCE);
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
							char *sourcedir = dirname(argv[kArg_InputFile]);
							// get full path and filename for linked file
							char linkfilefullpath[strlen(sourcedir) + strlen(linkfilename) + 4];
							sprintf(linkfilefullpath, "%s/%s", sourcedir, linkfilename);
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
									if (linestr_copy[i] == '>')
									{
										i++;	// advance past the tag closure bracket
										break;
									}
									if (linestr_copy[i] == 0)
										break;
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
