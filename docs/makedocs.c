#include <stdio.h>
#include <string.h>
#include <ctype.h>


#define LINK_TAG	"link"
#define LINK_SOURCE	"href="
//#define LINK_SOURCE_ALT	"HREF="


#ifndef strcasestr
char * strcasestr(const char *big, const char *little)
{
	if ( (big == NULL) || (little == NULL) )
		return NULL;
	for (int i=0; i < strlen(big) - strlen(little); i++)
	{
		if (strncasecmp(big + i, little, strlen(little)) == 0)
			return (char*) (big + i);
	}
	return NULL;
}
#endif


int main()
{
	FILE *sourcef = fopen("/Users/marc/dfx/docs/geometer.html", "r");
	if (sourcef == NULL)
	{
		printf("could not open input file\n");
		return 1;
	}
	FILE *destf = fopen("/Users/marc/Desktop/Geometer.html", "w");
	if (destf == NULL)
	{
		printf("could not create output file\n");
		return 2;
	}

	while ( !feof(sourcef) )
	{
		size_t linesize;
		char *linestr = fgetln(sourcef, &linesize);
		if (linestr == NULL)
			continue;

		for (size_t i=0; i < linesize; i++)
		{
			if (linestr[i] == '<')
			{
				char firstword[linesize];
				firstword[0] = 0;
				char linestr_copy[linesize];
				for (size_t j=0; j < linesize; j++)
					linestr_copy[j] = linestr[j];
				linestr_copy[linesize-1] = 0;
				if (sscanf(linestr_copy + i + 1, "%s", firstword) > 0)
				{
					if (strncasecmp(firstword, LINK_TAG, strlen(LINK_TAG)) == 0)
					{
						char *linksource = strcasestr(linestr_copy + i, LINK_SOURCE);
//						char *linksource = strstr(linestr_copy + i, LINK_SOURCE);
//						if (linksource == NULL)
//							linksource = strstr(linestr_copy + i, LINK_SOURCE_ALT);
						if (linksource != NULL)
						{
							linksource += strlen(LINK_SOURCE);
							char linkfilename[linesize];
							linkfilename[0] = 0;
							if (linksource[0] == '"')
							{
								linksource++;
								long k = 0;
								while ( (linksource[k] != '"') && (linksource[k] != 0) )
								{
									linkfilename[k] = linksource[k];
									k++;
								}
								linkfilename[k] = 0;
							}
							else
							{
								long k = 0;
								while ( (linksource[k] != 0) && (linksource[k] != '>') && !isspace(linksource[k]) )
								{
									linkfilename[k] = linksource[k];
									k++;
								}
								linkfilename[k] = 0;
							}
							char linkfilefullpath[1024];
							strcpy(linkfilefullpath, "/Users/marc/dfx/docs/");
							strcat(linkfilefullpath, linkfilename);
							FILE *linkf = fopen(linkfilefullpath, "r");
							if (linkf != NULL)
							{
								while (!feof(linkf))
								{
									int got = fgetc(linkf);
									if (got != EOF)
										fputc(got, destf);
								}
								fclose(linkf);
								break;
							}
						}
					}
				}
			}
			fprintf(destf, "%c", linestr[i]);
		}
	}

	fclose(sourcef);
	fclose(destf);
}
