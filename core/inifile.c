/*
 *  Copyright(C) 2006 Neuros Technology International LLC. 
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *  
 *
 *  This program is distributed in the hope that, in addition to its 
 *  original purpose to support Neuros hardware, it will be useful 
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Inifile routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-06-20 EY
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "key-value-pair.h"

#define MAXLINE	1024

static int FileCopy (FILE* sf, FILE* df)
{
    char line [MAXLINE + 1];
    
    while (fgets (line, MAXLINE + 1, sf) != NULL)
        if (fputs (line, df) == EOF) {
            return COOL_INI_FILEIOFAILED;
        }

    return COOL_INI_OK;
}

static char* get_section_name (char *section_line)
{
    char* current;
    char* tail;
    char* name;

    if (!section_line)
        return NULL;

    current = section_line;

    while (*current == ' ' ||  *current == '\t') current++; 

    if (*current == ';' || *current == '#')
        return NULL;

    if (*current++ == '[')
        while (*current == ' ' ||  *current == '\t') current ++;
    else
        return NULL;

    name = tail = current;
    while (*tail != ']' && *tail != '\n' &&
          *tail != ';' && *tail != '#' && *tail != '\0')
          tail++;     
    *tail = '\0';
    while (*tail == ' ' || *tail == '\t') {
        *tail = '\0';
        tail--; 
    }

    return name;
}

static int LocateSection(FILE* fp, const char* pSection, FILE* bak_fp)
{
    char szBuff[MAXLINE + 1];
    char *name;

    while (1) {
        if (!fgets(szBuff, MAXLINE, fp)) {
            if (feof (fp))
                return COOL_INI_SECTIONNOTFOUND;
            else
                return COOL_INI_FILEIOFAILED;
        }
        else if (bak_fp && fputs (szBuff, bak_fp) == EOF)
            return COOL_INI_FILEIOFAILED;
        
        name = get_section_name (szBuff);
        if (!name)
            continue;

        if (strcmp (name, pSection) == 0)
            return COOL_INI_OK; 
    }

    return COOL_INI_SECTIONNOTFOUND;
}

static int DeleteSection(FILE* fp, const char* pSection, FILE* bak_fp)
{
    char szBuff[MAXLINE + 1];
    char *name;
    int isFound = 0;

    rewind(fp);		
    rewind(bak_fp);
    while (1) {
        if (!fgets(szBuff, MAXLINE, fp)) {
            if (feof (fp))
                return COOL_INI_SECTIONNOTFOUND;
            else
                return COOL_INI_FILEIOFAILED;
        } 
		
        name = get_section_name (szBuff);

        if (!name) {
        	if (isFound){	
			continue;
		}
		else{			
			fputs (szBuff, bak_fp);
		}
        }
	else{
        	if (strcmp (name, pSection) == 0){			
        		isFound = 1;     
			continue;	
        	}
		else{
			isFound = 0;
			strcat(szBuff, "]\n");	
			fputs (szBuff, bak_fp);
		}
	}
    }

    return COOL_INI_SECTIONNOTFOUND;
}

static int get_key_value (char *key_line, char **mykey, char **myvalue)
{
    char* current;
    char* tail;
    char* value;

    if (!key_line)
        return -1;

    current = key_line;

    while (*current == ' ' ||  *current == '\t') current++; 

    if (*current == ';' || *current == '#')
        return -1;

    if (*current == '[')
        return 1;

    if (*current == '\n' || *current == '\0')
        return -1;

    tail = current;
    while (*tail != '=' && *tail != '\n' &&
          *tail != ';' && *tail != '#' && *tail != '\0')
          tail++;

    value = tail + 1;
    if (*tail != '=')
        *value = '\0'; 

    *tail-- = '\0';
    while (*tail == ' ' || *tail == '\t') {
        *tail = '\0';
        tail--; 
    }
        
    tail = value;
    while (*tail != '\n' && *tail != '\0') tail++;
    *tail = '\0'; 

    if (mykey)
        *mykey = current;
    if (myvalue)
        *myvalue = value;

    return 0;
}


static int DeleteKey(FILE* fp, const char* pSection, const char* pKey, FILE* tmp_fp)
{
	char szBuff[MAXLINE + 1 + 1];
	char* current;
	char* value;
	int ret;

	while (1) {
	    int bufflen;

	    if (!fgets(szBuff, MAXLINE, fp))
	        return COOL_INI_FILEIOFAILED;
	    bufflen = strlen (szBuff);
	    if (szBuff [bufflen - 1] == '\n')
	        szBuff [bufflen - 1] = '\0';

	    ret = get_key_value (szBuff, &current, &value);
	    if (ret < 0)
	        continue;
	    else if (ret > 0) {
	        fseek (fp, -bufflen, SEEK_CUR);
	        return COOL_INI_KEYNOTFOUND;
	    }
	        
	    if (strcmp (current, pKey) == 0) {
	        continue;
	    }
	    else if (tmp_fp && *current != '\0')
	        fprintf (tmp_fp, "%s=%s\n", current, value);
	}

	return COOL_INI_KEYNOTFOUND;
}

static int LocateKeyValue(FILE* fp, const char* pKey,
							  int bCurSection, char* pValue, int iLen,
							  FILE* bak_fp, char* nextSection)
{
    char szBuff[MAXLINE + 1 + 1];
    char* current;
    char* value;
    int ret;

    while (1) {
        int bufflen;

        if (!fgets(szBuff, MAXLINE, fp))
            return COOL_INI_FILEIOFAILED;
        bufflen = strlen (szBuff);
        if (szBuff [bufflen - 1] == '\n')
            szBuff [bufflen - 1] = '\0';

        ret = get_key_value (szBuff, &current, &value);
        if (ret < 0)
            continue;
        else if (ret > 0) {
            fseek (fp, -bufflen, SEEK_CUR);
            return COOL_INI_KEYNOTFOUND;
        }
            
        if (strcmp (current, pKey) == 0) {
            if (pValue)
                strncpy (pValue, value, iLen);

            return COOL_INI_OK; 
        }
        else if (bak_fp && *current != '\0')
            fprintf (bak_fp, "%s=%s\n", current, value);
    }

    return COOL_INI_KEYNOTFOUND;
}

static int CopyAndLocate (FILE* fp, FILE* tmp_fp, 
                const char* pSection, const char* pKey, char* tempSection)
{
    if (pSection && LocateSection (fp, pSection, tmp_fp) != COOL_INI_OK)
        return COOL_INI_SECTIONNOTFOUND;

    if (LocateKeyValue (fp, pKey, pSection != NULL, 
                NULL, 0, tmp_fp, tempSection) != COOL_INI_OK)
        return COOL_INI_KEYNOTFOUND;

    return COOL_INI_OK;
}

static int Delete(FILE* fp, FILE* tmp_fp, 
	const char* pSection, const char* pKey, char* tempSection)
{
	 if (pSection && LocateSection (fp, pSection, tmp_fp) != COOL_INI_OK)
        	return COOL_INI_SECTIONNOTFOUND;
	
	 if (pKey) {
		if (LocateKeyValue (fp, pKey, pSection != NULL, 
                NULL, 0, tmp_fp, tempSection) != COOL_INI_OK)
        		return COOL_INI_KEYNOTFOUND;
		DeleteKey(fp, pSection, pKey, tmp_fp);
		}
	 else {
	 	DeleteSection(fp, pSection, tmp_fp);
	 	}
	 return COOL_INI_OK;
}

int CoolSetCharValue (const char* pFile, const char* pSection,
                     const char* pKey, char* pValue)
{
    FILE* fp;
    FILE* tmp_fp;
    int rc;
    char tempSection [MAXLINE + 2];
	char tmp_nam [256];
	char *name;
	memset(tempSection, 0, sizeof(tempSection));
	strcpy(tmp_nam,pFile);
	name = strrchr(tmp_nam, '/');
	if(name)
		sprintf (name+1, "tmp-%lx", time(NULL));
	else
    		sprintf (tmp_nam, "tmp-%lx", time(NULL));
    if ((tmp_fp = fopen (tmp_nam, "w+")) == NULL)
        return COOL_INI_TMPFILEFAILED;
    if (!(fp = fopen (pFile, "r+"))) {
        fclose (tmp_fp);
        remove(tmp_nam);
        if (!(fp = fopen (pFile, "w"))) {
            return COOL_INI_FILEIOFAILED;
        }
        fprintf (fp, "[%s]\n", pSection);
        fprintf (fp, "%s=%s\n", pKey, pValue);
        fclose (fp);
        return COOL_INI_OK;
    }

    switch (CopyAndLocate (fp, tmp_fp, pSection, pKey, tempSection)) {
    case COOL_INI_SECTIONNOTFOUND:
        fprintf (tmp_fp, "\n[%s]\n", pSection);
        fprintf (tmp_fp, "%s=%s\n", pKey, pValue);
        break;
    
    case COOL_INI_KEYNOTFOUND:
        fprintf (tmp_fp, "%s=%s\n\n", pKey, pValue);
        fprintf (tmp_fp, "%s\n", tempSection);
    break;

    default:
        fprintf (tmp_fp, "%s=%s\n", pKey, pValue);
        break;
    }

    if ((rc = FileCopy (fp, tmp_fp)) != COOL_INI_OK)
        goto error;
    
    fclose (fp);
    if (!(fp = fopen (pFile, "w"))) {
        fclose (tmp_fp);
        remove(tmp_nam);

        return COOL_INI_FILEIOFAILED;
    }
    
    rewind (tmp_fp);
    rc = FileCopy (tmp_fp, fp);

error:
    fclose (fp);
    fclose (tmp_fp);

	remove(tmp_nam);
    return rc;
}

int CoolGetCharValue(const char* pFile, const char* pSection,
                               const char* pKey, char* pValue, int iLen)
{
    FILE* fp;
    char tempSection [MAXLINE + 2];

    if (!(fp = fopen(pFile, "r")))
         return COOL_INI_FILENOTFOUND;

    if (pSection)
         if (LocateSection (fp, pSection, NULL) != COOL_INI_OK) {
             fclose (fp);
             return COOL_INI_SECTIONNOTFOUND;
         }

    if (LocateKeyValue (fp, pKey, pSection != NULL, 
                pValue, iLen, NULL, tempSection) != COOL_INI_OK) {
         fclose (fp);
         return COOL_INI_KEYNOTFOUND;
    }

    fclose (fp);
    return COOL_INI_OK;
}

int  CoolGetIntValue(const char* pFile, const char* pSection,
                               const char* pKey, int* value)
{
    int ret;
    char szBuff [51];

    ret = CoolGetCharValue (pFile, pSection, pKey, szBuff, 50);
    if (ret < 0)
        return ret;

    *value = strtol (szBuff, NULL, 10);
    if ((*value == LONG_MIN || *value == LONG_MAX) && errno == ERANGE)
        return COOL_INI_INTCONV;

    return COOL_INI_OK;
}

int  CoolGetUnlongValue(const char* pFile, const char* pSection,
                               const char* pKey, unsigned long* value)
{
    int ret;
    char szBuff [51];

    ret = CoolGetCharValue (pFile, pSection, pKey, szBuff, 50);
    if (ret < 0)
        return ret;

    *value = strtoul (szBuff, NULL, 10);
    if (/*(*value == LONG_MIN || *value == LONG_MAX) &&*/ errno == ERANGE)
        return COOL_INI_INTCONV;

    return COOL_INI_OK;
}

int CoolDeleteSectionKey(const char* pFile, const char* pSection, const char* pKey)
{
	FILE* fp;
    	FILE* tmp_fp;
    	int rc;
   	char tempSection [MAXLINE + 2];
	char tmp_nam [256];
	char *name;
	memset(tempSection, 0, sizeof(tempSection));
	strcpy(tmp_nam,pFile);
	name = strrchr(tmp_nam, '/');
	if(name)
		sprintf (name+1, "tmp-%lx", time(NULL));
	else
    	sprintf (tmp_nam, "tmp-%lx", time(NULL));
	if ((tmp_fp = fopen (tmp_nam, "w+")) == NULL)
	    return COOL_INI_TMPFILEFAILED;
	if (!(fp = fopen (pFile, "r+"))) {
	    fclose (tmp_fp);
	    remove(tmp_nam);
	    if (!(fp = fopen (pFile, "w"))) {
	        return COOL_INI_FILEIOFAILED;
	    }
	    fclose (fp);
	    return COOL_INI_OK;
	}	

	Delete (fp, tmp_fp, pSection, pKey, tempSection);

	if ((rc = FileCopy (fp, tmp_fp)) != COOL_INI_OK)
        goto error;
    
	fclose (fp);
	if (!(fp = fopen (pFile, "w"))) {
	    fclose (tmp_fp);
	    remove(tmp_nam);
	    return COOL_INI_FILEIOFAILED;
	}

	rewind (tmp_fp);
	rc = FileCopy (tmp_fp, fp);

error:
	fclose (fp);
	fclose (tmp_fp);
	remove(tmp_nam);
   	return rc;
}

