#if !defined(_CRT_SECURE_NO_WARNINGS) && defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <time.h>

#define LANGUAGE_EXTENSIONS

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#include <direct.h>
#define strdup _strdup
#define _CRT_SECURE_NO_WARNINGS
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "cprime_lib.h"

#ifndef NEW
#define NEW(C) mallocinit(sizeof(C), & C);
#endif


void* mallocinit(int size, void* value)
{
    void* p = malloc(size);
    if (p) memcpy(p, value, size);
    return p;
}

#define CAST(FROM, TO) \
static inline struct TO *  FROM##_As_##TO( struct FROM*  p)\
{\
if (p != NULL &&  p->Type == TO##_ID)\
    return ( struct TO * )p;\
  return NULL;\
}\
static inline  struct FROM *  TO##_As_##FROM(struct TO*  p)\
{\
    return (  struct FROM * )p;\
}

#define CASTSAME(FROM, TO) \
static inline struct TO * FROM##_As_##TO(struct FROM* p) { return (struct TO * ) p; }\
static inline struct FROM * TO##_As_##FROM(struct TO* p) { return (struct FROM *) p; }



struct StrArray
{
    const char** data;
    int size;
    int capacity;
};

#define STRARRAY_INIT { 0 }

int StrArray_Push(struct StrArray* p, const char* textSource)
{
    char* text = strdup(textSource);
    if (text)
    {
        if (p->size + 1 > p->capacity)
        {
            int n = p->capacity * 2;
            if (n == 0)
            {
                n = 1;
            }
            const char** pnew = p->data;
            pnew = (const char**)realloc((void*)pnew, n * sizeof(const char*));
            if (pnew)
            {
                p->data = pnew;
                p->capacity = n;
            }
            else
            {
                free(text);
                text = 0;
            }
        }
        if (text)
        {
            p->data[p->size] = text;
            p->size++;
        }
    }
    return text != 0;
}

void StrArray_Destroy(struct StrArray* p)
{
    for (int i = 0; i < p->size; i++)
    {
        free((void*)p->data[i]);
    }
    free((void*)p->data);
}

struct Stream
{
    char* Text;
    int TextLen;

    wchar_t Character;
    int Position;
    int Line;
    int Column;
};


#define CPRIME_MAX_DRIVE 255
#define CPRIME_MAX_DIR 255
#define CPRIME_MAX_FNAME 255
#define CPRIME_MAX_EXT 255
#define CPRIME_MAX_PATH 260



void SplitPath(const char* path, char* drv, char* dir, char* name, char* ext)
{
    const char* end; /* end of processed char */
    const char* p;      /* search pointer */
    const char* s;      /* copy pointer */
    /* extract drive name */
    if (path[0] && path[1] == ':')
    {
        if (drv)
        {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    }
    else if (drv)
        *drv = '\0';
    /* search for end of char or stream separator */
    for (end = path; *end && *end != ':'; )
        end++;
    /* search for begin of file extension */
    for (p = end; p > path && *--p != '\\' && *p != '/'; )
        if (*p == '.')
        {
            end = p;
            break;
        }
    if (ext)
        for (s = end; (*ext = *s++); )
            ext++;
    /* search for end of directory name */
    for (p = end; p > path; )
        if (*--p == '\\' || *p == '/')
        {
            p++;
            break;
        }
    if (name)
    {
        for (s = p; s < end; )
            *name++ = *s++;
        *name = '\0';
    }
    if (dir)
    {
        for (s = path; s < p; )
            *dir++ = *s++;
        *dir = '\0';
    }
}

void MkDir(char* path)
{
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    _mkdir(path);
#else
    mkdir(path, 0777);
#endif
}

void MakePath(char* path, char* drv, char* dir, char* name, char* ext)
{
    if (drv && drv[0] != '\0')
    {
        while (*drv)
        {
            *path = *drv;
            path++;
            drv++;
        }
        //*path = ':';
        //path++;
        // *path = '\\';
        // path++;
    }
    if (dir && dir[0] != '\0')
    {
        while (*dir)
        {
            *path = *dir;
            path++;
            dir++;
        }
        //  *path = '\\';
        // path++;
    }
    while (*name)
    {
        *path = *name;
        path++;
        name++;
    }
    //*path = '.';
    //path++;
    while (*ext)
    {
        *path = *ext;
        path++;
        ext++;
    }
    *path = '\0';
}

bool IsInPath(const char* filePath, const char* path)
{
    while (*path)
    {
        if (toupper(*path) != toupper(*filePath))
        {
            return false;
        }
        if (*path == '\0')
            break;
        path++;
        filePath++;
    }
    return true;
}

bool IsFullPath(const char* path)
{
    if (path != NULL)
    {
        if ((path[0] >= 'a' && path[0] <= 'z') ||
            (path[0] >= 'A' && path[0] <= 'Z'))
        {
            if (path[1] == ':')
            {
                if (path[2] == '\\' || path[2] == '/')
                {
                    //Ve se tem pontos ..
                    const char* p = &path[2];
                    while (*p)
                    {
                        if ((*p == '.' && *(p - 1) == '\\') ||
                            (*p == '.' && *(p - 1) == '/'))
                        {
                            return false;
                        }
                        p++;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}


/*mock file system*/
struct EmulatedFile
{
    const char* path;
    const char* content;
};

/*
  Sample:
   struct EmulatedFile *files[] = {
        &(struct EmulatedFile) {"c:\\file1.c", "int main() { return 1; }"},
        NULL
    };

    s_emulatedFiles = files;

    ..using mock..

    s_emulatedFiles = NULL;
*/

static struct EmulatedFile** s_emulatedFiles;

void GetFullPathS(const char* fileName, char* out)
{
    if (s_emulatedFiles)
    {
        if (IsFullPath(fileName))
        {
            //nao altera
            strcpy(out, fileName);
        }
        else
        {
            /*arquivos emulados estao todos em c:*/
            strcpy(out, "c:/");
            strcat(out, fileName);
        }
        return;
    }
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    _fullpath(
        out,
        fileName,
        CPRIME_MAX_PATH);
#else
    realpath(fileName, out);
#endif
}

bool FileExists(const char* fullPath)
{
    if (s_emulatedFiles)
    {
        struct EmulatedFile** pEmulatedFile = s_emulatedFiles;
        while (*pEmulatedFile)
        {
            char buffer[CPRIME_MAX_PATH] = { 0 };
            GetFullPathS((*pEmulatedFile)->path, buffer);
            if (strcmp(buffer, fullPath) == 0)
            {
                return true;
            }
            pEmulatedFile++;
        }
        return false;
    }
    bool bFileExists = false;
    FILE* fp = fopen(fullPath, "rb");
    if (fp)
    {
        bFileExists = true;
        fclose(fp);
    }
    return bFileExists;
}

void GetFullDirS(const char* fileName, char* out)
{
    char buffer[CPRIME_MAX_PATH] = { 0 };
    if (s_emulatedFiles)
    {
        strcpy(out, "c:\\");
        //strcat(out, fileName);
        return;
    }
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    _fullpath(
        buffer,
        fileName,
        CPRIME_MAX_PATH);
#else
    realpath(fileName, buffer);
#endif
    char drive[CPRIME_MAX_DRIVE];
    char dir[CPRIME_MAX_DIR];
    char fname[CPRIME_MAX_FNAME];
    char ext[CPRIME_MAX_EXT];
    SplitPath(buffer, drive, dir, fname, ext); // C4996
    snprintf(out, CPRIME_MAX_PATH, "%s%s", drive, dir);
}


static bool fread2(void* buffer, size_t size, size_t count, FILE* stream, size_t* sz)
{
    *sz = 0;//out

    bool result = false;
    size_t n = fread(buffer, size, count, stream);
    if (n == count)
    {
        *sz = n;
        result = true;
    }
    else if (n < count)
    {
        if (feof(stream))
        {
            *sz = n;
            result = true;
        }
    }
    return result;
}

char* readfile(const char* path)
{
    char* result = NULL;

    struct stat info;
    if (stat(path, &info) == 0)
    {
        char* data = (char*)malloc(sizeof(char) * info.st_size + 1);
        if (data != NULL)
        {
            FILE* file = fopen(path, "r");
            if (file != NULL)
            {
                if (info.st_size >= 3)
                {
                    size_t n = 0;
                    if (fread2(data, 1, 3, file, &n))
                    {
                        if (n == 3)
                        {
                            if ((unsigned char)data[0] == 0xEF &&
                                (unsigned char)data[1] == 0xBB &&
                                (unsigned char)data[2] == 0xBF)
                            {
                                if (fread2(data, 1, info.st_size - 3, file, &n))
                                {
                                    //ok
                                    data[n] = 0;
                                    result = data;
                                    data = 0;
                                }
                            }
                            else if (fread2(data + 3, 1, info.st_size - 3, file, &n))
                            {
                                data[3 + n] = 0;
                                result = data;
                                data = 0;
                            }
                        }
                        else
                        {
                            data[n] = 0;
                            result = data;
                            data = 0;
                        }
                    }
                }
                else
                {
                    size_t n = 0;
                    if (fread2(data, 1, info.st_size, file, &n))
                    {
                        data[n] = 0;
                        result = data;
                        data = 0;
                    }
                }
                fclose(file);
            }
            free(data);
        }
    }
    return result;
}

bool LoadFile(const char* filename, const char** out, int* szOut)
{
    if (s_emulatedFiles)
    {
        struct EmulatedFile** pEmulatedFile = s_emulatedFiles;
        while (*pEmulatedFile)
        {
            char buffer[CPRIME_MAX_PATH] = { 0 };
            GetFullPathS((*pEmulatedFile)->path, buffer);
            if (strcmp(buffer, filename) == 0)
            {
                size_t len = strlen((*pEmulatedFile)->content);
                *szOut = (int)len;
                *out = strdup((*pEmulatedFile)->content);
                return true;
            }
            pEmulatedFile++;
        }
        return false;
    }
    bool result = 0;
    char* buffer = readfile(filename);
    if (buffer != NULL)
    {
        *out = buffer;
        *szOut = strlen(buffer);
        result = 1;
    }
    return result;
}

bool Stream_OpenFile(struct Stream* pStream, const char* fullPath)
{
    pStream->Line = 1;
    pStream->Column = 1;
    pStream->Position = 0;
    bool result = LoadFile(fullPath, (const char**)&pStream->Text,
                           &pStream->TextLen);
    if (result)
    {
        if (pStream->Text != NULL &&
            pStream->Text[0] != '\0')
        {
            //unicode?
            pStream->Character = pStream->Text[0];
        }
        else
        {
            pStream->Character = '\0';
        }
    }
    return result;
}

bool Stream_OpenString(struct Stream* pStream, const char* Text)
{
    pStream->Line = 1;
    pStream->Column = 1;
    pStream->Position = 0;
    pStream->Text = strdup(Text);
    if (Text != NULL)
    {
        pStream->TextLen = (int)strlen(Text);
    }
    else
    {
        pStream->TextLen = 0;
    }
    if (pStream->Text != NULL &&
        pStream->Text[0] != '\0')
    {
        //unicode?
        pStream->Character = pStream->Text[0];
    }
    else
    {
        pStream->Character = '\0';
    }
    return true;
}

void Stream_Destroy(struct Stream* pStream)
{
    free((void*)pStream->Text);
}

wchar_t Stream_LookAhead(struct Stream* pStream)
{
    if (pStream->Position + 1 >= pStream->TextLen)
    {
        return '\0';
    }
    return pStream->Text[pStream->Position + 1];
}

wchar_t Stream_LookAhead2(struct Stream* pStream)
{
    if (pStream->Position + 2 >= pStream->TextLen)
    {
        return '\0';
    }
    return pStream->Text[pStream->Position + 2];
}

void Stream_Match(struct Stream* pStream)
{
    if (pStream->Position >= pStream->TextLen)
    {
        pStream->Character = L'\0';
        return;
    }
    pStream->Column++;
    pStream->Position++;
    if (pStream->Position == pStream->TextLen)
    {
        pStream->Character = '\0';
    }
    else
    {
        pStream->Character = pStream->Text[pStream->Position];
    }
    if (pStream->Character == '\n')
    {
        pStream->Line++;
        pStream->Column = 0;
    }
}

bool Stream_MatchChar(struct Stream* pStream, wchar_t ch)
{
    bool b = pStream->Character == ch;
    Stream_Match(pStream);
    return b;
}


#define STRBUILDER_INIT { 0, 0, 0 }
#define STRBUILDER_DEFAULT_SIZE 17


#define STRBUILDER_INIT { 0, 0, 0 }
#define STRBUILDER_DEFAULT_SIZE 17

void StrBuilder_Init(struct StrBuilder* p);

bool StrBuilder_Reserve(struct StrBuilder* p, int nelements);

void StrBuilder_Attach(struct StrBuilder* wstr,
                       char* psz,
                       int nBytes);


void StrBuilder_Clear(struct StrBuilder* wstr);


void StrBuilder_Init(struct StrBuilder* p)
{
    p->c_str = NULL;
    p->size = 0;
    p->capacity = 0;
}

void StrBuilder_Swap(struct StrBuilder* str1, struct StrBuilder* str2)
{
    struct StrBuilder temp_Moved = *str1;
    *str1 = *str2;
    *str2 = temp_Moved;
}

void StrBuilder_Destroy(struct StrBuilder* p)
{
    if (p)
    {
        free(p->c_str);
        p->c_str = NULL;
        p->size = 0;
        p->capacity = 0;
    }
}

bool StrBuilder_Reserve(struct StrBuilder* p, int nelements)
{
    bool r = true;
    if (nelements > p->capacity)
    {
        char* pnew = (char*)realloc(p->c_str,
                                    (nelements + 1) * sizeof(p->c_str[0]));
        if (pnew)
        {
            if (p->c_str == NULL)
            {
                pnew[0] = '\0';
            }
            p->c_str = pnew;
            p->capacity = nelements;
        }
        else
        {
            r = false /*nomem*/;
        }
    }
    return r;
}

static bool StrBuilder_Grow(struct StrBuilder* p, int nelements)
{
    bool r = true;
    if (nelements > p->capacity)
    {
        int new_nelements = p->capacity + p->capacity / 2;
        if (new_nelements < nelements)
        {
            new_nelements = nelements;
        }
        r = StrBuilder_Reserve(p, new_nelements);
    }
    return r;
}

bool StrBuilder_SetN(struct StrBuilder* p, const char* source, int nelements)
{
    bool r = StrBuilder_Grow(p, nelements);
    if (r)
    {
        strncpy(p->c_str, /*p->capacity + 1,*/ source, nelements);
        p->c_str[nelements] = '\0';
        p->size = nelements;
    }
    return r;
}

bool StrBuilder_Set(struct StrBuilder* p, const char* source)
{
    bool r = true;
    if (source == NULL)
    {
        StrBuilder_Clear(p);
    }
    else
    {
        int n = (int)strlen(source);
        StrBuilder_Clear(p);
        if (n > 0)
        {
            r = StrBuilder_SetN(p, source, (int)strlen(source));
        }
    }
    return r;
}

bool StrBuilder_AppendN(struct StrBuilder* p, const char* source, int nelements)
{
    if (source == NULL || source[0] == '\0')
    {
        return true;
    }
    bool r = StrBuilder_Grow(p, p->size + nelements);
    if (r == true)
    {
        strncpy(p->c_str + p->size,
                /*(p->capacity + 1) - p->size,*/
                source,
                nelements);
        p->c_str[p->size + nelements] = '\0';
        p->size += nelements;
    }
    return r;
}

bool StrBuilder_Append(struct StrBuilder* p, const char* source)
{
    if (source == NULL || source[0] == '\0')
    {
        return true;
    }
    return StrBuilder_AppendN(p, source, (int)strlen(source));
}


bool StrBuilder_AppendIdent(struct StrBuilder* p, int nspaces, const char* source)
{
    for (int i = 0; i < nspaces; i++)
    {
        StrBuilder_Append(p, " ");
    }
    return StrBuilder_Append(p, source);
}


void StrBuilder_Clear(struct StrBuilder* p)
{
    if (p->c_str != NULL)
    {
        p->c_str[0] = '\0';
        p->size = 0;
    }
}

void StrBuilder_Attach(struct StrBuilder* pStrBuilder, char* psz, int nBytes)
{
    if (psz != NULL)
    {
        //assert(nBytes > 0);
        StrBuilder_Destroy(pStrBuilder);
        pStrBuilder->c_str = psz;
        pStrBuilder->capacity = nBytes - 1;
        pStrBuilder->size = 0;
        pStrBuilder->c_str[0] = '\0';
    }
}

bool StrBuilder_AppendWChar(struct StrBuilder* p, wchar_t wch)
{
#ifdef USE_UTF8
    char buffer[5] = { 0 };
    int nbytes = EncodeCharToUTF8Bytes(wch, buffer);
    return StrBuilder_AppendN(p, buffer, nbytes);
#else
    char ch = (char)wch;
    return StrBuilder_AppendN(p, &ch, 1);
#endif
}

bool StrBuilder_AppendChar(struct StrBuilder* p, char ch)
{
    return StrBuilder_AppendN(p, &ch, 1);
}

bool StrBuilder_AppendW(struct StrBuilder* p, const wchar_t* psz)
{
    bool result = false;
    while (*psz)
    {
        result = StrBuilder_AppendWChar(p, *psz);
        if (result != true)
        {
            break;
        }
        psz++;
    }
    return result;
}

void StrBuilder_Trim(struct StrBuilder* p)
{
    struct StrBuilder temp;
    StrBuilder_Init(&temp);
    StrBuilder_Reserve(&temp, p->size);
    bool bCopy = false;
    for (int i = 0; i < p->size; i++)
    {
        char ch = p->c_str[i];
        if (!bCopy && ch != ' ')
        {
            //a partir de agora copia
            bCopy = true;
        }
        if (bCopy)
            StrBuilder_AppendChar(&temp, ch);
    }
    //indice do ultimo que nao eh espaco
    int k = ((int)(temp.size)) - 1;
    for (; k >= 0; k--)
    {
        char ch = temp.c_str[k];
        if (ch != ' ')
        {
            break;
        }
    }
    temp.c_str[k + 1] = '\0';
    temp.size = k + 1;
    StrBuilder_Swap(&temp, p);
    StrBuilder_Destroy(&temp);
}

void StrBuilder_AppendFmtV(struct StrBuilder* p, const char* fmt, va_list va)
{
    char buffer[500];
    vsnprintf(buffer, 500, fmt, va);
    StrBuilder_Append(p, buffer);
}

void StrBuilder_AppendFmt(struct StrBuilder* p, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[500];
    vsnprintf(buffer, 500, fmt, args);
    StrBuilder_Append(p, buffer);
    va_end(args);
}

void StrBuilder_AppendFmtIdent(struct StrBuilder* p, int nspaces, const char* fmt, ...)
{
    for (int i = 0; i < nspaces; i++)
    {
        StrBuilder_Append(p, " ");
    }
    va_list args;
    va_start(args, fmt);
    char buffer[500];
    vsnprintf(buffer, 500, fmt, args);
    StrBuilder_Append(p, buffer);
    va_end(args);
}

void StrBuilder_AppendFmtLn(struct StrBuilder* p, int nspaces, const char* fmt, ...)
{
    for (int i = 0; i < nspaces; i++)
    {
        StrBuilder_Append(p, " ");
    }
    va_list args;
    va_start(args, fmt);
    char buffer[500];
    vsnprintf(buffer, 500, fmt, args);
    StrBuilder_Append(p, buffer);
    va_end(args);
    StrBuilder_Append(p, "\n");
}

struct HashMapEntry
{
    struct HashMapEntry* pNext;
    unsigned int HashValue;
    char* Key;
    void* pValue;
};

struct HashMap
{
    struct HashMapEntry** pHashTable;
    unsigned int nHashTableSize;
    int  nCount;
};

#define HASHMAP_INIT { NULL, 0, 0 }

static void KeyValue_Delete(struct HashMapEntry* p)
{
    if (p != NULL)
    {
        free((void*)p->Key);
        free((void*)p);
    }
}

static struct HashMapEntry* HashMap_GetAssocAt(
    struct HashMap* pMap,
    const char* Key,
    unsigned int* nHashBucket,
    unsigned int* HashValue);


static unsigned int String2_HashKey(const char* Key)
{
    // hash key to unsigned int value by pseudorandomizing transform
    // (algorithm copied from STL char hash in xfunctional)
    unsigned int uHashVal = 2166136261U;
    unsigned int uFirst = 0;
    unsigned int uLast = (unsigned int)strlen(Key);
    unsigned int uStride = 1 + uLast / 10;
    for (; uFirst < uLast; uFirst += uStride)
    {
        uHashVal = 16777619U * uHashVal ^ (unsigned int)Key[uFirst];
    }
    return (uHashVal);
}

void HashMap_RemoveAll(struct HashMap* pMap, void(*DeleteFunc)(void*))
{
    if (pMap->pHashTable != NULL)
    {
        for (unsigned int nHash = 0;
             nHash < pMap->nHashTableSize;
             nHash++)
        {
            struct HashMapEntry* pKeyValue =
                pMap->pHashTable[nHash];
            while (pKeyValue != NULL)
            {
                struct HashMapEntry* pKeyValueCurrent = pKeyValue;
                if (DeleteFunc)
                    DeleteFunc(pKeyValueCurrent->pValue);
                pKeyValue = pKeyValue->pNext;
                KeyValue_Delete(pKeyValueCurrent);
            }
        }
        free(pMap->pHashTable);
        pMap->pHashTable = NULL;
        pMap->nCount = 0;
    }
}

void HashMap_Destroy(struct HashMap* pMap, void(*DeleteFunc)(void*))
{
    HashMap_RemoveAll(pMap, DeleteFunc);
}

static struct HashMapEntry* HashMap_GetAssocAt(
    struct HashMap* pMap,
    const char* Key,
    unsigned int* nHashBucket,
    unsigned int* HashValue)
{
    if (pMap->pHashTable == NULL)
    {
        *HashValue = 0;
        *nHashBucket = 0;
        return NULL;
    }
    *HashValue = String2_HashKey(Key);
    *nHashBucket = *HashValue % pMap->nHashTableSize;
    struct HashMapEntry* pResult = NULL;
    struct HashMapEntry* pKeyValue =
        pMap->pHashTable[*nHashBucket];
    for (; pKeyValue != NULL; pKeyValue = pKeyValue->pNext)
    {
        if (pKeyValue->HashValue == *HashValue &&
            strcmp(pKeyValue->Key, Key) == 0)
        {
            pResult = pKeyValue;
            break;
        }
    }
    return pResult;
}

bool HashMap_Lookup(struct HashMap* pMap, const char* Key, void** rValue)
{
    bool bResult = false;
    unsigned int nHashBucket, HashValue;
    struct HashMapEntry* pKeyValue = HashMap_GetAssocAt(pMap,
                                                        Key,
                                                        &nHashBucket,
                                                        &HashValue);
    if (pKeyValue != NULL)
    {
        *rValue = pKeyValue->pValue;
        bResult = true;
    }
    return bResult;
}

bool HashMap_LookupKey(struct HashMap* pMap, const char* Key, const char** rKey)
{
    bool bResult = false;
    unsigned int nHashBucket, HashValue;
    struct HashMapEntry* pKeyValue = HashMap_GetAssocAt(pMap,
                                                        Key,
                                                        &nHashBucket,
                                                        &HashValue);
    if (pKeyValue != NULL)
    {
        *rKey = pKeyValue->Key;
        bResult = true;
    }
    return bResult;
}

bool HashMap_RemoveKey(struct HashMap* pMap, const char* Key, void** ppValue)
{
    *ppValue = 0;
    bool bResult = false;
    if (pMap->pHashTable != NULL)
    {
        unsigned int HashValue =
            String2_HashKey(Key);
        struct HashMapEntry** ppKeyValuePrev =
            &pMap->pHashTable[HashValue % pMap->nHashTableSize];
        struct HashMapEntry* pKeyValue = *ppKeyValuePrev;
        for (; pKeyValue != NULL; pKeyValue = pKeyValue->pNext)
        {
            if ((pKeyValue->HashValue == HashValue) &&
                (strcmp(pKeyValue->Key, Key) == 0))
            {
                // remove from list
                *ppKeyValuePrev = pKeyValue->pNext;
                *ppValue = pKeyValue->pValue;
                KeyValue_Delete(pKeyValue);
                bResult = true;
                break;
            }
            ppKeyValuePrev = &pKeyValue->pNext;
        }
    }
    return bResult;
}

int HashMap_SetAt(struct HashMap* pMap, const char* Key, void* newValue, void** ppPreviousValue)
{
    int result = 0;
    *ppPreviousValue = NULL;
    if (pMap->pHashTable == NULL)
    {
        if (pMap->nHashTableSize < 1)
        {
            pMap->nHashTableSize = 1000;
        }
        struct HashMapEntry** pHashTable =
            (struct HashMapEntry**)malloc(sizeof(struct HashMapEntry*) * pMap->nHashTableSize);
        if (pHashTable != NULL)
        {
            memset(pHashTable, 0, sizeof(struct HashMapEntry*) * pMap->nHashTableSize);
            pMap->pHashTable = pHashTable;
        }
    }
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct HashMapEntry* pKeyValue =
            HashMap_GetAssocAt(pMap,
                               Key,
                               &nHashBucket,
                               &HashValue);
        if (pKeyValue == NULL)
        {
            pKeyValue = (struct HashMapEntry*)malloc(sizeof(struct HashMapEntry) * 1);
            pKeyValue->HashValue = HashValue;
            pKeyValue->pValue = newValue;
            pKeyValue->Key = strdup(Key);
            pKeyValue->pNext = pMap->pHashTable[nHashBucket];
            pMap->pHashTable[nHashBucket] = pKeyValue;
            pMap->nCount++;
            result = 0;
        }
        else
        {
            result = 1;
            *ppPreviousValue = pKeyValue->pValue;
            pKeyValue->pValue = newValue;
            free(pKeyValue->Key);
            pKeyValue->Key = strdup(Key);
        }
    }
    return result;
}

void HashMap_Delete(struct HashMap* p, void(*DeleteFunc)(void*))
{
    if (p != NULL)
    {
        HashMap_Destroy(p, DeleteFunc);
        free((void*)p);
    }
}


#define CAST(FROM, TO) \
static inline struct TO *  FROM##_As_##TO( struct FROM*  p)\
{\
if (p != NULL &&  p->Type == TO##_ID)\
    return ( struct TO * )p;\
  return NULL;\
}\
static inline  struct FROM *  TO##_As_##FROM(struct TO*  p)\
{\
    return (  struct FROM * )p;\
}

#define CASTSAME(FROM, TO) \
static inline struct TO * FROM##_As_##TO(struct FROM* p) { return (struct TO * ) p; }\
static inline struct FROM * TO##_As_##FROM(struct TO* p) { return (struct FROM *) p; }



static void SymbolMap_KeyValue_Delete(struct SymbolMapItem* p)
{
    if (p)
    {
        free(p->Key);
        free(p);
    }
}

struct SymbolMapItem* SymbolMap_GetAssocAt(struct SymbolMap* pMap,
    const char* Key,
                                           unsigned int* nHashBucket,
                                           unsigned int* HashValue);


void SymbolMap_RemoveAll(struct SymbolMap* pMap)
{
    if (pMap->pHashTable != NULL)
    {
        for (int nHash = 0;
             nHash < pMap->nHashTableSize;
             nHash++)
        {
            struct SymbolMapItem* pKeyValue =
                pMap->pHashTable[nHash];
            while (pKeyValue != NULL)
            {
                struct SymbolMapItem* pKeyValueCurrent = pKeyValue;
                pKeyValue = pKeyValue->pNext;
                SymbolMap_KeyValue_Delete(pKeyValueCurrent);
            }
        }
        free(pMap->pHashTable);
        pMap->pHashTable = NULL;
        pMap->nCount = 0;
    }
}

void SymbolMap_Destroy(struct SymbolMap* pMap)
{
    SymbolMap_RemoveAll(pMap);
}

struct SymbolMapItem* SymbolMap_FindBucket(struct SymbolMap* pMap, const char* Key)
{
    if (pMap->pHashTable == NULL)
    {
        return NULL;
    }
    unsigned int HashValue = String2_HashKey(Key);
    unsigned int nHashBucket = HashValue % pMap->nHashTableSize;
    struct SymbolMapItem* pKeyValue =
        pMap->pHashTable[nHashBucket];
    return pKeyValue;
}

struct SymbolMapItem* SymbolMap_GetAssocAt(
    struct SymbolMap* pMap,
    const char* Key,
    unsigned int* nHashBucket,
    unsigned int* HashValue)
{
    if (pMap->pHashTable == NULL)
    {
        *HashValue = 0;
        *nHashBucket = 0;
        return NULL;
    }
    *HashValue = String2_HashKey(Key);
    *nHashBucket = *HashValue % pMap->nHashTableSize;
    struct SymbolMapItem* pResult = NULL;
    struct SymbolMapItem* pKeyValue =
        pMap->pHashTable[*nHashBucket];
    for (; pKeyValue != NULL; pKeyValue = pKeyValue->pNext)
    {
        if (pKeyValue->HashValue == *HashValue &&
            strcmp(pKeyValue->Key, Key) == 0)
        {
            pResult = pKeyValue;
            break;
        }
    }
    return pResult;
}

struct TypePointer* SymbolMap_Find(struct SymbolMap* pMap, const char* Key)
{
    struct TypePointer* pTypePointer = NULL;
    unsigned int nHashBucket, HashValue;
    struct SymbolMapItem* pKeyValue = SymbolMap_GetAssocAt(pMap,
                                                           Key,
                                                           &nHashBucket,
                                                           &HashValue);
    if (pKeyValue != NULL)
    {
        pTypePointer = pKeyValue->pValue;
    }
    if (pTypePointer == NULL && pMap->pPrevious != NULL)
    {
        pTypePointer = SymbolMap_Find(pMap->pPrevious, Key);
    }
    return pTypePointer;
}

bool SymbolMap_LookupKey(struct SymbolMap* pMap,
                         const char* Key,
                         const char** rKey)
{
    bool bResult = false;
    unsigned int nHashBucket, HashValue;
    struct SymbolMapItem* pKeyValue = SymbolMap_GetAssocAt(pMap,
                                                           Key,
                                                           &nHashBucket,
                                                           &HashValue);
    if (pKeyValue != NULL)
    {
        *rKey = pKeyValue->Key;
        bResult = true;
    }
    return bResult;
}

bool SymbolMap_RemoveKey(struct SymbolMap* pMap,
                         const char* Key,
                         struct TypePointer** ppValue)
{
    *ppValue = 0;
    bool bResult = false;
    if (pMap->pHashTable != NULL)
    {
        unsigned int HashValue =
            String2_HashKey(Key);
        struct SymbolMapItem** ppKeyValuePrev =
            &pMap->pHashTable[HashValue % pMap->nHashTableSize];
        struct SymbolMapItem* pKeyValue = *ppKeyValuePrev;
        for (; pKeyValue != NULL; pKeyValue = pKeyValue->pNext)
        {
            if ((pKeyValue->HashValue == HashValue) &&
                (strcmp(pKeyValue->Key, Key) == 0))
            {
                // remove from list
                *ppKeyValuePrev = pKeyValue->pNext;
                *ppValue = pKeyValue->pValue;
                SymbolMap_KeyValue_Delete(pKeyValue);
                bResult = true;
                break;
            }
            ppKeyValuePrev = &pKeyValue->pNext;
        }
    }
    return bResult;
}

int SymbolMap_SetAt(struct SymbolMap* pMap,
                    const char* Key,
                    struct TypePointer* newValue)
{
    int result = 0;
    if (pMap->pHashTable == NULL)
    {
        if (pMap->nHashTableSize < 1)
        {
            pMap->nHashTableSize = 1000;
        }
        struct SymbolMapItem** pHashTable =
            (struct SymbolMapItem**)malloc(sizeof(struct SymbolMapItem*) * pMap->nHashTableSize);
        if (pHashTable != NULL)
        {
            memset(pHashTable, 0, sizeof(struct SymbolMapItem*) * pMap->nHashTableSize);
            pMap->pHashTable = pHashTable;
        }
    }
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 Key,
                                 &nHashBucket,
                                 &HashValue);
        //if (pKeyValue == NULL)
        {
            pKeyValue = (struct SymbolMapItem*)malloc(sizeof(struct SymbolMapItem) * 1);
            pKeyValue->HashValue = HashValue;
            pKeyValue->pValue = newValue;
            pKeyValue->Key = strdup(Key);
            pKeyValue->pNext = pMap->pHashTable[nHashBucket];
            pMap->pHashTable[nHashBucket] = pKeyValue;
            pMap->nCount++;
            result = 0;
        }
        //else
        //{
        //    result = 1;
        //    pKeyValue->pValue = newValue;
        //    strset(pKeyValue->Key, Key);
        //}
    }
    return result;
}

void SymbolMap_Init(struct SymbolMap* p)
{
    struct SymbolMap temp = SYMBOLMAP_INIT;
    *p = temp;
}

void SymbolMap_Swap(struct SymbolMap* pA, struct SymbolMap* pB)
{
    struct SymbolMap temp = *pA;
    *pA = *pB;
    *pB = temp;
}


const char* PrintType(enum Type type)
{
    switch (type)
    {
        case Null_ID:
        case Declaration_ID:
            return "TDeclaration_ID";
        case StaticAssertDeclaration_ID:
        case EofDeclaration_ID:
        case SingleTypeSpecifier_ID:
            return "TSingleTypeSpecifier_ID";
        case EnumSpecifier_ID:
            return "TEnumSpecifier_ID";
        case StructUnionSpecifier_ID:
            return "TStructUnionSpecifier_ID";
        case StorageSpecifier_ID:
        case AtomicTypeSpecifier_ID:
        case TemplateTypeSpecifier_ID:
        case StructDeclaration_ID:
        case AlignmentSpecifier_ID:
        case TypeQualifier_ID:
        case FunctionSpecifier_ID:
        case CompoundStatement_ID:
        case ExpressionStatement_ID:
        case SwitchStatement_ID:
        case LabeledStatement_ID:
        case ForStatement_ID:
        case JumpStatement_ID:
        case AsmStatement_ID:
        case WhileStatement_ID:
        case DoStatement_ID:
        case IfStatement_ID:
        case TypeName_ID:
        case InitializerListType_ID:
        case PrimaryExpression_ID:
        case UnaryExpressionOperator_ID:
        case CastExpressionType_ID:
        case PrimaryExpressionValue_ID:
        case PrimaryExpressionLiteral_ID:
        case PostfixExpression_ID:
        case BinaryExpression_ID:
        case TernaryExpression_ID:
        case Enumerator_ID:
            break;
        default:
            break;
    }
    return "";
}

static void SymbolMap_PrintCore(struct SymbolMap* pMap, int* n)
{
    if (pMap->pPrevious)
    {
        SymbolMap_PrintCore(pMap->pPrevious, n);
        (*n)++;
    }
    for (int k = 0; k < *n; k++)
    {
        printf(" ");
    }
    if (pMap->pHashTable != NULL)
    {
        for (int i = 0; i < pMap->nHashTableSize; i++)
        {
            struct SymbolMapItem* pSymbolMapItem = pMap->pHashTable[i];
            while (pSymbolMapItem != NULL)
            {
                printf("%s = %s\n", pSymbolMapItem->Key, PrintType(pSymbolMapItem->pValue->Type));
                pSymbolMapItem = pSymbolMapItem->pNext;
            }
        }
    }
}


CAST(DeclarationSpecifier, StorageSpecifier)
CAST(DeclarationSpecifier, FunctionSpecifier)
CAST(DeclarationSpecifier, AlignmentSpecifier)
CAST(DeclarationSpecifier, SingleTypeSpecifier)
CAST(DeclarationSpecifier, EnumSpecifier)

CAST(TypeSpecifier, SingleTypeSpecifier)
CAST(TypeSpecifier, EnumSpecifier)
CAST(TypeSpecifier, StructUnionSpecifier)
CAST(DeclarationSpecifier, StructUnionSpecifier)
CAST(SpecifierQualifier, StructUnionSpecifier)
CAST(TypeSpecifier, AtomicTypeSpecifier)

CAST(AnyDeclaration, Declaration)

void SymbolMap_Print(struct SymbolMap* pMap)
{
    int n = 0;
    SymbolMap_PrintCore(pMap, &n);
}

bool SymbolMap_IsTypeName(struct SymbolMap* pMap, const char* identifierName)
{
    bool bIsTypeName = false;
    bool foundResult = false;
    while (pMap)
    {
        struct SymbolMapItem* pBucket =
            SymbolMap_FindBucket(pMap, identifierName);
        while (pBucket)
        {
            if (pBucket->pValue->Type == Declaration_ID &&
                strcmp(pBucket->Key, identifierName) == 0)
            {
                foundResult = true;
                struct Declaration* pDeclaration =
                    (struct Declaration*)pBucket->pValue;
                for (int i = 0; i < pDeclaration->Specifiers.Size; i++)
                {
                    struct DeclarationSpecifier* pItem = pDeclaration->Specifiers.pData[i];
                    if (pItem->Type == StorageSpecifier_ID)
                    {
                        struct StorageSpecifier* pStorageSpecifier =
                            (struct StorageSpecifier*)pItem;
                        if (pStorageSpecifier->Token == TK_TYPEDEF)
                        {
                            bIsTypeName = true;
                            break;
                        }
                    }
                }
            }
            if (foundResult)
                break;
            pBucket = pBucket->pNext;
        }
        if (foundResult)
            break;
        pMap = pMap->pPrevious;
    }
    return bIsTypeName;
}

struct EnumSpecifier* SymbolMap_FindCompleteEnumSpecifier(struct SymbolMap* pMap, const char* enumName)
{
    struct EnumSpecifier* pResult = NULL;
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 enumName,
                                 &nHashBucket,
                                 &HashValue);
        while (pKeyValue != NULL)
        {
            if (pKeyValue->pValue->Type == EnumSpecifier_ID &&
                strcmp(pKeyValue->Key, enumName) == 0)
            {
                struct EnumSpecifier* pEnumSpecifier =
                    (struct EnumSpecifier*)pKeyValue->pValue;
                if (pEnumSpecifier->EnumeratorList.pHead != NULL)
                {
                    pResult = pEnumSpecifier; //complete definition found
                    break;
                }
            }
            pKeyValue = pKeyValue->pNext;
        }
    }
    return pResult;
}

struct Declaration* SymbolMap_FindFunction(struct SymbolMap* pMap, const char* funcName)
{
    struct Declaration* pDeclaration = NULL;
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 funcName,
                                 &nHashBucket,
                                 &HashValue);
        while (pKeyValue != NULL)
        {
            //Obs enum struct e union compartilham um mapa unico
            if (pKeyValue->pValue->Type == Declaration_ID)
            {
                if (strcmp(pKeyValue->Key, funcName) == 0)
                {
                    pDeclaration =
                        (struct Declaration*)pKeyValue->pValue;
                    break;
                }
            }
            pKeyValue = pKeyValue->pNext;
        }
    }
    return pDeclaration;
}

struct StructUnionSpecifier* SymbolMap_FindCompleteStructUnionSpecifier(struct SymbolMap* pMap, const char* structTagName)
{
    if (structTagName == NULL)
    {
        return NULL;
    }
    struct StructUnionSpecifier* pStructUnionSpecifier = NULL;
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 structTagName,
                                 &nHashBucket,
                                 &HashValue);
        while (pKeyValue != NULL)
        {
            //Obs enum struct e union compartilham um mapa unico
            if (pKeyValue->pValue->Type == StructUnionSpecifier_ID)
            {
                if (strcmp(pKeyValue->Key, structTagName) == 0)
                {
                    pStructUnionSpecifier =
                        (struct StructUnionSpecifier*)pKeyValue->pValue;
                    if (pStructUnionSpecifier->StructDeclarationList.Size > 0 ||
                        pStructUnionSpecifier->UnionSet.pHead != NULL)
                    {
                        //Se achou definicao completa pode sair
                        //se achou um _union pode sair tb
                        //pois nao tem definicao completa de union
                        break;
                    }
                }
            }
            pKeyValue = pKeyValue->pNext;
        }
    }
    return pStructUnionSpecifier;
}

struct Declaration* SymbolMap_FindTypedefDeclarationTarget(struct SymbolMap* pMap,
    const char* typedefName)
{
    struct Declaration* pDeclarationResult = NULL;
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 typedefName,
                                 &nHashBucket,
                                 &HashValue);
        while (pKeyValue != NULL)
        {
            if (pKeyValue->pValue->Type == Declaration_ID &&
                strcmp(pKeyValue->Key, typedefName) == 0)
            {
                struct Declaration* pDeclaration =
                    (struct Declaration*)pKeyValue->pValue;
                //typedef X Y;
                bool bIsTypedef = false;
                const char* indirectTypedef = NULL;
                for (int i = 0; i < pDeclaration->Specifiers.Size; i++)
                {
                    struct DeclarationSpecifier* pItem = pDeclaration->Specifiers.pData[i];
                    switch (pItem->Type)
                    {
                        case StorageSpecifier_ID:
                        {
                            struct StorageSpecifier* pStorageSpecifier =
                                (struct StorageSpecifier*)pItem;
                            if (pStorageSpecifier->Token == TK_TYPEDEF)
                            {
                                bIsTypedef = true;
                            }
                        }
                        break;
                        case SingleTypeSpecifier_ID:
                        {
                            struct SingleTypeSpecifier* pSingleTypeSpecifier =
                                (struct SingleTypeSpecifier*)pItem;
                            if (pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
                            {
                                indirectTypedef = pSingleTypeSpecifier->TypedefName;
                            }
                        }
                        break;
                        default:
                            //assert(false);
                            break;
                    }
                }
                if (!bIsTypedef)
                {
                    //Nao eh um typedef
                    break;
                }
                else
                {
                    if (indirectTypedef != NULL)
                    {
                        //eh um typedef indireto
                        pDeclarationResult =
                            SymbolMap_FindTypedefDeclarationTarget(pMap, indirectTypedef);
                    }
                    else
                    {
                        //'e um typedef direto - retorna a declaracao que ele aparece
                        pDeclarationResult = pDeclaration;
                    }
                    break;
                }
            }
            pKeyValue = pKeyValue->pNext;
        }
    }
    return pDeclarationResult;
}

//Acha o tipo final de um typedef
//e vai somando as partes dos declaratos
//por exemplo no meio do caminho dos typedefs
//pode ter ponteiros e depois const etc.
struct DeclarationSpecifiers* SymbolMap_FindTypedefTarget(struct SymbolMap* pMap,
    const char* typedefName,
                                                          struct Declarator* declarator)
{
    //struct TDeclaration* pDeclarationResult = NULL;
    struct DeclarationSpecifiers* pSpecifiersResult = NULL;
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 typedefName,
                                 &nHashBucket,
                                 &HashValue);
        while (pKeyValue != NULL)
        {
            if (pKeyValue->pValue->Type == Declaration_ID &&
                strcmp(pKeyValue->Key, typedefName) == 0)
            {
                struct Declaration* pDeclaration =
                    (struct Declaration*)pKeyValue->pValue;
                //typedef X Y;
                bool bIsTypedef = false;
                const char* indirectTypedef = NULL;
                for (int i = 0; i < pDeclaration->Specifiers.Size; i++)
                {
                    struct DeclarationSpecifier* pItem = pDeclaration->Specifiers.pData[i];
                    switch (pItem->Type)
                    {
                        case StorageSpecifier_ID:
                        {
                            struct StorageSpecifier* pStorageSpecifier =
                                (struct StorageSpecifier*)pItem;
                            if (pStorageSpecifier->Token == TK_TYPEDEF)
                            {
                                bIsTypedef = true;
                            }
                        }
                        break;
                        case SingleTypeSpecifier_ID:
                        {
                            struct SingleTypeSpecifier* pSingleTypeSpecifier =
                                (struct SingleTypeSpecifier*)pItem;
                            if (pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
                            {
                                indirectTypedef = pSingleTypeSpecifier->TypedefName;
                            }
                        }
                        break;
                        default:
                            //assert(false);
                            break;
                    }
                }
                if (!bIsTypedef)
                {
                    //Nao eh um typedef
                    break;
                }
                else
                {
                    if (indirectTypedef != NULL)
                    {
                        struct Declarator* pDeclarator =
                            Declaration_FindDeclarator(pDeclaration, typedefName);
                        if (pDeclarator)
                        {
                            //copiar o pointer list deste typedef para o outro
                            for (struct Pointer* pItem = pDeclarator->PointerList.pHead;
                                 pItem != NULL;
                                 pItem = pItem->pNext)
                            {
                                struct Pointer* pNew = NEW((struct Pointer)POINTER_INIT);
                                Pointer_CopyFrom(pNew, pItem);
                                PointerList_PushBack(&declarator->PointerList, pNew);
                            }
                            //eh um typedef indireto
                            pSpecifiersResult =
                                SymbolMap_FindTypedefTarget(pMap, indirectTypedef, declarator);
                        }
                        else
                        {
                            //assert(false);
                        }
                    }
                    else
                    {
                        //'e um typedef direto - retorna a declaracao que ele aparece
                        pSpecifiersResult = &pDeclaration->Specifiers;
                    }
                    break;
                }
            }
            pKeyValue = pKeyValue->pNext;
        }
    }
    return pSpecifiersResult;// &pDeclarationResult->Specifiers;
}



//Acha o primeiro typedef
//somas as partes do declarator
struct DeclarationSpecifiers* SymbolMap_FindTypedefFirstTarget(struct SymbolMap* pMap,
    const char* typedefName,
                                                               struct Declarator* declarator)
{
    //struct TDeclaration* pDeclarationResult = NULL;
    struct DeclarationSpecifiers* pSpecifiersResult = NULL;
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 typedefName,
                                 &nHashBucket,
                                 &HashValue);
        while (pKeyValue != NULL)
        {
            if (pKeyValue->pValue->Type == Declaration_ID &&
                strcmp(pKeyValue->Key, typedefName) == 0)
            {
                struct Declaration* pDeclaration =
                    (struct Declaration*)pKeyValue->pValue;
                //typedef X Y;
                bool bIsTypedef = false;
                const char* indirectTypedef = NULL;
                for (int i = 0; i < pDeclaration->Specifiers.Size; i++)
                {
                    struct DeclarationSpecifier* pItem = pDeclaration->Specifiers.pData[i];
                    switch (pItem->Type)
                    {
                        case StorageSpecifier_ID:
                        {
                            struct StorageSpecifier* pStorageSpecifier =
                                (struct StorageSpecifier*)pItem;
                            if (pStorageSpecifier->Token == TK_TYPEDEF)
                            {
                                bIsTypedef = true;
                            }
                        }
                        break;
                        case SingleTypeSpecifier_ID:
                        {
                            struct SingleTypeSpecifier* pSingleTypeSpecifier =
                                (struct SingleTypeSpecifier*)pItem;
                            if (pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
                            {
                                indirectTypedef = pSingleTypeSpecifier->TypedefName;
                            }
                        }
                        break;
                        default:
                            //assert(false);
                            break;
                    }
                }
                if (!bIsTypedef)
                {
                    //Nao eh um typedef
                    break;
                }
                else
                {
                    if (indirectTypedef != NULL)
                    {
                        struct Declarator* pDeclarator =
                            Declaration_FindDeclarator(pDeclaration, typedefName);
                        if (pDeclarator)
                        {
                            //copiar o pointer list deste typedef para o outro
                            for (struct Pointer* pItem = pDeclarator->PointerList.pHead;
                                 pItem != NULL;
                                 pItem = pItem->pNext)
                            {
                                struct Pointer* pNew = NEW((struct Pointer)POINTER_INIT);
                                Pointer_CopyFrom(pNew, pItem);
                                PointerList_PushBack(&declarator->PointerList, pNew);
                            }
                            //eh um typedef indireto
                            pSpecifiersResult = &pDeclaration->Specifiers;
                            //pSpecifiersResult =
                            //SymbolMap_FindTypedefTarget(pMap, indirectTypedef, declarator);
                        }
                        else
                        {
                            //assert(false);
                        }
                    }
                    else
                    {
                        //'e um typedef direto - retorna a declaracao que ele aparece
                        struct Declarator* pDeclarator =
                            Declaration_FindDeclarator(pDeclaration, typedefName);
                        //copiar o pointer list deste typedef para o outro
                        for (struct Pointer* pItem = pDeclarator->PointerList.pHead;
                             pItem != NULL;
                             pItem = pItem->pNext)
                        {
                            struct Pointer* pNew = NEW((struct Pointer)POINTER_INIT);
                            Pointer_CopyFrom(pNew, pItem);
                            PointerList_PushBack(&declarator->PointerList, pNew);
                        }
                        pSpecifiersResult = &pDeclaration->Specifiers;
                    }
                    break;
                }
            }
            pKeyValue = pKeyValue->pNext;
        }
    }
    return pSpecifiersResult;// &pDeclarationResult->Specifiers;
}
struct TypeSpecifier* SymbolMap_FindTypedefSpecifierTarget(struct SymbolMap* pMap,
    const char* typedefName)
{
    /*Sample:
    struct X;
    typedef struct X X;
    struct X { int i;  };
    typedef X Y;
    */
    struct TypeSpecifier* pSpecifierTarget = NULL;
    struct Declaration* pDeclaration =
        SymbolMap_FindTypedefDeclarationTarget(pMap, typedefName);
    if (pDeclaration)
    {
        for (int i = 0; i < pDeclaration->Specifiers.Size; i++)
        {
            struct DeclarationSpecifier* pItem = pDeclaration->Specifiers.pData[i];
            switch (pItem->Type)
            {
                case SingleTypeSpecifier_ID:
                    pSpecifierTarget = (struct TypeSpecifier*)pItem;
                    break;
                case StructUnionSpecifier_ID:
                {
                    struct StructUnionSpecifier* pStructUnionSpecifier =
                        (struct StructUnionSpecifier*)pItem;
                    if (pStructUnionSpecifier->StructDeclarationList.Size == 0)
                    {
                        if (pStructUnionSpecifier->Tag != NULL)
                        {
                            pSpecifierTarget = (struct TypeSpecifier*)SymbolMap_FindCompleteStructUnionSpecifier(pMap, pStructUnionSpecifier->Tag);
                        }
                        else
                        {
                            //assert(false);
                        }
                    }
                    else
                    {
                        pSpecifierTarget = (struct TypeSpecifier*)pStructUnionSpecifier;
                    }
                }
                break;
                case EnumSpecifier_ID:
                {
                    struct EnumSpecifier* pEnumSpecifier =
                        (struct EnumSpecifier*)pItem;
                    if (pEnumSpecifier->EnumeratorList.pHead == NULL)
                    {
                        if (pEnumSpecifier->Tag != NULL)
                        {
                            pEnumSpecifier = SymbolMap_FindCompleteEnumSpecifier(pMap, pEnumSpecifier->Tag);
                        }
                        else
                        {
                            //assert(false);
                        }
                    }
                    else
                    {
                        pSpecifierTarget = (struct TypeSpecifier*)pEnumSpecifier;
                    }
                }
                break;
                default:
                    break;
            }
            if (pSpecifierTarget != NULL)
            {
                //ja achou
                break;
            }
        }
    }
    return pSpecifierTarget;
}


#define FILEINFO_INIT {0}
void FileInfo_Delete(struct FileInfo* p);

typedef struct HashMap FileInfoMap;

void FileInfoMap_Destroy(FileInfoMap* p);
bool FileInfoMap_Set(FileInfoMap* map, const char* key, struct FileInfo* data);
struct FileInfo* FileInfoMap_Find(FileInfoMap* map, const char* key);


void FileInfo_Destroy(struct FileInfo* p)
{
    free((void*)p->FullPath);
    free((void*)p->IncludePath);
}

void FileInfo_Delete(struct FileInfo* p)
{
    if (p != NULL)
    {
        FileInfo_Destroy(p);
        free((void*)p);
    }
}

static void FileInfo_DeleteVoid(void* p)
{
    FileInfo_Delete((struct FileInfo*)p);
}

void FileInfoMap_Destroy(FileInfoMap* p)
{
    HashMap_Destroy(p, FileInfo_DeleteVoid);
}

void FileArray_Init(struct FileArray* p);
void FileArray_Destroy(struct FileArray* p);
void FileArray_PushBack(struct FileArray* p, struct FileInfo* pItem);
void FileArray_Reserve(struct FileArray* p, int n);



void FileArray_Init(struct FileArray* p)
{
    p->pItems = NULL;
    p->Size = 0;
    p->Capacity = 0;
}
void FileArray_Destroy(struct FileArray* p)
{
    for (int i = 0; i < p->Size; i++)
    {
        FileInfo_Delete(p->pItems[i]);
    }
    free((void*)p->pItems);
}

void FileArray_Reserve(struct FileArray* p, int n)
{
    if (n > p->Capacity)
    {
        struct FileInfo** pnew = p->pItems;
        pnew = (struct FileInfo**)realloc(pnew, n * sizeof(struct FileInfo*));
        if (pnew)
        {
            p->pItems = pnew;
            p->Capacity = n;
        }
    }
}

void FileArray_PushBack(struct FileArray* p, struct FileInfo* pItem)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        FileArray_Reserve(p, n);
    }
    p->pItems[p->Size] = pItem;
    p->Size++;
}

bool FileInfoMap_Set(FileInfoMap* map, const char* key, struct FileInfo* pFile)
{
    // tem que ser case insensitive!
    //assert(IsFullPath(key));
    // converter
    // Ajusta o file index de acordo com a entrada dele no mapa
    pFile->FileIndex = map->nCount;
    void* pFileOld = 0;
    bool result = HashMap_SetAt(map, key, pFile, &pFileOld);
    assert(pFileOld == 0);
    free(pFile->FullPath);
    pFile->FullPath = strdup(key);
    return result;
}

struct FileInfo* FileInfoMap_Find(FileInfoMap* map, const char* key)
{
    // tem que ser case insensitive!
    void* pFile = 0;
    bool b = HashMap_Lookup(map, key, &pFile);
    return b ? pFile : 0;
}


#define LOCALSTRBUILDER_INIT {0}

void LocalStrBuilder_Init(struct LocalStrBuilder* p);

void LocalStrBuilder_Swap(struct LocalStrBuilder* pA, struct LocalStrBuilder* pB);

void LocalStrBuilder_Destroy(struct LocalStrBuilder* p);

void LocalStrBuilder_Reserve(struct LocalStrBuilder* p, int nelements);

void LocalStrBuilder_Print(struct LocalStrBuilder* p);

void LocalStrBuilder_Clear(struct LocalStrBuilder* p);

void LocalStrBuilder_Grow(struct LocalStrBuilder* p, int nelements);

void LocalStrBuilder_Append(struct LocalStrBuilder* p, const char* source);

void LocalStrBuilder_AppendChar(struct LocalStrBuilder* p, char ch);

void LocalStrBuilder_Set(struct LocalStrBuilder* p, const char* source);

const char* TokenToNameString(enum TokenType tk)
{
    switch (tk)
    {
        case TK_NONE:
            return "TK_NONE";
        case TK_BOF:
            return "TK_BOF";
        case TK_EOF:
            return "TK_EOF";
        case TK_ENDMARK:
            return "TK_ENDMARK";
        case TK_LINE_COMMENT:
            return "TK_LINE_COMMENT";
        case TK_COMMENT:
            return "TK_COMMENT";
        case TK_OPEN_COMMENT:
            return "TK_OPEN_COMMENT";
        case TK_CLOSE_COMMENT:
            return "TK_CLOSE_COMMENT";
        case TK_STRING_LITERAL:
            return "TK_STRING_LITERAL";
        case TK_IDENTIFIER:
            return "TK_IDENTIFIER";
        case TK_SPACES:
            return "TK_SPACES";
        case TK_DECIMAL_INTEGER:
            return "TK_DECIMAL_INTEGER";
        case TK_HEX_INTEGER:
            return "TK_HEX_INTEGER";
        case TK_OCTAL_INTEGER:
            return "TK_OCTAL_INTEGER";
        case TK_FLOAT_NUMBER:
            return "TK_FLOAT_NUMBER";
        case TK_MACROPLACEHOLDER:
            return "TK_MACROPLACEHOLDER";
        case TK_BREAKLINE:
            return "TK_BREAKLINE";
        case TK_BACKSLASHBREAKLINE:
            return "TK_BACKSLASHBREAKLINE";
        case CHAR1:
            return "CHAR1";
        case CHARACTER_TABULATION:
            return "CHARACTER_TABULATION";
        case TK_PREPROCESSOR:
            return "TK_PREPROCESSOR";
        case TK_ERROR:
            return "TK_ERROR";
        case TK_EXCLAMATION_MARK:
            return "TK_EXCLAMATION_MARK";
        case TK_QUOTATION_MARK:
            return "TK_QUOTATION_MARK";
        case TK_NUMBER_SIGN:
            return "TK_NUMBER_SIGN";
        case TK_DOLLAR_SIGN:
            return "TK_DOLLAR_SIGN";
        case TK_PERCENT_SIGN:
            return "TK_PERCENT_SIGN";
        case TK_AMPERSAND:
            return "TK_AMPERSAND";
        case TK_APOSTROPHE:
            return "TK_APOSTROPHE";
        case TK_LEFT_PARENTHESIS:
            return "TK_LEFT_PARENTHESIS";
        case TK_RIGHT_PARENTHESIS:
            return "TK_RIGHT_PARENTHESIS";
        case TK_ASTERISK:
            return "TK_ASTERISK";
        case TK_PLUS_SIGN:
            return "TK_PLUS_SIGN";
        case TK_COMMA:
            return "TK_COMMA";
        case TK_HYPHEN_MINUS:
            return "TK_HYPHEN_MINUS";
        case TK_HYPHEN_MINUS_NEG:
            return "TK_HYPHEN_MINUS_NEG";
        case TK_FULL_STOP:
            return "TK_FULL_STOP";
        case TK_SOLIDUS:
            return "TK_SOLIDUS";
        case TK_COLON:
            return "TK_COLON";
        case TK_SEMICOLON:
            return "TK_SEMICOLON";
        case TK_LESS_THAN_SIGN:
            return "TK_LESS_THAN_SIGN";
        case TK_EQUALS_SIGN:
            return "TK_EQUALS_SIGN";
        case TK_GREATER_THAN_SIGN:
            return "TK_GREATER_THAN_SIGN";
        case TK_QUESTION_MARK:
            return "TK_QUESTION_MARK";
        case TK_COMMERCIAL_AT:
            return "TK_COMMERCIAL_AT";
        case TK_LEFT_SQUARE_BRACKET:
            return "TK_LEFT_SQUARE_BRACKET";
        case REVERSE_SOLIDUS:
            return "REVERSE_SOLIDUS";
        case TK_RIGHT_SQUARE_BRACKET:
            return "TK_RIGHT_SQUARE_BRACKET";
        case TK_CIRCUMFLEX_ACCENT:
            return "TK_CIRCUMFLEX_ACCENT";
        case TK_LOW_LINE:
            return "TK_LOW_LINE";
        case TK_GRAVE_ACCENT:
            return "TK_GRAVE_ACCENT";
        case TK_LEFT_CURLY_BRACKET:
            return "TK_LEFT_CURLY_BRACKET";
        case TK_VERTICAL_LINE:
            return "TK_VERTICAL_LINE";
        case TK_RIGHT_CURLY_BRACKET:
            return "TK_RIGHT_CURLY_BRACKET";
        case TK_TILDE:
            return "TK_TILDE";
        case TK_ARROW:
            return "TK_ARROW";
        case TK_PLUSPLUS:
            return "TK_PLUSPLUS";
        case TK_MINUSMINUS:
            return "TK_MINUSMINUS";
        case TK_LESSLESS:
            return "TK_LESSLESS";
        case TK_GREATERGREATER:
            return "TK_GREATERGREATER";
        case TK_LESSEQUAL:
            return "TK_LESSEQUAL";
        case TK_GREATEREQUAL:
            return "TK_GREATEREQUAL";
        case TK_EQUALEQUAL:
            return "TK_EQUALEQUAL";
        case TK_NOTEQUAL:
            return "TK_NOTEQUAL";
        case TK_ANDAND:
            return "TK_ANDAND";
        case TK_OROR:
            return "TK_OROR";
        case TK_MULTIEQUAL:
            return "TK_MULTIEQUAL";
        case TK_DIVEQUAL:
            return "TK_DIVEQUAL";
        case TK_PERCENT_EQUAL:
            return "TK_PERCENT_EQUAL";
        case TK_PLUSEQUAL:
            return "TK_PLUSEQUAL";
        case TK_MINUS_EQUAL:
            return "TK_MINUS_EQUAL";
        case TK_ANDEQUAL:
            return "TK_ANDEQUAL";
        case TK_CARETEQUAL:
            return "TK_CARETEQUAL";
        case TK_OREQUAL:
            return "TK_OREQUAL";
        case TK_NUMBERNUMBER:
            return "TK_NUMBERNUMBER";
        case TK_LESSCOLON:
            return "TK_LESSCOLON";
        case TK_COLONGREATER:
            return "TK_COLONGREATER";
        case TK_LESSPERCENT:
            return "TK_LESSPERCENT";
        case TK_PERCENTGREATER:
            return "TK_PERCENTGREATER";
        case TK_PERCENTCOLON:
            return "TK_PERCENTCOLON";
        case TK_DOTDOTDOT:
            return "TK_DOTDOTDOT";
        case TK_GREATERGREATEREQUAL:
            return "TK_GREATERGREATEREQUAL";
        case TK_LESSLESSEQUAL:
            return "TK_LESSLESSEQUAL";
        case TK_PERCENTCOLONPERCENTCOLON:
            return "TK_PERCENTCOLONPERCENTCOLON";
        case TK_CHAR_LITERAL:
            return "TK_CHAR_LITERAL";
        case TK_AUTO:
            return "TK_AUTO";
        case TK_BREAK:
            return "TK_BREAK";
        case TK_CASE:
            return "TK_CASE";
        case TK_CHAR:
            return "TK_CHAR";
        case TK_CONST:
            return "TK_CONST";
        case TK_CONTINUE:
            return "TK_CONTINUE";
        case TK_DEFAULT:
            return "TK_DEFAULT";
        case TK_DO:
            return "TK_DO";
        case TK_DOUBLE:
            return "TK_DOUBLE";
        case TK_ELSE:
            return "TK_ELSE";
        case TK_ENUM:
            return "TK_ENUM";
        case TK_EXTERN:
            return "TK_EXTERN";
        case TK_FLOAT:
            return "TK_FLOAT";
        case TK_FOR:
            return "TK_FOR";
        case TK_GOTO:
            return "TK_GOTO";
        case TK_IF:
            return "TK_IF";
        case TK_TRY:
            return "TK_TRY";
        case TK_DEFER:
            return "TK_DEFER";
        case TK_INT:
            return "TK_INT";
        case TK_LONG:
            return "TK_LONG";
        case TK__INT8:
            return "TK__INT8";
        case TK__INT16:
            return "TK__INT16";
        case TK__INT32:
            return "TK__INT32";
        case TK__INT64:
            return "TK__INT64";
        case TK__WCHAR_T:
            return "TK__WCHAR_T";
        case TK___DECLSPEC:
            return "TK___DECLSPEC";
        case TK_REGISTER:
            return "TK_REGISTER";
        case TK_RETURN:
            return "TK_RETURN";
        case TK_THROW:
            return "TK_THROW";
        case TK_SHORT:
            return "TK_SHORT";
        case TK_SIGNED:
            return "TK_SIGNED";
        case TK_SIZEOF:
            return "TK_SIZEOF";
        case TK_STATIC:
            return "TK_STATIC";
        case TK_STRUCT:
            return "TK_STRUCT";
        case TK_SWITCH:
            return "TK_SWITCH";
        case TK_TYPEDEF:
            return "TK_TYPEDEF";
        case TK_UNION:
            return "TK_UNION";
        case TK_UNSIGNED:
            return "TK_UNSIGNED";
        case TK_VOID:
            return "TK_VOID";
        case TK_VOLATILE:
            return "TK_VOLATILE";
        case TK_WHILE:
            return "TK_WHILE";
        case TK_CATCH:
            return "TK_CATCH";
        case TK__THREAD_LOCAL:
            return "TK__THREAD_LOCAL";
        case TK__BOOL:
            return "TK__BOOL";
        case TK__COMPLEX:
            return "TK__COMPLEX";
        case TK__ATOMIC:
            return "TK__ATOMIC";
        case TK_RESTRICT:
            return "TK_RESTRICT";
        case TK__STATIC_ASSERT:
            return "TK__STATIC_ASSERT";
        case TK_INLINE:
            return "TK_INLINE";
        case TK__INLINE:
            return "TK__INLINE";
        case TK__FORCEINLINE:
            return "TK__FORCEINLINE";
        case TK__NORETURN:
            return "TK__NORETURN";
        case TK__ALIGNAS:
            return "TK__ALIGNAS";
        case TK__GENERIC:
            return "TK__GENERIC";
        case TK__IMAGINARY:
            return "TK__IMAGINARY";
        case TK__ALINGOF:
            return "TK__ALINGOF";
        case TK__ASM:
            return "TK__ASM";
        case TK__PRAGMA:
            return "TK__PRAGMA";
        case TK__C99PRAGMA:
            return "TK__C99PRAGMA";
        case TK_PRE_INCLUDE:
            return "TK_PRE_INCLUDE";
        case TK_PRE_PRAGMA:
            return "TK_PRE_PRAGMA";
        case TK_PRE_IF:
            return "TK_PRE_IF";
        case TK_PRE_ELIF:
            return "TK_PRE_ELIF";
        case TK_PRE_IFNDEF:
            return "TK_PRE_IFNDEF";
        case TK_PRE_IFDEF:
            return "TK_PRE_IFDEF";
        case TK_PRE_ENDIF:
            return "TK_PRE_ENDIF";
        case TK_PRE_ELSE:
            return "TK_PRE_ELSE";
        case TK_PRE_ERROR:
            return "TK_PRE_ERROR";
        case TK_PRE_LINE:
            return "TK_PRE_LINE";
        case TK_PRE_UNDEF:
            return "TK_PRE_UNDEF";
        case TK_PRE_DEFINE:
            return "TK_PRE_DEFINE";
        case TK_MACRO_CALL:
            return "TK_MACRO_CALL";
        case TK_MACRO_EOF:
            return "TK_MACRO_EOF";
        case TK_FILE_EOF:
            return "TK_FILE_EOF";
        case TK_NEW:
            return "TK_NEW";
        default:
            break;
    }
    return "";
}

const char* TokenToString(enum TokenType tk)
{
    switch (tk)
    {
        case TK_NONE:
            return "None";
        case TK_BOF:
            return "Bof";
        case TK_EOF:
            return "Eof";
        case TK_LINE_COMMENT:
            return "LINE_COMMENT";
        case TK_COMMENT:
            return "COMMENT";
        case TK_CLOSE_COMMENT:
            return "CLOSE_COMMENT";
        case TK_OPEN_COMMENT:
            return "OPEN_COMMENT";
        case TK_STRING_LITERAL:
            return "LITERALSTR";
        case TK_IDENTIFIER:
            return "IDENTIFIER";
        case TK_SPACES:
            return "SPACES";
        case TK_DECIMAL_INTEGER:
            return "TK_DECIMAL_INTEGER";
        case TK_HEX_INTEGER:
            return "TK_HEX_INTEGER";
        case TK_FLOAT_NUMBER:
            return "TK_FLOAT_NUMBER";
        case TK_BREAKLINE:
            return "BREAKLINE";
        case TK_BACKSLASHBREAKLINE:
            return "TK_BACKSLASHBREAKLINE";
        case TK_PREPROCESSOR:
            return "PREPROCESSOR";
        case CHARACTER_TABULATION:
            return "CHARACTER_TABULATION";
        case TK_EXCLAMATION_MARK:
            return "!";// = '!';
        case TK_QUOTATION_MARK:
            return "\"";//,// = '\"';
        case TK_NUMBER_SIGN:
            return "#";//,// = '#';
        case TK_DOLLAR_SIGN:
            return "$";//,// = '$';
        case TK_PERCENT_SIGN:
            return "%";//,// = '%';
        case TK_AMPERSAND:
            return "&";//,// = '&';
        case TK_APOSTROPHE:
            return "'";//,// = '\'';
        case TK_LEFT_PARENTHESIS:
            return "(";//,// = '(';
        case TK_RIGHT_PARENTHESIS:
            return ")";//,// = ')';
        case TK_ASTERISK:
            return "*";//,// = '*';
        case TK_PLUS_SIGN:
            return "+";//,// = '+';
        case TK_COMMA:
            return ",";//,// = ',';
        case TK_HYPHEN_MINUS:
            return "-";//,// = '-';
        case TK_FULL_STOP:
            return ".";//,// = '.';
        case TK_SOLIDUS:
            return "/";//,// = '/';
        case TK_COLON:
            return ":";//,// = ':';
        case TK_SEMICOLON:
            return ";";//,// = ';';
        case TK_LESS_THAN_SIGN:
            return "<";//,// = '<';
        case TK_EQUALS_SIGN:
            return "=";//,// = '=';
        case TK_GREATER_THAN_SIGN:
            return ">";//,// = '>';
        case TK_QUESTION_MARK:
            return "?";//,// = '\?';
        case TK_COMMERCIAL_AT:
            return "@";//,// = '@';
        case TK_LEFT_SQUARE_BRACKET:
            return "[";//,// = '[';
        case REVERSE_SOLIDUS:
            return "\\";//,// = '\\';
        case TK_RIGHT_SQUARE_BRACKET:
            return "]";//,// = ']';
        case TK_CIRCUMFLEX_ACCENT:
            return "^";// = '^';
        case TK_LOW_LINE:
            return "_";//,// = '_';
        case TK_GRAVE_ACCENT:
            return "`";//,// = '`';
        case TK_LEFT_CURLY_BRACKET:
            return "{";//,// = '{';
        case TK_VERTICAL_LINE:
            return "|";//,// = '|';
        case TK_RIGHT_CURLY_BRACKET:
            return "}";//,// = '}';
        case TK_TILDE:
            return "~";//,// = '~';
            break;
        case TK_AUTO:
            return "auto";
        case TK_BREAK:
            return "break";
        case TK_CASE:
            return "case";
        case TK_CHAR:
            return "char";
        case TK_CONST:
            return "const";
        case TK_CONTINUE:
            return "continue";
        case TK_DEFAULT:
            return "default";
        case TK_RESTRICT:
            return "restrict";
        case TK_INLINE:
            return "inline";
        case TK__INLINE:
            return "_inline";
        case TK__NORETURN:
            return "_Noreturn";
        case TK_DO:
            return "do";
        case TK_DOUBLE:
            return "double";
        case TK_ELSE:
            return "else";
        case TK_ENUM:
            return "enum";
        case TK_EXTERN:
            return "extern";
        case TK_FLOAT:
            return "float";
        case TK_FOR:
            return "for";
        case TK_GOTO:
            return "goto";
        case TK_IF:
            return "if";
        case TK_TRY:
            return "try";
        case TK_DEFER:
            return "defer";
        case TK_INT:
            return "int";
        case TK_LONG:
            return "long";
        case TK__INT8:
            return "__int8";
        case TK__INT16:
            return "__int16";
        case TK__INT32:
            return "__int32";
        case TK__INT64:
            return "__int64";
        case TK__WCHAR_T:
            return "__wchar_t";
        case TK_REGISTER:
            return "register";
        case TK_RETURN:
            return "return";
        case TK_THROW:
            return "throw";
        case TK_SHORT:
            return "short";
        case TK_SIGNED:
            return "signed";
        case TK_SIZEOF:
            return "sizeof";
        case TK_STATIC:
            return "static";
        case TK_STRUCT:
            return "struct";
        case TK_SWITCH:
            return "switch";
        case TK_TYPEDEF:
            return "typedef";
        case TK_UNION:
            return "union";
        case TK_UNSIGNED:
            return "unsigned";
        case TK_VOID:
            return "void";
        case TK__BOOL:
            return "_Bool";
            break;
        case TK_VOLATILE:
            return "volatile";
        case TK_WHILE:
            return "while";
        case TK_CATCH:
            return "catch";
        case TK_ARROW:
            return "->";
        case TK_PLUSPLUS:
            return "++";
        case TK_MINUSMINUS:
            return "--";
        case TK_EQUALEQUAL:
            return "==";
        case TK_NOTEQUAL:
            return "!=";
        case TK_LESSLESS:
            return "<<";
        case TK_GREATERGREATER:
            return ">>";
        case TK_LESSEQUAL:
            return "<=";
        case TK_GREATEREQUAL:
            return ">=";
        case TK_ANDAND:
            return "&&";
        case TK_OROR:
            return "||";
        case TK_MULTIEQUAL:
            return "*=";
        case TK_DIVEQUAL:
            return "/=";
        case TK_PERCENT_EQUAL:
            return "/%=";
        case TK_PLUSEQUAL:
            return "+=";
        case TK_MINUS_EQUAL:
            return "-=";
        case TK_ANDEQUAL:
            return "!=";
        case TK_CARETEQUAL:
            return "^=";
        case TK_OREQUAL:
            return "|=";
        case TK_NUMBERNUMBER:
            return "##";
        case TK_LESSCOLON:
            return "<:";
        case TK_COLONGREATER:
            return ":>";
        case TK_LESSPERCENT:
            return "<%";
        case TK_PERCENTGREATER:
            return "%>";
        case TK_PERCENTCOLON:
            return "%:";
        case TK_DOTDOTDOT:
            return "...";
        case TK_GREATERGREATEREQUAL:
            return ">>=";
        case TK_LESSLESSEQUAL:
            return "<<=";
        case TK_PERCENTCOLONPERCENTCOLON:
            return "/%:/%:";
        case TK_PRE_INCLUDE:
            return "TK_PRE_INCLUDE";
        case TK_PRE_DEFINE:
            return "TK_PRE_DEFINE";
            //
        case TK_MACRO_CALL:
            return "TK_MACRO_CALL";
        case TK_MACRO_EOF:
            return "TK_MACRO_EOF";
        case TK_FILE_EOF:
            return "TK_FILE_EOF";
        case TK_PRE_PRAGMA:
            return "TK_PRE_PRAGMA";
        default:
            //assert(false);
            break;
    }
    return "???";
}

void Token_Reset(struct Token* scannerItem)
{
    LocalStrBuilder_Clear(&scannerItem->lexeme);
    scannerItem->token = TK_ERROR;
}

void Token_Copy(struct Token* scannerItem,
                struct Token* other)
{
    scannerItem->token = other->token;
    LocalStrBuilder_Set(&scannerItem->lexeme, other->lexeme.c_str);
}

void Token_Swap(struct Token* scannerItem,
                struct Token* other)
{
    enum TokenType tk = other->token;
    other->token = scannerItem->token;
    scannerItem->token = tk;
    LocalStrBuilder_Swap(&scannerItem->lexeme, &other->lexeme);
}

void Token_Destroy(struct Token* scannerItem)
{
    LocalStrBuilder_Destroy(&scannerItem->lexeme);
}


void Token_Delete(struct Token* pScannerItem)
{
    if (pScannerItem != NULL)
    {
        Token_Destroy(pScannerItem);
        free((void*)pScannerItem);
    }
}


void TokenList_Destroy(struct TokenList* p)
{
    struct Token* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct Token* pItem = pCurrent;
        pCurrent = pCurrent->pNext;
        Token_Delete(pItem);
    }
}

void TokenList_Init(struct TokenList* p)
{
    p->pHead = NULL;
    p->pTail = NULL;
}

void TokenList_Clear(struct TokenList* p)
{
    TokenList_Destroy(p);
    TokenList_Init(p);
}



#define PPTOKEN_INIT { PPTokenType_Other, NULL, TOKENSET_INIT }

void PPToken_Destroy(struct PPToken* p);

struct PPToken* PPToken_Create(const char* s, enum PPTokenType token);
struct PPToken* PPToken_Clone(struct PPToken* p);
void PPToken_Delete(struct PPToken* p);

void PPToken_Swap(struct PPToken* pA, struct PPToken* pB);


bool PPToken_IsIdentifier(struct PPToken* pHead);
bool PPToken_IsSpace(struct PPToken* pHead);
bool PPToken_IsStringizingOp(struct PPToken* pHead);
bool PPToken_IsConcatOp(struct PPToken* pHead);
bool PPToken_IsStringLit(struct PPToken* pHead);
bool PPToken_IsCharLit(struct PPToken* pHead);
bool PPToken_IsOpenPar(struct PPToken* pHead);
bool PPToken_IsChar(struct PPToken* pHead, char ch);
bool PPToken_IsLexeme(struct PPToken* pHead, const char* ch);


#define TOKENSET_INIT { 0 }

void PPTokenSet_PushUnique(struct PPTokenSet* p, struct PPToken* pItem);

void TokenSetAppendCopy(struct PPTokenSet* pArrayTo, const struct PPTokenSet* pArrayFrom);

struct PPToken* PPTokenSet_Find(const struct PPTokenSet* pArray, const char* lexeme);

void PPTokenSet_Destroy(struct PPTokenSet* pArray);

void SetIntersection(const struct PPTokenSet* p1, const struct PPTokenSet* p2, struct PPTokenSet* pResult);



void TokenList_Swap(struct TokenList* a, struct TokenList* b)
{
    struct TokenList t = *a;
    *a = *b;
    *b = t;
}

void TokenList_PopFront(struct TokenList* pList)
{
    struct Token* p = pList->pHead;
    pList->pHead = pList->pHead->pNext;
    Token_Delete(p);
}

void TokenList_PushBack(struct TokenList* pList, struct Token* pItem)
{
    if ((pList)->pHead == NULL)
    {
        (pList)->pHead = (pItem);
        (pList)->pTail = (pItem);
    }
    else
    {
        (pList)->pTail->pNext = (pItem);
        (pList)->pTail = (pItem);
    }
}

struct Token* TokenList_PopFrontGet(struct TokenList* pList)
{
    struct Token* p = pList->pHead;
    pList->pHead = pList->pHead->pNext;
    return p;
}

void PPToken_Destroy(struct  PPToken* p)
{
    free((void*)p->Lexeme);
    PPTokenSet_Destroy(&p->HiddenSet);
}

void PPToken_Swap(struct  PPToken* pA, struct  PPToken* pB)
{
    struct  PPToken temp = *pA;
    *pA = *pB;
    *pB = temp;
}

struct PPToken* PPToken_Clone(struct  PPToken* p)
{
    struct PPToken* pNew = PPToken_Create(p->Lexeme, p->Token);
    TokenSetAppendCopy(&pNew->HiddenSet, &p->HiddenSet);
    return pNew;
}

struct PPToken* PPToken_Create(const char* s, enum PPTokenType token)
{
    struct PPToken* p = (struct PPToken*)malloc(sizeof(struct PPToken));
    if (p != 0)
    {
        struct PPToken t = PPTOKEN_INIT;
        *p = t;
        p->Lexeme = strdup(s);
        p->Token = token;
    }
    else
    {
        //assert(false);
    }
    return p;
}

void PPToken_Delete(struct PPToken* p)
{
    if (p != NULL)
    {
        PPToken_Destroy(p);
        free((void*)p);
    }
}

bool PPToken_IsIdentifier(struct PPToken* pHead)
{
    return pHead->Token == PPTokenType_Identifier;
}

bool PPToken_IsSpace(struct PPToken* pHead)
{
    if (pHead->Token == PPTokenType_Spaces)
    {
        return true;
    }
    return false;
}

bool PPToken_IsStringizingOp(struct PPToken* pHead)
{
    return pHead->Lexeme[0] == '#' &&
        pHead->Lexeme[1] == '\0';
}

bool PPToken_IsConcatOp(struct PPToken* pHead)
{
    return pHead->Lexeme[0] == '#' &&
        pHead->Lexeme[1] == '#' &&
        pHead->Lexeme[2] == '\0';
}

bool PPToken_IsStringLit(struct PPToken* pHead)
{
    return pHead->Token == PPTokenType_StringLiteral;
}

bool PPToken_IsCharLit(struct PPToken* pHead)
{
    return pHead->Token == PPTokenType_CharConstant;
}

bool PPToken_IsOpenPar(struct PPToken* pHead)
{
    return pHead->Lexeme[0] == '(' &&
        pHead->Lexeme[1] == '\0';
}

bool PPToken_IsChar(struct PPToken* pHead, char ch)
{
    return pHead->Lexeme[0] == ch &&
        pHead->Lexeme[1] == '\0';
}

bool PPToken_IsLexeme(struct PPToken* pHead, const char* lexeme)
{
    return strcmp(pHead->Lexeme, lexeme) == 0;
}

#define PPTOKENARRAY_INIT {0}

void PPTokenArray_Reserve(struct PPTokenArray* p, int nelements);

void PPTokenArray_Pop(struct PPTokenArray* p);

struct PPToken* PPTokenArray_PopFront(struct PPTokenArray* p);


void PPTokenArray_PushBack(struct PPTokenArray* p, struct PPToken* pItem);
void PPTokenArray_Clear(struct PPTokenArray* p);


void PPTokenArray_Destroy(struct PPTokenArray* st);
void PPTokenArray_Delete(struct PPTokenArray* st);
void PPTokenArray_Swap(struct PPTokenArray* p1, struct PPTokenArray* p2);


void PPTokenArray_AppendCopy(struct PPTokenArray* pArrayTo, const struct PPTokenArray* pArrayFrom);
void PPTokenArray_AppendMove(struct PPTokenArray* pArrayTo, struct PPTokenArray* pArrayFrom);
void PPTokenArray_Print(const struct PPTokenArray* tokens);

struct PPToken* PPTokenArray_Find(const struct PPTokenArray* pArray, const char* lexeme);
void PPTokenArray_Erase(struct PPTokenArray* pArray, int begin, int end);


/*
  based on
  https://github.com/dspinellis/cscout/blob/084d64dc7a0c5466dc2d505c1ca16fb303eb2bf1/src/macro.cpp
*/


void Macro_Destroy(struct Macro* p)
{
    free((void*)p->Name);
    PPTokenArray_Destroy(&p->ReplacementList);
    PPTokenArray_Destroy(&p->FormalArguments);
}

void Macro_Delete(struct Macro* p)
{
    if (p != NULL)
    {
        Macro_Destroy(p);
        free((void*)p);
    }
}

static bool FillIn(struct PPTokenArray* ts, bool get_more, struct PPTokenArray* removed);

static void Glue(const struct PPTokenArray* lsI, const struct PPTokenArray* rsI, struct PPTokenArray* out);

// Return a new token sequence with hs added to the hide set of every element of ts
static void HidenSetAdd(const struct PPTokenSet* hs, const struct PPTokenArray* ts, struct PPTokenArray* pOut)
{
    PPTokenArray_Clear(pOut);
    for (int i = 0; i < ts->Size; i++)
    {
        struct PPToken* t = ts->pItems[i];
        for (int k = 0; k < hs->Size; k++)
        {
            PPTokenSet_PushUnique(&t->HiddenSet, PPToken_Clone(hs->pItems[k]));
        }
        PPTokenArray_PushBack(pOut, PPToken_Clone(t));
    }
    //printf("hsadd returns: ");
    PPTokenArray_Print(pOut);
    //printf("\n");
}

#define MACROMAP_INIT { NULL, 0, 0 }

int MacroMap_SetAt(struct MacroMap* pMap, const char* Key, struct Macro* newValue);

bool MacroMap_Lookup(const struct MacroMap* pMap, const char* Key, struct Macro** rValue);

bool MacroMap_RemoveKey(struct MacroMap* pMap, const char* Key);

void MacroMap_Init(struct MacroMap* p);

void MacroMap_Destroy(struct MacroMap* p);

void MacroMap_Swap(struct MacroMap* pA, struct MacroMap* pB);

struct Macro* MacroMap_Find(const struct MacroMap* pMap, const char* Key);


static void ExpandMacro(const struct PPTokenArray* pTokenSequence,
                        const struct MacroMap* macros,
                        bool get_more,
                        bool skip_defined,
                        bool evalmode,
                        struct Macro* caller,
                        struct PPTokenArray* pOutputSequence);

/*
Retorna o indice do primeiro token que n√£o for espa√ßo
a partir e incluindo o indice start.
Return -1 se n√£o achar.
*/
static int FindNoSpaceIndex(const struct PPTokenArray* pArray, int start)
{
    int result = -1;
    for (int i = start; i < pArray->Size; i++)
    {
        if (!PPToken_IsSpace(pArray->pItems[i]))
        {
            result = i;
            break;
        }
    }
    return result;
}

// Return s with all \ and " characters \ escaped
static void AppendEscaped(struct StrBuilder* strBuilder,
                          const char* source)
{
    while (*source)
    {
        switch (*source)
        {
            case '\\':
            case '"':
                StrBuilder_AppendChar(strBuilder, '\\');
                // FALTHROUGH
            default:
                StrBuilder_AppendChar(strBuilder, *source);
        }
        source++;
    }
}

/*
* Convert a list of tokens into a char as specified by the # operator
* Multiple spaces are converted to a single space, \ and " are
* escaped
*/
static void AppendStringize(struct StrBuilder* strBuilder, const struct PPTokenArray* ts)
{
    /*
    Each occurrence of white space between the argument¬ís
    preprocessing tokens becomes a single space character in
    the character char literal.
    */
    /*
    White space before the first preprocessing token and after the
    last preprocessing token composing the argument is deleted.
    */
    StrBuilder_Append(strBuilder, "\"");
    bool seen_space = true;   // To delete leading spaces
    for (int i = 0; i < ts->Size; i++)
    {
        struct PPToken* pToken = ts->pItems[i];
        if (PPToken_IsSpace(pToken))
        {
            if (seen_space)
                continue;
            else
                seen_space = true;
            StrBuilder_Append(strBuilder, " ");
        }
        else if (PPToken_IsStringLit(pToken))
        {
            seen_space = false;
            StrBuilder_Append(strBuilder, "\\\"");
            AppendEscaped(strBuilder, pToken->Lexeme);
            StrBuilder_Append(strBuilder, "\\\"");
        }
        else if (PPToken_IsCharLit(pToken))
        {
            seen_space = false;
            StrBuilder_AppendChar(strBuilder, '\'');
            AppendEscaped(strBuilder, pToken->Lexeme);
            StrBuilder_AppendChar(strBuilder, '\'');
        }
        else
        {
            seen_space = false;
            StrBuilder_Append(strBuilder, pToken->Lexeme);
        }
    }
    StrBuilder_Append(strBuilder, "\"");
    // Remove trailing spaces
    StrBuilder_Trim(strBuilder);
}


#define TOKENARRAYMAP_INIT { NULL, 0, 0 }

int TokenArrayMap_SetAt(struct TokenArrayMap* pMap,
                        const char* Key,
                        struct PPTokenArray* newValue);

bool TokenArrayMap_Lookup(const struct TokenArrayMap* pMap,
                          const char* Key,
                          struct PPTokenArray** rValue);

bool TokenArrayMap_RemoveKey(struct TokenArrayMap* pMap,
                             const char* Key);

void TokenArrayMap_Destroy(struct TokenArrayMap* p);


void TokenArrayMap_Swap(struct TokenArrayMap* pA, struct TokenArrayMap* pB);



/*
* Substitute the arguments args appearing in the input sequence is
* Result is created in the output sequence os and finally has the specified
* hide set added to it, before getting returned.
*/
static void SubstituteArgs(const struct MacroMap* macros,
                           const struct PPTokenArray* isOriginal,   //macro
                           const struct TokenArrayMap* args,
                           struct PPTokenSet* hs,
                           bool skip_defined,
                           bool evalmode,
                           struct Macro* pCaller,
                           struct PPTokenArray* pOutputSequence)
{
    PPTokenArray_Clear(pOutputSequence);
    //Trabalha com uma copia
    struct PPTokenArray is = PPTOKENARRAY_INIT;
    PPTokenArray_AppendCopy(&is, isOriginal);
    struct PPTokenArray os = PPTOKENARRAY_INIT;
    struct PPToken* head = NULL;
    while (is.Size > 0)
    {
        //printf("subst: is=");
        PPTokenArray_Print(&is);
        //printf(" os=");
        PPTokenArray_Print(&os);
        //printf("\n");
        //assert(head == NULL);
        head = PPTokenArray_PopFront(&is);
        if (PPToken_IsStringizingOp(head))
        {
            /*
            Each # preprocessing token in the replacement list for
            a function-like macro shall be followed by a parameter
            as the next preprocessing token in the replacement list.
            */
            // Stringizing operator
            int idx = FindNoSpaceIndex(&is, 0);
            struct PPTokenArray* aseq;
            if (idx != -1 &&
                args != NULL &&
                TokenArrayMap_Lookup(args, is.pItems[idx]->Lexeme, &aseq))
            {
                /*
                If, in the replacement list, a parameter is immediately
                preceded by a # preprocessing token, both are replaced
                by a single character char literal preprocessing token that
                contains the spelling of the preprocessing token sequence
                for the corresponding argument.
                */
                struct StrBuilder strBuilder = STRBUILDER_INIT;
                AppendStringize(&strBuilder, aseq);
                PPTokenArray_Erase(&is, 0, idx + 1);
                //TODO token tipo?
                PPTokenArray_PushBack(&os, PPToken_Create(strBuilder.c_str, PPTokenType_Other));
                StrBuilder_Destroy(&strBuilder);
                PPToken_Delete(head);
                head = NULL;
                continue;
            }
        }
        else if (PPToken_IsConcatOp(head))
        {
            /*
            If, in the replacement list of a function-like macro,
            a parameter is immediately preceded or followed by
            a ## preprocessing token, the parameter is replaced by
            the corresponding argument¬ís preprocessing token sequence;
            */
            int idx = FindNoSpaceIndex(&is, 0);
            if (idx != -1)
            {
                struct PPTokenArray* aseq;
                if (TokenArrayMap_Lookup(args, is.pItems[idx]->Lexeme, &aseq))
                {
                    PPTokenArray_Erase(&is, 0, idx + 1);
                    // Only if actuals can be empty
                    if (aseq->Size > 0)
                    {
                        struct PPTokenArray os2 = PPTOKENARRAY_INIT;
                        Glue(&os, aseq, &os2);
                        PPTokenArray_Swap(&os2, &os);
                        PPTokenArray_Destroy(&os2);
                    }
                }
                else
                {
                    struct PPTokenArray t = PPTOKENARRAY_INIT;
                    PPTokenArray_PushBack(&t, PPToken_Clone(is.pItems[idx]));
                    PPTokenArray_Erase(&is, 0, idx + 1);
                    struct PPTokenArray os2 = PPTOKENARRAY_INIT;
                    Glue(&os, &t, &os2);
                    PPTokenArray_Swap(&os2, &os);
                    PPTokenArray_Destroy(&os2);
                    PPTokenArray_Destroy(&t);
                }
                PPToken_Delete(head);
                head = NULL;
                continue;
            }
        }
        else
        {
            int idx = FindNoSpaceIndex(&is, 0);
            if (idx != -1 &&
                PPToken_IsConcatOp(is.pItems[idx]))
            {
                /*
                * Implement the following gcc extension:
                *  "`##' before a
                *   rest argument that is empty discards the preceding sequence of
                *   non-whitespace characters from the macro definition.  (If another macro
                *   argument precedes, none of it is discarded.)"
                * Otherwise, break to process a non-formal argument in the default way
                */
                struct PPTokenArray* aseq;
                if (!TokenArrayMap_Lookup(args, head->Lexeme, &aseq))
                {
                    /*
                    if (m.get_is_vararg())
                    {
                    ti2 = find_nonspace(ti + 1, is.end());

                    if (ti2 != is.end() && (ai = find_formal_argument(args, *ti2)) != args.end() && ai->second.size() == 0)
                    {
                    // All conditions satisfied; discard elements:
                    // <non-formal> <##> <empty-formal>
                    is.erase(is.begin(), ++ti2);
                    continue;
                    }
                    }
                    */
                    // Non-formal arguments don't deserve special treatment
                    PPTokenArray_PushBack(&os, head);
                    head = NULL; //moved
                }
                else
                {
                    // Paste but not expand LHS, RHS
                    // Only if actuals can be empty
                    if (aseq->Size == 0)
                    {
                        // Erase including ##
                        PPTokenArray_Erase(&is, 0, idx + 1);
                        int idx2 = FindNoSpaceIndex(&is, 0);
                        if (idx2 != -1)
                        {
                            struct PPTokenArray* aseq2;
                            if (!TokenArrayMap_Lookup(args, is.pItems[idx2]->Lexeme, &aseq2))
                            {
                                // Erase the ## RHS
                                PPTokenArray_Erase(&is, 0, idx + 1);
                                PPTokenArray_AppendCopy(&os, aseq);
                            }
                        }
                    }
                    else
                    {
                        // Erase up to ##
                        PPTokenArray_Print(&is);
                        //printf("-\n");
                        PPTokenArray_Erase(&is, 0, idx);
                        PPTokenArray_Print(&is);
                        //printf("-\n");
                        PPTokenArray_AppendCopy(&os, aseq);
                    }
                }
                PPToken_Delete(head);
                head = NULL;
                continue;
            }
            struct PPTokenArray* argseq = NULL;
            if (args != NULL &&
                TokenArrayMap_Lookup(args, head->Lexeme, &argseq))
            {
                //expand head
                struct PPTokenArray expanded = PPTOKENARRAY_INIT;
                ExpandMacro(argseq, macros, false, skip_defined, evalmode, pCaller, &expanded);
                PPTokenArray_AppendMove(&os, &expanded);
                PPTokenArray_Destroy(&expanded);
                PPToken_Delete(head);
                head = NULL;
                continue;
            }
        }
        PPTokenArray_PushBack(&os, head);
        head = NULL; //moved
    }
    //assert(head == NULL);
    struct PPTokenArray os2 = PPTOKENARRAY_INIT;
    HidenSetAdd(hs, &os, &os2);
    PPTokenArray_Swap(pOutputSequence, &os2);
    PPTokenArray_Destroy(&os);
    PPTokenArray_Destroy(&os2);
    PPTokenArray_Destroy(&is);
}

/*
* Return a macro argument token from tokens
* Used by gather_args.
* If get_more is true when tokens is exhausted read using pdtoken::getnext_noexpand
* (see explanation on that method's comment for why we use pdtoken, rather than pltoken)
* Leave in tokens the first token not gathered.
* If want_space is true return spaces, otherwise discard them
*/
static void ArgToken(struct PPTokenArray* tokens,
                     bool get_more,
                     bool want_space,
                     struct PPToken* token)
{
    struct PPToken* pToken = PPTokenArray_PopFront(tokens);
    PPToken_Swap(pToken, token);
    PPToken_Delete(pToken);
    pToken = NULL;
    /*  if (want_space)
      {
        if (tokens->Size > 0)
        {
        Token *pToken = PPTokenArray_PopFront(tokens);
        PPToken_Swap(pToken, token);
        PPToken_Destroy(pToken);
          return;
        }

        else if (get_more)
        {
          //Pdtoken t;
          //t.getnext_noexpand();
          //return (t);
        }

      else
      {
        String2_Set(&token->Lexeme, NULL);
        //return Ptoken(EOF, "");
      }
      }

      else
      {
        while (tokens->Size > 0 && PPToken_IsSpace(tokens->pItems[0]))
        {
        Token* p = PPTokenArray_PopFront(tokens);
        PPToken_Delete(p);
        }

        if (tokens->Size > 0)
        {
        Token* p = PPTokenArray_PopFront(tokens);
        PPToken_Swap(p, token);
        PPToken_Delete(p);

          return;
        }

        else if (get_more)
        {
          //Pdtoken t;

          //do
          //{
            //t.getnext_noexpand_nospc();
          //}
          //while (t.get_code() != EOF && t.is_space());

          //return (t);
        }

      else
      {
        String2_Set(&token->Lexeme, NULL);
        //return Ptoken(EOF, "");
      }
      }  */
}

/*
* Get the macro arguments specified in formal_args, initiallly by
* removing them from tokens, then, if get_more is true,
from pdtoken.getnext_noexpand.
* The opening bracket has already been gathered.
* Build the map from formal name to argument value args.
* Return in close the closing bracket token (used for its hideset)
* Return true if ok, false on error.
*/
static bool GatherArgs(struct PPTokenArray* tokens,
                       const struct PPTokenArray* formal_args,
                       struct TokenArrayMap* args,
                       bool get_more,
                       bool is_vararg,
                       struct PPToken* close)
{
    struct PPToken t = PPTOKEN_INIT;
    for (int i = 0; i < formal_args->Size; i++)
    {
        struct PPTokenArray* pV = NEW((struct PPTokenArray)PPTOKENARRAY_INIT);
        TokenArrayMap_SetAt(args,
                            formal_args->pItems[i]->Lexeme,
                            pV);
        char terminate;
        if (i + 1 == formal_args->Size)
        {
            terminate = ')';
        }
        else if (is_vararg && i + 2 == formal_args->Size)
        {
            // Vararg last argument is optional; terminate with ) or ,
            terminate = '.';
        }
        else
        {
            terminate = ',';
        }
        int bracket = 0;
        // Get a single argument
        for (;;)
        {
            ArgToken(tokens, get_more, true, &t);
            //printf("tokens=");
            PPTokenArray_Print(tokens);
            //printf("\n");
            if (bracket == 0 && (
                (terminate == '.' && (PPToken_IsChar(&t, ',') || PPToken_IsChar(&t, ')'))) ||
                (terminate != '.' && PPToken_IsChar(&t, terminate))))
            {
                break;
            }
            if (PPToken_IsChar(&t, '('))
            {
                bracket++;
            }
            else if (PPToken_IsChar(&t, ')'))
            {
                bracket--;
            }
            else if (PPToken_IsChar(&t, '\0')) //EOF
            {
                /*
                * @error
                * The end of file was reached while
                * gathering a macro's arguments
                */
                //printf("macro [%s] EOF while reading function macro arguments", name);
                return (false);
            }
            else
            {
            }
            PPTokenArray_PushBack(pV, PPToken_Clone(&t));
        }
        //printf("Gather args returns: ");
        PPTokenArray_Print(pV);
        //printf("\n");
        // Check if varargs last optional argument was not supplied
        if (terminate == '.' && PPToken_IsChar(&t, ')'))
        {
            i++;
            struct PPTokenArray* pV2 = NEW((struct PPTokenArray)PPTOKENARRAY_INIT);
            TokenArrayMap_SetAt(args,
                                formal_args->pItems[i]->Lexeme,
                                pV2);
            // Instantiate argument with an empty value list
            //args[(*i).get_val()];
            break;
        }
        free(close->Lexeme);
        close->Lexeme = strdup(t.Lexeme);
    }
    if (formal_args->Size == 0)
    {
        ArgToken(tokens, get_more, false, &t);
        if (PPToken_IsChar(&t, ')'))
        {
            /*
            * @error
            * The arguments to a function-like macro did
            * not terminate with a closing bracket
            */
            //printf("macro [%s] close bracket expected for function-like macro", name);
            return (false);
        }
    }
    PPToken_Destroy(&t);
    return (true);
}

/*
* Remove from tokens and return the elements comprising the arguments to the defined
* operator, * such as "defined X" or "defined(X)"
* This is the rule when processing #if #elif expressions
*/
static void GatherDefinedOperator(struct PPTokenArray* tokens,
                                  const struct MacroMap* macros,
                                  struct PPTokenArray* result)
{
    //struct PPTokenArray tokens = PPTOKENARRAY_INIT;
    //PPTokenArray_AppendCopy(&tokens, tokensIn);
    // Skip leading space
    while (PPToken_IsSpace(tokens->pItems[0]))
    {
        struct PPToken* pp = PPTokenArray_PopFront(tokens);
        PPTokenArray_PushBack(result, pp);
    }
    if ((PPToken_IsIdentifier(tokens->pItems[0])))
    {
        // defined X form
        if (MacroMap_Find(macros, tokens->pItems[0]->Lexeme) != NULL)
        {
            struct PPToken* pp0 = PPTokenArray_PopFront(tokens);
            free(pp0->Lexeme);
            pp0->Lexeme = strdup("1");
            PPTokenArray_PushBack(result, pp0);
        }
        else
        {
            struct PPToken* pp0 = PPTokenArray_PopFront(tokens);
            free(pp0->Lexeme);
            pp0->Lexeme = strdup("0");
            PPTokenArray_PushBack(result, pp0);
        }
        return;
    }
    else if ((PPToken_IsChar(tokens->pItems[0], '(')))
    {
        // defined (X) form
        PPToken_Delete(PPTokenArray_PopFront(tokens));
        // Skip spaces
        while (PPToken_IsSpace(tokens->pItems[0]))
        {
            struct PPToken* pp = PPTokenArray_PopFront(tokens);
            PPTokenArray_PushBack(result, pp);
        }
        if (!PPToken_IsIdentifier(tokens->pItems[0]))
        {
            //goto error;
        }
        if (MacroMap_Find(macros, tokens->pItems[0]->Lexeme) != NULL)
        {
            struct PPToken* pp0 = PPTokenArray_PopFront(tokens);
            free(pp0->Lexeme);
            pp0->Lexeme = strdup("1");
            PPTokenArray_PushBack(result, pp0);
        }
        else
        {
            struct PPToken* pp0 = PPTokenArray_PopFront(tokens);
            free(pp0->Lexeme);
            pp0->Lexeme = strdup("0");
            PPTokenArray_PushBack(result, pp0);
        }
        //PPToken* pp = PPTokenArray_PopFront(&tokens);
        //PPTokenArray_PushBack(result, pp);
        // Skip spaces
        while (PPToken_IsSpace(tokens->pItems[0]))
        {
            struct PPToken* pp = PPTokenArray_PopFront(tokens);
            PPTokenArray_PushBack(result, pp);
        }
        if (!PPToken_IsChar(tokens->pItems[0], ')'))
        {
            //goto error;
        }
        PPToken_Delete(PPTokenArray_PopFront(tokens));
        //PPTokenArray_PushBack(result, pp);
        return;
    }
    else
    {
    }
}


static void ExpandMacro(const struct PPTokenArray* tsOriginal,
                        const struct MacroMap* macros,
                        bool get_more,
                        bool skip_defined,
                        bool evalmode,
                        struct Macro* caller,
                        struct PPTokenArray* pOutputSequence2)
{
    PPTokenArray_Clear(pOutputSequence2);
    struct PPTokenArray r = PPTOKENARRAY_INIT;
    struct PPTokenArray ts = PPTOKENARRAY_INIT;
    PPTokenArray_AppendCopy(&ts, tsOriginal);
    //printf("Expanding: ");
    PPTokenArray_Print(&ts);
    //printf("\n");
    struct PPToken* pHead = NULL; //muito facil ter leaks
    while (ts.Size > 0)
    {
        //printf("r = ");
        PPTokenArray_Print(&r);
        //printf("\n");
        //assert(pHead == NULL);
        pHead =
            PPTokenArray_PopFront(&ts);
        if (!PPToken_IsIdentifier(pHead))
        {
            PPTokenArray_PushBack(&r, pHead);
            pHead = NULL; //moved
            continue;
        }
        if (skip_defined &&
            PPToken_IsIdentifier(pHead) &&
            PPToken_IsLexeme(pHead, "defined"))
        {
            struct PPTokenArray result = PPTOKENARRAY_INIT;
            GatherDefinedOperator(&ts, macros, &result);
            PPTokenArray_AppendMove(&r, &result);
            PPToken_Delete(pHead);
            pHead = NULL;
            PPTokenArray_Destroy(&result);
            continue;
        }
        struct Macro* pMacro = MacroMap_Find(macros, pHead->Lexeme);
        if (pMacro == NULL)
        {
            //if eval mode se nao achar a macro
            //ela vira zero
            if (evalmode)
            {
                free(pHead->Lexeme);
                pHead->Lexeme = strdup("0");
                pHead->Token = PPTokenType_Number;
            }
            // Nothing to do if the identifier is not a macro
            PPTokenArray_PushBack(&r, pHead);
            pHead = NULL; //moved
            continue;
        }
        struct PPToken* pFound =
            PPTokenSet_Find(&pHead->HiddenSet, pMacro->Name);
        if (pFound)
        {
            // Skip the head token if it is in the hideset
            //printf("Skipping (head is in HS)\n");
            PPTokenArray_PushBack(&r, pHead);
            pHead = NULL;//moved
            continue;
        }
        struct PPTokenArray removed_spaces = PPTOKENARRAY_INIT;
        //printf("replacing for %s tokens=", pMacro->Name);
        PPTokenArray_Print(&ts);
        //printf("\n");
        if (!pMacro->bIsFunction)
        {
            // Object-like macro
            //printf("Object-like macro\n");
            struct PPTokenSet hiddenSet = TOKENSET_INIT;
            TokenSetAppendCopy(&hiddenSet, &pHead->HiddenSet);
            PPTokenSet_PushUnique(&hiddenSet, PPToken_Create(pHead->Lexeme, pHead->Token));
            PPToken_Delete(pHead);
            pHead = NULL; //usado deletado
            struct PPTokenArray s = PPTOKENARRAY_INIT;
            SubstituteArgs(macros,
                           &pMacro->ReplacementList,
                           NULL, //empty args
                           &hiddenSet,
                           skip_defined,
                           evalmode,
                           caller,
                           &s);
            PPTokenArray_AppendMove(&s, &ts);
            PPTokenArray_Swap(&s, &ts);
            PPTokenArray_Destroy(&s);
            PPTokenSet_Destroy(&hiddenSet);
            caller = pMacro;
        }
        else if (FillIn(&ts, get_more, &removed_spaces) &&
                 PPToken_IsOpenPar(ts.pItems[0]))
        {
            //printf("Application of a function-like macro\n");
            // struct Map from formal name to value
            struct TokenArrayMap args = TOKENARRAYMAP_INIT;
            PPToken_Delete(PPTokenArray_PopFront(&ts));
            struct PPToken close = PPTOKEN_INIT;
            if (!GatherArgs(&ts,
                &pMacro->FormalArguments,
                &args,
                get_more,
                false, /*m.is_vararg,*/
                &close))
            {
                PPToken_Destroy(&close);
                PPToken_Delete(pHead);
                pHead = NULL;//deletado
                continue; // Attempt to bail-out on error
            }
            /*
            After the arguments for the invocation of a function-like
            macro have been identified, argument substitution takes place.
            */
            struct PPTokenSet hs = TOKENSET_INIT;
            //merge head and close
            SetIntersection(&pHead->HiddenSet,
                            &close.HiddenSet,
                            &hs);
            PPTokenSet_PushUnique(&hs, PPToken_Create(pMacro->Name, PPTokenType_Identifier));
            PPToken_Delete(pHead);
            pHead = NULL;//deletado
            struct PPTokenArray s = PPTOKENARRAY_INIT;
            SubstituteArgs(macros,
                           &pMacro->ReplacementList,
                           &args,
                           &hs,
                           skip_defined,
                           evalmode,
                           caller,
                           &s);
            PPTokenArray_AppendMove(&s, &ts);
            PPTokenArray_Swap(&s, &ts);
            caller = pMacro;
            PPTokenSet_Destroy(&hs);
            PPTokenArray_Destroy(&s);
            TokenArrayMap_Destroy(&args);
            PPToken_Destroy(&close);
        }
        else
        {
            // Function-like macro name lacking a (
            //printf("splicing: [");
            PPTokenArray_Print(&removed_spaces);
            //printf("]\n");
            PPTokenArray_AppendMove(&removed_spaces, &ts);
            PPTokenArray_Swap(&removed_spaces, &ts);
            PPTokenArray_PushBack(&r, pHead);
            pHead = NULL; //moved
        }
        //PPTokenArray_Contains(pHead->HiddenSet, pMacro->Name);
        PPTokenArray_Destroy(&removed_spaces);
    }
    //assert(pHead == NULL);
    PPTokenArray_Swap(&r, pOutputSequence2);
    PPTokenArray_Destroy(&r);
    PPTokenArray_Destroy(&ts);
}


/*
* Try to ensure that ts has at least one non-space token
* Return true if this is the case
* Return any discarded space tokens in removed
*/
static bool FillIn(struct PPTokenArray* ts, bool get_more, struct PPTokenArray* removed)
{
    while (ts->Size > 0 &&
           PPToken_IsSpace(ts->pItems[0]))
    {
        PPTokenArray_PushBack(removed, PPTokenArray_PopFront(ts));
    }
    if (ts->Size > 0)
    {
        return true;
    }
    /*if (get_more)
    {
    Pdtoken t;

    for (;;)
    {
    t.getnext_noexpand();

    if (t.get_code() == EOF)
    return (false);

    else if (t.is_space())
    removed.push_back(t);

    else
    break;
    }

    ts.push_back(t);
    return (true);
    }*/
    return (false);
}


// Paste last of left side with first of right side

static void Glue(const struct PPTokenArray* lsI,
                 const struct PPTokenArray* rsI,
                 struct PPTokenArray* out)
{
    struct PPTokenArray ls = PPTOKENARRAY_INIT;
    PPTokenArray_AppendCopy(&ls, lsI);
    struct PPTokenArray rs = PPTOKENARRAY_INIT;
    PPTokenArray_AppendCopy(&rs, rsI);
    PPTokenArray_Clear(out);
    if (ls.Size == 0)
    {
        PPTokenArray_Swap(out, &rs);
    }
    else
    {
        while (ls.Size > 0 &&
               PPToken_IsSpace(ls.pItems[ls.Size - 1]))
        {
            PPTokenArray_Pop(&ls);
        }
        while (rs.Size > 0 && PPToken_IsSpace(rs.pItems[0]))
        {
            struct PPToken* tk = PPTokenArray_PopFront(&rs);
            PPToken_Delete(tk);
            tk = NULL;
        }
        if (ls.Size == 0 &&
            rs.Size == 0)
        {
            PPTokenArray_Swap(out, &ls);
        }
        else
        {
            //Junta o ultimo token do lado esquerdo
            //com o primeiro do lado direito
            struct StrBuilder strNewLexeme = STRBUILDER_INIT;
            if (ls.Size > 0)
            {
                //printf("glue LS: ");
                //printf("%s", ls.pItems[ls.Size - 1]->Lexeme);
                //printf("\n");
                StrBuilder_Append(&strNewLexeme, ls.pItems[ls.Size - 1]->Lexeme);
                PPTokenArray_Pop(&ls);
            }
            if (rs.Size > 0)
            {
                //printf("glue RS: ");
                //printf("%s", rs.pItems[0]->Lexeme);
                //printf("\n");
                StrBuilder_Append(&strNewLexeme, rs.pItems[0]->Lexeme);
                PPTokenArray_Pop(&rs);
            }
            //tipo?
            PPTokenArray_PushBack(&ls, PPToken_Create(strNewLexeme.c_str, PPTokenType_Other));
            StrBuilder_Destroy(&strNewLexeme);
            PPTokenArray_AppendMove(&ls, &rs);
            PPTokenArray_Swap(out, &ls);
        }
    }
    //printf("glue returns: ");
    PPTokenArray_Print(out);
    //printf("\n");
    PPTokenArray_Destroy(&ls);
    PPTokenArray_Destroy(&rs);
}

static void ExpandMacroToText(const struct PPTokenArray* pTokenSequence,
                              const struct MacroMap* macros,
                              bool get_more,
                              bool skip_defined,
                              bool evalmode,
                              struct Macro* caller,
                              struct StrBuilder* strBuilder)
{
    StrBuilder_Clear(strBuilder);
    struct PPTokenArray tks = PPTOKENARRAY_INIT;
    ExpandMacro(pTokenSequence,
                macros,
                get_more,
                skip_defined,
                evalmode,
                caller,
                &tks);
    for (int i = 0; i < tks.Size; i++)
    {
        if (tks.pItems[i]->Token == PPTokenType_Spaces)
        {
            StrBuilder_Append(strBuilder, " ");
        }
        else
        {
            StrBuilder_Append(strBuilder, tks.pItems[i]->Lexeme);
        }
    }
    PPTokenArray_Destroy(&tks);
}


int MacroMap_SetAt(struct MacroMap* pMap,
                   const char* Key,
                   struct Macro* newValue)
{
    void* pPrevious;
    int r = HashMap_SetAt((struct HashMap*)pMap, Key, newValue, &pPrevious);
    Macro_Delete((struct Macro*)pPrevious);
    return r;
}

bool MacroMap_Lookup(const struct MacroMap* pMap,
                     const char* Key,
                     struct Macro** rValue)
{
    return HashMap_Lookup((struct HashMap*)pMap,
                          Key,
                          (void**)rValue);
}

struct Macro* MacroMap_Find(const struct MacroMap* pMap, const char* Key)
{
    void* p = NULL;
    HashMap_Lookup((struct HashMap*)pMap,
                   Key,
                   &p);
    return (struct Macro*)p;
}


bool MacroMap_RemoveKey(struct MacroMap* pMap, const char* Key)
{
    struct Macro* pItem;
    bool r = HashMap_RemoveKey((struct HashMap*)pMap, Key, (void**)&pItem);
    if (r)
    {
        Macro_Delete(pItem);
    }
    return r;
}

void MacroMap_Init(struct MacroMap* p)
{
    struct MacroMap t = MACROMAP_INIT;
    *p = t;
}

static void Macro_DeleteVoid(void* p)
{
    Macro_Delete((struct Macro*)p);
}

void MacroMap_Destroy(struct MacroMap* p)
{
    HashMap_Destroy((struct HashMap*)p, Macro_DeleteVoid);
}


void MacroMap_Swap(struct MacroMap* pA, struct MacroMap* pB)
{
    struct MacroMap t = *pA;
    *pA = *pB;
    *pB = t;
}


/*
   A funÁ„o do BasicScanner È retornar os tokens n„o crus ainda
   n„o prÈ-processados.
*/
struct BasicScanner
{
    struct Stream stream;
    struct StrBuilder lexeme;

    char NameOrFullPath[CPRIME_MAX_PATH];
    char FullDir[CPRIME_MAX_PATH];

    bool bLineStart; /*controle interno para gerar o token TK_PREPROCESSOR*/
    bool bMacroExpanded; /*se true ao fechar gera TK_MACRO_EOF caso contrario TK_FILE_EOF*/
    int FileIndex;
    enum TokenType token;

    /*
     BasicScanner s„o empilhados (ex em cada include) e usamos
     um ponteiro para o anterior.
    */
    struct BasicScanner* pPrevious;
};

bool BasicScanner_Init(struct BasicScanner* pBasicScanner, const char* name, const char* Text);

const char* BasicScanner_Lexeme(struct BasicScanner* scanner);

bool BasicScanner_IsLexeme(struct BasicScanner* scanner, const char* psz);

void BasicScanner_Match(struct BasicScanner* scanner);

bool BasicScanner_MatchToken(struct BasicScanner* scanner, enum TokenType token);

bool BasicScanner_InitFile(struct BasicScanner* pScanner, const char* fileName);

bool BasicScanner_CreateFile(const char* fileName, struct BasicScanner** pp);

void BasicScanner_Delete(struct BasicScanner* pScanner);

bool BasicScanner_Create(struct BasicScanner** pp, const char* name, const char* Text);

void BasicScanner_Destroy(struct BasicScanner* pScanner);

static bool IsPreprocessorTokenPhase(enum TokenType token);


//Quando existe um include ou macro È expandida
//eh empilhado um basicscanner
struct BasicScannerStack
{
    struct BasicScanner* pTop;
};

#define ITEM_STACK_INIT NULL
void BasicScannerStack_Init(struct BasicScannerStack* stack);
void BasicScannerStack_Push(struct BasicScannerStack* stack, struct BasicScanner* pItem);
struct BasicScanner* BasicScannerStack_PopGet(struct BasicScannerStack* stack);
void BasicScannerStack_Pop(struct BasicScannerStack* stack);
void BasicScannerStack_PopIfNotLast(struct BasicScannerStack* stack);
void BasicScannerStack_Destroy(struct BasicScannerStack* stack);

#define ForEachBasicScanner(pItem, stack)\
    for (struct BasicScanner* pItem = stack;\
      pItem;\
      pItem = pItem->pPrevious)

enum FileIncludeType
{
    FileIncludeTypeQuoted,
    FileIncludeTypeIncludes,
    FileIncludeTypeFullPath,
};


struct PPStateStack
{
    enum PPState
    {
        PPState_NONE, // inclui
        PPState_I1,   // inclui
        PPState_I0,
        PPState_E0,
        PPState_E1, // inclui
    }*pItems;

    int Size;
    int Capacity;
};


struct Scanner
{
    //Stack de basicscanner
    struct BasicScannerStack stack;

    //Mapa dos defines
    struct MacroMap  Defines2;

    //Stack usado para #if #else etc
    struct PPStateStack StackIfDef;

    FileInfoMap FilesIncluded;

    //Lista de diretorios de include
    struct StrArray IncludeDir;


    //char para debug
    struct StrBuilder DebugString;

    //char que mantem o erro
    struct StrBuilder ErrorString;

    //True indica error
    bool bError;

    /*estes tokens sao acumulados quando se olha a frente do cursor*/
    struct TokenList AcumulatedTokens;

    struct TokenList PreviousList;
    int Level;
};

void Scanner_Match(struct Scanner* pScanner);
enum TokenType Scanner_TokenAt(struct Scanner* pScanner, int index);
const char* Scanner_LexemeAt(struct Scanner* pScanner, int index);
bool Scanner_IsActiveAt(struct Scanner* pScanner, int index);

struct IntegerStack
{
    int* pData;
    int Size;
    int Capacity;
};

#define INTEGER_STACK_INIT {0}

struct PrintCodeOptions
{
    struct OutputOptions Options;

    struct TryBlockStatement* pCurrentTryBlock;

    ///////////
    //controle interno

    struct IntegerStack Stack;// = 0;
    bool bInclude;// = true;
    int IdentationLevel;
    //



    struct StrBuilder sbPreDeclaration;
    struct HashMap instantiations;
    struct StrBuilder sbInstantiations;

    /*defer acumulado, usado no throw, return*/
    struct StrBuilder sbDeferGlobal;
    /*defer escopo atual*/
    struct StrBuilder sbDeferLocal;
    struct StrBuilder returnType;
};

#define CODE_PRINT_OPTIONS_INIT {OUTPUTOPTIONS_INIT, NULL, INTEGER_STACK_INIT, true, 0, STRBUILDER_INIT, HASHMAP_INIT, STRBUILDER_INIT, STRBUILDER_INIT, STRBUILDER_INIT, STRBUILDER_INIT}

void PrintCodeOptions_Destroy(struct PrintCodeOptions* options);

void SyntaxTree_PrintCodeToFile(struct SyntaxTree* pSyntaxTree,
                                struct OutputOptions* options,
                                const char* fileName);

void SyntaxTree_PrintCodeToString(struct SyntaxTree* pSyntaxTree,
                                  struct OutputOptions* options,
                                  struct StrBuilder* output);


void TTypeName_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct TypeName* p, struct StrBuilder* fp);


CAST(SpecifierQualifier, StorageSpecifier)
CAST(SpecifierQualifier, AlignmentSpecifier)
CAST(SpecifierQualifier, SingleTypeSpecifier)

CAST(SpecifierQualifier, TypeQualifier)
CAST(SpecifierQualifier, EnumSpecifier)

void TSpecifierQualifierList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                       struct PrintCodeOptions* options,
                                       struct SpecifierQualifierList* pDeclarationSpecifiers,
                                       struct StrBuilder* fp);


void TDeclarationSpecifiers_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct DeclarationSpecifiers* pDeclarationSpecifiers, struct StrBuilder* fp);



struct Parser
{
    // indica presenca de erro no parser
    bool bError;

    // mensagem de erro
    struct StrBuilder ErrorMessage;

    // scanner ja prÈ-processado
    struct Scanner Scanner;


    struct SymbolMap GlobalScope;
    struct SymbolMap* pCurrentScope;

    /*
    indica se o parser esta no modo de fazer o parser das expressoes fo
    if do preprocessador aonda o identificador vira 1 ou 0
    */
    bool bPreprocessorEvalFlag;

};

bool Parser_InitFile(struct Parser* parser, const char* fileName);

bool Parser_InitString(struct Parser* parser, const char* name, const char* Text);

void Parser_Destroy(struct Parser* parser);

bool Parser_HasError(struct Parser* pParser);

const char* GetCompletationMessage(struct Parser* parser);

bool BuildSyntaxTreeFromFile(const char* filename,
                             const char* configFileName,
                             struct SyntaxTree* pSyntaxTree);

void ConstantExpression(struct Parser* ctx, struct Expression** ppExpression);
enum TokenType Parser_MatchToken(struct Parser* parser, enum TokenType tk, struct TokenList* listOpt);

enum TokenType Parser_LookAheadToken(struct Parser* parser);

bool BuildSyntaxTreeFromString(const char* sourceCode, struct SyntaxTree* pSyntaxTree);


static void Scanner_PushToken(struct Scanner* pScanner,
                              enum TokenType token,
                              const char* lexeme,
                              bool bActive,
                              int line,
                              int fileIndex);


struct BasicScanner* Scanner_Top(struct Scanner* pScanner);

void Scanner_MatchDontExpand(struct Scanner* pScanner);

enum PPTokenType TokenToPPToken(enum TokenType token)
{
    enum PPTokenType result = PPTokenType_Other;
    switch (token)
    {
        case TK_AUTO:
        case TK_BREAK:
        case TK_CASE:
        case TK_CHAR:
        case TK_CONST:
        case TK_CONTINUE:
        case TK_DEFAULT:
        case TK_DO:
        case TK_DOUBLE:
        case TK_ELSE:
        case TK_ENUM:
        case TK_EXTERN:
        case TK_FLOAT:
        case TK_FOR:
        case TK_GOTO:
        case TK_IF:
        case TK_TRY:
        case TK_DEFER:
        case TK_INT:
        case TK_LONG:
            ////////////////
            //Microsoft - specific
        case TK__INT8:
        case TK__INT16:
        case TK__INT32:
        case TK__INT64:
        case TK___DECLSPEC:
        case TK__WCHAR_T:
            ////////////////
        case TK_REGISTER:
        case TK_RETURN:
        case TK_THROW:
        case TK_SHORT:
        case TK_SIGNED:
        case TK_SIZEOF:
        case TK_STATIC:
        case TK_STRUCT:
        case TK_SWITCH:
        case TK_TYPEDEF:
        case  TK_UNION:
        case TK_UNSIGNED:
        case TK_VOID:
        case TK_VOLATILE:
        case TK_WHILE:
        case TK_CATCH:
        case TK__THREAD_LOCAL:
        case TK__BOOL:
        case TK__COMPLEX:
        case TK__ATOMIC:
        case TK_RESTRICT:
        case TK__STATIC_ASSERT:
        case TK_INLINE:
        case TK__INLINE://ms
        case TK__FORCEINLINE: //ms
        case TK__NORETURN:
        case TK__ALIGNAS:
        case TK__GENERIC:
        case  TK__IMAGINARY:
        case TK__ALINGOF:
        case TK_IDENTIFIER:
            result = PPTokenType_Identifier;
            break;
        case TK_LINE_COMMENT:
        case TK_COMMENT:
        case TK_OPEN_COMMENT:
        case TK_CLOSE_COMMENT:
        case TK_SPACES:
            result = PPTokenType_Spaces;
            break;
        case TK_HEX_INTEGER:
        case TK_DECIMAL_INTEGER:
        case TK_FLOAT_NUMBER:
            result = PPTokenType_Number;
            break;
        case TK_CHAR_LITERAL:
            result = PPTokenType_CharConstant;
            break;
        case TK_STRING_LITERAL:
            result = PPTokenType_StringLiteral;
            break;
        case TK_ARROW:
        case TK_PLUSPLUS:
        case TK_MINUSMINUS:
        case TK_LESSLESS:
        case TK_GREATERGREATER:
        case TK_LESSEQUAL:
        case TK_GREATEREQUAL:
        case TK_EQUALEQUAL:
        case TK_NOTEQUAL:
        case TK_ANDAND:
        case TK_OROR:
        case TK_MULTIEQUAL:
        case TK_DIVEQUAL:
        case TK_PERCENT_EQUAL:
        case TK_PLUSEQUAL:
        case TK_MINUS_EQUAL:
        case TK_ANDEQUAL:
        case TK_CARETEQUAL:
        case TK_OREQUAL:
        case TK_NUMBERNUMBER:
        case TK_LESSCOLON:
        case TK_COLONGREATER:
        case TK_LESSPERCENT:
        case TK_PERCENTGREATER:
        case TK_PERCENTCOLON:
        case TK_DOTDOTDOT:
        case TK_GREATERGREATEREQUAL:
        case TK_LESSLESSEQUAL:
        case TK_PERCENTCOLONPERCENTCOLON:
        case     TK_EXCLAMATION_MARK:// = '!';
        case    TK_QUOTATION_MARK:// = '\"';
        case    TK_NUMBER_SIGN:// = '#';
        case    TK_DOLLAR_SIGN:// = '$';
        case     TK_PERCENT_SIGN:// = '%';
        case    TK_AMPERSAND:// = '&';
        case     TK_APOSTROPHE:// = '\'';
        case    TK_LEFT_PARENTHESIS:// = '(';
        case    TK_RIGHT_PARENTHESIS:// = ')';
        case    TK_ASTERISK:// = '*';
        case    TK_PLUS_SIGN:// = '+';
        case    TK_COMMA:// = ':';
        case    TK_HYPHEN_MINUS:// = '-';
        case    TK_HYPHEN_MINUS_NEG:// = '-'; //nao retorna no basic char mas eh usado para saber se eh - unario
        case    TK_FULL_STOP:// = '.';
        case    TK_SOLIDUS:// = '/';
        case    TK_COLON:// = ':';
        case    TK_SEMICOLON:// = ';';
        case    TK_LESS_THAN_SIGN:// = '<';
        case    TK_EQUALS_SIGN:// = '=';
        case    TK_GREATER_THAN_SIGN:// = '>';
        case    TK_QUESTION_MARK:// = '\?';
        case    TK_COMMERCIAL_AT:// = '@';
        case     TK_LEFT_SQUARE_BRACKET:// = '[';
        case    REVERSE_SOLIDUS:// = '\\';
        case     TK_RIGHT_SQUARE_BRACKET:// = ']';
        case    TK_CIRCUMFLEX_ACCENT:// = '^';
        case    TK_LOW_LINE:// = '_';
        case    TK_GRAVE_ACCENT:// = '`';
        case    TK_LEFT_CURLY_BRACKET:// = '{';
        case    TK_VERTICAL_LINE:// = '|';
        case    TK_RIGHT_CURLY_BRACKET:// = '}';
        case    TK_TILDE: // ~
            result = PPTokenType_Punctuator;
            break;
        default:
            //assert(false);
            result = PPTokenType_Punctuator;
            break;
    }
    return result;
}

static bool IsIncludeState(enum PPState e)
{
    return e == PPState_NONE || e == PPState_I1 || e == PPState_E1;
}

void PPStateStack_Init(struct PPStateStack* p)
{
    p->pItems = NULL;
    p->Size = 0;
    p->Capacity = 0;
}
void PPStateStack_Destroy(struct PPStateStack* p)
{
    free(p->pItems);
}

void PPStateStack_Pop(struct PPStateStack* p)
{
    if (p->Size > 0)
    {
        p->Size--;
    }
}

void PPStateStack_Reserve(struct PPStateStack* p, int n)
{
    if (n > p->Capacity)
    {
        enum PPState* pnew = p->pItems;
        pnew = (enum PPState*)realloc(pnew, n * sizeof(enum PPState));
        if (pnew)
        {
            p->pItems = pnew;
            p->Capacity = n;
        }
    }
}

void PPStateStack_PushBack(struct PPStateStack* p, enum PPState e)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        PPStateStack_Reserve(p, n);
    }
    p->pItems[p->Size] = e;
    p->Size++;
}

enum PPState StateTop(struct Scanner* pScanner)
{
    if (pScanner->StackIfDef.Size == 0)
        return PPState_NONE;
    return pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 1];
}

void StatePush(struct Scanner* pScanner, enum PPState s)
{
    PPStateStack_PushBack(&pScanner->StackIfDef, s);
}

void StatePop(struct Scanner* pScanner)
{
    PPStateStack_Pop(&pScanner->StackIfDef);
}

void Scanner_GetError(struct Scanner* pScanner, struct StrBuilder* str)
{
    StrBuilder_Append(str, pScanner->DebugString.c_str);
    StrBuilder_Append(str, "\n");
    ForEachBasicScanner(p, pScanner->stack.pTop)
    {
        StrBuilder_AppendFmt(str, "%s(%d)\n",
                             p->NameOrFullPath,
                             p->stream.Line);
    }
}

void Scanner_GetFilePositionString(struct Scanner* pScanner, struct StrBuilder* sb)
{
    struct BasicScanner* pScannerTop = Scanner_Top(pScanner);
    if (pScannerTop != NULL)
    {
        StrBuilder_Set(sb,
                       pScannerTop->NameOrFullPath);
    }
    if (pScannerTop)
    {
        StrBuilder_AppendFmt(sb, "(%d): ", pScannerTop->stream.Line);
    }
    else
    {
        StrBuilder_Append(sb, "(1): ");
    }
}

void Scanner_SetError(struct Scanner* pScanner, const char* fmt, ...)
{
    if (!pScanner->bError)
    {
        pScanner->bError = true;
        if (Scanner_Top(pScanner))
        {
            StrBuilder_AppendFmt(&pScanner->ErrorString, "%s(%d) :",
                                 Scanner_Top(pScanner)->NameOrFullPath,
                                 Scanner_Top(pScanner)->stream.Line);
        }
        else
        {
            StrBuilder_Append(&pScanner->ErrorString, "(0) :");
        }
        va_list args;
        va_start(args, fmt);
        StrBuilder_AppendFmtV(&pScanner->ErrorString, fmt, args);
        va_end(args);
    }
}

void Scanner_PrintDebug(struct Scanner* pScanner)
{
    printf("include stack---\n");
    ForEachBasicScanner(p, pScanner->stack.pTop)
    {
        printf("%s(%d):\n", p->NameOrFullPath, p->stream.Line);
    }
    printf("---\n");
}

static bool AddStandardMacro(struct Scanner* pScanner, const char* name,
                             const char* value)
{
    struct Macro* pDefine1 = NEW((struct Macro)MACRO_INIT);
    pDefine1->Name = strdup(name);
    // TODO tipo do token
    PPTokenArray_PushBack(&pDefine1->ReplacementList,
                          PPToken_Create(value, PPTokenType_Other));
    pDefine1->FileIndex = 0;
    MacroMap_SetAt(&pScanner->Defines2, name, pDefine1);
    return true;
}


static bool Scanner_InitCore(struct Scanner* pScanner)
{
    pScanner->Level = 0;
    TokenList_Init(&pScanner->PreviousList);
    TokenList_Init(&pScanner->AcumulatedTokens);
    // FileInfoMap_init
    // pScanner->IncludeDir
    pScanner->FilesIncluded = (struct HashMap)HASHMAP_INIT;
    pScanner->Defines2 = (struct MacroMap)MACROMAP_INIT;
    StrBuilder_Init(&pScanner->DebugString);
    StrBuilder_Init(&pScanner->ErrorString);
    pScanner->bError = false;
    PPStateStack_Init(&pScanner->StackIfDef);
    BasicScannerStack_Init(&pScanner->stack);
    pScanner->IncludeDir = (struct StrArray)STRARRAY_INIT;
    // Indica que foi feita uma leitura especulativa
    // pScanner->bHasLookAhead = false;
    //  pScanner->pLookAheadPreviousScanner = NULL;
    // Valor lido na leitura especulativa
    // Token_Init(&pScanner->LookAhead);
    //__FILE__ __LINE__ __DATE__ __STDC__  __STD_HOSTED__  __TIME__
    //__STD_VERSION__
    //
    //https://docs.microsoft.com/pt-br/cpp/preprocessor/predefined-macros?view=msvc-160
    AddStandardMacro(pScanner, "__ptr64", " "); /*removing microsoft especific*/
    AddStandardMacro(pScanner, "__alignof", " "); /*removing microsoft especific*/
    AddStandardMacro(pScanner, "_M_IX86", "400"); /*platform*/
    AddStandardMacro(pScanner, "WIN32", "1");
    AddStandardMacro(pScanner, "_CONSOLE", "1");
    AddStandardMacro(pScanner, "_WIN32", "1");
    AddStandardMacro(pScanner, "__CPRIME__", "1");
    AddStandardMacro(pScanner, "__LINE__", "0");
    AddStandardMacro(pScanner, "__FILE__", "\"__FILE__\"");
    AddStandardMacro(pScanner, "__DATE__", "\"__DATE__\"");
    AddStandardMacro(pScanner, "__TIME__", "\"__TIME__\"");
    AddStandardMacro(pScanner, "__STDC__", "1");
    AddStandardMacro(pScanner, "__COUNTER__", "0");
    // AddStandardMacro(pScanner, "__STD_HOSTED__", "1");
    Scanner_PushToken(pScanner, TK_BOF, "", true, -1, -1);
    return true;
}

bool Scanner_InitString(struct Scanner* pScanner, const char* name,
                        const char* Text)
{
    Scanner_InitCore(pScanner);
    struct BasicScanner* pNewScanner;
    bool result =
        BasicScanner_Create(&pNewScanner, name, Text);
    BasicScannerStack_Push(&pScanner->stack, pNewScanner);
    return result;
}

bool PushExpandedMacro(struct Scanner* pScanner,

                       const char* callString,
                       const char* defineContent)
{
    if (pScanner->bError)
    {
        return false;
    }
    struct BasicScanner* pNewScanner;
    bool result = BasicScanner_Create(&pNewScanner, callString, /*defineName*/
                                      defineContent);
    if (result == true)
    {
        pNewScanner->bMacroExpanded = true;
        BasicScanner_Match(pNewScanner); // inicia
        BasicScannerStack_Push(&pScanner->stack, pNewScanner);
    }
    return result;
}

bool Scanner_GetFullPathS(struct Scanner* pScanner,
                          const char* fileName,
                          bool bQuotedForm,
                          char* fullPathOut)
{
    if (pScanner->bError)
    {
        return false;
    }
    bool bFullPathFound = false;
    // https://msdn.microsoft.com/en-us/library/36k2cdd4.aspx
    /*
    bQuotedForm
    The preprocessor searches for include files in this order:
    1) In the same directory as the file that contains the #include statement.
    2) In the directories of the currently opened include files, in the reverse
    order in which they were opened. The search begins in the directory of the
    parent include file and continues upward through the directories of any
    grandparent include files. 3) Along the path that's specified by each /I
    compiler option. 4) Along the paths that are specified by the INCLUDE
    environment variable.
    */
    if (bQuotedForm)
    {
        // char s = NULL;
        // GetFullPath(fileName, &s);
        // free(s);
        if (IsFullPath(fileName))
        {
            // Se ja vier com fullpath?? este caso esta cobrindo
            // mas deve ter uma maneira melhor de verificar se eh um fullpath ja
            bFullPathFound = true;
            strncpy(fullPathOut, fileName, CPRIME_MAX_PATH);
        }
        else
        {
            if (pScanner->stack.pTop != NULL)
            {
                // tenta nos diretorios ja abertos
                struct StrBuilder path = STRBUILDER_INIT;
                // for (int i = (int)pScanner->stack.size - 1; i >= 0; i--)
                ForEachBasicScanner(p, pScanner->stack.pTop)
                {
                    // struct BasicScanner* p = (struct BasicScanner*)pScanner->stack.pItems[i];
                    StrBuilder_Set(&path, p->FullDir);
                    StrBuilder_Append(&path, fileName);
                    bool bFileExists = FileExists(path.c_str);
                    if (bFileExists)
                    {
                        GetFullPathS(path.c_str, fullPathOut);
                        bFullPathFound = true;
                        break;
                    }
                }
                StrBuilder_Destroy(&path);
            }
            else
            {
                GetFullPathS(fileName, fullPathOut);
                bool bFileExists = FileExists(fullPathOut);
                if (bFileExists)
                {
                    bFullPathFound = true;
                }
            }
        }
    }
    /*
    Angle-bracket form
    The preprocessor searches for include files in this order:
    1) Along the path that's specified by each /I compiler option.
    2) When compiling occurs on the command line, along the paths that are
    specified by the INCLUDE environment variable.
    */
    if (!bFullPathFound)
    {
        struct StrBuilder path = STRBUILDER_INIT;
        if (pScanner->IncludeDir.size == 0)
        {
            GetFullPathS(fileName, fullPathOut);
            bool bFileExists = FileExists(fullPathOut);
            if (bFileExists)
            {
                bFullPathFound = true;
            }
        }
        for (int i = 0; i < pScanner->IncludeDir.size; i++)
        {
            const char* pItem = pScanner->IncludeDir.data[i];
            StrBuilder_Set(&path, pItem);
            //barra para o outro lado funciona
            //windows e linux
            StrBuilder_Append(&path, "/");
            StrBuilder_Append(&path, fileName);
            bool bFileExists = FileExists(path.c_str);
            if (bFileExists)
            {
                strncpy(fullPathOut, path.c_str, CPRIME_MAX_PATH);
                bFullPathFound = true;
                break;
            }
        }
        StrBuilder_Destroy(&path);
    }
    return bFullPathFound;
}

bool Scanner_IsAlreadyIncluded(struct Scanner* pScanner,
                               const char* includeFileName,
                               enum FileIncludeType fileIncludeType)
{
    bool bResult = false;
    char fullPath[CPRIME_MAX_PATH] = { 0 };
    bool bHasFullPath = false;
    switch (fileIncludeType)
    {
        case FileIncludeTypeQuoted:
        case FileIncludeTypeIncludes:
            bHasFullPath = Scanner_GetFullPathS(pScanner, includeFileName,
                                                fileIncludeType == FileIncludeTypeQuoted,
                                                fullPath);
            break;
        case FileIncludeTypeFullPath:
            strncpy(fullPath, includeFileName, CPRIME_MAX_PATH);
            bHasFullPath = true;
            break;
    };
    if (bHasFullPath)
    {
        struct FileInfo* pFile = FileInfoMap_Find(&pScanner->FilesIncluded, fullPath);
        if (pFile != NULL)
        {
            bResult = true;
        }
    }
    return bResult;
}

void Scanner_IncludeFile(struct Scanner* pScanner,
                         const char* includeFileName,
                         enum FileIncludeType fileIncludeType,
                         bool bSkipBof)
{
    if (pScanner->bError)
    {
        return;
    }
    char fullPath[CPRIME_MAX_PATH] = { 0 };
    // Faz a procura nos diretorios como se tivesse adicinando o include
    // seguindo as mesmas regras. Caso o include seja possivel ele retorna o path
    // completo  este path completo que eh usado para efeitos do pragma once.
    bool bHasFullPath = false;
    switch (fileIncludeType)
    {
        case FileIncludeTypeQuoted:
        case FileIncludeTypeIncludes:
            bHasFullPath = Scanner_GetFullPathS(pScanner, includeFileName,
                                                fileIncludeType == FileIncludeTypeQuoted,
                                                fullPath);
            break;
        case FileIncludeTypeFullPath:
            strncpy(fullPath, includeFileName, CPRIME_MAX_PATH);
            bHasFullPath = true;
            break;
    };
    if (bHasFullPath)
    {
        struct FileInfo* pFile = FileInfoMap_Find(&pScanner->FilesIncluded, fullPath);
        if (pFile != NULL && pFile->PragmaOnce)
        {
            // foi marcado como pragma once.. nao faz nada
            // tenho q enviar um comando
            Scanner_PushToken(pScanner, TK_FILE_EOF, "pragma once file", true, -1, -1);
        }
        else
        {
            if (pFile == NULL)
            {
                pFile = NEW((struct FileInfo)FILEINFO_INIT);
                pFile->IncludePath = strdup(includeFileName);
                FileInfoMap_Set(&pScanner->FilesIncluded, fullPath, pFile); // pfile Moved
            }
            struct BasicScanner* pNewScanner = NULL;
            bool result = BasicScanner_CreateFile(fullPath, &pNewScanner);
            if (result == true)
            {
                if (pFile)
                {
                    pNewScanner->FileIndex = pFile->FileIndex;
                    if (bSkipBof)
                    {
                        BasicScanner_Match(pNewScanner);
                    }
                    BasicScannerStack_Push(&pScanner->stack, pNewScanner);
                }
                else
                {
                    Scanner_SetError(pScanner, "mem");
                }
            }
            else
            {
                Scanner_SetError(pScanner, "Cannot open source file: '%s': No such file or directory", fullPath);
            }
        }
    }
    else
    {
        Scanner_SetError(pScanner, "Cannot open include file: '%s': No such file or directory", includeFileName);
    }
}

const char* Scanner_GetStreamName(struct Scanner* pScanner)
{
    const char* streamName = NULL;
    struct BasicScanner* p = Scanner_Top(pScanner);
    streamName = p ? p->NameOrFullPath : NULL;
    return streamName;
}

bool Scanner_Init(struct Scanner* pScanner)
{
    return Scanner_InitCore(pScanner);
}

void Scanner_Destroy(struct Scanner* pScanner)
{
    BasicScannerStack_Destroy(&pScanner->stack);
    MacroMap_Destroy(&pScanner->Defines2);
    PPStateStack_Destroy(&pScanner->StackIfDef);
    FileInfoMap_Destroy(&pScanner->FilesIncluded);
    StrArray_Destroy(&pScanner->IncludeDir);
    StrBuilder_Destroy(&pScanner->DebugString);
    StrBuilder_Destroy(&pScanner->ErrorString);
    TokenList_Destroy(&pScanner->AcumulatedTokens);
    TokenList_Destroy(&pScanner->PreviousList);
}

void Scanner_Reset(struct Scanner* pScanner)
{
    //Basically this function was created to allow
    //inclusion of new file Scanner_IncludeFile
    //after scanner reach EOF  (See GetSources)
    //After eof we need to Reset. The reset
    //is not general.
    //A better aprouch would be just make this work
    //correclty without reset.
    BasicScannerStack_Destroy(&pScanner->stack);
    BasicScannerStack_Init(&pScanner->stack);
    //mantem
    //MacroMap_Destroy(&pScanner->Defines2);
    PPStateStack_Destroy(&pScanner->StackIfDef);
    PPStateStack_Init(&pScanner->StackIfDef);
    //mantem
    //FileInfoMap_Destroy(&pScanner->FilesIncluded);
    //mantem
    //StrArray_Destroy(&pScanner->IncludeDir);
    //FileNodeList_Destroy(&pScanner->Sources);
    //FileNodeList_Init(&pScanner->Sources);
    StrBuilder_Destroy(&pScanner->DebugString);
    StrBuilder_Init(&pScanner->DebugString);
    StrBuilder_Destroy(&pScanner->ErrorString);
    StrBuilder_Init(&pScanner->ErrorString);
    TokenList_Destroy(&pScanner->AcumulatedTokens);
    TokenList_Init(&pScanner->AcumulatedTokens);
    pScanner->bError = false;
}

int Scanner_GetFileIndex(struct Scanner* pScanner)
{
    if (pScanner->bError)
    {
        return -1;
    }
    int fileIndex = -1;
    ForEachBasicScanner(pBasicScanner, pScanner->stack.pTop)
    {
        fileIndex = pBasicScanner->FileIndex;
        if (fileIndex >= 0)
        {
            break;
        }
    }
    // //assert(fileIndex >= 0);
    return fileIndex;
}


struct BasicScanner* Scanner_Top(struct Scanner* pScanner)
{
    return pScanner->stack.pTop;
}


void IgnorePreProcessorv2(struct BasicScanner* pBasicScanner, struct StrBuilder* strBuilder)
{
    while (pBasicScanner->token != TK_EOF &&
           pBasicScanner->token != TK_FILE_EOF)
    {
        if (pBasicScanner->token == TK_BREAKLINE)
        {
            // StrBuilder_Append(strBuilder, pTop->lexeme.c_str);
            BasicScanner_Match(pBasicScanner);
            break;
        }
        StrBuilder_Append(strBuilder, pBasicScanner->lexeme.c_str);
        BasicScanner_Match(pBasicScanner);
    }
}

void GetDefineString(struct Scanner* pScanner, struct StrBuilder* strBuilder)
{
    for (;;)
    {
        enum TokenType token = Scanner_Top(pScanner)->token;
        if (token == TK_EOF)
        {
            break;
        }
        if (token == TK_BREAKLINE)
        {
            // deixa o break line
            // BasicScanner_Match(Scanner_Top(pScanner));
            break;
        }
        if (token == TK_OPEN_COMMENT ||
            token == TK_CLOSE_COMMENT ||
            token == TK_COMMENT ||
            token == TK_LINE_COMMENT)
        {
            // transforma em espa√ßos
            StrBuilder_Append(strBuilder, " ");
        }
        else
        {
            StrBuilder_Append(strBuilder, BasicScanner_Lexeme(Scanner_Top(pScanner)));
        }
        BasicScanner_Match(Scanner_Top(pScanner));
    }
}

struct Macro* Scanner_FindPreprocessorItem2(struct Scanner* pScanner, const char* key)
{
    struct Macro* pMacro = MacroMap_Find(&pScanner->Defines2, key);
    return pMacro;
}

bool Scanner_IsLexeme(struct Scanner* pScanner, const char* psz)
{
    return BasicScanner_IsLexeme(Scanner_Top(pScanner), psz);
}

int PreprocessorExpression(struct Parser* parser)
{
    // Faz o parser da express√£o
    struct Expression* pExpression = NULL;
    ConstantExpression(parser, &pExpression);
    //..a partir da arvore da express√£o
    // calcula o valor
    // TODO pegar error
    int r;
    if (!EvaluateConstantExpression(pExpression, &r))
    {
        Scanner_SetError(&parser->Scanner, "error evaluating expression");
    }
    Expression_Delete(pExpression);
    return r;
}

int EvalExpression(const char* s, struct Scanner* pScanner)
{
    struct MacroMap* pDefines = &pScanner->Defines2;
    // printf("%s = ", s);
    // TODO avaliador de expressoes para pre processador
    // https://gcc.gnu.org/onlinedocs/gcc-3.0.2/cpp_4.html#SEC38
    struct Parser parser;
    Parser_InitString(&parser, "eval expression", s);
    parser.bPreprocessorEvalFlag = true;
    if (pDefines)
    {
        // usa o mapa de macros para o pre-processador
        MacroMap_Swap(&parser.Scanner.Defines2, pDefines);
    }
    //    Scanner_Match(&parser.Scanner);
    int iRes = PreprocessorExpression(&parser);
    // printf(" %d\n", iRes);
    if (pDefines)
    {
        MacroMap_Swap(&parser.Scanner.Defines2, pDefines);
    }
    if (parser.bError)
    {
        Scanner_SetError(pScanner, parser.ErrorMessage.c_str);
    }
    if (parser.Scanner.bError)
    {
        Scanner_SetError(pScanner, parser.Scanner.ErrorString.c_str);
    }
    Parser_Destroy(&parser);
    return iRes;
}

static void GetMacroArguments(struct BasicScanner* pBasicScanner,
                              struct Macro* pMacro, struct PPTokenArray* ppTokenArray,
                              struct StrBuilder* strBuilder)
{
    // StrBuilder_Append(strBuilderResult, Scanner_LexemeAt(pScanner));
    // TODO aqui nao pode ser o current
    const char* lexeme = pBasicScanner->lexeme.c_str;
    enum TokenType token = pBasicScanner->token;
    // verificar se tem parametros
    int nArgsExpected = pMacro->FormalArguments.Size; // pMacro->bIsFunction;
    int nArgsFound = 0;
    // fazer uma lista com os parametros
    if (token == TK_LEFT_PARENTHESIS)
    {
        // Adiciona o nome da macro
        struct PPToken* ppTokenName = PPToken_Create(pMacro->Name, PPTokenType_Identifier);
        PPTokenArray_PushBack(ppTokenArray, ppTokenName);
        // Match do (
        struct PPToken* ppToken3 = PPToken_Create(lexeme, TokenToPPToken(token));
        PPTokenArray_PushBack(ppTokenArray, ppToken3);
        StrBuilder_Append(strBuilder, lexeme);
        BasicScanner_Match(pBasicScanner);
        token = pBasicScanner->token;
        lexeme = pBasicScanner->lexeme.c_str;
        // comeca com 1
        nArgsFound = 1;
        int iInsideParentesis = 1;
        for (;;)
        {
            if (token == TK_LEFT_PARENTHESIS)
            {
                struct PPToken* ppToken = PPToken_Create(lexeme, TokenToPPToken(token));
                PPTokenArray_PushBack(ppTokenArray, ppToken);
                StrBuilder_Append(strBuilder, lexeme);
                BasicScanner_Match(pBasicScanner);
                token = pBasicScanner->token;
                lexeme = pBasicScanner->lexeme.c_str;
                iInsideParentesis++;
            }
            else if (token == TK_RIGHT_PARENTHESIS)
            {
                if (iInsideParentesis == 1)
                {
                    struct PPToken* ppToken = PPToken_Create(lexeme, TokenToPPToken(token));
                    PPTokenArray_PushBack(ppTokenArray, ppToken);
                    StrBuilder_Append(strBuilder, lexeme);
                    BasicScanner_Match(pBasicScanner);
                    token = pBasicScanner->token;
                    lexeme = pBasicScanner->lexeme.c_str;
                    break;
                }
                iInsideParentesis--;
                struct PPToken* ppToken = PPToken_Create(lexeme, TokenToPPToken(token));
                PPTokenArray_PushBack(ppTokenArray, ppToken);
                StrBuilder_Append(strBuilder, lexeme);
                BasicScanner_Match(pBasicScanner);
                token = pBasicScanner->token;
                lexeme = pBasicScanner->lexeme.c_str;
            }
            else if (token == TK_COMMA)
            {
                if (iInsideParentesis == 1)
                {
                    nArgsFound++;
                }
                else
                {
                    // continuar...
                }
                // StrBuilder_Append(strBuilderResult, Scanner_LexemeAt(pScanner));
                struct PPToken* ppToken2 = PPToken_Create(lexeme, TokenToPPToken(token));
                PPTokenArray_PushBack(ppTokenArray, ppToken2);
                StrBuilder_Append(strBuilder, lexeme);
                BasicScanner_Match(pBasicScanner);
                token = pBasicScanner->token;
                lexeme = pBasicScanner->lexeme.c_str;
            }
            else
            {
                // StrBuilder_Append(strBuilderResult, Scanner_LexemeAt(pScanner));
                struct PPToken* ppToken2 = PPToken_Create(lexeme, TokenToPPToken(token));
                PPTokenArray_PushBack(ppTokenArray, ppToken2);
                StrBuilder_Append(strBuilder, lexeme);
                BasicScanner_Match(pBasicScanner);
                token = pBasicScanner->token;
                lexeme = pBasicScanner->lexeme.c_str;
            }
        }
    }
    if (nArgsExpected != nArgsFound)
    {
        if (nArgsFound == 0 && nArgsExpected > 0)
        {
            // erro
        }
        else
        {
            if (nArgsExpected > nArgsFound)
            {
                // Scanner_SetError(pScanner, "Illegal macro call. Too few arguments
                // error");
            }
            else
            {
                // Scanner_SetError(pScanner, "Illegal macro call. Too many arguments
                // error.");
            }
            //assert(false);
            // JObj_PrintDebug(pMacro);
            // Scanner_PrintDebug(pScanner);
        }
    }
    // tODO se nao for macro tem que pegar todo
    // o match feito e devolver.
    // nome da macro e espacos..
    // return false;
}

enum TokenType FindPreToken(const char* lexeme)
{
    enum TokenType token = TK_NONE;
    if (strcmp(lexeme, "include") == 0)
    {
        token = TK_PRE_INCLUDE;
    }
    else if (strcmp(lexeme, "pragma") == 0)
    {
        token = TK_PRE_PRAGMA;
    }
    else if (strcmp(lexeme, "if") == 0)
    {
        token = TK_PRE_IF;
    }
    else if (strcmp(lexeme, "elif") == 0)
    {
        token = TK_PRE_ELIF;
    }
    else if (strcmp(lexeme, "ifndef") == 0)
    {
        token = TK_PRE_IFNDEF;
    }
    else if (strcmp(lexeme, "ifdef") == 0)
    {
        token = TK_PRE_IFDEF;
    }
    else if (strcmp(lexeme, "endif") == 0)
    {
        token = TK_PRE_ENDIF;
    }
    else if (strcmp(lexeme, "else") == 0)
    {
        token = TK_PRE_ELSE;
    }
    else if (strcmp(lexeme, "error") == 0)
    {
        token = TK_PRE_ERROR;
    }
    else if (strcmp(lexeme, "line") == 0)
    {
        token = TK_PRE_LINE;
    }
    else if (strcmp(lexeme, "undef") == 0)
    {
        token = TK_PRE_UNDEF;
    }
    else if (strcmp(lexeme, "define") == 0)
    {
        token = TK_PRE_DEFINE;
    }
    return token;
}

void GetPPTokens(struct BasicScanner* pBasicScanner, struct PPTokenArray* pptokens,
                 struct StrBuilder* strBuilder)
{
    enum TokenType token = pBasicScanner->token;
    const char* lexeme = pBasicScanner->lexeme.c_str;
    // Corpo da macro
    while (token != TK_BREAKLINE && token != TK_EOF && token != TK_FILE_EOF)
    {
        StrBuilder_Append(strBuilder, lexeme);
        if (token != TK_BACKSLASHBREAKLINE)
        {
            // TODO comentarios entram como espaco
            struct       PPToken* ppToken = PPToken_Create(lexeme, TokenToPPToken(token));
            PPTokenArray_PushBack(pptokens, ppToken);
        }
        BasicScanner_Match(pBasicScanner);
        token = pBasicScanner->token;
        lexeme = pBasicScanner->lexeme.c_str;
    }
    // Remove os espa√ßos do fim
    while (pptokens->Size > 0 &&
           pptokens->pItems[pptokens->Size - 1]->Token == PPTokenType_Spaces)
    {
        PPTokenArray_Pop(pptokens);
    }
}

static void Scanner_MatchAllPreprocessorSpaces(struct BasicScanner* pBasicScanner,
                                               struct StrBuilder* strBuilder)
{
    enum TokenType token = pBasicScanner->token;
    while (token == TK_SPACES || token == TK_BACKSLASHBREAKLINE ||
           token == TK_COMMENT ||
           token == TK_OPEN_COMMENT ||
           token == TK_CLOSE_COMMENT)
    {
        StrBuilder_Append(strBuilder, pBasicScanner->lexeme.c_str);
        BasicScanner_Match(pBasicScanner);
        token = pBasicScanner->token;
    }
}

void ParsePreDefinev2(struct Scanner* pScanner, struct StrBuilder* strBuilder)
{
    struct BasicScanner* pBasicScanner = Scanner_Top(pScanner);
    // objetivo eh montar a macro e colocar no mapa
    struct Macro* pNewMacro = NEW((struct Macro)MACRO_INIT);
    enum TokenType token = pBasicScanner->token;
    const char* lexeme = pBasicScanner->lexeme.c_str;
    pNewMacro->Name = strdup(lexeme);
    StrBuilder_Append(strBuilder, lexeme);
    // Match nome da macro
    BasicScanner_Match(pBasicScanner);
    token = pBasicScanner->token;
    lexeme = pBasicScanner->lexeme.c_str;
    // Se vier ( √© macro com par√¢metros
    if (token == TK_LEFT_PARENTHESIS)
    {
        pNewMacro->bIsFunction = true;
        StrBuilder_Append(strBuilder, lexeme);
        // Match (
        BasicScanner_Match(pBasicScanner);
        for (;;)
        {
            Scanner_MatchAllPreprocessorSpaces(pBasicScanner, strBuilder);
            token = pBasicScanner->token;
            lexeme = pBasicScanner->lexeme.c_str;
            if (token == TK_RIGHT_PARENTHESIS)
            {
                // Match )
                StrBuilder_Append(strBuilder, lexeme);
                BasicScanner_Match(pBasicScanner);
                break;
            }
            if (token == TK_BREAKLINE || token == TK_EOF)
            {
                // oopss
                break;
            }
            token = pBasicScanner->token;
            lexeme = pBasicScanner->lexeme.c_str;
            struct       PPToken* ppToken = PPToken_Create(lexeme, TokenToPPToken(token));
            PPTokenArray_PushBack(&pNewMacro->FormalArguments, ppToken);
            StrBuilder_Append(strBuilder, lexeme);
            BasicScanner_Match(pBasicScanner);
            Scanner_MatchAllPreprocessorSpaces(pBasicScanner, strBuilder);
            token = pBasicScanner->token;
            lexeme = pBasicScanner->lexeme.c_str;
            if (token == TK_COMMA)
            {
                // Match ,
                StrBuilder_Append(strBuilder, lexeme);
                BasicScanner_Match(pBasicScanner);
                // tem mais
            }
        }
    }
    else
    {
        Scanner_MatchAllPreprocessorSpaces(pBasicScanner, strBuilder);
    }
    Scanner_MatchAllPreprocessorSpaces(pBasicScanner, strBuilder);
    GetPPTokens(pBasicScanner, &pNewMacro->ReplacementList, strBuilder);
    MacroMap_SetAt(&pScanner->Defines2, pNewMacro->Name, pNewMacro);
    // breakline ficou...
}

int EvalPre(struct Scanner* pScanner, struct StrBuilder* sb)
{
    if (pScanner->bError)
    {
        return 0;
    }
    // pega todos os tokens ate o final da linha expande e
    // avalia
    // usado no #if #elif etc.
    struct BasicScanner* pBasicScanner = Scanner_Top(pScanner);
    struct PPTokenArray pptokens = PPTOKENARRAY_INIT;
    GetPPTokens(pBasicScanner, &pptokens, sb);
    struct StrBuilder strBuilder = STRBUILDER_INIT;
    ExpandMacroToText(&pptokens, &pScanner->Defines2, false, true, true, NULL,
                      &strBuilder);
    int iRes = EvalExpression(strBuilder.c_str, pScanner);
    if (pScanner->bError)
    {
        //assert(false);
    }
    /*struct StrBuilder sb1 = STRBUILDER_INIT;
    Scanner_GetFilePositionString(pScanner, &sb1);
    printf("%s \n", sb1.c_str);
    printf("#if ");
    for (int i = 0; i < pptokens.Size; i++)
    {
      printf("%s", pptokens.pItems[i]->Lexeme);
    }
    printf(" == %d \n\n",iRes);
    StrBuilder_Destroy(&sb1);
    */
    StrBuilder_Destroy(&strBuilder);
    PPTokenArray_Destroy(&pptokens);
    return iRes;
}

static void Scanner_PushToken(struct Scanner* pScanner,
                              enum TokenType token,
                              const char* lexeme,
                              bool bActive,
                              int line,
                              int fileIndex)
{
    if (pScanner->bError)
    {
        return;
    }
    struct Token* pNew = NEW((struct Token)TOKEN_INIT);
    LocalStrBuilder_Set(&pNew->lexeme, lexeme);
    pNew->token = token;
    pNew->bActive = bActive; //;
    pNew->Line = line;
    pNew->FileIndex = fileIndex;
    TokenList_PushBack(&pScanner->AcumulatedTokens, pNew);
}

// Atencao
// Esta funcao eh complicada.
//
// Ela faz uma parte da expansao da macro que pode virar um "tetris"
// aonde o colapso de uma expansao vira outra expansao
// a unica garantia sao os testes.
//
void Scanner_BuyIdentifierThatCanExpandAndCollapse(struct Scanner* pScanner)
{
    enum PPState state = StateTop(pScanner);
    struct BasicScanner* pBasicScanner = Scanner_Top(pScanner);
    //assert(pBasicScanner != NULL);
    enum TokenType token = pBasicScanner->token;
    const char* lexeme = pBasicScanner->lexeme.c_str;
    int line = pBasicScanner->stream.Line;
    int fileIndex = pBasicScanner->FileIndex;
    //assert(token == TK_IDENTIFIER);
    if (!IsIncludeState(state))
    {
        struct Token* pNew = NEW((struct Token)TOKEN_INIT);
        LocalStrBuilder_Set(&pNew->lexeme, pBasicScanner->lexeme.c_str);
        pNew->token = pBasicScanner->token;
        pNew->bActive = false;
        pNew->Line = line;
        pNew->FileIndex = fileIndex;
        TokenList_PushBack(&pScanner->AcumulatedTokens, pNew);
        // Match do identificador
        BasicScanner_Match(pBasicScanner);
        return;
    }
    struct Macro* pMacro2 = Scanner_FindPreprocessorItem2(pScanner, lexeme);
    if (pMacro2 == NULL)
    {
        // nao eh macro
        struct Token* pNew = NEW((struct Token)TOKEN_INIT);
        LocalStrBuilder_Set(&pNew->lexeme, pBasicScanner->lexeme.c_str);
        pNew->token = pBasicScanner->token;
        pNew->bActive = true;
        pNew->Line = line;
        pNew->FileIndex = fileIndex;
        TokenList_PushBack(&pScanner->AcumulatedTokens, pNew);
        // Match do identificador
        BasicScanner_Match(pBasicScanner);
        return;
    }
    if (pBasicScanner->bMacroExpanded &&
        strcmp(pMacro2->Name, pBasicScanner->NameOrFullPath) == 0)
    {
        // ja estou expandindo esta mesma macro
        // nao eh macro
        struct Token* pNew = NEW((struct Token)TOKEN_INIT);
        LocalStrBuilder_Set(&pNew->lexeme, pBasicScanner->lexeme.c_str);
        pNew->token = pBasicScanner->token;
        pNew->bActive = true;
        pNew->Line = line;
        pNew->FileIndex = fileIndex;
        TokenList_PushBack(&pScanner->AcumulatedTokens, pNew);
        // Match do identificador
        BasicScanner_Match(pBasicScanner);
        return;
    }
    struct Macro* pFirstMacro = pMacro2;
    // Match do identificador do nome da macro funcao
    BasicScanner_Match(pBasicScanner);
    bool bExitLoop = false;
    do
    {
        if (!pMacro2->bIsFunction)
        {
            struct PPTokenArray ppTokenArray = PPTOKENARRAY_INIT;
            // o nome eh a propria expansao
            struct       PPToken* ppTokenName =
                PPToken_Create(pMacro2->Name, TokenToPPToken(TK_IDENTIFIER));
            PPTokenArray_PushBack(&ppTokenArray, ppTokenName);
            struct StrBuilder strExpanded = STRBUILDER_INIT;
            ExpandMacroToText(&ppTokenArray, &pScanner->Defines2, false, false, false, NULL,
                              &strExpanded);
            // se expandir para identificador e ele for uma macro do tipo funcao
            // pode ser tetris
            struct Macro* pMacro3 = NULL;
            if (strExpanded.size > 0)
            {
                pMacro3 = Scanner_FindPreprocessorItem2(pScanner, strExpanded.c_str);
            }
            if (pMacro3)
            {
                if (pMacro3->bIsFunction)
                {
                    // Expandiu para uma identificador que √© macro funcao
                    pMacro2 = pMacro3;
                }
                else
                {
                    // ok expandiu
                    PushExpandedMacro(pScanner, pMacro2->Name, strExpanded.c_str);
                    Scanner_PushToken(pScanner, TK_MACRO_CALL, pMacro2->Name, true, line, fileIndex);
                    bExitLoop = true;
                }
            }
            else
            {
                // ok expandiu
                PushExpandedMacro(pScanner, pMacro2->Name, strExpanded.c_str);
                Scanner_PushToken(pScanner, TK_MACRO_CALL, pMacro2->Name, true, line, fileIndex);
                bExitLoop = true;
            }
            PPTokenArray_Destroy(&ppTokenArray);
            StrBuilder_Destroy(&strExpanded);
        }
        else
        {
            //√© uma fun√ß√£o
            // Procurar pelo (
            struct TokenList LocalAcumulatedTokens = { 0 };
            token = pBasicScanner->token;
            lexeme = pBasicScanner->lexeme.c_str;
            while (token == TK_SPACES ||
                   token == TK_COMMENT ||
                   token == TK_OPEN_COMMENT ||
                   token == TK_CLOSE_COMMENT)
            {
                // StrBuilder_Append(strBuilder, lexeme);
                /////////////
                struct Token* pNew = NEW((struct Token)TOKEN_INIT);
                LocalStrBuilder_Set(&pNew->lexeme, lexeme);
                pNew->token = token;
                pNew->bActive = true;
                pNew->Line = line;
                pNew->FileIndex = fileIndex;
                TokenList_PushBack(&LocalAcumulatedTokens, pNew);
                ////////////
                BasicScanner_Match(pBasicScanner);
                token = pBasicScanner->token;
                lexeme = pBasicScanner->lexeme.c_str;
            }
            if (token == TK_LEFT_PARENTHESIS)
            {
                struct StrBuilder strCallString = STRBUILDER_INIT;
                struct StrBuilder strExpanded = STRBUILDER_INIT;
                struct PPTokenArray ppTokenArray = PPTOKENARRAY_INIT;
                StrBuilder_Set(&strCallString, pFirstMacro->Name);
                // eh uma chamada da macro funcao
                // coletar argumentos e expandir
                GetMacroArguments(pBasicScanner,
                                  pMacro2,
                                  &ppTokenArray,
                                  &strCallString);
                ExpandMacroToText(&ppTokenArray,
                                  &pScanner->Defines2,
                                  false,
                                  false,
                                  false,
                                  NULL,
                                  &strExpanded);
                /////////////////////////////////
                // se expandir para identificador e ele for uma macro do tipo funcao
                // pode ser tetris
                struct Macro* pMacro3 = NULL;
                if (strExpanded.size > 0)
                {
                    pMacro3 = Scanner_FindPreprocessorItem2(pScanner, strExpanded.c_str);
                }
                if (pMacro3)
                {
                    if (pMacro3->bIsFunction)
                    {
                        // Expandiu para uma identificador que √© macro funcao
                        pMacro2 = pMacro3;
                    }
                    else
                    {
                        // ok expandiu
                        PushExpandedMacro(pScanner, pMacro2->Name, strExpanded.c_str);
                        Scanner_PushToken(pScanner, TK_MACRO_CALL, pMacro2->Name, true, line, fileIndex);
                    }
                }
                else
                {
                    // ok expandiu
                    PushExpandedMacro(pScanner, pMacro2->Name, strExpanded.c_str);
                    Scanner_PushToken(pScanner, TK_MACRO_CALL, strCallString.c_str, true, line, fileIndex);
                    bExitLoop = true;
                }
                ///////////////////////
                PPTokenArray_Destroy(&ppTokenArray);
                StrBuilder_Destroy(&strExpanded);
                StrBuilder_Destroy(&strCallString);
            }
            else
            {
                // macro call
                // B
                // endcall
                // espacos
                if (pFirstMacro != pMacro2)
                {
                    // nao era uma chamada da macro funcao
                    struct Token* pNew = NEW((struct Token)TOKEN_INIT);
                    LocalStrBuilder_Append(&pNew->lexeme, pFirstMacro->Name);
                    pNew->token = TK_MACRO_CALL;
                    pNew->bActive = true;
                    pNew->Line = line;
                    pNew->FileIndex = fileIndex;
                    TokenList_PushBack(&pScanner->AcumulatedTokens, pNew);
                }
                struct Token* pNew0 = NEW((struct Token)TOKEN_INIT);
                LocalStrBuilder_Append(&pNew0->lexeme, pMacro2->Name);
                pNew0->token = TK_IDENTIFIER;
                pNew0->bActive = true;
                pNew0->Line = line;
                pNew0->FileIndex = fileIndex;
                TokenList_PushBack(&pScanner->AcumulatedTokens, pNew0);
                if (pFirstMacro != pMacro2)
                {
                    struct Token* pNew2 = NEW((struct Token)TOKEN_INIT);
                    pNew2->token = TK_MACRO_EOF;
                    pNew2->bActive = true;
                    pNew2->Line = line;
                    pNew2->FileIndex = fileIndex;
                    TokenList_PushBack(&pScanner->AcumulatedTokens, pNew2);
                }
                if (LocalAcumulatedTokens.pHead != NULL)
                {
                    TokenList_PushBack(&pScanner->AcumulatedTokens, LocalAcumulatedTokens.pHead);
                    LocalAcumulatedTokens.pHead = NULL;
                    LocalAcumulatedTokens.pTail = NULL;
                }
                // tODO espacos
                // a macro eh uma funcao mas isso nao eh a chamada da macro
                // pq nao foi encontrado (
                bExitLoop = true;
            }
            TokenList_Destroy(&LocalAcumulatedTokens);
        }
        if (bExitLoop)
            break;
    }
    while (!bExitLoop);
}

void Scanner_BuyTokens(struct Scanner* pScanner)
{
    // Sempre compra uma carta nova do monte do baralho.
    // se a carta servir ele coloca na mesa.
    // se comprar uma carta macro expande e coloca em cima
    // do baralho novas cartas sou coloca na mesa
    //(sim eh complicado)
    if (pScanner->bError)
    {
        return;
    }
    struct BasicScanner* pBasicScanner = Scanner_Top(pScanner);
    if (pBasicScanner == NULL)
    {
        // acabaram todos os montes de tokens (cartas do baralho)
        return;
    }
    enum TokenType token = pBasicScanner->token;
    const char* lexeme = pBasicScanner->lexeme.c_str;
    int line = pBasicScanner->stream.Line;
    int fileIndex = pBasicScanner->FileIndex;
    if (token == TK_BOF)
    {
        BasicScanner_Match(pBasicScanner);
        token = pBasicScanner->token;
        lexeme = pBasicScanner->lexeme.c_str;
    }
    while (token == TK_EOF)
    {
        // ok remove este baralho vazio
        BasicScannerStack_Pop(&pScanner->stack);
        // proximo baralho
        pBasicScanner = Scanner_Top(pScanner);
        if (pBasicScanner != NULL)
        {
            // vai removendo enquanto sao baralhos vazios
            token = pBasicScanner->token;
            lexeme = pBasicScanner->lexeme.c_str;
        }
        else
        {
            break;
        }
    }
    if (token == TK_FILE_EOF)
    {
        if (pScanner->stack.pTop->pPrevious == NULL)
        {
            // se eh o unico arquivo TK_FILE_EOF vira eof
            token = TK_EOF;
        }
    }
    if (token == TK_EOF)
    {
        // n√£o sobrou nenhum baralho nao tem o que comprar
        struct Token* pNew = NEW((struct Token)TOKEN_INIT);
        pNew->token = TK_EOF;
        pNew->bActive = true;
        pNew->Line = line;
        pNew->FileIndex = fileIndex;
        TokenList_PushBack(&pScanner->AcumulatedTokens, pNew);
        return;
    }
    struct StrBuilder strBuilder = STRBUILDER_INIT;
    enum PPState state = StateTop(pScanner);
    bool bActive0 = IsIncludeState(state);
    if (token == TK_PREPROCESSOR)
    {
        // Match #
        StrBuilder_Append(&strBuilder, pBasicScanner->lexeme.c_str);
        BasicScanner_Match(pBasicScanner);
        // Match ' '
        Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
        lexeme = pBasicScanner->lexeme.c_str;
        token = pBasicScanner->token;
        enum TokenType preToken = FindPreToken(lexeme);
        if (preToken == TK_PRE_INCLUDE)
        {
            // Match include
            StrBuilder_Append(&strBuilder, lexeme);
            BasicScanner_Match(pBasicScanner);
            lexeme = pBasicScanner->lexeme.c_str;
            token = pBasicScanner->token;
            if (IsIncludeState(state))
            {
                // Match espacos
                Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
                lexeme = pBasicScanner->lexeme.c_str;
                token = pBasicScanner->token;
                if (token == TK_STRING_LITERAL)
                {
                    char* fileName;
                    fileName = strdup(lexeme + 1);
                    StrBuilder_Append(&strBuilder, lexeme);
                    BasicScanner_Match(pBasicScanner);
                    fileName[strlen(fileName) - 1] = 0;
                    // tem que ser antes de colocar o outro na pilha
                    IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                    Scanner_PushToken(pScanner, TK_PRE_INCLUDE, strBuilder.c_str, true, line, fileIndex);
                    Scanner_IncludeFile(pScanner, fileName, FileIncludeTypeQuoted, true);
                    free(fileName);
                    // break;
                }
                else if (token == TK_LESS_THAN_SIGN)
                {
                    StrBuilder_Append(&strBuilder, lexeme);
                    BasicScanner_Match(pBasicScanner);
                    lexeme = pBasicScanner->lexeme.c_str;
                    token = pBasicScanner->token;
                    struct StrBuilder path = STRBUILDER_INIT;
                    for (;;)
                    {
                        StrBuilder_Append(&strBuilder, lexeme);
                        if (token == TK_GREATER_THAN_SIGN)
                        {
                            BasicScanner_Match(pBasicScanner);
                            lexeme = pBasicScanner->lexeme.c_str;
                            token = pBasicScanner->token;
                            break;
                        }
                        if (token == TK_BREAKLINE)
                        {
                            // oopps
                            break;
                        }
                        StrBuilder_Append(&path, lexeme);
                        BasicScanner_Match(pBasicScanner);
                        lexeme = pBasicScanner->lexeme.c_str;
                        token = pBasicScanner->token;
                    }
                    IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                    Scanner_PushToken(pScanner, TK_PRE_INCLUDE, strBuilder.c_str, true, line, fileIndex);
                    Scanner_IncludeFile(pScanner, path.c_str, FileIncludeTypeIncludes,
                                        true);
                    StrBuilder_Destroy(&path);
                }
            }
            else
            {
                // TODO active
                Scanner_PushToken(pScanner, TK_SPACES, strBuilder.c_str, false, line, fileIndex);
            }
        }
        else if (preToken == TK_PRE_PRAGMA)
        {
            // Match pragma
            StrBuilder_Append(&strBuilder, lexeme);
            BasicScanner_Match(pBasicScanner);
            if (IsIncludeState(state))
            {
                Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
                if (BasicScanner_IsLexeme(pBasicScanner, "once"))
                {
                    const char* fullPath = Scanner_Top(pScanner)->NameOrFullPath;
                    struct FileInfo* pFile = FileInfoMap_Find(&pScanner->FilesIncluded, fullPath);
                    if (pFile == NULL)
                    {
                        pFile = NEW((struct FileInfo)FILEINFO_INIT);
                        pFile->PragmaOnce = true;
                        FileInfoMap_Set(&pScanner->FilesIncluded, fullPath, pFile);
                    }
                    else
                    {
                        pFile->PragmaOnce = true;
                    }
                    //
                    IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                    Scanner_PushToken(pScanner, TK_PRE_PRAGMA, strBuilder.c_str, true, line, fileIndex);
                }
                else if (BasicScanner_IsLexeme(Scanner_Top(pScanner), "message"))
                {
                    // Match message
                    StrBuilder_Append(&strBuilder, lexeme);
                    BasicScanner_Match(pBasicScanner);
                    if (IsIncludeState(state))
                    {
                        GetDefineString(pScanner, &strBuilder);
                        IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                        Scanner_PushToken(pScanner, TK_PRE_PRAGMA, strBuilder.c_str, true, line, fileIndex);
                    }
                    else
                    {
                        Scanner_PushToken(pScanner, preToken, strBuilder.c_str, false, line, fileIndex);
                    }
                }
                else if (BasicScanner_IsLexeme(Scanner_Top(pScanner), "dir"))
                {
                    StrBuilder_Append(&strBuilder, lexeme);
                    BasicScanner_Match(pBasicScanner);
                    Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
                    lexeme = pBasicScanner->lexeme.c_str;
                    char* fileName;
                    fileName = strdup(lexeme + 1);
                    Scanner_Match(pScanner);
                    fileName[strlen(fileName) - 1] = 0;
                    StrArray_Push(&pScanner->IncludeDir, fileName);
                    free(fileName);
                    //
                    IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                    Scanner_PushToken(pScanner, TK_PRE_PRAGMA, strBuilder.c_str, true, line, fileIndex);
                }
                else if (BasicScanner_IsLexeme(pBasicScanner, "expand"))
                {
                    StrBuilder_Append(&strBuilder, "expand");
                    BasicScanner_Match(pBasicScanner);
                    Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
                    IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                    Scanner_PushToken(pScanner, TK_PRE_PRAGMA, strBuilder.c_str, true, line, fileIndex);
                }
                else
                {
                    //
                    IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                    Scanner_PushToken(pScanner, TK_PRE_PRAGMA, strBuilder.c_str, true, line, fileIndex);
                }
            }
            else
            {
                IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                Scanner_PushToken(pScanner, preToken, strBuilder.c_str, false, line, fileIndex);
            }
        }
        else if (preToken == TK_PRE_IF || preToken == TK_PRE_IFDEF ||
                 preToken == TK_PRE_IFNDEF)
        {
            StrBuilder_Append(&strBuilder, pBasicScanner->lexeme.c_str);
            BasicScanner_Match(pBasicScanner);
            Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
            lexeme = pBasicScanner->lexeme.c_str;
            switch (state)
            {
                case PPState_NONE:
                case PPState_I1:
                case PPState_E1:
                {
                    int iRes = 0;
                    if (preToken == TK_PRE_IF)
                    {
                        iRes = EvalPre(pScanner, &strBuilder);
                    }
                    else
                    {
                        bool bFound = Scanner_FindPreprocessorItem2(pScanner, lexeme) != NULL;
                        if (preToken == TK_PRE_IFDEF)
                        {
                            iRes = bFound ? 1 : 0;
                        }
                        else // if (preToken == TK_PRE_IFNDEF)
                        {
                            iRes = !bFound ? 1 : 0;
                        }
                    }
                    if (iRes != 0)
                    {
                        StatePush(pScanner, PPState_I1);
                    }
                    else
                    {
                        StatePush(pScanner, PPState_I0);
                    }
                }
                break;
                case PPState_I0:
                    StatePush(pScanner, PPState_I0);
                    break;
                case PPState_E0:
                    StatePush(pScanner, PPState_E0);
                    break;
            }
            state = StateTop(pScanner);
            bool bActive = IsIncludeState(state);
            IgnorePreProcessorv2(pBasicScanner, &strBuilder);
            Scanner_PushToken(pScanner, preToken, strBuilder.c_str, bActive, line, fileIndex);
        }
        else if (preToken == TK_PRE_ELIF)
        {
            // Match elif
            StrBuilder_Append(&strBuilder, pBasicScanner->lexeme.c_str);
            BasicScanner_Match(pBasicScanner);
            Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
            switch (state)
            {
                case PPState_NONE:
                case PPState_I1:
                    pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 1] = PPState_E0;
                    break;
                case PPState_I0:
                {
                    int iRes = EvalPre(pScanner, &strBuilder);
                    if (pScanner->StackIfDef.Size >= 2)
                    {
                        if ((pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 2] ==
                            PPState_I1 ||
                            pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 2] ==
                            PPState_E1))
                        {
                            if (iRes)
                            {
                                pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 1] = PPState_I1;
                            }
                        }
                    }
                    else
                    {
                        if (iRes)
                        {
                            pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 1] = PPState_I1;
                        }
                    }
                }
                break;
                case PPState_E0:
                    break;
                case PPState_E1:
                    //assert(0);
                    break;
            }
            state = StateTop(pScanner);
            bool bActive = IsIncludeState(state);
            IgnorePreProcessorv2(pBasicScanner, &strBuilder);
            Scanner_PushToken(pScanner, TK_PRE_ELIF, strBuilder.c_str, bActive, line, fileIndex);
        }
        else if (preToken == TK_PRE_ENDIF)
        {
            // Match elif
            StrBuilder_Append(&strBuilder, pBasicScanner->lexeme.c_str);
            BasicScanner_Match(pBasicScanner);
            Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
            IgnorePreProcessorv2(pBasicScanner, &strBuilder);
            StatePop(pScanner);
            state = StateTop(pScanner);
            bool bActive = IsIncludeState(state);
            Scanner_PushToken(pScanner, TK_PRE_ENDIF, strBuilder.c_str, bActive, line, fileIndex);
        }
        else if (preToken == TK_PRE_ELSE)
        {
            // Match else
            StrBuilder_Append(&strBuilder, pBasicScanner->lexeme.c_str);
            BasicScanner_Match(pBasicScanner);
            switch (state)
            {
                case PPState_NONE:
                    //assert(0);
                    break;
                case PPState_I1:
                    pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 1] = PPState_E0;
                    break;
                case PPState_I0:
                    if (pScanner->StackIfDef.Size >= 2)
                    {
                        if ((pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 2] ==
                            PPState_I1 ||
                            pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 2] ==
                            PPState_E1))
                        {
                            pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 1] = PPState_E1;
                        }
                    }
                    else
                    {
                        pScanner->StackIfDef.pItems[pScanner->StackIfDef.Size - 1] = PPState_E1;
                    }
                    break;
                case PPState_E0:
                    break;
                case PPState_E1:
                    //assert(false);
                    break;
            }
            state = StateTop(pScanner);
            bool bActive = IsIncludeState(state);
            IgnorePreProcessorv2(pBasicScanner, &strBuilder);
            Scanner_PushToken(pScanner, TK_PRE_ELSE, strBuilder.c_str, bActive, line, fileIndex);
        }
        else if (preToken == TK_PRE_ERROR)
        {
            // Match error
            StrBuilder_Append(&strBuilder, lexeme);
            BasicScanner_Match(pBasicScanner);
            if (IsIncludeState(state))
            {
                struct StrBuilder str = STRBUILDER_INIT;
                StrBuilder_Append(&str, ": #error : ");
                GetDefineString(pScanner, &str);
                Scanner_SetError(pScanner, str.c_str);
                StrBuilder_Destroy(&str);
                IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                Scanner_PushToken(pScanner, TK_PRE_ERROR, strBuilder.c_str, true, line, fileIndex);
            }
            else
            {
                Scanner_PushToken(pScanner, preToken, strBuilder.c_str, false, line, fileIndex);
            }
        }
        else if (preToken == TK_PRE_LINE)
        {
            if (IsIncludeState(state))
            {
                // Match line
                StrBuilder_Append(&strBuilder, pBasicScanner->lexeme.c_str);
                BasicScanner_Match(pBasicScanner);
                IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                Scanner_PushToken(pScanner, TK_PRE_LINE, strBuilder.c_str, true, line, fileIndex);
            }
            else
            {
                Scanner_PushToken(pScanner, preToken, strBuilder.c_str, false, line, fileIndex);
            }
        }
        else if (preToken == TK_PRE_UNDEF)
        {
            if (IsIncludeState(state))
            {
                // Match undef
                StrBuilder_Append(&strBuilder, pBasicScanner->lexeme.c_str);
                BasicScanner_Match(pBasicScanner);
                lexeme = BasicScanner_Lexeme(Scanner_Top(pScanner));
                MacroMap_RemoveKey(&pScanner->Defines2, lexeme);
                IgnorePreProcessorv2(pBasicScanner, &strBuilder);
                Scanner_PushToken(pScanner, TK_PRE_UNDEF, strBuilder.c_str, true, line, fileIndex);
            }
            else
            {
                Scanner_PushToken(pScanner, preToken, strBuilder.c_str, false, line, fileIndex);
            }
        }
        else if (preToken == TK_PRE_DEFINE)
        {
            // Match define
            StrBuilder_Append(&strBuilder, lexeme);
            BasicScanner_Match(pBasicScanner);
            // Match all ' '
            Scanner_MatchAllPreprocessorSpaces(pBasicScanner, &strBuilder);
            bool bActive = IsIncludeState(state);
            if (bActive)
            {
                ParsePreDefinev2(pScanner, &strBuilder);
            }
            IgnorePreProcessorv2(pBasicScanner, &strBuilder);
            Scanner_PushToken(pScanner, TK_PRE_DEFINE, strBuilder.c_str, bActive, line, fileIndex);
        }
        // break;
    } //#
    else if (token == TK__PRAGMA || token == TK__C99PRAGMA)
    {
        BasicScanner_Match(pBasicScanner); //skip __pragma or _Pragma
        int count = 0;
        for (;;)
        {
            if (pBasicScanner->token == TK_LEFT_PARENTHESIS)
                count++;
            else if (pBasicScanner->token == TK_RIGHT_PARENTHESIS)
                count--;
            BasicScanner_Match(pBasicScanner);
            if (count == 0 || pBasicScanner->token == TK_EOF)
                break;
        }
        //TODO (tem que mudar lah no gerador)
        Scanner_PushToken(pScanner, TK_PRE_PRAGMA, "", false, line, fileIndex);
    }
    else if (token == TK_IDENTIFIER)
    {
        // codigo complicado tetris
        Scanner_BuyIdentifierThatCanExpandAndCollapse(pScanner);
    }
    else
    {
        struct Token* pNew = NEW((struct Token)TOKEN_INIT);
        LocalStrBuilder_Set(&pNew->lexeme, pBasicScanner->lexeme.c_str);
        pNew->token = pBasicScanner->token;
        pNew->bActive = bActive0;
        pNew->Line = line;
        pNew->FileIndex = fileIndex;
        TokenList_PushBack(&pScanner->AcumulatedTokens, pNew);
        BasicScanner_Match(pBasicScanner);
    }
    StrBuilder_Destroy(&strBuilder);
}

void PrintPreprocessedToFileCore(FILE* fp, struct Scanner* scanner)
{
    while (Scanner_TokenAt(scanner, 0) != TK_EOF)
    {
        enum TokenType token = Scanner_TokenAt(scanner, 0);
        const char* lexeme = Scanner_LexemeAt(scanner, 0);
        if (Scanner_IsActiveAt(scanner, 0))
        {
            switch (token)
            {
                // enum Tokens para linhas do pre processador
                case TK_PRE_INCLUDE:
                case TK_PRE_PRAGMA:
                case TK_PRE_IF:
                case TK_PRE_ELIF:
                case TK_PRE_IFNDEF:
                case TK_PRE_IFDEF:
                case TK_PRE_ENDIF:
                case TK_PRE_ELSE:
                case TK_PRE_ERROR:
                case TK_PRE_LINE:
                case TK_PRE_UNDEF:
                case TK_PRE_DEFINE:
                    fprintf(fp, "\n");
                    break;
                    // fim tokens preprocessador
                case TK_LINE_COMMENT:
                case TK_COMMENT:
                case TK_OPEN_COMMENT:
                case TK_CLOSE_COMMENT:
                    fprintf(fp, " ");
                    break;
                case TK_BOF:
                    break;
                case TK_MACRO_CALL:
                case TK_MACRO_EOF:
                case TK_FILE_EOF:
                    break;
                default:
                    fprintf(fp, "%s", lexeme);
                    break;
            }
        }
        Scanner_Match(scanner);
    }
}

void PrintPreprocessedToFile(const char* fileIn, const char* configFileName)
{
    char fullFileNamePath[CPRIME_MAX_PATH] = { 0 };
    GetFullPathS(fileIn, fullFileNamePath);
    struct Scanner scanner;
    Scanner_Init(&scanner);
    Scanner_IncludeFile(&scanner, fullFileNamePath, FileIncludeTypeFullPath,
                        false);
    if (configFileName != NULL)
    {
        char configFullPath[CPRIME_MAX_PATH] = { 0 };
        GetFullPathS(configFileName, configFullPath);
        Scanner_IncludeFile(&scanner, configFullPath, FileIncludeTypeFullPath,
                            true);
        Scanner_Match(&scanner);
    }
    ///
    char drive[CPRIME_MAX_DRIVE];
    char dir[CPRIME_MAX_DIR];
    char fname[CPRIME_MAX_FNAME];
    char ext[CPRIME_MAX_EXT];
    SplitPath(fullFileNamePath, drive, dir, fname, ext); // C4996
    char fileNameOut[CPRIME_MAX_DRIVE + CPRIME_MAX_DIR + CPRIME_MAX_FNAME + CPRIME_MAX_EXT + 1];
    strcat(fname, "_pre");
    MakePath(fileNameOut, drive, dir, fname, ".c");
    FILE* fp = fopen(fileNameOut, "w");
    PrintPreprocessedToFileCore(fp, &scanner);
    fclose(fp);
    Scanner_Destroy(&scanner);
}

void PrintPreprocessedToStringCore2(struct StrBuilder* fp, struct Scanner* scanner)
{
    while (Scanner_TokenAt(scanner, 0) != TK_EOF)
    {
        enum TokenType token = Scanner_TokenAt(scanner, 0);
        const char* lexeme = Scanner_LexemeAt(scanner, 0);
        if (Scanner_IsActiveAt(scanner, 0))
        {
            switch (token)
            {
                // enum Tokens para linhas do pre processador
                case TK_PRE_INCLUDE:
                case TK_PRE_PRAGMA:
                case TK_PRE_IF:
                case TK_PRE_ELIF:
                case TK_PRE_IFNDEF:
                case TK_PRE_IFDEF:
                case TK_PRE_ENDIF:
                case TK_PRE_ELSE:
                case TK_PRE_ERROR:
                case TK_PRE_LINE:
                case TK_PRE_UNDEF:
                case TK_PRE_DEFINE:
                    StrBuilder_Append(fp, "\n");
                    break;
                    // fim tokens preprocessador
                case TK_LINE_COMMENT:
                case TK_COMMENT:
                case TK_OPEN_COMMENT:
                case TK_CLOSE_COMMENT:
                    StrBuilder_Append(fp, " ");
                    break;
                case TK_BOF:
                    break;
                case TK_MACRO_CALL:
                case TK_MACRO_EOF:
                case TK_FILE_EOF:
                    break;
                default:
                    StrBuilder_Append(fp, lexeme);
                    break;
            }
        }
        Scanner_Match(scanner);
    }
}

void PrintPreprocessedToString2(struct StrBuilder* fp, const char* input, const char* configFileName)
{
    struct Scanner scanner;
    Scanner_InitString(&scanner, "name", input);
    if (configFileName != NULL)
    {
        char configFullPath[CPRIME_MAX_PATH] = { 0 };
        GetFullPathS(configFileName, configFullPath);
        Scanner_IncludeFile(&scanner, configFullPath, FileIncludeTypeFullPath,
                            true);
        Scanner_Match(&scanner);
    }
    PrintPreprocessedToStringCore2(fp, &scanner);
    Scanner_Destroy(&scanner);
}

void PrintPreprocessedToConsole(const char* fileIn,
                                const char* configFileName)
{
    char fullFileNamePath[CPRIME_MAX_PATH] = { 0 };
    GetFullPathS(fileIn, fullFileNamePath);
    struct Scanner scanner;
    Scanner_Init(&scanner);
    Scanner_IncludeFile(&scanner, fullFileNamePath, FileIncludeTypeFullPath,
                        false);
    if (configFileName != NULL)
    {
        char configFullPath[CPRIME_MAX_PATH] = { 0 };
        GetFullPathS(configFileName, configFullPath);
        Scanner_IncludeFile(&scanner, configFullPath, FileIncludeTypeFullPath,
                            true);
        Scanner_Match(&scanner);
    }
    PrintPreprocessedToFileCore(stdout, &scanner);
    Scanner_Destroy(&scanner);
}

int Scanner_GetNumberOfScannerItems(struct Scanner* pScanner)
{
    int nCount = 1; // contando com o "normal"
    for (struct Token* pItem = (&pScanner->AcumulatedTokens)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        nCount++;
    }
    return nCount;
}

struct Token* Scanner_ScannerItemAt(struct Scanner* pScanner, int index)
{
    // item0 item1 ..itemN
    //^
    // posicao atual
    struct Token* pScannerItem = NULL;
    if (!pScanner->bError)
    {
        // conta o numero de itens empilhados
        int nCount = 0;
        for (struct Token* pItem = (&pScanner->AcumulatedTokens)->pHead; pItem != NULL; pItem = pItem->pNext)
        {
            nCount++;
        }
        // precisa comprar tokens?
        if (index >= nCount)
        {
            for (int i = nCount; i <= index; i++)
            {
                // comprar mais tokens
                Scanner_BuyTokens(pScanner);
            }
            pScannerItem = pScanner->AcumulatedTokens.pTail;
        }
        else
        {
            // nao precisa comprar eh so pegar
            int n = 0;
            for (struct Token* pItem = (&pScanner->AcumulatedTokens)->pHead; pItem != NULL; pItem = pItem->pNext)
            {
                if (n == index)
                {
                    pScannerItem = pItem;
                    break;
                }
                n++;
            }
        }
    }
    return pScannerItem;
}

void Scanner_PrintItems(struct Scanner* pScanner)
{
    printf("-----\n");
    int n = Scanner_GetNumberOfScannerItems(pScanner);
    for (int i = 0; i < n; i++)
    {
        struct Token* pItem = Scanner_ScannerItemAt(pScanner, i);
        printf("%d : %s %s\n", i, pItem->lexeme.c_str, TokenToString(pItem->token));
    }
    printf("-----\n");
}

int Scanner_FileIndexAt(struct Scanner* pScanner, int index)
{
    struct Token* pScannerItem = Scanner_ScannerItemAt(pScanner, index);
    if (pScannerItem)
    {
        return pScannerItem->FileIndex;
    }
    return 0;
}

int Scanner_LineAt(struct Scanner* pScanner, int index)
{
    struct Token* pScannerItem = Scanner_ScannerItemAt(pScanner, index);
    if (pScannerItem)
    {
        return pScannerItem->Line;
    }
    return 0;
}

bool Scanner_IsActiveAt(struct Scanner* pScanner, int index)
{
    struct Token* pScannerItem = Scanner_ScannerItemAt(pScanner, index);
    if (pScannerItem)
    {
        return pScannerItem->bActive;
    }
    return false;
}

enum TokenType Scanner_TokenAt(struct Scanner* pScanner, int index)
{
    struct Token* pScannerItem = Scanner_ScannerItemAt(pScanner, index);
    if (pScannerItem)
    {
        return pScannerItem->token;
    }
    return TK_EOF;
}

const char* Scanner_LexemeAt(struct Scanner* pScanner, int index)
{
    struct Token* pScannerItem = Scanner_ScannerItemAt(pScanner, index);
    if (pScannerItem)
    {
        return pScannerItem->lexeme.c_str;
    }
    return "";
}


void Scanner_MatchDontExpand(struct Scanner* pScanner)
{
    if (!pScanner->bError)
    {
        if (pScanner->AcumulatedTokens.pHead != NULL)
        {
            TokenList_PopFront(&pScanner->AcumulatedTokens);
        }
        else
        {
            struct BasicScanner* pTopScanner = Scanner_Top(pScanner);
            if (pTopScanner == NULL)
            {
                return;
            }
            BasicScanner_Match(pTopScanner);
            enum TokenType token = pTopScanner->token;
            while (token == TK_EOF && pScanner->stack.pTop->pPrevious != NULL)
            {
                //assert(pScanner->AcumulatedTokens.pHead == NULL);
                BasicScannerStack_PopIfNotLast(&pScanner->stack);
                pTopScanner = Scanner_Top(pScanner);
                token = pTopScanner->token;
            }
        }
    }
}


void Scanner_MatchGet(struct Scanner* pScanner, struct Token** pp)
{
    if (pScanner->AcumulatedTokens.pHead != NULL)
    {
        *pp = TokenList_PopFrontGet(&pScanner->AcumulatedTokens);
        if (pScanner->AcumulatedTokens.pHead == NULL)
        {
            Scanner_BuyTokens(pScanner);
        }
    }
}

void Scanner_Match(struct Scanner* pScanner)
{
    if (pScanner->AcumulatedTokens.pHead != NULL)
    {
        TokenList_PopFront(&pScanner->AcumulatedTokens);
        if (pScanner->AcumulatedTokens.pHead == NULL)
        {
            Scanner_BuyTokens(pScanner);
        }
    }
}

void BasicScannerStack_Init(struct BasicScannerStack* stack)
{
    stack->pTop = NULL;
}

void BasicScannerStack_Push(struct BasicScannerStack* stack, struct BasicScanner* pItem)
{
    if (stack->pTop == NULL)
    {
        stack->pTop = pItem;
    }
    else
    {
        pItem->pPrevious = stack->pTop;
        stack->pTop = pItem;
    }
}

struct BasicScanner* BasicScannerStack_PopGet(struct BasicScannerStack* stack)
{
    struct BasicScanner* pItem = NULL;
    if (stack->pTop != NULL)
    {
        pItem = stack->pTop;
        stack->pTop = pItem->pPrevious;
    }
    return pItem;
}

void BasicScannerStack_PopIfNotLast(struct BasicScannerStack* stack)
{
    //assert(*stack != NULL);
    if (stack->pTop->pPrevious != NULL)
    {
        BasicScanner_Delete(BasicScannerStack_PopGet(stack));
    }
}

void BasicScannerStack_Pop(struct BasicScannerStack* stack)
{
    BasicScanner_Delete(BasicScannerStack_PopGet(stack));
}

void BasicScannerStack_Destroy(struct BasicScannerStack* stack)
{
    struct BasicScanner* pItem = stack->pTop;
    while (pItem)
    {
        struct BasicScanner* p = pItem;
        pItem = pItem->pPrevious;
        BasicScanner_Delete(p);
    }
}


void TFileMapToStrArray(FileInfoMap* map, struct FileArray* arr)
{
    FileArray_Reserve(arr, map->nCount);
    arr->Size = map->nCount;
    for (int i = 0; i < (int)map->nHashTableSize; i++)
    {
        struct HashMapEntry* data = map->pHashTable[i];
        for (struct HashMapEntry* pCurrent = data;
             pCurrent;
             pCurrent = pCurrent->pNext)
        {
            struct FileInfo* pFile = (struct FileInfo*)pCurrent->pValue;
            if (pFile->FileIndex >= 0 &&
                pFile->FileIndex < (int)arr->Size)
            {
                arr->pItems[pFile->FileIndex] = pFile;
                pCurrent->pValue = NULL; //movido para array
            }
        }
    }
}


/*
retorna somento os tokens visiveis pelo parser
e os outros sao retornados na lista acumulada
*/
enum TokenType FinalMatch(struct Scanner* scanner, struct TokenList* listOpt)
{
    if (listOpt)
    {
        TokenList_Swap(&scanner->PreviousList, listOpt);
    }
    /*
     agora vai puxando os tokens e acumlando em parser->ClueList
     ate que venha um token da fase de parser
    */
    Scanner_Match(scanner);
    struct Token* pToken = Scanner_ScannerItemAt(scanner, 0);
    while (pToken &&
           pToken->token != TK_EOF &&
           pToken->token != TK_NONE &&
           (!pToken->bActive || IsPreprocessorTokenPhase(pToken->token))
           )
    {
        enum TokenType currentTokenType = pToken->token;
        struct Token* pCurrent = 0;
        Scanner_MatchGet(scanner, &pCurrent);
        if (scanner->Level == 0 ||
            (scanner->Level == 1 && currentTokenType == TK_FILE_EOF))
        {
            pCurrent->pNext = 0;
            TokenList_PushBack(&scanner->PreviousList, pCurrent /*movido*/);
            pCurrent = NULL;
        }
        else
        {
            //ok nao eh usado entao eh deletado
            Token_Delete(pCurrent);
        }
        if (currentTokenType == TK_PRE_INCLUDE)
            scanner->Level++;
        else if (currentTokenType == TK_FILE_EOF)
            scanner->Level--;
        pToken = Scanner_ScannerItemAt(scanner, 0);
    }
    return pToken ? pToken->token : TK_ERROR;
}


void PPTokenArray_Reserve(struct PPTokenArray* p, int nelements)
{
    if (nelements > p->Capacity)
    {
        struct PPToken** pnew = p->pItems;
        pnew = (struct PPToken**)realloc(pnew, nelements * sizeof(struct PPToken*));
        if (pnew)
        {
            p->pItems = pnew;
            p->Capacity = nelements;
        }
    }
}

struct PPToken* PPTokenArray_PopFront(struct PPTokenArray* p)
{
    struct PPToken* pItem = NULL;
    if (p->Size > 0)
    {
        pItem = p->pItems[0];
        if (p->Size > 1)
        {
            memmove(p->pItems, p->pItems + 1, sizeof(void*) * (p->Size - 1));
        }
        p->Size--;
    }
    return pItem;
}

void PPTokenArray_Pop(struct PPTokenArray* p)
{
    void* pItem = 0;
    if (p->Size > 0)
    {
        pItem = p->pItems[p->Size - 1];
        p->pItems[p->Size - 1] = NULL;
        p->Size--;
    }
    PPToken_Delete(pItem);
}

void PPTokenArray_PushBack(struct PPTokenArray* p, struct PPToken* pItem)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        PPTokenArray_Reserve(p, n);
    }
    p->pItems[p->Size] = pItem;
    p->Size++;
}

void PPTokenArray_Clear(struct PPTokenArray* p)
{
    for (int i = 0; i < p->Size; i++)
    {
        PPToken_Delete(p->pItems[i]);
    }
    free(p->pItems);
    p->pItems = NULL;
    p->Size = 0;
    p->Capacity = 0;
}


void PPTokenArray_Destroy(struct PPTokenArray* st)
{
    for (int i = 0; i < st->Size; i++)
    {
        PPToken_Delete(st->pItems[i]);
    }
    free((void*)st->pItems);
}

void PPTokenArray_Swap(struct PPTokenArray* p1, struct PPTokenArray* p2)
{
    struct PPTokenArray temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

void PPTokenArray_Delete(struct PPTokenArray* st)
{
    if (st != NULL)
    {
        PPTokenArray_Destroy(st);
        free((void*)st);
    }
}


void PPTokenArray_AppendTokensCopy(struct PPTokenArray* pArray, struct PPToken** pToken, int len)
{
    for (int i = 0; i < len; i++)
    {
        PPTokenArray_PushBack(pArray, PPToken_Clone(pToken[i]));
    }
}
void PPTokenArray_AppendTokensMove(struct PPTokenArray* pArray, struct PPToken** pToken, int len)
{
    for (int i = 0; i < len; i++)
    {
        PPTokenArray_PushBack(pArray, pToken[i]);
        pToken[i] = NULL;
    }
}

void PPTokenArray_AppendCopy(struct PPTokenArray* pArrayTo, const struct PPTokenArray* pArrayFrom)
{
    for (int i = 0; i < pArrayFrom->Size; i++)
    {
        PPTokenArray_PushBack(pArrayTo, PPToken_Clone(pArrayFrom->pItems[i]));
    }
}

void PPTokenArray_AppendMove(struct PPTokenArray* pArrayTo, struct PPTokenArray* pArrayFrom)
{
    for (int i = 0; i < pArrayFrom->Size; i++)
    {
        PPTokenArray_PushBack(pArrayTo, pArrayFrom->pItems[i]);
        pArrayFrom->pItems[i] = NULL;
    }
}

struct PPToken* PPTokenArray_Find(const struct PPTokenArray* pArray, const char* lexeme)
{
    struct PPToken* pFound = NULL;
    for (int i = 0; i < pArray->Size; i++)
    {
        if (strcmp(lexeme, pArray->pItems[i]->Lexeme) == 0)
        {
            pFound = pArray->pItems[i];
            break;
        }
    }
    return pFound;
}

void PPTokenArray_Print(const struct PPTokenArray* tokens)
{
    if (tokens->Size == 0)
    {
        //printf("(empty)");
    }
    //for (int i = 0; i < tokens->Size; i++)
    //{
    //printf(" '%s' ", tokens->pItems[i]->Lexeme);
    //}
    //printf("\n");
}


void PPTokenArray_Erase(struct PPTokenArray* pArray, int begin, int end)
{
    for (int i = begin; i < end; i++)
    {
        PPToken_Delete(pArray->pItems[i]);
    }
    if (pArray->Size > 1)
    {
        memmove(pArray->pItems + begin,
                pArray->pItems + end,
                sizeof(void*) * (pArray->Size - end));
    }
    pArray->Size = pArray->Size - end;
}


int TokenArrayMap_SetAt(struct TokenArrayMap* pMap,
                        const char* Key,
                        struct PPTokenArray* newValue)
{
    void* pPrevious;
    int r = HashMap_SetAt((struct HashMap*)pMap, Key, newValue, &pPrevious);
    PPTokenArray_Delete((struct PPTokenArray*)pPrevious);
    return r;
}

bool TokenArrayMap_Lookup(const struct TokenArrayMap* pMap,
                          const char* Key,
                          struct PPTokenArray** rValue)
{
    if (pMap == NULL)
    {
        return false;
    }
    return HashMap_Lookup((struct HashMap*)pMap,
                          Key,
                          (void**)rValue);
}

bool TokenArrayMap_RemoveKey(struct TokenArrayMap* pMap, const char* Key)
{
    struct PPTokenArray* pItem;
    bool r = HashMap_RemoveKey((struct HashMap*)pMap, Key, (void**)&pItem);
    if (r)
    {
        PPTokenArray_Delete(pItem);
    }
    return r;
}


static void PPTokenArray_DeleteVoid(void* p)
{
    PPTokenArray_Delete((struct PPTokenArray*)p);
}

void TokenArrayMap_Destroy(struct TokenArrayMap* p)
{
    HashMap_Destroy((struct HashMap*)p, &PPTokenArray_DeleteVoid);
}



void TokenArrayMap_Swap(struct TokenArrayMap* pA, struct TokenArrayMap* pB)
{
    struct TokenArrayMap t = TOKENARRAYMAP_INIT;
    *pA = *pB;
    *pB = t;
}

static void PPTokenSet_Push(struct PPTokenSet* p, struct PPToken* pItem)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        struct PPToken** pnew = p->pItems;
        pnew = (struct PPToken**)realloc(pnew, n * sizeof(struct PPToken*));
        if (pnew)
        {
            p->pItems = pnew;
            p->Capacity = n;
        }
    }
    p->pItems[p->Size] = pItem;
    p->Size++;
}

void PPTokenSet_PushUnique(struct PPTokenSet* p, struct PPToken* pItem) /*custom*/
{
    struct PPToken* pTk = PPTokenSet_Find(p, pItem->Lexeme);
    if (pTk == NULL)
    {
        PPTokenSet_Push(p, pItem);
    }
    else
    {
        PPToken_Delete(pItem);
    }
}


void TokenSetAppendCopy(struct PPTokenSet* pArrayTo, const struct PPTokenSet* pArrayFrom)
{
    for (int i = 0; i < pArrayFrom->Size; i++)
    {
        PPTokenSet_PushUnique(pArrayTo, PPToken_Clone(pArrayFrom->pItems[i]));
    }
}


struct PPToken* PPTokenSet_Find(const struct PPTokenSet* pArray, const char* lexeme)
{
    struct PPToken* pFound = NULL;
    for (int i = 0; i < pArray->Size; i++)
    {
        if (strcmp(lexeme, pArray->pItems[i]->Lexeme) == 0)
        {
            pFound = pArray->pItems[i];
            break;
        }
    }
    return pFound;
}


void PPTokenSet_Destroy(struct PPTokenSet* pArray)
{
    for (int i = 0; i < pArray->Size; i++)
    {
        PPToken_Delete(pArray->pItems[i]);
    }
    free((void*)pArray->pItems);
}

void SetIntersection(const struct PPTokenSet* p1,
                     const struct PPTokenSet* p2,
                     struct PPTokenSet* pResult)
{
    if (p1->Size != 0 && p2->Size != 0)
    {
        struct PPToken* first1 = p1->pItems[0];
        struct PPToken* last1 = p1->pItems[p1->Size];
        struct PPToken* first2 = p2->pItems[0];
        struct PPToken* last2 = p2->pItems[p2->Size];
        while (first1 != last1 && first2 != last2)
        {
            //if (comp(*first1, *first2))
            if (strcmp(first1->Lexeme, first2->Lexeme) == 0)
            {
                ++first1;
            }
            else
            {
                //if (!comp(*first2, *first1))
                if (strcmp(first2->Lexeme, first1->Lexeme) != 0)
                {
                    //*d_first++ = *first1++;
                    PPTokenSet_PushUnique(pResult, PPToken_Clone(first1));
                    first1++;
                    //*d_first++ = *first1++;
                    //d_first
                }
                ++first2;
            }
        }
    }
    else if (p1->Size == 0)
    {
        TokenSetAppendCopy(pResult, p1);
    }
    else if (p2->Size == 0)
    {
        TokenSetAppendCopy(pResult, p2);
    }
}

/*
http://en.cppreference.com/w/cpp/algorithm/set_intersection
template<class InputIt1, class InputIt2,
class OutputIt, class Compare>
OutputIt set_intersection(InputIt1 first1, InputIt1 last1,
InputIt2 first2, InputIt2 last2,
OutputIt d_first, Compare comp)
{
while (first1 != last1 && first2 != last2) {
if (comp(*first1, *first2)) {
++first1;
} else {
if (!comp(*first2, *first1)) {
*d_first++ = *first1++;
}
++first2;
}
}
return d_first;
}
*/


void OutputOptions_Destroy(struct OutputOptions* options)
{
    options;
}




void LocalStrBuilder_Init(struct LocalStrBuilder* p)
{
    p->capacity = LOCALSTRBUILDER_NCHARS;
    p->size = 0;
    p->c_str = p->chars;
}

void LocalStrBuilder_Swap(struct LocalStrBuilder* pA, struct LocalStrBuilder* pB)
{
    int bA = (pA->c_str == pA->chars);
    int bB = (pB->c_str == pB->chars);
    struct LocalStrBuilder temp = *pA;
    *pA = *pB;
    *pB = temp;
    if (bA)
    {
        pB->c_str = pB->chars;
    }
    if (bB)
    {
        pA->c_str = pA->chars;
    }
}

void LocalStrBuilder_Destroy(struct LocalStrBuilder* p)
{
    if (p->c_str != p->chars)
    {
        free(p->c_str);
    }
}

void LocalStrBuilder_Reserve(struct LocalStrBuilder* p, int nelements)
{
    if (nelements > p->capacity)
    {
        char* pnew = NULL;
        if (nelements <= LOCALSTRBUILDER_NCHARS)
        {
            pnew = p->chars;
            p->capacity = LOCALSTRBUILDER_NCHARS;
            p->c_str = pnew;
        }
        else
        {
            if (p->capacity <= LOCALSTRBUILDER_NCHARS)
            {
                pnew = (char*)malloc((nelements + 1) * sizeof(char));
                memcpy(pnew, p->chars, LOCALSTRBUILDER_NCHARS);
            }
            else
            {
                pnew = (char*)realloc(p->c_str, (nelements + 1) * sizeof(char));
            }
            p->c_str = pnew;
            if (p->size == 0)
            {
                pnew[0] = '\0';
            }
            p->capacity = nelements;
        }
    }
}

void LocalStrBuilder_Print(struct LocalStrBuilder* p)
{
    printf("size = %d, capacity = %d, c_str = '%s', internal buffer = %s \n",
           (int)p->size,
           (int)p->capacity,
           p->c_str,
           (p->c_str == p->chars ? "yes" : "no"));
}

void LocalStrBuilder_Clear(struct LocalStrBuilder* p)
{
    if (p->c_str)
    {
        p->c_str[0] = 0;
    }
    p->size = 0;
}

void LocalStrBuilder_Grow(struct LocalStrBuilder* p, int nelements)
{
    if (nelements > p->capacity)
    {
        int new_nelements = p->capacity + p->capacity / 2;
        if (new_nelements < nelements)
        {
            new_nelements = nelements;
        }
        LocalStrBuilder_Reserve(p, new_nelements);
    }
}

void LocalStrBuilder_Append(struct LocalStrBuilder* p, const char* source)
{
    while (*source)
    {
        LocalStrBuilder_AppendChar(p, *source);
        source++;
    }
}


void LocalStrBuilder_Set(struct LocalStrBuilder* p, const char* source)
{
    LocalStrBuilder_Clear(p);
    if (source)
    {
        while (*source)
        {
            LocalStrBuilder_AppendChar(p, *source);
            source++;
        }
    }
}



void LocalStrBuilder_AppendChar(struct LocalStrBuilder* p, char ch)
{
    LocalStrBuilder_Grow(p, p->size + 1);
    p->c_str[p->size] = ch;
    p->c_str[p->size + 1] = 0;
    p->size++;
}




struct StructUnionSpecifier* FindStructUnionSpecifierByName(struct SyntaxTree* pSyntaxTree,
    const char* structTagNameOrTypedef);


void Parameter_PrintNameMangling(struct Parameter* p,
                                 struct StrBuilder* fp);

static void IntanciateTypeIfNecessary(struct SyntaxTree* pSyntaxTree,
                                      struct PrintCodeOptions* options,
                                      struct DeclarationSpecifiers* pSpecifiers,
                                      char* buffer,
                                      int bufferSize);

void UnionTypeDefault(struct SyntaxTree* pSyntaxTree,
                      struct PrintCodeOptions* options,
                      struct StructUnionSpecifier* pStructUnionSpecifier,
                      struct ParameterTypeList* pArgsOpt, //parametros
                      const char* parameterName,
                      const char* functionSuffix,
                      struct StrBuilder* sbImplementation,
                      struct StrBuilder* sbDeclarations);

void InstanciateNew(struct SyntaxTree* pSyntaxTree,
                    struct PrintCodeOptions* options,
                    struct SpecifierQualifierList* pSpecifierQualifierList,//<-dupla para entender o tipo
                    struct Declarator* pDeclatator,                        //<-dupla para entender o tipo
                    const char* pInitExpressionText,
                    bool bPrintStatementEnd,
                    struct StrBuilder* fp);

void InstanciateVectorPush(struct SyntaxTree* pSyntaxTree,
                           struct PrintCodeOptions* options,
                           struct DeclarationSpecifiers* pDeclarationSpecifiers,
                           struct StrBuilder* fp);



bool InstantiateInitForStruct(struct SyntaxTree* pSyntaxTree,
                              struct StructUnionSpecifier* pStructUnionSpecifier,
                              struct PrintCodeOptions* options,
                              struct StrBuilder* fp);

bool InstantiateDestroyForStruct(struct SyntaxTree* pSyntaxTree,
                                 struct StructUnionSpecifier* pStructUnionSpecifier,
                                 struct PrintCodeOptions* options,
                                 struct StrBuilder* fp,
                                 struct StrBuilder* strDeclarations);
bool InstantiateDeleteForStruct(struct SyntaxTree* pSyntaxTree,
                                struct StructUnionSpecifier* pStructUnionSpecifier,
                                struct PrintCodeOptions* options,
                                struct StrBuilder* fp,
                                struct StrBuilder* decl);

void TExpression_CodePrintSpaces(struct SyntaxTree* pSyntaxTree,
                                 struct PrintCodeOptions* options,
                                 struct Expression* p,
                                 struct StrBuilder* fp);

void InstantiateNew(struct PrintCodeOptions* options, struct SyntaxTree* pSyntaxTree, struct PostfixExpression* p, struct StrBuilder* fp);

void InstanciateDestroy(struct SyntaxTree* pSyntaxTree,
                        struct PrintCodeOptions* options,
                        int identation,
                        struct SpecifierQualifierList* pSpecifierQualifierList,//<-dupla para entender o tipo
                        struct Declarator* pDeclatator,                        //<-dupla para entender o tipo
                        const char* pInitExpressionText,
                        bool bPrintStatementEnd,
                        struct StrBuilder* fp);

void InstanciateInit(struct SyntaxTree* pSyntaxTree,
                     struct PrintCodeOptions* options,
                     struct SpecifierQualifierList* pSpecifierQualifierList,//<-dupla para entender o tipo
                     struct Declarator* pDeclatator,                        //<-dupla para entender o tipo
                     struct Initializer* pInitializerOpt, //usado para inicializacao estatica
                     const char* pInitExpressionText, //(x->p->i = 0)
                     bool* pbHasInitializers,
                     struct StrBuilder* fp);

#define List_IsFirstItem(pList, pItem) ((pList)->pHead == (pItem))


void IntegerStack_PushBack(struct IntegerStack* pItems, int i)
{
    if (pItems->Size + 1 > pItems->Capacity)
    {
        int n = pItems->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        int* pnew = pItems->pData;
        pnew = (int*)realloc(pnew, n * sizeof(int));
        if (pnew)
        {
            pItems->pData = pnew;
            pItems->Capacity = n;
        }
    }
    pItems->pData[pItems->Size] = i;
    pItems->Size++;
}

void IntegerStack_Pop(struct IntegerStack* pItems)
{
    if (pItems->Size > 0)
        pItems->Size--;
    //else
    //assert(false);
}



void IntegerStack_Destroy(struct IntegerStack* pItems)
{
    free((void*)pItems->pData);
}

static int global_lambda_counter = 0;
static int try_statement_index = 0;


void PrintCodeOptions_Destroy(struct PrintCodeOptions* options)
{
    OutputOptions_Destroy(&options->Options);
    IntegerStack_Destroy(&options->Stack);
    StrBuilder_Destroy(&options->sbPreDeclaration);
    StrBuilder_Destroy(&options->sbInstantiations);
    StrBuilder_Destroy(&options->sbDeferGlobal);
    StrBuilder_Destroy(&options->sbDeferLocal);
    StrBuilder_Destroy(&options->returnType);
    HashMap_Destroy(&options->instantiations, NULL);
}


void TSpecifierQualifierList_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct SpecifierQualifierList* pDeclarationSpecifiers, struct StrBuilder* fp);

void TTypeName_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct TypeName* p, struct StrBuilder* fp);

static void TInitializer_CodePrint(struct SyntaxTree* pSyntaxTree,
                                   struct PrintCodeOptions* options,
                                   struct Declarator* pDeclarator,
                                   struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                   struct Initializer* pTInitializer,

                                   struct StrBuilder* fp);

static void TInitializerList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                       struct PrintCodeOptions* options,
                                       struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                       struct Declarator* pDeclarator,
                                       struct InitializerList* p,

                                       struct StrBuilder* fp);


static void TInitializerListItem_CodePrint(struct SyntaxTree* pSyntaxTree,
                                           struct PrintCodeOptions* options,
                                           struct Declarator* pDeclarator,
                                           struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                           struct InitializerListItem* p,

                                           struct StrBuilder* fp);


struct TypeQualifier* TTypeQualifier_Clone(struct TypeQualifier* p);
bool TypeQualifier_Compare(struct TypeQualifier* p1, struct TypeQualifier* p2);

#define TYPEQUALIFIERLIST_INIT  {0}

void TypeQualifierList_Destroy(struct TypeQualifierList* p);
void TypeQualifierList_PushBack(struct TypeQualifierList* p, struct TypeQualifier* pItem);
void TypeQualifierList_CopyFrom(struct TypeQualifierList* dest, struct TypeQualifierList* src);
void TypeQualifier_Delete(struct TypeQualifier* p);

static void TTypeQualifierList_CodePrint(struct PrintCodeOptions* options, struct TypeQualifierList* p, struct StrBuilder* fp);

static void TAnyDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct AnyDeclaration* pDeclaration, struct StrBuilder* fp);

CAST(AnyStructDeclaration, StructDeclaration)
CAST(AnyStructDeclaration, StaticAssertDeclaration)
CAST(AnyStructDeclaration, EofDeclaration)

static void TAnyStructDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct AnyStructDeclaration* p, struct StrBuilder* fp);
static void TTypeQualifier_CodePrint(struct PrintCodeOptions* options, struct TypeQualifier* p, struct StrBuilder* fp);
static void TDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct Declaration* p, struct StrBuilder* fp);
static void TExpression_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct Expression* p, struct StrBuilder* fp);
static void TStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct Statement* p, struct StrBuilder* fp);
static void TBlockItem_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct BlockItem* p, struct StrBuilder* fp);

static void TPointer_CodePrint(struct PrintCodeOptions* options, struct Pointer* pPointer, struct StrBuilder* fp);
static void TParameter_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct Parameter* p, struct StrBuilder* fp);

bool IsActive(struct PrintCodeOptions* options)
{
    bool bResult = false;
    if (options->bInclude)
    {
        if (options->Stack.Size == 0 ||
            options->Stack.pData[options->Stack.Size - 1] == 1)
        {
            bResult = true;
        }
    }
    return bResult;
}

void Output_Append(struct StrBuilder* p,
                   struct PrintCodeOptions* options,
                   const char* source)
{
    if (IsActive(options))
    {
        StrBuilder_Append(p, source);
    }
    else
    {
        //nao eh p include
    }
}

static void TNodeClueList_CodePrint(struct PrintCodeOptions* options, struct TokenList* list,
                                    struct StrBuilder* fp)
{
    if (options->Options.bCannonical)
    {
        // no modo cannonical quem coloca os espacos
        //eh a funcao especializada
        //para que o tipo seja somente ele
        return;
    };
    for (struct Token* pNodeClue = list->pHead;
         pNodeClue != NULL;
         pNodeClue = pNodeClue->pNext)
    {
        switch (pNodeClue->token)
        {
            case TK_PRE_INCLUDE:
            {
                bool bIncludeFile = true;
                if (options->Stack.Size > 0 &&
                    options->Stack.pData[options->Stack.Size - 1] == 0)
                {
                    bIncludeFile = false;
                }
                else
                {
                    bIncludeFile = false;
                }
                if (bIncludeFile)
                {
                    IntegerStack_PushBack(&options->Stack, bIncludeFile);
                }
                else
                {
                    Output_Append(fp, options, pNodeClue->lexeme.c_str);
                    Output_Append(fp, options, "\n");
                    IntegerStack_PushBack(&options->Stack, bIncludeFile);
                }
            }
            break;
            case TK_FILE_EOF:
                IntegerStack_Pop(&options->Stack);
                //options->IncludeLevel--;
                ////assert(IncludeLevel > 0);
                //bInclude = true;
                break;
            case TK_PRE_DEFINE:
                //TODO gerar macros como init
                Output_Append(fp, options, pNodeClue->lexeme.c_str);
                Output_Append(fp, options, "\n");
                break;
            case TK_PRE_PRAGMA:
                if (strcmp(pNodeClue->lexeme.c_str, "#pragma expand on") == 0)
                {
                    options->Options.bExpandMacros = true;
                }
                else if (strcmp(pNodeClue->lexeme.c_str, "#pragma expand off") == 0)
                {
                    options->Options.bExpandMacros = false;
                }
                else
                {
                    Output_Append(fp, options, pNodeClue->lexeme.c_str);
                }
                Output_Append(fp, options, "\n");
                break;
            case TK_PRE_UNDEF:
            case TK_PRE_IF:
            case TK_PRE_ENDIF:
            case TK_PRE_ELSE:
            case TK_PRE_IFDEF:
            case TK_PRE_IFNDEF:
            case TK_PRE_ELIF:
                Output_Append(fp, options, pNodeClue->lexeme.c_str);
                Output_Append(fp, options, "\n");
                break;
            case TK_OPEN_COMMENT:
            case TK_CLOSE_COMMENT:
                //Output_Append(fp, options, pNodeClue->lexeme.c_str);
                break;
            case TK_COMMENT:
                if (options->Options.bIncludeComments)
                {
                    Output_Append(fp, options, pNodeClue->lexeme.c_str);
                }
                else
                {
                    Output_Append(fp, options, " ");
                }
                break;
            case TK_LINE_COMMENT:
                if (options->Options.bIncludeComments)
                {
                    Output_Append(fp, options, pNodeClue->lexeme.c_str);
                }
                else
                {
                }
                break;
            case TK_BREAKLINE:
                Output_Append(fp, options, "\n");
                break;
            case TK_MACRO_CALL:
                if (options->Options.bExpandMacros)
                {
                }
                else
                {
                    //if (!strstr(pNodeClue->lexeme.c_str, "ForEachListItem"))
                    //{
                    Output_Append(fp, options, pNodeClue->lexeme.c_str);
                    options->bInclude = false;
                    //}
                }
                break;
            case TK_MACRO_EOF:
                if (options->Options.bExpandMacros)
                {
                }
                else
                {
                    options->bInclude = true;
                }
                break;
            case TK_SPACES:
                Output_Append(fp, options, pNodeClue->lexeme.c_str);
                break;
                //case NodeClueTypeNone:
            default:
                Output_Append(fp, options, pNodeClue->lexeme.c_str);
                break;
        }
    }
}

static void TCompoundStatement_CodePrint(struct SyntaxTree* pSyntaxTree,
                                         struct PrintCodeOptions* options,
                                         struct CompoundStatement* p,
                                         struct StrBuilder* fp)
{

    /*faz uma copia do deferlocal do escopo de cima*/
    struct StrBuilder  sbDeferLocalCopy = { 0 };
    StrBuilder_Swap(&sbDeferLocalCopy, &options->sbDeferLocal);

    struct StrBuilder  sbDeferGlobalCopy = { 0 };
    StrBuilder_Set(&sbDeferGlobalCopy, options->sbDeferGlobal.c_str);

    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "{");

    for (int j = 0; j < p->BlockItemList.size; j++)
    {
        struct BlockItem* pBlockItem = p->BlockItemList.data[j];
        TBlockItem_CodePrint(pSyntaxTree, options, pBlockItem, fp);
    }



    /*na saida normal imprime defer local*/
    Output_Append(fp, options, options->sbDeferLocal.c_str);

    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, "}");

    /*restaura escopo local de cima*/
    StrBuilder_Swap(&sbDeferLocalCopy, &options->sbDeferLocal);
    StrBuilder_Destroy(&sbDeferLocalCopy);

    StrBuilder_Swap(&sbDeferGlobalCopy, &options->sbDeferGlobal);
    StrBuilder_Destroy(&sbDeferGlobalCopy);


    //restaura copia
    //StrBuilder_Set(&options->sbDeferGlobal, copy.c_str);

    //
    //StrBuilder_Swap(&deferLocalCopy, &options->sbDeferLocal);
}


static void TLabeledStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct LabeledStatement* p, struct StrBuilder* fp)
{
    if (p->token == TK_CASE)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, "case");
        if (p->pExpression)
        {
            TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
        }
        else
        {
            //assert(false);
        }
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, ":");
        TStatement_CodePrint(pSyntaxTree, options, p->pStatementOpt, fp);
    }
    else if (p->token == TK_DEFAULT)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, "default");
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, ":");
        TStatement_CodePrint(pSyntaxTree, options, p->pStatementOpt, fp);
    }
    else if (p->token == TK_IDENTIFIER)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, p->Identifier);
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, ":");
        TStatement_CodePrint(pSyntaxTree, options, p->pStatementOpt, fp);
    }
}

static void TForStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct ForStatement* p, struct StrBuilder* fp)
{
    //StrBuilder_Clear(&options->sbDeferLoop);
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "for");
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, "(");
    if (p->pInitDeclarationOpt)
    {
        TAnyDeclaration_CodePrint(pSyntaxTree, options, p->pInitDeclarationOpt, fp);
        if (p->pExpression2)
        {
            TExpression_CodePrint(pSyntaxTree, options, p->pExpression2, fp);
        }
        TNodeClueList_CodePrint(options, &p->ClueList2, fp);
        Output_Append(fp, options, ";");
        TExpression_CodePrint(pSyntaxTree, options, p->pExpression3, fp);
    }
    else
    {
        TExpression_CodePrint(pSyntaxTree, options, p->pExpression1, fp);
        TNodeClueList_CodePrint(options, &p->ClueList2, fp);
        Output_Append(fp, options, ";");
        TExpression_CodePrint(pSyntaxTree, options, p->pExpression2, fp);
        TNodeClueList_CodePrint(options, &p->ClueList3, fp);
        Output_Append(fp, options, ";");
        TExpression_CodePrint(pSyntaxTree, options, p->pExpression3, fp);
    }
    TNodeClueList_CodePrint(options, &p->ClueList4, fp);
    Output_Append(fp, options, ")");
    TStatement_CodePrint(pSyntaxTree, options, p->pStatement, fp);
}


static void TWhileStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct WhileStatement* p, struct StrBuilder* fp)
{
    //StrBuilder_Clear(&options->sbDeferLoop);
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "while");
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, "(");
    TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
    TNodeClueList_CodePrint(options, &p->ClueList2, fp);
    Output_Append(fp, options, ")");
    TStatement_CodePrint(pSyntaxTree, options, p->pStatement, fp);
}



static void TDoStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct DoStatement* p, struct StrBuilder* fp)
{
    //StrBuilder_Clear(&options->sbDeferLoop);
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "do");
    TStatement_CodePrint(pSyntaxTree, options, p->pStatement, fp);
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    if (p->pExpression != NULL)
    {
        Output_Append(fp, options, "while");
        TNodeClueList_CodePrint(options, &p->ClueList2, fp);
        Output_Append(fp, options, "(");
        TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
        TNodeClueList_CodePrint(options, &p->ClueList3, fp);
        Output_Append(fp, options, ")");
        TNodeClueList_CodePrint(options, &p->ClueList4, fp);
        Output_Append(fp, options, ";");
    }
    else
    {
        Output_Append(fp, options, " while(0);");
    }
}

static void TTryBlockStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct TryBlockStatement* p, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueListTry, fp);

    struct TryBlockStatement* pOld = options->pCurrentTryBlock;
    options->pCurrentTryBlock = p;

    try_statement_index++;
    char indestrs[20];
    snprintf(indestrs, sizeof(indestrs), "%d", try_statement_index);

    TNodeClueList_CodePrint(options, &p->ClueListTry, fp);

    Output_Append(fp, options, "if (1) /*try*/");



    TCompoundStatement_CodePrint(pSyntaxTree, options, p->pCompoundStatement, fp);

    if (p->pCompoundCatchStatement)
    {
        TNodeClueList_CodePrint(options, &p->ClueListCatch, fp);
        TNodeClueList_CodePrint(options, &p->ClueListLeftPar, fp);
        TNodeClueList_CodePrint(options, &p->ClueListRightPar, fp);

        Output_Append(fp, options, "else /*catch*/ _catch_label");
        Output_Append(fp, options, indestrs);
        Output_Append(fp, options, ":");

        TCompoundStatement_CodePrint(pSyntaxTree, options, p->pCompoundCatchStatement, fp);
    }
    else
    {
        Output_Append(fp, options, "_catch_label");
        Output_Append(fp, options, indestrs);
        Output_Append(fp, options, ":;");
    }

    //Output_Append(fp, options, "} /*end try-block*/");

    try_statement_index--;
    options->pCurrentTryBlock = pOld;
}



static void TExpressionStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct ExpressionStatement* p, struct StrBuilder* fp)
{
    TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, ";");
}

static void TDeclarator_CodePrint(struct SyntaxTree* pSyntaxTree,
                                  struct PrintCodeOptions* options,
                                  struct Declarator* p,
                                  struct StrBuilder* fp);

#define JUMPSTATEMENT_INIT {JumpStatement_ID}
void JumpStatement_Delete(struct JumpStatement* p);

static void TDirectDeclarator_CodePrint(struct SyntaxTree* pSyntaxTree,
                                        struct PrintCodeOptions* options,
                                        struct DirectDeclarator* pDirectDeclarator,
                                        struct StrBuilder* fp);

static void DeferStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct DeferStatement* p, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);

    struct StrBuilder  sb = { 0 };
    TStatement_CodePrint(pSyntaxTree, options, p->pStatement, &sb);
    StrBuilder_Append(&sb, options->sbDeferLocal.c_str);
    StrBuilder_Swap(&options->sbDeferLocal, &sb);
    StrBuilder_Destroy(&sb);

    struct StrBuilder  sb2 = { 0 };
    TStatement_CodePrint(pSyntaxTree, options, p->pStatement, &sb2);
    StrBuilder_Append(&sb2, options->sbDeferGlobal.c_str);
    StrBuilder_Swap(&options->sbDeferGlobal, &sb2);
    StrBuilder_Destroy(&sb2);

}

static void TJumpStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct JumpStatement* p, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    switch (p->token)
    {
        case TK_GOTO:
            Output_Append(fp, options, "goto");
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            Output_Append(fp, options, p->Identifier);
            break;
        case  TK_CONTINUE:
            //Output_Append(fp, options, options->sbDeferLoop.c_str);
            Output_Append(fp, options, "continue");
            break;
        case TK_BREAK:
            //Output_Append(fp, options, options->sbDeferLoop.c_str);
            Output_Append(fp, options, "break");
            break;
        case TK_THROW:
        {
            if (options->pCurrentTryBlock)
            {
                Output_Append(fp, options, "/*throw*/");

                char try_statement_index_string[20];
                snprintf(try_statement_index_string, sizeof(try_statement_index_string), "%d", try_statement_index);
                if (options->sbDeferGlobal.size > 0)
                {
                    /*neste caso adicionamos um {*/
                    Output_Append(fp, options, "{");
                }
                Output_Append(fp, options, options->sbDeferGlobal.c_str);
                Output_Append(fp, options, " goto _catch_label");

                Output_Append(fp, options, try_statement_index_string);

                if (options->sbDeferGlobal.size > 0)
                {
                    Output_Append(fp, options, ";");
                    Output_Append(fp, options, "}");
                }
            }
            else
            {
                Output_Append(fp, options, "#error throw can be used only inside try-blocks");
            }

        }
        break;

        case TK_RETURN:
            if (p->pExpression)
            {
                if (options->sbDeferGlobal.size != 0)
                {
                    Output_Append(fp, options, " ");
                    Output_Append(fp, options, options->returnType.c_str);
                    Output_Append(fp, options, " _result = ");
                    TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
                    Output_Append(fp, options, ";");
                    Output_Append(fp, options, options->sbDeferGlobal.c_str);
                    Output_Append(fp, options, "return _result");
                }
                else
                {
                    Output_Append(fp, options, "return");
                    TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
                }

            }
            else
            {
                if (options->sbDeferGlobal.size > 0)
                {
                    Output_Append(fp, options, options->sbDeferGlobal.c_str);
                }
                Output_Append(fp, options, "return");
            }
            break;
        default:
            //assert(false);
            break;
    }
    TNodeClueList_CodePrint(options, &p->ClueList2, fp);
    Output_Append(fp, options, ";");
}

#define ASMSTATEMENT_INIT {AsmStatement_ID}
void AsmStatement_Delete(struct AsmStatement* p);

static void TAsmStatement_CodePrint(struct PrintCodeOptions* options, struct AsmStatement* p, struct StrBuilder* fp)
{
    p;
    Output_Append(fp, options, "\"type\":\"asm-statement\"");
}

static void TSwitchStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct SwitchStatement* p, struct StrBuilder* fp)
{
    //StrBuilder_Clear(&options->sbDeferLoop);
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "switch");
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, "(");
    TExpression_CodePrint(pSyntaxTree, options, p->pConditionExpression, fp);
    TNodeClueList_CodePrint(options, &p->ClueList2, fp);
    Output_Append(fp, options, ")");
    TStatement_CodePrint(pSyntaxTree, options, p->pExpression, fp);
}




static void TIfStatement_CodePrint(struct SyntaxTree* pSyntaxTree,
                                   struct PrintCodeOptions* options,
                                   struct IfStatement* p,
                                   struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    if (p->pInitDeclarationOpt || p->pInitialExpression)
    {
        //quantos de espacamento?

        if (p->pInitDeclarationOpt != NULL)
        {
            Output_Append(fp, options, "{");
            TAnyDeclaration_CodePrint(pSyntaxTree, options, p->pInitDeclarationOpt, fp);
        }
        else
        {
            TExpression_CodePrint(pSyntaxTree, options, p->pInitialExpression, fp);
            Output_Append(fp, options, ";");
        }

        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, "if");
        Output_Append(fp, options, "(");
    }
    else
    {
        Output_Append(fp, options, "if");
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, "(");
    }
    TExpression_CodePrint(pSyntaxTree, options, p->pConditionExpression, fp);
    TNodeClueList_CodePrint(options, &p->ClueList4, fp);
    Output_Append(fp, options, ")");

    if (p->pStatement->Type != CompoundStatement_ID)
        Output_Append(fp, options, "");
    if (p->pStatement)
    {
        if (p->pDeferExpression)
        {
            struct StrBuilder  sb2 = { 0 };
            TExpression_CodePrint(pSyntaxTree, options, p->pDeferExpression, &sb2);
            StrBuilder_Append(&sb2, options->sbDeferGlobal.c_str);
            StrBuilder_Append(&sb2, ";");
            StrBuilder_Swap(&options->sbDeferGlobal, &sb2);
            StrBuilder_Destroy(&sb2);


            /*para defer ficar dentro do if (true)*/
            Output_Append(fp, options, "{");
            TStatement_CodePrint(pSyntaxTree, options, p->pStatement, fp);

            TExpression_CodePrint(pSyntaxTree, options, p->pDeferExpression, fp);
            Output_Append(fp, options, ";");
            Output_Append(fp, options, "}");
        }
        else
            TStatement_CodePrint(pSyntaxTree, options, p->pStatement, fp);
    }
    if (p->pElseStatement)
    {
        TNodeClueList_CodePrint(options, &p->ClueList3, fp);
        Output_Append(fp, options, "else");
        TStatement_CodePrint(pSyntaxTree, options, p->pElseStatement, fp);
    }
    if (p->pInitDeclarationOpt)
    {
        Output_Append(fp, options, "}");
    }
}

static void TStatement_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct Statement* p, struct StrBuilder* fp)
{
    if (p == NULL)
    {
        return;
    }
    switch (p->Type)
    {
        case ExpressionStatement_ID:
            TExpressionStatement_CodePrint(pSyntaxTree, options, (struct ExpressionStatement*)p, fp);
            break;
        case SwitchStatement_ID:
            //aqui nao vai
            TSwitchStatement_CodePrint(pSyntaxTree, options, (struct SwitchStatement*)p, fp);
            break;
        case LabeledStatement_ID:
            TLabeledStatement_CodePrint(pSyntaxTree, options, (struct LabeledStatement*)p, fp);
            break;
        case ForStatement_ID:
            TForStatement_CodePrint(pSyntaxTree, options, (struct ForStatement*)p, fp);
            break;
        case DeferStatement_ID:
            DeferStatement_CodePrint(pSyntaxTree, options, (struct DeferStatement*)p, fp);
            break;
        case JumpStatement_ID:
            TJumpStatement_CodePrint(pSyntaxTree, options, (struct JumpStatement*)p, fp);
            break;
        case AsmStatement_ID:
            TAsmStatement_CodePrint(options, (struct AsmStatement*)p, fp);
            break;
        case CompoundStatement_ID:
            TCompoundStatement_CodePrint(pSyntaxTree, options, (struct CompoundStatement*)p, fp);
            break;
        case IfStatement_ID:
            TIfStatement_CodePrint(pSyntaxTree, options, (struct IfStatement*)p, fp);
            break;
        case DoStatement_ID:
            TDoStatement_CodePrint(pSyntaxTree, options, (struct DoStatement*)p, fp);
            break;
        case TryBlockStatement_ID:
            TTryBlockStatement_CodePrint(pSyntaxTree, options, (struct TryBlockStatement*)p, fp);
            break;
        default:
            //assert(false);
            break;
    }
}

static void TStaticAssertDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree,
                                               struct PrintCodeOptions* options,
                                               struct StaticAssertDeclaration* p,
                                               struct StrBuilder* fp);

static void TBlockItem_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct BlockItem* p, struct StrBuilder* fp)
{
    if (p == NULL)
    {
        //assert(false);
        return;
    }
    switch (p->Type)
    {
        case EofDeclaration_ID:
            break;
        case StaticAssertDeclaration_ID:
            TStaticAssertDeclaration_CodePrint(pSyntaxTree, options, (struct StaticAssertDeclaration*)p, fp);
            break;
        case SwitchStatement_ID:
            TSwitchStatement_CodePrint(pSyntaxTree, options, (struct SwitchStatement*)p, fp);
            break;
        case JumpStatement_ID:
            TJumpStatement_CodePrint(pSyntaxTree, options, (struct JumpStatement*)p, fp);
            break;
        case DeferStatement_ID:
            DeferStatement_CodePrint(pSyntaxTree, options, (struct DeferStatement*)p, fp);
            break;
        case ForStatement_ID:
            TForStatement_CodePrint(pSyntaxTree, options, (struct ForStatement*)p, fp);
            break;
        case IfStatement_ID:
            TIfStatement_CodePrint(pSyntaxTree, options, (struct IfStatement*)p, fp);
            break;
        case WhileStatement_ID:
            TWhileStatement_CodePrint(pSyntaxTree, options, (struct WhileStatement*)p, fp);
            break;
        case DoStatement_ID:
            TDoStatement_CodePrint(pSyntaxTree, options, (struct DoStatement*)p, fp);
            break;
        case TryBlockStatement_ID:
            TTryBlockStatement_CodePrint(pSyntaxTree, options, (struct TryBlockStatement*)p, fp);
            break;

        case Declaration_ID:
            TDeclaration_CodePrint(pSyntaxTree, options, (struct Declaration*)p, fp);
            //Output_Append(fp, options,  "\n");
            break;
        case LabeledStatement_ID:
            TLabeledStatement_CodePrint(pSyntaxTree, options, (struct LabeledStatement*)p, fp);
            break;
        case CompoundStatement_ID:
            TCompoundStatement_CodePrint(pSyntaxTree, options, (struct CompoundStatement*)p, fp);
            break;
        case ExpressionStatement_ID:
            TExpressionStatement_CodePrint(pSyntaxTree, options, (struct ExpressionStatement*)p, fp);
            break;
        case AsmStatement_ID:
            TAsmStatement_CodePrint(options, (struct AsmStatement*)p, fp);
            break;
        default:
            //assert(false);
            break;
    }
}



bool GetType(const char* source,
             struct StrBuilder* strBuilderType)
{
    while (*source &&
           *source != '_')
    {
        StrBuilder_AppendChar(strBuilderType, *source);
        source++;
    }
    return *source == '_';
}


bool GetTypeAndFunction(const char* source,
                        struct StrBuilder* strBuilderType,
                        struct StrBuilder* strBuilderFunc)
{
    while (*source &&
           *source != '_')
    {
        StrBuilder_AppendChar(strBuilderType, *source);
        source++;
    }
    while (*source)
    {
        StrBuilder_AppendChar(strBuilderFunc, *source);
        source++;
    }
    return *source == '_';
}

static void ParameterTypeList_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct ParameterTypeList* p, struct StrBuilder* fp);


static void TPrimaryExpressionLambda_CodePrint(struct SyntaxTree* pSyntaxTree,
                                               struct PrintCodeOptions* options,
                                               struct PrimaryExpressionLambda* p,
                                               struct StrBuilder* fp)
{
    //Output_Append(fp, options, "l1");
    //Output_Append
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    StrBuilder_AppendFmt(fp, "_lambda_%d", global_lambda_counter);
    struct StrBuilder sb = STRBUILDER_INIT;
    struct TParameterList* pParameterTypeListOpt = &p->TypeName.Declarator.pDirectDeclarator->Parameters.ParameterList;

    if (pParameterTypeListOpt)
    {
        //TNodeClueList_CodePrint(options, &p->ClueList2, &sb);
        Output_Append(&sb, options, "\n");

        //StrBuilder_AppendFmt(&sb, p->ClueList0);
        
        StrBuilder_AppendFmt(&sb, "static ");
        TSpecifierQualifierList_CodePrint(pSyntaxTree, options, &p->TypeName.SpecifierQualifierList, &sb);
        StrBuilder_AppendFmt(&sb, " ");

        StrBuilder_AppendFmt(&sb, "_lambda_%d(", global_lambda_counter);
        //Output_Append(&sb, options, "static void func_l1(");
        ParameterTypeList_CodePrint(pSyntaxTree, options, pParameterTypeListOpt, &sb);
        //TNodeClueList_CodePrint(options, &p->ClueList3, &sb);
        Output_Append(&sb, options, ")");
    }
    global_lambda_counter++;
    TCompoundStatement_CodePrint(pSyntaxTree, options, p->pCompoundStatement, &sb);
    Output_Append(&options->sbPreDeclaration, options, "\n");
    StrBuilder_Append(&options->sbPreDeclaration, sb.c_str);
    StrBuilder_Destroy(&sb);
#if NORMAL
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "[");
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, "]");
    if (p->pParameterTypeListOpt)
    {
        TNodeClueList_CodePrint(options, &p->ClueList2, fp);
        Output_Append(fp, options, "(");
        ParameterTypeList_CodePrint(pSyntaxTree, options, p->pParameterTypeListOpt, fp);
        TNodeClueList_CodePrint(options, &p->ClueList3, fp);
        Output_Append(fp, options, ")");
    }
    TCompoundStatement_CodePrint(pSyntaxTree, options, p->pCompoundStatement, fp);
#endif
}

const char* GetFuncName(struct Expression* pExpression)
{
    if (pExpression == NULL)
    {
        return NULL;
    }

    if (pExpression->Type == PrimaryExpressionValue_ID)
    {
        struct PrimaryExpressionValue* pPrimaryExpressionValue =
            (struct PrimaryExpressionValue*)(pExpression);
        return pPrimaryExpressionValue->lexeme;
    }
    return NULL;
}


struct Declaration* GetDeclaration(struct Expression* pExpression, struct Declarator** ppDeclarator)
{
    *ppDeclarator = 0;
    if (pExpression == NULL)
        return NULL;
    if (pExpression->Type == PrimaryExpressionValue_ID)
    {
        struct PrimaryExpressionValue* pPrimaryExpressionValue =
            (struct PrimaryExpressionValue*)(pExpression);
        struct Declarator* pDeclarator =
            Declaration_FindDeclarator(pPrimaryExpressionValue->pDeclaration, pPrimaryExpressionValue->lexeme);
        *ppDeclarator = pDeclarator;
        return pPrimaryExpressionValue->pDeclaration;
    }
    else if (pExpression->Type == BinaryExpression_ID)
    {
        struct BinaryExpression* pBinaryExpression =
            (struct BinaryExpression*)(pExpression);
        if (pBinaryExpression->token == TK_COMMA)
        {
            //retorna do 1 argumento
            return GetDeclaration(pBinaryExpression->pExpressionLeft, ppDeclarator);
        }
        //struct TDeclarator* pDeclarator =
        //  TDeclaration_FindDeclarator(pBinaryExpression->pDeclaration, pPrimaryExpressionValue->lexeme);
        // *ppDeclarator = pDeclarator;
        return 0;// pPrimaryExpressionValue->pDeclaration;
    }
    else if (pExpression->Type == UnaryExpressionOperator_ID)
    {
        struct UnaryExpressionOperator* pUnaryExpressionOperator
            = (struct UnaryExpressionOperator*)(pExpression);
        return GetDeclaration(pUnaryExpressionOperator->pExpressionRight, ppDeclarator);
    }
    return NULL;
}
const char* GetVarName(struct Expression* pExpression)
{
    if (pExpression->Type == UnaryExpressionOperator_ID)
    {
        struct UnaryExpressionOperator* pUnaryExpressionOperator =
            (struct UnaryExpressionOperator*)(pExpression);
        if (pUnaryExpressionOperator->pExpressionRight->Type == PrimaryExpressionValue_ID)
        {
            struct PrimaryExpressionValue* pPrimaryExpressionValue =
                (struct PrimaryExpressionValue*)(pUnaryExpressionOperator->pExpressionRight);
            return pPrimaryExpressionValue->lexeme;
        }
    }
    return NULL;
}

void InstantiateNew(struct PrintCodeOptions* options,
                    struct SyntaxTree* pSyntaxTree,
                    struct PostfixExpression* p,
                    struct StrBuilder* fp)
{
    if (!IsActive(options))
        return;
    void* pv;
    bool bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, "CPRIME_CREATE", &pv);
    if (!bAlreadyInstantiated)
    {
        void* p0;
        HashMap_SetAt(&options->instantiations, "CPRIME_CREATE", pv, &p0);
        StrBuilder_Append(&options->sbPreDeclaration, "static void* mallocinit(int size, void* value);\n");
        StrBuilder_Append(&options->sbPreDeclaration, "#ifndef NEW\n");
        StrBuilder_Append(&options->sbPreDeclaration, "#define NEW(...) mallocinit(sizeof(__VA_ARGS__), & __VA_ARGS__)\n");
        StrBuilder_Append(&options->sbPreDeclaration, "#endif\n");
        StrBuilder_Append(&options->sbInstantiations, "\n");
        StrBuilder_Append(&options->sbInstantiations, "static void* mallocinit(int size, void* value) {\n");
        StrBuilder_Append(&options->sbInstantiations, "   void* memcpy(void* restrict s1, const void* restrict s2, size_t n);\n");
        StrBuilder_Append(&options->sbInstantiations, "   void* p = malloc(size);\n");
        StrBuilder_Append(&options->sbInstantiations, "   if (p) memcpy(p, value, size);\n");
        StrBuilder_Append(&options->sbInstantiations, "   return p;\n");
        StrBuilder_Append(&options->sbInstantiations, "}\n");
    }
    //imprimir espacos
    TExpression_CodePrintSpaces(pSyntaxTree, options, p->pExpressionLeft, fp);
    struct StrBuilder strtemp = STRBUILDER_INIT;
    Output_Append(&strtemp, options, "(");
    bool bOld = options->Options.bCannonical;
    options->Options.bCannonical = true;
    TTypeName_CodePrint(pSyntaxTree, options, p->pTypeName, &strtemp);
    options->Options.bCannonical = bOld;
    Output_Append(&strtemp, options, ")");
    if (p->InitializerList.pHead)
    {
        /*se for vazio ele gera a macro e nao precisa { */
        Output_Append(&strtemp, options, "{");
    }
    TInitializerList_CodePrint(pSyntaxTree,
                               options,
                               (struct DeclarationSpecifiers*)&p->pTypeName->SpecifierQualifierList,
                               &p->pTypeName->Declarator,
                               &p->InitializerList,
                               &strtemp);
    if (p->InitializerList.pHead)
    {
        /*se for vazio ele gera a macro e nao precisa { */
        Output_Append(&strtemp, options, "}");
    }
    //}
    //else
    //{
    //  Output_Append(&strtemp, options, "{}");
    //}
    StrBuilder_Append(fp, "NEW(");
    StrBuilder_Append(fp, strtemp.c_str);
    StrBuilder_Append(fp, ")");
    StrBuilder_Destroy(&strtemp);
}

void ArgumentExpression_CodePrint(struct SyntaxTree* pSyntaxTree,
                                  struct PrintCodeOptions* options,
                                  struct ArgumentExpression* p,
                                  struct StrBuilder* fp)
{
    if (p->bHasComma)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, ",");
    }
    TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
}

void ArgumentExpressionList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                      struct PrintCodeOptions* options,
                                      struct ArgumentExpressionList* p,
                                      struct StrBuilder* fp)
{
    for (int i = 0; i < p->size; i++)
    {
        ArgumentExpression_CodePrint(pSyntaxTree,
                                     options,
                                     p->data[i],
                                     fp);
    }
}


void TPostfixExpression_CodePrint(struct SyntaxTree* pSyntaxTree,
                                  struct PrintCodeOptions* options,
                                  struct PostfixExpression* p,
                                  struct StrBuilder* fp)
{
    //bool bIsPointer = false;
    {
        /* (struct X) {} */
        if (p->token != TK_NEW && p->pTypeName) //ta BEEEM estranho este teste
        {
            if (p->pExpressionLeft)
            {
                TExpression_CodePrint(pSyntaxTree, options, p->pExpressionLeft, fp);
            }
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            Output_Append(fp, options, "(");
            TTypeName_CodePrint(pSyntaxTree, options, p->pTypeName, fp);
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            Output_Append(fp, options, ")");
            //pSpecifierQualifierList = &p->pTypeName->SpecifierQualifierList;
            //bIsPointer = TPointerList_IsPointer(&p->pTypeName->Declarator.PointerList);
            //falta imprimeir typename
            //TTypeName_Print*
            if (p->InitializerList.pHead)
            {
                /*
                se for vazio ele gera a macro e nao precisa {
                case contrario precisa
                */
                Output_Append(fp, options, "{");
            }
            TInitializerList_CodePrint(pSyntaxTree,
                                       options,
                                       (struct DeclarationSpecifiers*)&p->pTypeName->SpecifierQualifierList,
                                       NULL,
                                       &p->InitializerList,
                                       fp);
            if (p->InitializerList.pHead)
            {
                /*se for vazio ele gera a macro e nao precisa { */
                Output_Append(fp, options, "}");
            }
        }
    }
    switch (p->token)
    {
        case TK_NEW:
            InstantiateNew(options,
                           pSyntaxTree,
                           p,
                           fp);
#if 0
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            Output_Append(fp, options, "new");
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            Output_Append(fp, options, "(");
            TTypeName_CodePrint(pSyntaxTree, options, p->pTypeName, fp);
            Output_Append(fp, options, ")");
            if (p->InitializerList.pHead)
            {
                TInitializerList_CodePrint(pSyntaxTree,
                                           options,
                                           &p->pTypeName->SpecifierQualifierList,
                                           &p->pTypeName->Declarator,
                                           &p->InitializerList,
                                           fp);
            }
            else
            {
                Output_Append(fp, options, "{}");
            }
#endif
            break;
        case TK_FULL_STOP:
            if (p->pExpressionLeft)
            {
                TExpression_CodePrint(pSyntaxTree, options, p->pExpressionLeft, fp);
            }
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            Output_Append(fp, options, ".");
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            Output_Append(fp, options, p->Identifier);
            break;
        case TK_ARROW:
            if (p->pExpressionLeft)
            {
                TExpression_CodePrint(pSyntaxTree, options, p->pExpressionLeft, fp);
            }
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            Output_Append(fp, options, "->");
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            Output_Append(fp, options, p->Identifier);
            break;
        case TK_LEFT_SQUARE_BRACKET:
            if (p->pExpressionLeft)
            {
                TExpression_CodePrint(pSyntaxTree, options, p->pExpressionLeft, fp);
            }
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            Output_Append(fp, options, "[");
            TExpression_CodePrint(pSyntaxTree, options, p->pExpressionRight, fp);
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            Output_Append(fp, options, "]");
            break;
        case TK_LEFT_PARENTHESIS:
        {
            const char* funcName = GetFuncName(p->pExpressionLeft);
            if (funcName != NULL && strcmp(funcName, "typename") == 0)
            {
                /*colocar uma string literal no lugar*/
                if (p->ArgumentExpressionList.size == 1)
                {
                    struct StrBuilder str = STRBUILDER_INIT;
                    GetTypeName(p->ArgumentExpressionList.data[0]->pExpression, &str);
                    StrBuilder_Append(fp, "\"");
                    StrBuilder_Append(fp, str.c_str);
                    StrBuilder_Append(fp, "\"");
                    StrBuilder_Destroy(&str);
                }
                else
                {
                    //wrong number of argument
                }
            }
            else if (funcName != NULL && strcmp(funcName, "destroy") == 0)
            {
                if (p->ArgumentExpressionList.size == 1)
                {
                    struct TypeName* pTypeName = Expression_GetTypeName(p->ArgumentExpressionList.data[0]->pExpression);
                    struct StrBuilder strtemp = STRBUILDER_INIT;
                    struct StrBuilder instantiate = STRBUILDER_INIT;
                    TExpression_CodePrint(pSyntaxTree, options, p->ArgumentExpressionList.data[0]->pExpression, &strtemp);
                    InstanciateDestroy(pSyntaxTree,
                                       options,
                                       0,
                                       &pTypeName->SpecifierQualifierList,
                                       &pTypeName->Declarator,
                                       strtemp.c_str,
                                       false,
                                       &instantiate);
                    //imprimir espacos na frente
                    TExpression_CodePrintSpaces(pSyntaxTree, options, p->pExpressionLeft, fp);
                    if (instantiate.size > 0)
                    {
                        StrBuilder_Append(fp, instantiate.c_str);
                    }
                    /*se este cara for vazio nao precisaria fazer nada remover chamada*/
                    StrBuilder_Destroy(&strtemp);
                    StrBuilder_Destroy(&instantiate);
                }
                else
                {
                    //wrong number of argument
                }
            }
            else
            {
                bool bHandled = false;
                struct TypeName* pTypeName = NULL;
                if (p->ArgumentExpressionList.size == 1)
                {
                    pTypeName = Expression_GetTypeName(p->ArgumentExpressionList.data[0]->pExpression);
                    if (pTypeName)
                    {
                        struct StructUnionSpecifier* pStructUnionSpecifier =
                            (struct StructUnionSpecifier*)
                            DeclarationSpecifiers_GetMainSpecifier((struct DeclarationSpecifiers*)&pTypeName->SpecifierQualifierList, StructUnionSpecifier_ID);
                        if (pStructUnionSpecifier)
                        {
                            if (pStructUnionSpecifier->Tag)
                            {
                                pStructUnionSpecifier = FindStructUnionSpecifierByName(pSyntaxTree, pStructUnionSpecifier->Tag);
                            }
                            if (pStructUnionSpecifier->UnionSet.pHead != NULL)
                            {
                                bHandled = true;
                                //imprimir espacos na frente
                                TExpression_CodePrintSpaces(pSyntaxTree, options, p->pExpressionLeft, fp);
                                //se nao tem nome vou gerar um nome com todos
                                char structTagName[1000] = { 0 };
                                GetOrGenerateStructTagName(pStructUnionSpecifier, structTagName, sizeof(structTagName));
                                //nome da funcao
                                Output_Append(fp, options, structTagName);
                                Output_Append(fp, options, "_");
                                Output_Append(fp, options, funcName);
                                UnionTypeDefault(pSyntaxTree,
                                                 options,
                                                 pStructUnionSpecifier,
                                                 NULL,
                                                 "p",
                                                 funcName,
                                                 &options->sbInstantiations,
                                                 &options->sbPreDeclaration);
                            }
                        }
                        else
                        {
                            if (pTypeName->Declarator.pDirectDeclarator &&
                                pTypeName->Declarator.pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeAutoArray &&
                                (funcName != NULL && strcmp(funcName, "push") == 0))
                            {
                                //auto array
                                bHandled = true;
                                struct Declarator* pDeclarator = 0;
                                struct Declaration* pDecl = GetDeclaration(p->pExpressionRight, &pDeclarator);
                                struct StrBuilder strtemp = STRBUILDER_INIT;
                                struct StrBuilder instantiate = STRBUILDER_INIT;
                                TExpression_CodePrint(pSyntaxTree, options, p->pExpressionRight, &strtemp);
                                //TDeclarationSpecifiers_PrintNameMangling(&pDecl->Specifiers, &instantiate);
                                InstanciateVectorPush(pSyntaxTree,
                                                      options,
                                                      &pDecl->Specifiers,
                                                      &instantiate);
                                /*
                                InstanciateDestroy(pSyntaxTree,
                                                   options,
                                                   0,
                                                   (struct TSpecifierQualifierList*)(&pDecl->Specifiers),
                                                   pDeclarator,
                                                   strtemp.c_str,
                                                   false,
                                                   &instantiate);
                                                   */
                                                   //imprimir espacos na frente
                                TExpression_CodePrintSpaces(pSyntaxTree, options, p->pExpressionLeft, fp);
                                if (instantiate.size > 0)
                                {
                                    StrBuilder_Append(fp, instantiate.c_str);
                                    //StrBuilder_Append(fp, "&");
                                }
                                /*se este cara for vazio nao precisaria fazer nada remover chamada*/
                                StrBuilder_Destroy(&strtemp);
                                StrBuilder_Destroy(&instantiate);
                            }
                        }
                    }
                }
                if (p->pExpressionLeft && !bHandled)
                {
                    TExpression_CodePrint(pSyntaxTree, options, p->pExpressionLeft, fp);
                }
                //Do lado esquerdo vem o nome da funcao p->pExpressionLeft
                TNodeClueList_CodePrint(options, &p->ClueList0, fp);
                Output_Append(fp, options, "(");
                ArgumentExpressionList_CodePrint(pSyntaxTree, options, &p->ArgumentExpressionList, fp);
                TNodeClueList_CodePrint(options, &p->ClueList1, fp);
                Output_Append(fp, options, ")");
            }

        }
        break;
        case TK_PLUSPLUS:
            if (p->pExpressionLeft)
            {
                TExpression_CodePrint(pSyntaxTree, options, p->pExpressionLeft, fp);
            }
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            Output_Append(fp, options, "++");
            break;
        case TK_MINUSMINUS:
            if (p->pExpressionLeft)
            {
                TExpression_CodePrint(pSyntaxTree, options, p->pExpressionLeft, fp);
            }
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            Output_Append(fp, options, "--");
            break;
        default:
            //assert(false);
            break;
    }
    if (p->pNext)
    {
        TPostfixExpression_CodePrint(pSyntaxTree, options, p->pNext, fp);
    }
}

void TExpression_CodePrintSpaces(struct SyntaxTree* pSyntaxTree,
                                 struct PrintCodeOptions* options,
                                 struct Expression* p,
                                 struct StrBuilder* fp)
{
    if (p == NULL)
    {
        ////assert(false);
        return;
    }
    switch (p->Type)
    {
        case BinaryExpression_ID:
        {
            struct BinaryExpression* pBinaryExpression = (struct BinaryExpression*)p;
            TExpression_CodePrintSpaces(pSyntaxTree, options, pBinaryExpression->pExpressionLeft, fp);
        }
        break;
        case TernaryExpression_ID:
        {
            struct TernaryExpression* pTernaryExpression =
                (struct TernaryExpression*)p;
            TExpression_CodePrintSpaces(pSyntaxTree, options, pTernaryExpression->pExpressionLeft, fp);
        }
        break;
        case PrimaryExpressionLiteral_ID:
        {
            struct PrimaryExpressionLiteral* pPrimaryExpressionLiteral
                = (struct PrimaryExpressionLiteral*)p;
            for (struct PrimaryExpressionLiteralItem* pItem = (&pPrimaryExpressionLiteral->List)->pHead; pItem != NULL; pItem = pItem->pNext)
            {
                TNodeClueList_CodePrint(options, &pItem->ClueList0, fp);
            }
        }
        break;
        case PrimaryExpressionValue_ID:
        {
            struct PrimaryExpressionValue* pPrimaryExpressionValue =
                (struct PrimaryExpressionValue*)p;
            TNodeClueList_CodePrint(options, &pPrimaryExpressionValue->ClueList0, fp);
        }
        ///true;
        break;
        case PrimaryExpressionLambda_ID:
        {
            assert(false);
        }
        break;
        case PostfixExpression_ID:
        {
            assert(false);
        }
        break;
        case UnaryExpressionOperator_ID:
        {
            struct UnaryExpressionOperator* pTUnaryExpressionOperator =
                (struct UnaryExpressionOperator*)p;
            TNodeClueList_CodePrint(options, &pTUnaryExpressionOperator->ClueList0, fp);
        }
        break;
        case CastExpressionType_ID:
        {
            struct CastExpressionType* pCastExpressionType =
                (struct CastExpressionType*)p;
            TNodeClueList_CodePrint(options, &pCastExpressionType->ClueList0, fp);
        }
        break;
        default:
            //assert(false);
            break;
    }
}

/*
 dada uma expressao retorn o tipo da expressao na forma de string
 mangled;
 exemplo:
  unsigned int  a;
  a <- expressao a retorna unsigned_int.
 */

static void TExpression_CodePrint(struct SyntaxTree* pSyntaxTree,
                                  struct PrintCodeOptions* options,
                                  struct Expression* p,
                                  struct StrBuilder* fp)
{
    if (p == NULL)
    {
        ////assert(false);
        return;
    }
    switch (p->Type)
    {
        case BinaryExpression_ID:
        {
            struct BinaryExpression* pBinaryExpression = (struct BinaryExpression*)p;
            TExpression_CodePrint(pSyntaxTree, options, pBinaryExpression->pExpressionLeft, fp);
            TNodeClueList_CodePrint(options, &pBinaryExpression->ClueList0, fp);
            Output_Append(fp, options, TokenToString(pBinaryExpression->token));
            TExpression_CodePrint(pSyntaxTree, options, ((struct BinaryExpression*)p)->pExpressionRight, fp);
        }
        break;
        case TernaryExpression_ID:
        {
            struct TernaryExpression* pTernaryExpression =
                (struct TernaryExpression*)p;
            TExpression_CodePrint(pSyntaxTree, options, pTernaryExpression->pExpressionLeft, fp);
            TNodeClueList_CodePrint(options, &pTernaryExpression->ClueList0, fp);
            Output_Append(fp, options, "?");
            TExpression_CodePrint(pSyntaxTree, options, pTernaryExpression->pExpressionMiddle, fp);
            TNodeClueList_CodePrint(options, &pTernaryExpression->ClueList1, fp);
            Output_Append(fp, options, ":");
            TExpression_CodePrint(pSyntaxTree, options, pTernaryExpression->pExpressionRight, fp);
        }
        break;
        case PrimaryExpressionLiteral_ID:
        {
            struct PrimaryExpressionLiteral* pPrimaryExpressionLiteral
                = (struct PrimaryExpressionLiteral*)p;
            for (struct PrimaryExpressionLiteralItem* pItem = (&pPrimaryExpressionLiteral->List)->pHead; pItem != NULL; pItem = pItem->pNext)
            {
                TNodeClueList_CodePrint(options, &pItem->ClueList0, fp);
                Output_Append(fp, options, pItem->lexeme);
            }
        }
        break;
        case PrimaryExpressionValue_ID:
        {
            struct PrimaryExpressionValue* pPrimaryExpressionValue =
                (struct PrimaryExpressionValue*)p;
            if (pPrimaryExpressionValue->pExpressionOpt != NULL)
            {
                TNodeClueList_CodePrint(options, &pPrimaryExpressionValue->ClueList0, fp);
                Output_Append(fp, options, "(");
                TExpression_CodePrint(pSyntaxTree, options, pPrimaryExpressionValue->pExpressionOpt, fp);
                TNodeClueList_CodePrint(options, &pPrimaryExpressionValue->ClueList1, fp);
                Output_Append(fp, options, ")");
            }
            else
            {
                TNodeClueList_CodePrint(options, &pPrimaryExpressionValue->ClueList0, fp);
                Output_Append(fp, options, pPrimaryExpressionValue->lexeme);
            }
        }
        ///true;
        break;
        case PrimaryExpressionLambda_ID:
        {
            struct PrimaryExpressionLambda* pPostfixExpressionCore =
                (struct PrimaryExpressionLambda*)p;
            TPrimaryExpressionLambda_CodePrint(pSyntaxTree, options, pPostfixExpressionCore, fp);
        }
        break;
        case PostfixExpression_ID:
        {
            struct PostfixExpression* pPostfixExpressionCore =
                (struct PostfixExpression*)p;
            TPostfixExpression_CodePrint(pSyntaxTree, options, pPostfixExpressionCore, fp);
        }
        break;
        case UnaryExpressionOperator_ID:
        {
            struct UnaryExpressionOperator* pTUnaryExpressionOperator =
                (struct UnaryExpressionOperator*)p;
            TNodeClueList_CodePrint(options, &pTUnaryExpressionOperator->ClueList0, fp);
            if (pTUnaryExpressionOperator->token == TK_SIZEOF)
            {
                if (pTUnaryExpressionOperator->TypeName.SpecifierQualifierList.Size > 0)
                {
                    Output_Append(fp, options, "sizeof");
                    TNodeClueList_CodePrint(options, &pTUnaryExpressionOperator->ClueList1, fp);
                    Output_Append(fp, options, "(");
                    TTypeName_CodePrint(pSyntaxTree, options, &pTUnaryExpressionOperator->TypeName, fp);
                    TNodeClueList_CodePrint(options, &pTUnaryExpressionOperator->ClueList2, fp);
                    Output_Append(fp, options, ")");
                }
                else
                {
                    Output_Append(fp, options, "sizeof");
                    TExpression_CodePrint(pSyntaxTree, options, pTUnaryExpressionOperator->pExpressionRight, fp);
                    Output_Append(fp, options, "");
                }
            }
            else
            {
                Output_Append(fp, options, TokenToString(((struct BinaryExpression*)p)->token));
                TExpression_CodePrint(pSyntaxTree, options, pTUnaryExpressionOperator->pExpressionRight, fp);
            }
        }
        break;
        case CastExpressionType_ID:
        {
            struct CastExpressionType* pCastExpressionType =
                (struct CastExpressionType*)p;
            TNodeClueList_CodePrint(options, &pCastExpressionType->ClueList0, fp);
            Output_Append(fp, options, "(");
            TTypeName_CodePrint(pSyntaxTree, options, &pCastExpressionType->TypeName, fp);
            TNodeClueList_CodePrint(options, &pCastExpressionType->ClueList1, fp);
            Output_Append(fp, options, ")");
            TExpression_CodePrint(pSyntaxTree, options, pCastExpressionType->pExpression, fp);
        }
        break;
        default:
            //assert(false);
            break;
    }
}



static void TEnumerator_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct Enumerator* pTEnumerator, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &pTEnumerator->ClueList0, fp);
    Output_Append(fp, options, pTEnumerator->Name);
    if (pTEnumerator->pConstantExpression)
    {
        TNodeClueList_CodePrint(options, &pTEnumerator->ClueList1, fp);
        Output_Append(fp, options, "=");
        TExpression_CodePrint(pSyntaxTree, options, pTEnumerator->pConstantExpression, fp);
    }
    else
    {
        //vou criar uma expressionp enum?
    }
    if (pTEnumerator->bHasComma)
    {
        TNodeClueList_CodePrint(options, &pTEnumerator->ClueList2, fp);
        Output_Append(fp, options, ",");
    }
}

static void TEnumSpecifier_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct EnumSpecifier* p, struct StrBuilder* fp)
{
    //true;
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "enum");
    if (options->Options.bCannonical)
    {
        Output_Append(fp, options, " ");
    }
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, p->Tag);
    TNodeClueList_CodePrint(options, &p->ClueList2, fp);
    if (p->EnumeratorList.pHead != NULL)
    {
        Output_Append(fp, options, "{");
        for (struct Enumerator* pTEnumerator = (&p->EnumeratorList)->pHead; pTEnumerator != NULL; pTEnumerator = pTEnumerator->pNext)
        {
            TEnumerator_CodePrint(pSyntaxTree, options, pTEnumerator, fp);
        }
        TNodeClueList_CodePrint(options, &p->ClueList3, fp);
        Output_Append(fp, options, "}");
    }
}

static void TUnionSetItem_CodePrint(struct PrintCodeOptions* options, struct UnionSetItem* p, struct StrBuilder* fp)
{
    if (p->Token == TK_STRUCT)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, "struct");
    }
    else if (p->Token == TK_UNION)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, "union");
    }
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, p->Name);
    if (p->TokenFollow == TK_VERTICAL_LINE)
    {
        TNodeClueList_CodePrint(options, &p->ClueList2, fp);
        Output_Append(fp, options, "|");
    }
}

static void TUnionSet_CodePrint(struct PrintCodeOptions* options, struct UnionSet* p, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    if (options->Options.Target == CompilerTarget_C99)
    {
        Output_Append(fp, options, "/*");
    }
    Output_Append(fp, options, "<");
    struct UnionSetItem* pCurrent = p->pHead;
    while (pCurrent)
    {
        TUnionSetItem_CodePrint(options, pCurrent, fp);
        pCurrent = pCurrent->pNext;
    }
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, ">");
    if (options->Options.Target == CompilerTarget_C99)
    {
        Output_Append(fp, options, "*/");
    }
}

static void TStructUnionSpecifier_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct StructUnionSpecifier* p, struct StrBuilder* fp)
{
    if (options->Options.bCannonical)
    {
    }
    else
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    }
    //true;
    if (p->StructDeclarationList.Size > 0)
    {
        if (p->Token == TK_STRUCT)
        {
            Output_Append(fp, options, "struct");
        }
        else if (p->Token == TK_UNION)
        {
            Output_Append(fp, options, "union");
        }
        if (options->Options.bCannonical)
        {
            Output_Append(fp, options, " ");
        }
        if (p->UnionSet.pHead != NULL)
        {
            TUnionSet_CodePrint(options, &p->UnionSet, fp);
        }
        //TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    }
    else
    {
        if (p->Token == TK_STRUCT)
        {
            Output_Append(fp, options, "struct");
        }
        else if (p->Token == TK_UNION)
        {
            Output_Append(fp, options, "union");
        }
        if (options->Options.bCannonical)
        {
            Output_Append(fp, options, " ");
        }
        if (p->UnionSet.pHead != NULL)
        {
            TUnionSet_CodePrint(options, &p->UnionSet, fp);
        }
    }
    if (options->Options.bCannonical)
    {
    }
    else
    {
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    }
    if (p->Tag)
    {
        Output_Append(fp, options, p->Tag);
    }
    else
    {
        if (p->UnionSet.pHead != NULL)
        {
            char structTagName[100] = { 0 };
            GetOrGenerateStructTagName(p, structTagName, sizeof(structTagName));
            Output_Append(fp, options, structTagName);
        }
    }
    if (p->StructDeclarationList.Size > 0)
    {
        TNodeClueList_CodePrint(options, &p->ClueList2, fp);
        Output_Append(fp, options, "{");
        for (int i = 0; i < p->StructDeclarationList.Size; i++)
        {
            struct AnyStructDeclaration* pStructDeclaration = p->StructDeclarationList.pItems[i];
            TAnyStructDeclaration_CodePrint(pSyntaxTree, options, pStructDeclaration, fp);
        }
        TNodeClueList_CodePrint(options, &p->ClueList3, fp);
        Output_Append(fp, options, "}");
    }
}


static void TSingleTypeSpecifier_CodePrint(struct PrintCodeOptions* options, struct SingleTypeSpecifier* p, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    //true;
    /*if (p->Token2 == TK_STRUCT)
    {
      if (options->Target == CompilerTarget_Annotated)
      {
        //acrescenta
        Output_Append(fp, options, "struct ");
      }
      Output_Append(fp, options, p->TypedefName);
    }
    else if (p->Token2 == TK_UNION)
    {
      if (options->Target == CompilerTarget_Annotated)
      {
        //acrescenta
        Output_Append(fp, options, "union ");
      }
      Output_Append(fp, options, p->TypedefName);

    }
    else if (p->Token2 == TK_ENUM)
    {
      if (options->Target == CompilerTarget_Annotated)
      {
        //acrescenta
        Output_Append(fp, options, "enum ");
      }
      Output_Append(fp, options, p->TypedefName);

    }
    else
    */
    if (p->Token2 == TK_IDENTIFIER)
    {
        Output_Append(fp, options, p->TypedefName);
    }
    else
    {
        Output_Append(fp, options,
                      TokenToString(p->Token2));
    }
}

static void TDesignator_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct Designator* p, struct StrBuilder* fp)
{
    //    if (b)
    //      Output_Append(fp, options,  ",");
    //
    if (p->Name)
    {
        //.identifier
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, ".");
        Output_Append(fp, options, p->Name);
        Output_Append(fp, options, "=");
        TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
    }
    else
    {
        //[constant-expression]
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        TExpression_CodePrint(pSyntaxTree, options, p->pExpression, fp);
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    }
}

#define List_HasOneItem(pList) \
 ((pList)->pHead != NULL && (pList)->pHead == (pList)->pTail)

#define List_Back(pList) \
  ((pList)->pTail)

static void TInitializerList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                       struct PrintCodeOptions* options,
                                       struct DeclarationSpecifiers* pDeclarationSpecifiers, //<- usadao para construir {}
                                       struct Declarator* pDeclatator,                        //<-dupla para entender o tipo
                                       struct InitializerList* p,

                                       struct StrBuilder* fp)
{
    if (List_HasOneItem(p) &&
        List_Back(p)->pInitializer == NULL/* &&
      pSpecifierQualifierList != NULL*/)
    {
        //a partir de {} e um tipo consegue gerar o final
        struct StrBuilder sb = STRBUILDER_INIT;
        bool bHasInitializers = false;
        InstanciateInit(pSyntaxTree,
                        options,
                        (struct SpecifierQualifierList*)(pDeclarationSpecifiers),
                        pDeclatator,                        //<-dupla para entender o tipo
                        NULL,
                        NULL,
                        &bHasInitializers,
                        &sb);
        if (bHasInitializers)
        {
            Output_Append(fp, options, sb.c_str);
        }
        else
        {
            Output_Append(fp, options, "{0}");
        }
        StrBuilder_Destroy(&sb);
    }
    else
    {
        if ((p)->pHead == NULL)
        {
            struct StrBuilder sb = STRBUILDER_INIT;
            bool bHasInitializers = false;
            InstanciateInit(pSyntaxTree,
                            options,
                            (struct SpecifierQualifierList*)(pDeclarationSpecifiers),
                            pDeclatator,
                            NULL,
                            NULL,
                            &bHasInitializers,
                            &sb);
            Output_Append(fp, options, sb.c_str);
            StrBuilder_Destroy(&sb);
        }
        else
        {

            for (struct InitializerListItem* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
            {
                if (!List_IsFirstItem(p, pItem))
                    Output_Append(fp, options, ",");
                TInitializerListItem_CodePrint(pSyntaxTree,
                                               options,
                                               pDeclatator,
                                               pDeclarationSpecifiers,
                                               pItem,
                                               fp);
            }

        }
    }
}
static char* strcatupper(char* dest, const char* src)
{
    char* rdest = dest;
    while (*dest)
        dest++;
    while (*src)
    {
        *dest = (char)toupper(*src);
        dest++;
        src++;
    }
    return rdest;
}

static void TInitializerListType_CodePrint(struct SyntaxTree* pSyntaxTree,
                                           struct PrintCodeOptions* options,
                                           struct Declarator* pDeclarator,
                                           struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                           struct InitializerListType* p,
                                           struct StrBuilder* fp)
{
    /*
    default { ... }
    {}
    */
    if (p->InitializerList.pHead == NULL)
    {
        //TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        struct Initializer* pInitializer = NULL;
        //p->InitializerList.pHead ?
        //p->InitializerList.pHead->pInitializer : NULL;
        if (options->Options.Target == CompilerTarget_CXX)
        {
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            Output_Append(fp, options, "{");
            TNodeClueList_CodePrint(options, &p->ClueList2, fp);
            Output_Append(fp, options, "}");
        }
        else  if (options->Options.Target == CompilerTarget_C99)
        {
            TNodeClueList_CodePrint(options, &p->ClueList0, fp);
            //if (options->Options.Target == CompilerTarget_Annotated)
            //{
            //  Output_Append(fp, options, "/*");
            //}
            //Output_Append(fp, options, "default");
            //if (options->Options.Target == CompilerTarget_Annotated)
            //{
            //  Output_Append(fp, options, "*/");
            //}
            TNodeClueList_CodePrint(options, &p->ClueList1, fp);
            if (p->InitializerList.pHead)
                TNodeClueList_CodePrint(options, &p->InitializerList.pHead->ClueList, fp);
            bool bHasInitializers = false;
            InstanciateInit(pSyntaxTree,
                            options,
                            (struct SpecifierQualifierList*)(pDeclarationSpecifiers),
                            pDeclarator,                        //<-dupla para entender o tipo
                            pInitializer,
                            NULL,/*args*/
                            &bHasInitializers,
                            fp);
        }
    }
    else
    {
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, "{");
        TInitializerList_CodePrint(pSyntaxTree,
                                   options,
                                   pDeclarationSpecifiers,
                                   pDeclarator,
                                   &p->InitializerList,
                                   fp);
        TNodeClueList_CodePrint(options, &p->ClueList2, fp);
        Output_Append(fp, options, "}");
    }
}


static void TInitializer_CodePrint(struct SyntaxTree* pSyntaxTree,
                                   struct PrintCodeOptions* options,
                                   struct Declarator* pDeclarator,
                                   struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                   struct Initializer* pTInitializer,
                                   struct StrBuilder* fp)
{
    if (pTInitializer == NULL)
    {
        return;
    }
    if (pTInitializer->Type == InitializerListType_ID)
    {
        TInitializerListType_CodePrint(pSyntaxTree,
                                       options,
                                       pDeclarator,
                                       pDeclarationSpecifiers,
                                       (struct InitializerListType*)pTInitializer, fp);
    }
    else
    {
        TExpression_CodePrint(pSyntaxTree, options, (struct Expression*)pTInitializer, fp);
    }
}


static void TPointerList_CodePrint(struct PrintCodeOptions* options,
                                   struct PointerList* p,
                                   struct StrBuilder* fp)
{
    for (struct Pointer* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        TPointer_CodePrint(options, pItem, fp);
    }
}


static void TParameterList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                     struct PrintCodeOptions* options,
                                     struct ParameterList* p,
                                     struct StrBuilder* fp)
{
    for (struct Parameter* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        TParameter_CodePrint(pSyntaxTree, options, pItem, fp);
    }
}

static void ParameterTypeList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                        struct PrintCodeOptions* options,
                                        struct ParameterTypeList* p,
                                        struct StrBuilder* fp)
{
    TParameterList_CodePrint(pSyntaxTree, options, &p->ParameterList, fp);
    if (p->bVariadicArgs)
    {
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, "...");
    }
}

static void TDeclarator_CodePrint(struct SyntaxTree* pSyntaxTree,
                                  struct PrintCodeOptions* options,
                                  struct Declarator* p,
                                  struct StrBuilder* fp);

static void TDirectDeclarator_CodePrint(struct SyntaxTree* pSyntaxTree,
                                        struct PrintCodeOptions* options,
                                        struct DirectDeclarator* pDirectDeclarator,
                                        struct StrBuilder* fp)
{
    if (pDirectDeclarator == NULL)
    {
        return;
    }
    if (pDirectDeclarator->Identifier)
    {
        TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList0, fp);
        if (pDirectDeclarator->bOverload)
        {
            struct StrBuilder sb = STRBUILDER_INIT;
            ParameterList_PrintNameMangling(&pDirectDeclarator->Parameters.ParameterList,
                                            &sb);
            Output_Append(fp, options, pDirectDeclarator->Identifier);
            Output_Append(fp, options, sb.c_str);
            StrBuilder_Destroy(&sb);
        }
        else
        {
            Output_Append(fp, options, pDirectDeclarator->Identifier);
        }
    }
    else  if (pDirectDeclarator->pDeclarator)
    {
        //( declarator )
        TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList0, fp);
        Output_Append(fp, options, "(");
        TDeclarator_CodePrint(pSyntaxTree, options, pDirectDeclarator->pDeclarator, fp);
        TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList1, fp);
        Output_Append(fp, options, ")");
    }
    if (pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeArray)
    {
        /*
        direct-declarator [ type-qualifier-listopt assignment-expressionopt ]
        direct-declarator [ static type-qualifier-listopt assignment-expression ]
        direct-declarator [ type-qualifier-list static assignment-expression ]
        */
        TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList2, fp);
        Output_Append(fp, options, "[");
        if (pDirectDeclarator->pExpression)
        {
            TExpression_CodePrint(pSyntaxTree, options, pDirectDeclarator->pExpression, fp);
        }
        TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList3, fp);
        Output_Append(fp, options, "]");
    }
    else if (pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeAutoArray)
    {
    }
    if (pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeFunction)
    {
        //( parameter-type-list )
        TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList2, fp);
        Output_Append(fp, options, "(");
        ParameterTypeList_CodePrint(pSyntaxTree, options, &pDirectDeclarator->Parameters, fp);
        TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList3, fp);
        Output_Append(fp, options, ")");
        if (pDirectDeclarator->bOverload)
        {
            if (options->Options.Target == CompilerTarget_CXX)
            {
                TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList4, fp);
                Output_Append(fp, options, "overload");
            }
        }
    }
    if (pDirectDeclarator->pDirectDeclarator)
    {
        //fprintf(fp, "\"direct-declarator\":");
        TDirectDeclarator_CodePrint(pSyntaxTree, options, pDirectDeclarator->pDirectDeclarator, fp);
    }
    //fprintf(fp, "}");
}


static void TDeclarator_CodePrint(struct SyntaxTree* pSyntaxTree,
                                  struct PrintCodeOptions* options,
                                  struct Declarator* p,
                                  struct StrBuilder* fp)
{
    TPointerList_CodePrint(options, &p->PointerList, fp);
    TDirectDeclarator_CodePrint(pSyntaxTree, options, p->pDirectDeclarator, fp);
}


void TStructDeclarator_CodePrint(struct SyntaxTree* pSyntaxTree,
                                 struct PrintCodeOptions* options,
                                 struct SpecifierQualifierList* pSpecifierQualifierList,
                                 struct InitDeclarator* p,
                                 struct StrBuilder* fp)
{
    TDeclarator_CodePrint(pSyntaxTree, options, p->pDeclarator, fp);
    if (p->pInitializer)
    {
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        if (options->Options.Target == CompilerTarget_C99)
        {
            Output_Append(fp, options, "/*");
        }
        Output_Append(fp, options, "=");
        struct PrintCodeOptions options2 = *options;
        options2.Options.bExpandMacros = true;
        options2.Options.bIncludeComments = false;
        TInitializer_CodePrint(pSyntaxTree,
                               &options2,
                               p->pDeclarator,
                               (struct DeclarationSpecifiers*)pSpecifierQualifierList,
                               p->pInitializer,
                               fp);
        if (options->Options.Target == CompilerTarget_C99)
        {
            Output_Append(fp, options, "*/");
        }
    }
}

static void TStructDeclaratorList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                            struct PrintCodeOptions* options,
                                            struct SpecifierQualifierList* pSpecifierQualifierList,
                                            struct StructDeclaratorList* p,
                                            struct StrBuilder* fp)
{
    for (struct InitDeclarator* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        if (!List_IsFirstItem(p, pItem))
        {
            TNodeClueList_CodePrint(options, &pItem->ClueList0, fp);
            Output_Append(fp, options, ",");
        }
        TStructDeclarator_CodePrint(pSyntaxTree, options, pSpecifierQualifierList, pItem, fp);
    }
}

static void TStructDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree,
                                         struct PrintCodeOptions* options,
                                         struct StructDeclaration* p,
                                         struct StrBuilder* fp)
{
    TSpecifierQualifierList_CodePrint(pSyntaxTree, options, &p->SpecifierQualifierList, fp);
    TStructDeclaratorList_CodePrint(pSyntaxTree,
                                    options,
                                    &p->SpecifierQualifierList,
                                    &p->DeclaratorList, fp);
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, ";");
}

static void TAnyStructDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct AnyStructDeclaration* p, struct StrBuilder* fp)
{
    switch (p->Type)
    {
        case StructDeclaration_ID:
            TStructDeclaration_CodePrint(pSyntaxTree, options, (struct StructDeclaration*)p, fp);
            break;
        default:
            //assert(false);
            break;
    }
}

static void StorageSpecifier_CodePrint(struct PrintCodeOptions* options, struct StorageSpecifier* p, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, TokenToString(p->Token));
}

static void TFunctionSpecifier_CodePrint(struct PrintCodeOptions* options, struct FunctionSpecifier* p, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, TokenToString(p->Token));
}

static void TTypeQualifier_CodePrint(struct PrintCodeOptions* options, struct TypeQualifier* p, struct StrBuilder* fp)
{
    //TODO nao pode colocr isso se veio de comentario
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    if (p->Token == TK_AUTO)
    {
        if (options->Options.Target == CompilerTarget_C99)
        {
            Output_Append(fp, options, "/*");
        }
        Output_Append(fp, options, "auto");
        if (options->Options.Target == CompilerTarget_C99)
        {
            Output_Append(fp, options, "*/");
        }
    }
    else
    {
        Output_Append(fp, options, TokenToString(p->Token));
    }
#ifdef LANGUAGE_EXTENSIONS
    if (p->Token == TK_SIZEOF)
    {
        //tODO ja esta nos comentarios
        //Output_Append(fp, options, "(");
        //Output_Append(fp, options, p->SizeIdentifier);
        //Output_Append(fp, options, ")");
        if (options->Options.Target == CompilerTarget_C99)
        {
            //Output_Append(fp, options, "@*/");
        }
    }
#endif
}


static void TTypeQualifierList_CodePrint(struct PrintCodeOptions* options, struct TypeQualifierList* p, struct StrBuilder* fp)
{
    for (int i = 0; i < p->Size; i++)
    {
        struct TypeQualifier* pItem = p->Data[i];
        TTypeQualifier_CodePrint(options, pItem, fp);
    }
}

static void TPointer_CodePrint(struct PrintCodeOptions* options, struct Pointer* pPointer, struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &pPointer->ClueList0, fp);
    Output_Append(fp, options, "*");
    TTypeQualifierList_CodePrint(options, &pPointer->Qualifier, fp);
}

void TSpecifierQualifierList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                       struct PrintCodeOptions* options,
                                       struct SpecifierQualifierList* pDeclarationSpecifiers,
                                       struct StrBuilder* fp)
{
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        if (i > 0 && options->Options.bCannonical)
        {
            //gerar espaco entre eles para nao grudar no modo cannonico
            Output_Append(fp, options, " ");
        }
        struct SpecifierQualifier* pItem = pDeclarationSpecifiers->pData[i];
        switch (pItem->Type)
        {
            case SingleTypeSpecifier_ID:
                TSingleTypeSpecifier_CodePrint(options, (struct SingleTypeSpecifier*)pItem, fp);
                break;
            case StorageSpecifier_ID:
                StorageSpecifier_CodePrint(options, (struct StorageSpecifier*)pItem, fp);
                break;
            case TypeQualifier_ID:
                TTypeQualifier_CodePrint(options, (struct TypeQualifier*)pItem, fp);
                break;
            case FunctionSpecifier_ID:
                TFunctionSpecifier_CodePrint(options, (struct FunctionSpecifier*)pItem, fp);
                break;
                //case TAlignmentSpecifier_ID:
                ///TAlignmentSpecifier_CodePrint(pSyntaxTree, options, (struct TAlignmentSpecifier*)pItem,  fp);
                //break;
            case StructUnionSpecifier_ID:
                TStructUnionSpecifier_CodePrint(pSyntaxTree, options, (struct StructUnionSpecifier*)pItem, fp);
                break;
            case EnumSpecifier_ID:
                TEnumSpecifier_CodePrint(pSyntaxTree, options, (struct EnumSpecifier*)pItem, fp);
                break;
            default:
                //assert(false);
                break;
        }
    }
}



void TDeclarationSpecifiers_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct DeclarationSpecifiers* pDeclarationSpecifiers, struct StrBuilder* fp)
{
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        struct DeclarationSpecifier* pItem = pDeclarationSpecifiers->pData[i];
        switch (pItem->Type)
        {
            case SingleTypeSpecifier_ID:
                TSingleTypeSpecifier_CodePrint(options, (struct SingleTypeSpecifier*)pItem, fp);
                break;
            case StructUnionSpecifier_ID:
                TStructUnionSpecifier_CodePrint(pSyntaxTree, options, (struct StructUnionSpecifier*)pItem, fp);
                break;
            case EnumSpecifier_ID:
                TEnumSpecifier_CodePrint(pSyntaxTree, options, (struct EnumSpecifier*)pItem, fp);
                break;
            case StorageSpecifier_ID:
                StorageSpecifier_CodePrint(options, (struct StorageSpecifier*)pItem, fp);
                break;
            case TypeQualifier_ID:
                TTypeQualifier_CodePrint(options, (struct TypeQualifier*)pItem, fp);
                break;
            case FunctionSpecifier_ID:
                TFunctionSpecifier_CodePrint(options, (struct FunctionSpecifier*)pItem, fp);
                break;
                //case TAlignmentSpecifier_ID:
                ///TAlignmentSpecifier_CodePrint(pSyntaxTree, options, (struct TAlignmentSpecifier*)pItem,  fp);
                //break;
            default:
                //assert(false);
                break;
        }
    }
}

void TInitDeclarator_CodePrint(struct SyntaxTree* pSyntaxTree,
                               struct PrintCodeOptions* options,
                               struct Declarator* pDeclarator,
                               struct DeclarationSpecifiers* pDeclarationSpecifiers,
                               struct InitDeclarator* p,
                               struct StrBuilder* fp)
{
    TDeclarator_CodePrint(pSyntaxTree, options, p->pDeclarator, fp);
    if (p->pInitializer)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, "=");
        TInitializer_CodePrint(pSyntaxTree,
                               options,
                               pDeclarator,
                               pDeclarationSpecifiers,
                               p->pInitializer,
                               fp);
    }
}



int  TInitDeclaratorList_CodePrint(struct SyntaxTree* pSyntaxTree,
                                   bool bPrintAutoArray,
                                   struct PrintCodeOptions* options,
                                   struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                   struct InitDeclaratorList* p,
                                   struct StrBuilder* fp)
{
    int declaratorsPrintedCount = 0;
    //fprintf(fp, "[");
    for (struct InitDeclarator* pInitDeclarator = (p)->pHead; pInitDeclarator != NULL; pInitDeclarator = pInitDeclarator->pNext)
    {
        //if (!List_IsFirstItem(p, pInitDeclarator))
        //   Output_Append(fp, options, ",");
        bool bIsAutoArray = Declarator_IsAutoArray(pInitDeclarator->pDeclarator);
        if (bIsAutoArray == bPrintAutoArray)
        {
            if (declaratorsPrintedCount > 0)
                Output_Append(fp, options, ",");
            TInitDeclarator_CodePrint(pSyntaxTree,
                                      options,
                                      pInitDeclarator->pDeclarator,
                                      pDeclarationSpecifiers,
                                      pInitDeclarator,
                                      fp);
            declaratorsPrintedCount++;
        }
    }
    //  fprintf(fp, "]");
    return declaratorsPrintedCount;
}



struct StructUnionSpecifier* GetStructSpecifier(struct SyntaxTree* pSyntaxTree, struct DeclarationSpecifiers* specifiers);


struct TemplateVar
{
    const char* Name;
    const char* Value;
};

const char* FindValue(const char* name, int namesize, struct TemplateVar* args, int argssize)
{
    for (int i = 0; i < argssize; i++)
    {
        if (namesize == (int)strlen(args[i].Name) &&
            strncmp(name, args[i].Name, namesize) == 0)
        {
            return args[i].Value;
        }
    }
    return "?";
}

void StrBuilder_Template(struct StrBuilder* p,
                         const char* tmpt,
                         struct TemplateVar* vars,
                         int size,
                         int identationLevel)
{
    const char* pch = tmpt;
    //Move tudo para o lado de acordo com a identacao
    for (int i = 0; i < identationLevel * 4; i++)
    {
        StrBuilder_AppendChar(p, ' ');
    }
    //agora nove de acordo com os espacos
    while (*pch == ' ')
    {
        for (int j = 0; j < 4; j++)
        {
            StrBuilder_AppendChar(p, ' ');
        }
        pch++;
    }
    while (*pch)
    {
        if (*pch == '$')
        {
            pch++;
            const char* name = pch;
            int namesize = 0;
            if (*pch &&
                ((*pch >= 'a' && *pch <= 'z') ||
                (*pch >= 'A' && *pch <= 'Z') ||
                (*pch >= '_')))
            {
                pch++;
                namesize++;
                while (*pch &&
                       ((*pch >= 'a' && *pch <= 'z') ||
                       (*pch >= 'A' && *pch <= 'Z') ||
                       (*pch >= '0' && *pch <= '9') ||
                       (*pch >= '_'))) //$X_X
                {
                    pch++;
                    namesize++;
                }
            }
            const char* val =
                FindValue(name, namesize, vars, size);
            StrBuilder_Append(p, val);
        }
        else
        {
            //Este \b eh usado para juntar identificador
            //$part1_part2
            //$part1\b_part2
            //
            if (*pch == '\n')
            {
                StrBuilder_AppendChar(p, *pch);
                pch++;
                if (*pch != '\0') //se for o ultimo nao move
                {
                    //Move tudo para o lado de acordo com a identacao
                    for (int i = 0; i < identationLevel * 4; i++)
                    {
                        StrBuilder_AppendChar(p, ' ');
                    }
                    //agora nove de acordo com os espacos
                    while (*pch == ' ')
                    {
                        for (int j = 0; j < 4; j++)
                        {
                            StrBuilder_AppendChar(p, ' ');
                        }
                        pch++;
                    }
                }
            }
            else
            {
                if (*pch != '\b')
                {
                    StrBuilder_AppendChar(p, *pch);
                }
                pch++;
            }
        }
    }
}

void GetPrefixSuffix(const char* psz, struct StrBuilder* prefix, struct StrBuilder* suffix)
{
    while (*psz && *psz != '_')
    {
        StrBuilder_AppendChar(prefix, *psz);
        psz++;
    }
    if (*psz == '_')
        psz++;
    while (*psz)
    {
        StrBuilder_AppendChar(suffix, *psz);
        psz++;
    }
}


static int FindRuntimeID(struct SyntaxTree* pSyntaxTree,
                         struct StructUnionSpecifier* pStructUnionSpecifier,
                         struct StrBuilder* idname)
{
    if (pStructUnionSpecifier == NULL)
    {
        assert(false);
        return 0;
    }
    /*esta funcao na verdade tem que retornar o tipo e o nome, nao apenas o nome*/
    if (pStructUnionSpecifier->Tag)
    {
        pStructUnionSpecifier = SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pStructUnionSpecifier->Tag);
    }
    if (pStructUnionSpecifier == NULL)
    {
        assert(false);
        return 0;
    }
    int typeInt = 0;
    if (pStructUnionSpecifier != NULL && pStructUnionSpecifier->UnionSet.pHead != NULL)
    {
        pStructUnionSpecifier =
            SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pStructUnionSpecifier->UnionSet.pHead->Name);
        if (pStructUnionSpecifier != NULL && pStructUnionSpecifier->StructDeclarationList.Size > 0)
        {
            struct StructDeclaration* pStructDeclaration =
                AnyStructDeclaration_As_StructDeclaration(pStructUnionSpecifier->StructDeclarationList.pItems[0]);
            if (pStructDeclaration)
            {
                struct InitDeclarator* pStructDeclarator =
                    pStructDeclaration->DeclaratorList.pHead;
                //o primeiro item tem que ser o ID
                if (pStructDeclarator)
                {
                    const char* structDeclaratorName =
                        Declarator_GetName(pStructDeclarator->pDeclarator);
                    //if (TSpecifierQualifierList_IsAnyInteger(&pStructDeclaration->SpecifierQualifierList))
                    {
                        StrBuilder_Set(idname, structDeclaratorName);
                    }
                }
            }
        }
        else if (pStructUnionSpecifier != NULL && pStructUnionSpecifier->UnionSet.pHead)
        {
            return FindRuntimeID(pSyntaxTree,
                                 pStructUnionSpecifier,
                                 idname);
        }
    }
    return typeInt;
}


static int FindIDValue(struct SyntaxTree* pSyntaxTree,
                       const char* structOrTypeName,
                       struct StrBuilder* idname)
{
    ////////////
    struct Declaration* pFinalDecl =
        SyntaxTree_GetFinalTypeDeclaration(pSyntaxTree, structOrTypeName);
    int typeInt = 0;
    struct StructUnionSpecifier* pStructUnionSpecifier = NULL;
    if (pFinalDecl)
    {
        typeInt = 1; //typefef
        if (pFinalDecl->Specifiers.Size > 1)
        {
            pStructUnionSpecifier =
                DeclarationSpecifier_As_StructUnionSpecifier(pFinalDecl->Specifiers.pData[1]);
            if (pStructUnionSpecifier->Tag)
            {
                //procura a mais completa
                pStructUnionSpecifier =
                    SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pStructUnionSpecifier->Tag);
            }
        }
    }
    else
    {
        typeInt = 2; //struct
        pStructUnionSpecifier =
            SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, structOrTypeName);
    }
    //////////////
    if (pStructUnionSpecifier)
    {
        if (pStructUnionSpecifier->StructDeclarationList.Size > 0)
        {
            struct StructDeclaration* pStructDeclaration =
                AnyStructDeclaration_As_StructDeclaration(pStructUnionSpecifier->StructDeclarationList.pItems[0]);
            if (pStructDeclaration)
            {
                struct InitDeclarator* pStructDeclarator =
                    pStructDeclaration->DeclaratorList.pHead;
                //o primeiro item tem que ser o ID
                if (pStructDeclarator)
                {
                    //const char* structDeclaratorName =
                    //  TDeclarator_GetName(pStructDeclarator->pDeclarator);
                    //if (TSpecifierQualifierList_IsAnyInteger(&pStructDeclaration->SpecifierQualifierList))
                    {
                        struct PrintCodeOptions options2 = CODE_PRINT_OPTIONS_INIT;
                        TInitializer_CodePrint(pSyntaxTree, &options2, pStructDeclarator->pDeclarator,
                                               (struct DeclarationSpecifiers*)&pStructDeclaration->SpecifierQualifierList,
                                               pStructDeclarator->pInitializer, idname);
                        //StrBuilder_Set(idname, structDeclaratorName);
                    }
                }
            }
        }
    }
    return typeInt;
}

struct StructUnionSpecifier* FindStructUnionSpecifierByName(struct SyntaxTree* pSyntaxTree,
    const char* structTagNameOrTypedef)
{
    struct StructUnionSpecifier* p = SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, structTagNameOrTypedef);
    if (p != NULL)
    {
    }
    else
    {
        //eh preciso implementar um algoritmo
        //que dado um typedef ele encontre (monte) o tipo final.
        //eh montado pois deve somar a contruibuicao de cada typede
        /*
        * Exemplo:
          struct Tag { int i; };
          typedef struct Tag X;
          typedef const X T[2];
        */
    }
    return p;
}

void FindUnionSetOf(struct SyntaxTree* pSyntaxTree,
                    struct StructUnionSpecifier* pStructUnionSpecifier,
                    struct HashMap* map)
{
    if (pStructUnionSpecifier == NULL)
    {
        //assert(false);
        return;
    }
    if (pStructUnionSpecifier->Tag)
    {
        pStructUnionSpecifier = SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pStructUnionSpecifier->Tag);
    }
    if (pStructUnionSpecifier &&
        pStructUnionSpecifier->UnionSet.pHead != NULL)
    {
        struct UnionSetItem* pCurrent =
            pStructUnionSpecifier->UnionSet.pHead;
        while (pCurrent)
        {
            struct StructUnionSpecifier* pStructUnionSpecifier2 =
                SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pCurrent->Name);
            FindUnionSetOf(pSyntaxTree, pStructUnionSpecifier2, map);
            pCurrent = pCurrent->pNext;
        }
    }
    else
    {
        //void* typeInt;
        void* pp = 0;
        HashMap_SetAt(map, pStructUnionSpecifier->Tag, (void*)0, &pp);
    }
}

static const char* GetNullStr(struct SyntaxTree* pSyntaxTree);




static void TDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree,
                                   struct PrintCodeOptions* options,
                                   struct Declaration* p,
                                   struct StrBuilder* fp)
{
    /*
    * aqui tem que verificar se nao eh auto array pois vamos mexer no priont do especifier
    */
    bool bHasAutoArray =
        HasAutoArray(&p->InitDeclaratorList);
    if (bHasAutoArray)
    {
        struct StrBuilder local = STRBUILDER_INIT;
        //imprimir os que nao sao auto array antes
        TDeclarationSpecifiers_CodePrint(pSyntaxTree, options, &p->Specifiers, &local);
        int c = TInitDeclaratorList_CodePrint(pSyntaxTree,
                                              false,
                                              options,
                                              &p->Specifiers,
                                              &p->InitDeclaratorList, &local);
        Output_Append(&local, options, ";");
        if (c > 0)
        {
            Output_Append(fp, options, local.c_str);
        }
        StrBuilder_Destroy(&local);
        //depois imprimir os que sao
        char name[200] = { 0 };
        IntanciateTypeIfNecessary(pSyntaxTree, options, &p->Specifiers, name, 200);
        Output_Append(fp, options, name);
        TInitDeclaratorList_CodePrint(pSyntaxTree,
                                      true,
                                      options,
                                      &p->Specifiers,
                                      &p->InitDeclaratorList, fp);
    }
    else
    {
        TDeclarationSpecifiers_CodePrint(pSyntaxTree, options, &p->Specifiers, fp);
        //normal todos
        TInitDeclaratorList_CodePrint(pSyntaxTree,
                                      false,
                                      options,
                                      &p->Specifiers,
                                      &p->InitDeclaratorList, fp);
    }
    /*posso imprimir aqui embaixo o cara para aceitar int a[autp], b;*/
    if (p->pCompoundStatementOpt != NULL)
    {
        /*
        typedef struct Array { int data; ... } Items;
        void Items_Add(Items* p,int i) {...}
        void Items_Delete(Items* p,int i) {...}
        */
        StrBuilder_Clear(&options->returnType);
        bool bold = options->Options.bCannonical;
        options->Options.bCannonical = true;
        //TODO storagte nao vai
        TDeclarationSpecifiers_CodePrint(pSyntaxTree, options, &p->Specifiers, &options->returnType);
        if (p->InitDeclaratorList.pHead &&
            p->InitDeclaratorList.pHead->pDeclarator)
        {
            TPointerList_CodePrint(options, &p->InitDeclaratorList.pHead->pDeclarator->PointerList, &options->returnType);
        }
        options->Options.bCannonical = bold;
        if (p->pCompoundStatementOpt != NULL)
        {
            //
            //normal
            TCompoundStatement_CodePrint(pSyntaxTree,
                                         options,
                                         p->pCompoundStatementOpt,
                                         fp);
        }
    }
    else
    {
        TNodeClueList_CodePrint(options, &p->ClueList1, fp);
        Output_Append(fp, options, ";");
    }
    return;
}

void TTypeName_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct TypeName* p, struct StrBuilder* fp)
{
    TSpecifierQualifierList_CodePrint(pSyntaxTree, options, &p->SpecifierQualifierList, fp);
    TDeclarator_CodePrint(pSyntaxTree, options, &p->Declarator, fp);
}

static void IntanciateTypeIfNecessary(struct SyntaxTree* pSyntaxTree,
                                      struct PrintCodeOptions* options,
                                      struct DeclarationSpecifiers* pSpecifiers,
                                      char* buffer,
                                      int bufferSize)
{
    if (!IsActive(options))
        return;
    //bool bResult = false;
    struct StrBuilder name = STRBUILDER_INIT;
    DeclarationSpecifiers_PrintNameMangling(pSpecifiers, &name);
    // char buffer[200] = { 0 };
    snprintf(buffer, bufferSize, "struct vector_%s", name.c_str);
    void* pv = 0;
    bool bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, buffer, &pv);
    if (!bAlreadyInstantiated)
    {
        struct StrBuilder typeMangling = STRBUILDER_INIT;
        TDeclarationSpecifiers_CodePrint(pSyntaxTree, options, pSpecifiers, &typeMangling);
        void* p0 = 0;
        HashMap_SetAt(&options->instantiations, buffer, pv, &p0);
        StrBuilder_AppendFmt(&options->sbPreDeclaration, "#ifndef vector_%s_instantiation\n", name.c_str);
        StrBuilder_AppendFmt(&options->sbPreDeclaration, "#define vector_%s_instantiation\n", name.c_str);
        StrBuilder_AppendFmt(&options->sbPreDeclaration, "struct vector_%s { %s *data; int size, capacity; }; \n", name.c_str, typeMangling.c_str);
        StrBuilder_AppendFmt(&options->sbPreDeclaration, "#endif\n");
        StrBuilder_Destroy(&typeMangling);
    }
    StrBuilder_Destroy(&name);
}

static void TParameter_CodePrint(struct SyntaxTree* pSyntaxTree,
                                 struct PrintCodeOptions* options,
                                 struct Parameter* p,
                                 struct StrBuilder* fp)
{
    char name[200] = { 0 };
    bool bAutoArray = Declarator_IsAutoArray(&p->Declarator);
    if (bAutoArray)
    {
        IntanciateTypeIfNecessary(pSyntaxTree, options, &p->Specifiers, name, 200);
        Output_Append(fp, options, name);
        Output_Append(fp, options, "*");
        /*quando auto arrays sao parametros eles sao sempre por ponteiro*/
    }
    else
    {
        TDeclarationSpecifiers_CodePrint(pSyntaxTree, options, &p->Specifiers, fp);
    }
    TDeclarator_CodePrint(pSyntaxTree, options, &p->Declarator, fp);
    if (p->bHasComma)
    {
        TNodeClueList_CodePrint(options, &p->ClueList0, fp);
        Output_Append(fp, options, ",");
    }
}

static void TEofDeclaration_CodePrint(struct PrintCodeOptions* options,
                                      struct EofDeclaration* p,
                                      struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
}

static void TStaticAssertDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree,
                                               struct PrintCodeOptions* options,
                                               struct StaticAssertDeclaration* p,

                                               struct StrBuilder* fp)
{
    TNodeClueList_CodePrint(options, &p->ClueList0, fp);
    Output_Append(fp, options, "_StaticAssert");
    TNodeClueList_CodePrint(options, &p->ClueList1, fp);
    Output_Append(fp, options, "(");
    TExpression_CodePrint(pSyntaxTree, options, p->pConstantExpression, fp);
    Output_Append(fp, options, ",");
    TNodeClueList_CodePrint(options, &p->ClueList2, fp);
    TNodeClueList_CodePrint(options, &p->ClueList3, fp);
    Output_Append(fp, options, p->Text);
    TNodeClueList_CodePrint(options, &p->ClueList4, fp);
    Output_Append(fp, options, ")");
    TNodeClueList_CodePrint(options, &p->ClueList5, fp);
    Output_Append(fp, options, ";");
}

static void TAnyDeclaration_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct AnyDeclaration* pDeclaration, struct StrBuilder* fp)
{
    switch (pDeclaration->Type)
    {
        case EofDeclaration_ID:
            TEofDeclaration_CodePrint(options, (struct EofDeclaration*)pDeclaration, fp);
            break;
        case StaticAssertDeclaration_ID:
            TStaticAssertDeclaration_CodePrint(pSyntaxTree, options, (struct StaticAssertDeclaration*)pDeclaration, fp);
            break;
        case Declaration_ID:
            TDeclaration_CodePrint(pSyntaxTree, options, (struct Declaration*)pDeclaration, fp);
            break;
        default:
            //assert(false);
            break;
    }
}

static void TDesignatorList_CodePrint(struct SyntaxTree* pSyntaxTree, struct PrintCodeOptions* options, struct DesignatorList* p, struct StrBuilder* fp)
{
    for (struct Designator* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        if (!List_IsFirstItem(p, pItem))
        {
            Output_Append(fp, options, ",");
        }
        TDesignator_CodePrint(pSyntaxTree, options, pItem, fp);
    }
}


static void TInitializerListItem_CodePrint(struct SyntaxTree* pSyntaxTree,
                                           struct PrintCodeOptions* options,
                                           struct Declarator* pDeclarator,
                                           struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                           struct InitializerListItem* p,

                                           struct StrBuilder* fp)
{
    if (p->DesignatorList.pHead != NULL)
    {
        TDesignatorList_CodePrint(pSyntaxTree, options, &p->DesignatorList, fp);
    }
    TInitializer_CodePrint(pSyntaxTree,
                           options,
                           pDeclarator,
                           pDeclarationSpecifiers,
                           p->pInitializer,
                           fp);
    TNodeClueList_CodePrint(options, &p->ClueList, fp);
}



void SyntaxTree_PrintCodeToString(struct SyntaxTree* pSyntaxTree,
                                  struct OutputOptions* options0,
                                  struct StrBuilder* output)
{
    struct PrintCodeOptions options = CODE_PRINT_OPTIONS_INIT;
    options.Options = *options0;

    /*reseta contadores de geracao*/
    global_lambda_counter = 0;
    try_statement_index = 0;

    int k = 0;
    struct StrBuilder sb = STRBUILDER_INIT;
    StrBuilder_Reserve(&sb, 80 * 5000);
    for (int i = 0; i < pSyntaxTree->Declarations.Size; i++)
    {
        struct AnyDeclaration* pItem = pSyntaxTree->Declarations.pItems[i];
        StrBuilder_Clear(&options.sbPreDeclaration);
        struct StrBuilder sbDeclaration = STRBUILDER_INIT;
        TAnyDeclaration_CodePrint(pSyntaxTree, &options, pItem, &sbDeclaration);
        /*injeta as declaracoes geradas*/
        if (IsActive(&options) && options.sbPreDeclaration.size > 0)
        {
            //StrBuilder_Append(&sb, "\n#pragma region cprime\n");
            StrBuilder_Append(&sb, "\n");
            StrBuilder_Append(&sb, options.sbPreDeclaration.c_str);
            //StrBuilder_Append(&sb, "\n#pragma endregion\n");
            StrBuilder_Append(&sb, "\n");
        }
        StrBuilder_Append(&sb, sbDeclaration.c_str);
        StrBuilder_Destroy(&sbDeclaration);
        StrBuilder_Append(output, sb.c_str);
        StrBuilder_Clear(&sb);
        k++;
    }
    /*injeta as instanciacoes geradas*/
    if (options.sbInstantiations.size > 0)
    {
        //StrBuilder_Append(output, "\n#pragma region cprime\n");
        StrBuilder_Append(output, "\n");
        StrBuilder_Append(output, "\n");
        StrBuilder_Append(output, options.sbInstantiations.c_str);
        StrBuilder_Append(output, "\n");
        //StrBuilder_Append(output, "\n#pragma endregion\n");
    }
    StrBuilder_Destroy(&sb);
}


void SyntaxTree_PrintCodeToFile(struct SyntaxTree* pSyntaxTree,
                                struct OutputOptions* options0,
                                const char* outFileName)
{
    FILE* fp = fopen(outFileName, "w");
    if (fp == NULL)
    {
        printf("cannot open output file %s", outFileName);
        return;
    }
    struct StrBuilder output = STRBUILDER_INIT;
    StrBuilder_Reserve(&output, 10000);
    SyntaxTree_PrintCodeToString(pSyntaxTree,
                                 options0,
                                 &output);
    fprintf(fp, "%s", output.c_str);
    StrBuilder_Destroy(&output);
    fclose(fp);
}

static const char* GetFreeStr(struct SyntaxTree* pSyntaxTree)
{
    bool bCustomFree =
        SymbolMap_FindFunction(&pSyntaxTree->GlobalScope, "free") != NULL ||
        MacroMap_Find(&pSyntaxTree->Defines, "free") != NULL;
    return bCustomFree ? "free" : "free";
}


static const char* GetNullStr(struct SyntaxTree* pSyntaxTree)
{
    bool bHasFalse =
        MacroMap_Find(&pSyntaxTree->Defines, "NULL") != NULL;
    return bHasFalse ? "NULL" : "0";
}


void InstantiateDestroyForPolymorphicType(struct SyntaxTree* pSyntaxTree,
                                          struct PrintCodeOptions* options,
                                          struct StructUnionSpecifier* pTStructUnionSpecifier,
                                          struct ParameterTypeList* pArgsOpt, //parametros
                                          struct StrBuilder* fp)
{
    if (!IsActive(options))
        return;
    if (pTStructUnionSpecifier->Tag)
    {
        pTStructUnionSpecifier = SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pTStructUnionSpecifier->Tag);
    }
    char structName[200] = { 0 };
    GetOrGenerateStructTagName(pTStructUnionSpecifier, structName, sizeof(structName));
    const char* parameterName0 = "p";
    const char* functionSuffix = "Destroy";
    struct HashMap map = HASHMAP_INIT;
    FindUnionSetOf(pSyntaxTree, pTStructUnionSpecifier, &map);
    //instanciar cada um dos tipos
    for (int i = 0; i < (int)map.nHashTableSize; i++)
    {
        if (map.pHashTable[i])
        {
            struct StructUnionSpecifier* pStructUnionSpecifier =
                SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, map.pHashTable[i]->Key);
            if (pStructUnionSpecifier)
            {
                InstantiateDeleteForStruct(pSyntaxTree,
                                           pStructUnionSpecifier,
                                           options,
                                           &options->sbInstantiations,
                                           &options->sbPreDeclaration);
            }
            else
            {
                //ERROR TODO
                assert(false);
            }
        }
    }
    struct StrBuilder strid = STRBUILDER_INIT;
    FindRuntimeID(pSyntaxTree, pTStructUnionSpecifier, &strid);
    struct StrBuilder args = STRBUILDER_INIT;
    if (pArgsOpt != NULL)
    {
        //lista argumentos separados por virgula
        ParameterTypeList_GetArgsString(pArgsOpt, &args);
    }
    else
    {
        StrBuilder_Append(&args, parameterName0);
    }
    struct TemplateVar vars0[] =
    {
        {"p", parameterName0},
        {"id", strid.c_str}
    };
    StrBuilder_Template(fp,
                        "switch ($p->$id)\n"
                        "{\n",
                        vars0,
                        sizeof(vars0) / sizeof(vars0[0]),
                        options->IdentationLevel);
    StrBuilder_Destroy(&strid);
    for (int i = 0; i < (int)map.nHashTableSize; i++)
    {
        if (map.pHashTable[i])
        {
            struct StrBuilder idvalue = STRBUILDER_INIT;
            FindIDValue(pSyntaxTree,
                        (const char*)map.pHashTable[i]->Key,
                        &idvalue);
            struct TemplateVar vars[] =
            {
                {"type", (const char*)map.pHashTable[i]->Key},
                {"suffix", functionSuffix},
                {"value", idvalue.c_str},
                {"args", args.c_str}
            };
            //2 is struct
            StrBuilder_Template(fp,
                                " case $value:\n"
                                "  $type\b_$suffix((struct $type*)$args);\n"
                                " break;\n",
                                vars,
                                sizeof(vars) / sizeof(vars[0]),
                                options->IdentationLevel);
            StrBuilder_Destroy(&idvalue);
        }
    }
    StrBuilder_Template(fp,
                        " default:\n"
                        " break;\n"
                        "}\n",
                        NULL,
                        0,
                        options->IdentationLevel);
    StrBuilder_Destroy(&args);
#pragma message ( "leak?" )
    HashMap_Destroy(&map, NULL); //LEAK?
}


void UnionTypeDefault(struct SyntaxTree* pSyntaxTree,
                      struct PrintCodeOptions* options,
                      struct StructUnionSpecifier* pStructUnionSpecifier,
                      struct ParameterTypeList* pArgsOpt, //parametros
                      const char* parameterName0,
                      const char* functionSuffix,
                      struct StrBuilder* strImplementation,
                      struct StrBuilder* strDeclaration)
{
    if (!IsActive(options))
        return;
    char structName[200] = { 0 };
    GetOrGenerateStructTagName(pStructUnionSpecifier, structName, sizeof(structName));
    /*
    a declaracao dos tipos union eh sempre vazia
    e nos geramos o recheio da struct 1 vez
    verifica se ja foi gerado e caso nao tenha sido
    ele instancia o tipo (podemos usar isso p tipos template)
    */
    char buffer0[200] = { 0 };
    snprintf(buffer0, 200, "instance %s", structName);
    void* pv;
    bool bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, buffer0, &pv);
    if (!bAlreadyInstantiated)
    {
        struct StrBuilder strid = STRBUILDER_INIT;
        FindRuntimeID(pSyntaxTree, pStructUnionSpecifier, &strid);
        void* p0 = 0;
        HashMap_SetAt(&options->instantiations, buffer0, pv, &p0);
        StrBuilder_AppendFmt(&options->sbPreDeclaration, "\nstruct %s { int %s; };\n", structName, strid.c_str);
        StrBuilder_Destroy(&strid);
    }
    char buffer[200] = { 0 };
    strcat(buffer, structName);
    strcat(buffer, functionSuffix);
    bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, buffer, &pv);
    if (bAlreadyInstantiated)
    {
        return;// false;
    }
    void* p0;
    HashMap_SetAt(&options->instantiations, buffer, pv, &p0);
    StrBuilder_AppendFmt(strDeclaration, "static void %s_%s(struct %s* p);\n", structName, functionSuffix, structName);
    StrBuilder_AppendFmt(strImplementation, "\n");
    StrBuilder_AppendFmt(strImplementation, "static void %s_%s(struct %s* p)\n", structName, functionSuffix, structName);
    StrBuilder_Append(strImplementation, "{\n");
    struct HashMap map = HASHMAP_INIT;
    FindUnionSetOf(pSyntaxTree, pStructUnionSpecifier, &map);
    struct StrBuilder strid = STRBUILDER_INIT;
    FindRuntimeID(pSyntaxTree, pStructUnionSpecifier, &strid);
    struct StrBuilder args = STRBUILDER_INIT;
    if (pArgsOpt != NULL)
    {
        //lista argumentos separados por virgula
        ParameterTypeList_GetArgsString(pArgsOpt, &args);
    }
    else
    {
        StrBuilder_Append(&args, parameterName0);
    }
    struct TemplateVar vars0[] =
    {
        {"p", parameterName0},
        {"id", strid.c_str}
    };
    StrBuilder_Template(strImplementation,
                        "switch ($p->$id)\n"
                        "{\n",
                        vars0,
                        sizeof(vars0) / sizeof(vars0[0]),
                        1);
    StrBuilder_Destroy(&strid);
    for (int i = 0; i < (int)map.nHashTableSize; i++)
    {
        if (map.pHashTable[i])
        {
            struct StrBuilder idvalue = STRBUILDER_INIT;
            FindIDValue(pSyntaxTree,
                        (const char*)map.pHashTable[i]->Key,
                        &idvalue);
            struct TemplateVar vars[] =
            {
                {"type", (const char*)map.pHashTable[i]->Key},
                {"suffix", functionSuffix},
                {"value", idvalue.c_str},
                {"args", args.c_str}
            };
            //2 is struct
            StrBuilder_Template(strImplementation,
                                " case $value:\n"
                                "  $suffix\b_struct_\b$type\b_ptr((struct $type*)$args);\n"
                                " break;\n",
                                vars,
                                sizeof(vars) / sizeof(vars[0]),
                                1);
            StrBuilder_Destroy(&idvalue);
        }
    }
    StrBuilder_Template(strImplementation,
                        " default:\n"
                        " break;\n"
                        "}\n",
                        NULL,
                        0,
                        1);
    StrBuilder_Destroy(&args);
    StrBuilder_Append(strImplementation, "}\n");
#pragma message ( "leak?" )
    HashMap_Destroy(&map, NULL); //LEAK?
}


struct StructUnionSpecifier* GetStructSpecifier(struct SyntaxTree* pSyntaxTree, struct DeclarationSpecifiers* specifiers)
{
    if (specifiers == NULL)
        return NULL;
    struct StructUnionSpecifier* pTStructUnionSpecifier =
        DeclarationSpecifier_As_StructUnionSpecifier(specifiers->pData[0]);
    struct SingleTypeSpecifier* pSingleTypeSpecifier =
        DeclarationSpecifier_As_SingleTypeSpecifier(specifiers->pData[0]);
    if (pTStructUnionSpecifier == NULL)
    {
        if (pSingleTypeSpecifier != NULL &&
            pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
        {
            const char* typedefName = pSingleTypeSpecifier->TypedefName;
            struct Declaration* pDeclaration = SyntaxTree_GetFinalTypeDeclaration(pSyntaxTree, typedefName);
            if (pDeclaration)
            {
                if (pDeclaration->Specifiers.Size > 1)
                {
                    pTStructUnionSpecifier =
                        DeclarationSpecifier_As_StructUnionSpecifier(pDeclaration->Specifiers.pData[1]);
                }
            }
        }
    }
    //Procura pela definicao completa da struct
    if (pTStructUnionSpecifier &&
        pTStructUnionSpecifier->Tag != NULL)
    {
        pTStructUnionSpecifier =
            SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pTStructUnionSpecifier->Tag);
    }
    else  if (pSingleTypeSpecifier != NULL &&
              pSingleTypeSpecifier->Token2 == TK_STRUCT)
    {
        //Modelo C++ que o nome da struct ja eh suficiente
        pTStructUnionSpecifier =
            SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pSingleTypeSpecifier->TypedefName);
    }
    return pTStructUnionSpecifier;
}

void InstanciateInit(struct SyntaxTree* pSyntaxTree,
                     struct PrintCodeOptions* options,
                     struct SpecifierQualifierList* pSpecifierQualifierList,//<-dupla para entender o tipo
                     struct Declarator* pDeclatator,                        //<-dupla para entender o tipo
                     struct Initializer* pInitializerOpt, //usado para inicializacao estatica
                     const char* pInitExpressionText, //(x->p->i = 0)
                     bool* pbHasInitializers,
                     struct StrBuilder* fp)
{
    if (pInitializerOpt && pbHasInitializers)
    {
        *pbHasInitializers = true;
    }
    bool bIsPointerToObject = pDeclatator && PointerList_IsPointerToObject(&pDeclatator->PointerList);
    bool bIsAutoPointerToObject = pDeclatator && PointerList_IsAutoPointerToObject(&pDeclatator->PointerList);
    bool bIsAutoPointerToAutoPointer = pDeclatator && PointerList_IsAutoPointerToAutoPointer(&pDeclatator->PointerList);
    //bool bIsAutoPointerToPointer = TPointerList_IsAutoPointerToPointer(&pDeclatator->PointerList);
    //bool bIsPointer = pDeclatator && TPointerList_IsPointer(&pDeclatator->PointerList);
    struct DeclarationSpecifier* pMainSpecifier =
        SpecifierQualifierList_GetMainSpecifier(pSpecifierQualifierList);
    if (pMainSpecifier == NULL)
    {
        //error
        return;
    }
    if (pMainSpecifier->Type == SingleTypeSpecifier_ID)
    {
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            (struct SingleTypeSpecifier*)pMainSpecifier;
        if (pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
        {
            struct Declarator declarator = TDECLARATOR_INIT;
            //Pode ter uma cadeia de typdefs
            //ele vai entrandando em cada uma ...
            //ate que chega no fim recursivamente
            //enquanto ele vai andando ele vai tentando
            //algo com o nome do typedef
            struct DeclarationSpecifiers* pDeclarationSpecifiers =
                SymbolMap_FindTypedefFirstTarget(&pSyntaxTree->GlobalScope,
                                                 pSingleTypeSpecifier->TypedefName,
                                                 &declarator);
            if (pDeclarationSpecifiers)
            {
                for (struct Pointer* pItem = (&pDeclatator->PointerList)->pHead; pItem != NULL; pItem = pItem->pNext)
                {
                    struct Pointer* pNew = NEW((struct Pointer)POINTER_INIT);
                    Pointer_CopyFrom(pNew, pItem);
                    PointerList_PushBack(&declarator.PointerList, pNew);
                }
                //passa a informacao do tipo correto agora
                InstanciateInit(pSyntaxTree,
                                options,
                                (struct SpecifierQualifierList*)pDeclarationSpecifiers,
                                &declarator,
                                pInitializerOpt,
                                pInitExpressionText,
                                pbHasInitializers,
                                fp);
                Declarator_Destroy(&declarator);
            }
            else
            {
                //nao achou a declaracao
                //assert(false);
            }
        }
        else if (pSingleTypeSpecifier->Token2 == TK_STRUCT ||
                 pSingleTypeSpecifier->Token2 == TK_UNION)
        {
            struct StructUnionSpecifier* pStructUnionSpecifier = NULL;
            //Indica se consegui fazer sem entrar na struct
            bool bComplete = false;
            if (!bComplete) //se for para entrar na struct
            {
                if (pSingleTypeSpecifier &&
                    pSingleTypeSpecifier->TypedefName != NULL)
                {
                    //se nao eh completa tenta achar
                    //vou procurar a definicao completa da struct
                    pStructUnionSpecifier =
                        SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pSingleTypeSpecifier->TypedefName);
                }
                /////////////////////////////////////////////////////////////////////////////////////////////
                //DAQUI para baixo o codigo eh todo igual ao da struct
                //COMPARTILHAR
                bool bIsUnionTypes = pStructUnionSpecifier &&
                    pStructUnionSpecifier->UnionSet.pHead != NULL;
                if (pStructUnionSpecifier &&
                    pStructUnionSpecifier->StructDeclarationList.Size > 0)
                {
                    StrBuilder_AppendIdent(fp, 4 * options->IdentationLevel, "{");
                    if (bIsUnionTypes)
                    {
                    }
                    else
                    {
                        int variableCount = 0;
                        //ok tem a definicao completa da struct
                        for (int i = 0; i < pStructUnionSpecifier->StructDeclarationList.Size; i++)
                        {
                            struct AnyStructDeclaration* pAnyStructDeclaration =
                                pStructUnionSpecifier->StructDeclarationList.pItems[i];
                            struct StructDeclaration* pStructDeclaration =
                                AnyStructDeclaration_As_StructDeclaration(pAnyStructDeclaration);
                            if (pStructDeclaration != NULL)
                            {
                                struct InitDeclarator* pStructDeclarator =
                                    pStructDeclaration->DeclaratorList.pHead;
                                struct StrBuilder strVariableName = STRBUILDER_INIT;
                                struct StrBuilder strPonterSizeExpr = STRBUILDER_INIT;
                                while (pStructDeclarator)
                                {
                                    if (variableCount > 0)
                                    {
                                        StrBuilder_Append(fp, ", ");
                                    }
                                    variableCount++;
                                    //O padrao eh ser o inicializador do tipo
                                    struct Initializer* pStructMemberInitializer =
                                        pStructDeclarator->pInitializer;
                                    struct PrimaryExpressionValue initializerExpression = PRIMARYEXPRESSIONVALUE_INIT;
                                    StrBuilder_Clear(&strVariableName);
                                    StrBuilder_Clear(&strPonterSizeExpr);
                                    const char* structDeclaratorName =
                                        Declarator_GetName(pStructDeclarator->pDeclarator);
                                    StrBuilder_Append(&strVariableName, ".");
                                    StrBuilder_Append(&strVariableName, structDeclaratorName);
                                    InstanciateInit(pSyntaxTree,
                                                    options,
                                                    &pStructDeclaration->SpecifierQualifierList,
                                                    pStructDeclarator->pDeclarator,
                                                    pStructMemberInitializer,
                                                    strVariableName.c_str,
                                                    pbHasInitializers,
                                                    fp);
                                    //Variavel local
                                    PrimaryExpressionValue_Destroy(&initializerExpression);
                                    pStructDeclarator = (pStructDeclarator)->pNext;
                                }
                                StrBuilder_Destroy(&strVariableName);
                                StrBuilder_Destroy(&strPonterSizeExpr);
                            }
                        }
                    }
                    StrBuilder_AppendIdent(fp, 4 * options->IdentationLevel, "}");
                }
                else
                {
                    //error nao tem a definicao completa da struct
                    StrBuilder_AppendFmt(fp, "/*incomplete type %s*/\n", pInitExpressionText);
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////
            }//complete
        }
        else if (pSingleTypeSpecifier->Token2 == TK_ENUM)
        {
            //sample
            //enum E {A}; E* E_Create();
        }
        else
        {
            //nao eh typedef, deve ser int, double etc..
            if (pInitializerOpt)
            {
                StrBuilder_AppendFmtIdent(fp, 4 * options->IdentationLevel, "/*%s=*/", pInitExpressionText);
                struct PrintCodeOptions options2 = *options;
                TInitializer_CodePrint(pSyntaxTree, &options2, pDeclatator, (struct DeclarationSpecifiers*)pSpecifierQualifierList, pInitializerOpt, fp);
            }
            else
            {
                if (bIsPointerToObject || bIsAutoPointerToObject || bIsAutoPointerToAutoPointer)
                {
                    StrBuilder_AppendFmt(fp, "/*%s=*/%s", pInitExpressionText, GetNullStr(pSyntaxTree));
                }
                else
                {
                    StrBuilder_AppendFmt(fp, "/*%s=*/0", pInitExpressionText);
                }
            }
        }
    }
    else if (pMainSpecifier->Type == StructUnionSpecifier_ID)
    {
        struct StructUnionSpecifier* pStructUnionSpecifier =
            (struct StructUnionSpecifier*)pMainSpecifier;
        if (bIsPointerToObject || bIsAutoPointerToObject || bIsAutoPointerToAutoPointer)
        {
            StrBuilder_AppendFmt(fp, "%s", GetNullStr(pSyntaxTree));
        }
        else
        {
            /*
              vou ter que instanciar uma arvore toda da struct
              depois fazer override de cada item que tem designed initializer
            */
            InstantiateInitForStruct(pSyntaxTree, pStructUnionSpecifier, options, &options->sbPreDeclaration);
            char buffer[200] = { 0 };
            strcatupper(buffer, pStructUnionSpecifier->Tag);
            strcat(buffer, "_INIT");
            StrBuilder_AppendIdent(fp, 4 * options->IdentationLevel, buffer);
        }
    }
    else if (pMainSpecifier->Type == EnumSpecifier_ID)
    {
        struct EnumSpecifier* pEnumSpecifier =
            DeclarationSpecifier_As_EnumSpecifier(pMainSpecifier);
        pEnumSpecifier =
            SymbolMap_FindCompleteEnumSpecifier(&pSyntaxTree->GlobalScope, pEnumSpecifier->Tag);
        if (pEnumSpecifier == NULL)
        {
            assert(false);
        }
        if (pInitializerOpt)
        {
            StrBuilder_AppendFmtIdent(fp, 4 * options->IdentationLevel, "%s = ", pInitExpressionText);
            struct PrintCodeOptions options2 = *options;
            TInitializer_CodePrint(pSyntaxTree, &options2, pDeclatator, (struct DeclarationSpecifiers*)pSpecifierQualifierList, pInitializerOpt, fp);
        }
        else
        {
            /*coloca primeiro valor do enum*/
            const char* firstValue =
                pEnumSpecifier->EnumeratorList.pHead ? pEnumSpecifier->EnumeratorList.pHead->Name :
                "0";
            if (bIsPointerToObject || bIsAutoPointerToObject || bIsAutoPointerToAutoPointer)
            {
                StrBuilder_AppendFmt(fp, "/*%s=*/%s", pInitExpressionText, GetNullStr(pSyntaxTree));
            }
            else
            {
                StrBuilder_AppendFmt(fp, "/*%s=*/%s", pInitExpressionText, firstValue);
            }
        }
    }
    else
    {
        //assert(false);
    }
}


void InstanciateDestroy(struct SyntaxTree* pSyntaxTree,
                        struct PrintCodeOptions* options,
                        int identation,
                        struct SpecifierQualifierList* pSpecifierQualifierList,//<-dupla para entender o tipo
                        struct Declarator* pDeclatator,                        //<-dupla para entender o tipo
                        const char* pInitExpressionText,
                        bool bPrintStatementEnd,
                        struct StrBuilder* strInstantiations)
{
    if (!IsActive(options))
        return;
    bool bIsPointerToObject = PointerList_IsPointerToObject(&pDeclatator->PointerList);
    bool bIsAutoPointerToObject = PointerList_IsAutoPointerToObject(&pDeclatator->PointerList);
    //bool bIsPointer = TPointerList_IsPointer(&pDeclatator->PointerList);
    struct DeclarationSpecifier* pMainSpecifier =
        SpecifierQualifierList_GetMainSpecifier(pSpecifierQualifierList);
    if (pMainSpecifier == NULL)
    {
        //error
        return;
    }
    if (pMainSpecifier->Type == SingleTypeSpecifier_ID)
    {
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            (struct SingleTypeSpecifier*)pMainSpecifier;
        if (pSingleTypeSpecifier->TypedefName != NULL)
        {
            //bool bComplete = false;
            struct Declarator declarator = TDECLARATOR_INIT;
            //Pode ter uma cadeia de typdefs
            //ele vai entrandando em cada uma ...
            //ate que chega no fim recursivamente
            //enquanto ele vai andando ele vai tentando
            //algo com o nome do typedef
            struct DeclarationSpecifiers* pDeclarationSpecifiers =
                SymbolMap_FindTypedefFirstTarget(&pSyntaxTree->GlobalScope,
                                                 pSingleTypeSpecifier->TypedefName,
                                                 &declarator);
            if (pDeclarationSpecifiers)
            {
                /*adiciona outros modificadores*/
                for (struct Pointer* pItem = (&pDeclatator->PointerList)->pHead; pItem != NULL; pItem = pItem->pNext)
                {
                    struct Pointer* pNew = NEW((struct Pointer)POINTER_INIT);
                    Pointer_CopyFrom(pNew, pItem);
                    PointerList_PushBack(&declarator.PointerList, pNew);
                }
                //passa a informacao do tipo correto agora
                InstanciateDestroy(pSyntaxTree,
                                   options,
                                   0,
                                   (struct SpecifierQualifierList*)pDeclarationSpecifiers,
                                   &declarator,
                                   pInitExpressionText,
                                   bPrintStatementEnd,
                                   strInstantiations);
            }
            Declarator_Destroy(&declarator);
        }
        else
        {
            //nao eh typedef, deve ser int, double etc..
            if (bIsAutoPointerToObject)
            {
                StrBuilder_AppendFmtLn(strInstantiations, identation * 4, "%s((void*)%s);", GetFreeStr(pSyntaxTree), pInitExpressionText);
            }
        }
    }
    else if (pMainSpecifier->Type == StructUnionSpecifier_ID)
    {
        struct StructUnionSpecifier* pStructUnionSpecifier =
            (struct StructUnionSpecifier*)pMainSpecifier;
        if (bIsAutoPointerToObject)
        {
            char buffer[200] = { 0 };
            char structTagName[1000] = { 0 };
            GetOrGenerateStructTagName(pStructUnionSpecifier, structTagName, sizeof(structTagName));
            strcat(buffer, structTagName);
            strcat(buffer, "_Delete");
            StrBuilder_AppendFmtIdent(strInstantiations, identation * 4, "%s(%s)", buffer, pInitExpressionText);
            if (bPrintStatementEnd)
            {
                StrBuilder_Append(strInstantiations, ";");
                StrBuilder_Append(strInstantiations, "\n");
            }
            InstantiateDeleteForStruct(pSyntaxTree, pStructUnionSpecifier, options, &options->sbInstantiations, &options->sbPreDeclaration);
        }
        else if (bIsPointerToObject)
        {
            //warning
        }
        else
        {
            assert(pStructUnionSpecifier->Tag);
            char structTag[200] = { 0 };
            GetOrGenerateStructTagName(pStructUnionSpecifier, structTag, sizeof(structTag));
            char buffer[200] = { 0 };
            strcat(buffer, structTag);
            strcat(buffer, "_Destroy");
            StrBuilder_AppendFmtIdent(strInstantiations, identation * 4, "%s(&%s)", buffer, pInitExpressionText);
            if (bPrintStatementEnd)
            {
                StrBuilder_Append(strInstantiations, ";");
                StrBuilder_Append(strInstantiations, "\n");
            }
            InstantiateDestroyForStruct(pSyntaxTree, pStructUnionSpecifier, options, &options->sbInstantiations, &options->sbPreDeclaration);
        }
    }
    else if (pMainSpecifier->Type == EnumSpecifier_ID)
    {
        struct EnumSpecifier* pEnumSpecifier =
            DeclarationSpecifier_As_EnumSpecifier(pMainSpecifier);
        pEnumSpecifier =
            SymbolMap_FindCompleteEnumSpecifier(&pSyntaxTree->GlobalScope, pEnumSpecifier->Tag);
        if (bIsAutoPointerToObject)
        {
            StrBuilder_AppendFmtLn(strInstantiations, identation * 4, "%s((void*)%s);", GetFreeStr(pSyntaxTree), pInitExpressionText);
        }
    }
}



struct Declaration* FindOverloadedFunction(struct SymbolMap* pMap, const char* name, const char* nameMangling)
{
    struct Declaration* pDeclaration = NULL;
    if (pMap->pHashTable != NULL)
    {
        unsigned int nHashBucket, HashValue;
        struct SymbolMapItem* pKeyValue =
            SymbolMap_GetAssocAt(pMap,
                                 name,
                                 &nHashBucket,
                                 &HashValue);
        while (pKeyValue != NULL)
        {
            //Obs enum struct e union compartilham um mapa unico
            if (pKeyValue->pValue->Type == Declaration_ID)
            {
                if (strcmp(pKeyValue->Key, name) == 0)
                {
                    pDeclaration =
                        (struct Declaration*)pKeyValue->pValue;
                    struct InitDeclarator* pInitDeclarator =
                        pDeclaration->InitDeclaratorList.pHead;
                    if (pInitDeclarator &&
                        pInitDeclarator->pDeclarator &&
                        pInitDeclarator->pDeclarator->pDirectDeclarator)
                    {
                        if (strcmp(pInitDeclarator->pDeclarator->pDirectDeclarator->NameMangling, nameMangling) == 0)
                        {
                            return pDeclaration;
                        }
                    }
                }
            }
            pKeyValue = pKeyValue->pNext;
        }
    }
    return NULL;
}


/*
* Esta funÁ„o instancia o destroy da pStructUnionSpecifier e o todos os outros
* destroys utilizados por ela colocando a resposta em fp na ordem
*/
bool InstantiateDestroyForStruct(struct SyntaxTree* pSyntaxTree,
                                 struct StructUnionSpecifier* pStructUnionSpecifier,
                                 struct PrintCodeOptions* options,
                                 struct StrBuilder* strInstantiations,
                                 struct StrBuilder* strDeclarations)
{
    if (!IsActive(options))
        return false;
    if (pStructUnionSpecifier->Tag != NULL)
    {
        pStructUnionSpecifier =
            SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pStructUnionSpecifier->Tag);
    }
    if (pStructUnionSpecifier == NULL)
    {
        //inventar um nome???? inventar nome no parser?
        return false;
    }
    char structTagName[100] = { 0 };
    GetOrGenerateStructTagName(pStructUnionSpecifier, structTagName, sizeof(structTagName));
    char buffer[200] = { 0 };
    strcat(buffer, structTagName);
    strcat(buffer, "_Destroy");
    void* pv;
    bool bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, buffer, &pv);
    if (bAlreadyInstantiated)
    {
        return false;
    }
    /**/
    bool bIsUnionTypes = pStructUnionSpecifier &&
        pStructUnionSpecifier->UnionSet.pHead != NULL;
    if (bIsUnionTypes)
    {
        struct  StrBuilder sbLocal = STRBUILDER_INIT;
        StrBuilder_AppendFmt(&sbLocal, "\n");
        StrBuilder_AppendFmt(&sbLocal, "static void %s_Destroy(struct %s* p)\n", structTagName, structTagName);
        StrBuilder_Append(&sbLocal, "{\n");
        options->IdentationLevel = 1;
        InstantiateDestroyForPolymorphicType(pSyntaxTree,
                                             options,
                                             pStructUnionSpecifier,
                                             NULL, /*args*/
                                             &sbLocal);
        StrBuilder_AppendFmt(&sbLocal, "}\n");
        StrBuilder_Append(strInstantiations, sbLocal.c_str);
        StrBuilder_Destroy(&sbLocal);
        options->IdentationLevel = 0;
        StrBuilder_AppendFmt(strDeclarations, "static void %s_Destroy(struct %s* p);\n", structTagName, structTagName);
    }
    else if (pStructUnionSpecifier && pStructUnionSpecifier->StructDeclarationList.Size > 0)
    {
        options->IdentationLevel++;
        struct  StrBuilder sbLocal = STRBUILDER_INIT;
        StrBuilder_AppendFmt(&sbLocal, "\n");
        StrBuilder_AppendFmt(&sbLocal, "static void %s_Destroy(struct %s* p)\n", pStructUnionSpecifier->Tag, pStructUnionSpecifier->Tag);
        StrBuilder_Append(&sbLocal, "{\n");
        /*
        * conforme gerado pelo name manglind do overload
        * destroy_struct_%s_ptr
        */
        char nameMangled[200] = { 0 };
        snprintf(nameMangled, 200, "destroy_struct_%s_ptr", pStructUnionSpecifier->Tag);
        if (FindOverloadedFunction(&pSyntaxTree->GlobalScope, "destroy", nameMangled))
        {
            StrBuilder_AppendFmt(&sbLocal, "    %s(p);\n", nameMangled);
        }
        int variableCount = 0;
        //ok tem a definicao completa da struct
        for (int i = 0; i < pStructUnionSpecifier->StructDeclarationList.Size; i++)
        {
            struct AnyStructDeclaration* pAnyStructDeclaration =
                pStructUnionSpecifier->StructDeclarationList.pItems[i];
            struct StructDeclaration* pStructDeclaration =
                AnyStructDeclaration_As_StructDeclaration(pAnyStructDeclaration);
            if (pStructDeclaration != NULL)
            {
                struct InitDeclarator* pStructDeclarator =
                    pStructDeclaration->DeclaratorList.pHead;
                struct StrBuilder strVariableName = STRBUILDER_INIT;
                while (pStructDeclarator)
                {
                    variableCount++;
                    //O padrao eh ser o inicializador do tipo
                    //struct TInitializer* pStructMemberInitializer =
                    //  pStructDeclarator->pInitializer;
                    struct PrimaryExpressionValue initializerExpression = PRIMARYEXPRESSIONVALUE_INIT;
                    StrBuilder_Clear(&strVariableName);
                    const char* structDeclaratorName =
                        Declarator_GetName(pStructDeclarator->pDeclarator);
                    StrBuilder_Set(&strVariableName, "p->");
                    StrBuilder_Append(&strVariableName, structDeclaratorName);
                    //Se for destroy e sor
                    InstanciateDestroy(pSyntaxTree,
                                       options,
                                       1,
                                       &pStructDeclaration->SpecifierQualifierList,
                                       pStructDeclarator->pDeclarator,
                                       strVariableName.c_str,
                                       true,
                                       &sbLocal);
                    //Variavel local
                    PrimaryExpressionValue_Destroy(&initializerExpression);
                    pStructDeclarator = (pStructDeclarator)->pNext;
                }
                StrBuilder_Destroy(&strVariableName);
            }
        }
        StrBuilder_AppendFmt(&sbLocal, "}\n");
        StrBuilder_Append(strInstantiations, sbLocal.c_str);
        //deveria ser so para a primeira
        StrBuilder_AppendFmt(strDeclarations, "static void %s_Destroy(struct %s* p);\n", pStructUnionSpecifier->Tag, pStructUnionSpecifier->Tag);
        StrBuilder_Destroy(&sbLocal);
    }
    else
    {
        //error nao tem a definicao completa da struct
        StrBuilder_AppendFmt(strInstantiations, "/*incomplete type*/\n");
    }
    void* p0;
    HashMap_SetAt(&options->instantiations, buffer, pv, &p0);
    options->IdentationLevel--;
    return  true;
}


bool InstantiateDeleteForStruct(struct SyntaxTree* pSyntaxTree,
                                struct StructUnionSpecifier* pStructUnionSpecifier,
                                struct PrintCodeOptions* options,
                                struct StrBuilder* strInstantiations,
                                struct StrBuilder* strDeclarations)
{
    if (!IsActive(options))
        return false;
    char structTag[1000] = { 0 };
    GetOrGenerateStructTagName(pStructUnionSpecifier, structTag, sizeof(structTag));
    char buffer[200] = { 0 };
    strcat(buffer, structTag);
    strcat(buffer, "_Delete");
    void* pv;
    bool bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, buffer, &pv);
    if (bAlreadyInstantiated)
    {
        return false;
    }
    InstantiateDestroyForStruct(pSyntaxTree,
                                pStructUnionSpecifier,
                                options,
                                strInstantiations,
                                strDeclarations);
    /*injeta declaracao*/
    StrBuilder_AppendFmt(strDeclarations, "static void %s_Delete(struct %s* p);\n", structTag, structTag);
    /*injeta implementacao*/
    StrBuilder_AppendFmt(strInstantiations, "\n");
    StrBuilder_AppendFmt(strInstantiations, "static void %s_Delete(struct %s* p) {\n", structTag, structTag);
    StrBuilder_AppendFmt(strInstantiations, "    if (p) {\n");
    StrBuilder_AppendFmt(strInstantiations, "        %s_Destroy(p);\n", structTag);
    StrBuilder_AppendFmt(strInstantiations, "        %s(p); \n", GetFreeStr(pSyntaxTree));
    StrBuilder_AppendFmt(strInstantiations, "    }\n");
    StrBuilder_AppendFmt(strInstantiations, "}\n");
    void* p0;
    HashMap_SetAt(&options->instantiations, buffer, pv, &p0);
    return  true;
}



bool InstantiateInitForStruct(struct SyntaxTree* pSyntaxTree,
                              struct StructUnionSpecifier* pStructUnionSpecifier,
                              struct PrintCodeOptions* options,
                              struct StrBuilder* strInstantiations)
{
    if (pStructUnionSpecifier &&
        pStructUnionSpecifier->Tag != NULL)
    {
        /*procura definicao completa da struct*/
        pStructUnionSpecifier =
            SymbolMap_FindCompleteStructUnionSpecifier(&pSyntaxTree->GlobalScope, pStructUnionSpecifier->Tag);
    }
    if (pStructUnionSpecifier == NULL)
    {
        assert(false);
        return false;
    }
    /**/
    bool bIsUnionTypes = pStructUnionSpecifier && pStructUnionSpecifier->UnionSet.pHead != NULL;
    if (bIsUnionTypes)
    {
        options->IdentationLevel++;
        InstantiateDestroyForPolymorphicType(pSyntaxTree,
                                             options,
                                             pStructUnionSpecifier,
                                             NULL, /*args*/
                                             strInstantiations);
        options->IdentationLevel--;
    }
    else if (pStructUnionSpecifier && pStructUnionSpecifier->StructDeclarationList.Size > 0)
    {
        char buffer[200] = { 0 };
        strcatupper(buffer, pStructUnionSpecifier->Tag);
        strcat(buffer, "_INIT");
        void* pv;
        bool bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, buffer, &pv);
        if (bAlreadyInstantiated)
        {
            /*n„o precisa fazer nada*/
            return false;
        }
        void* p0;
        HashMap_SetAt(&options->instantiations, buffer, pv, &p0);
        options->IdentationLevel--;
        if (bIsUnionTypes)
        {
        }
        else
        {
            struct  StrBuilder sb = STRBUILDER_INIT;
            StrBuilder_AppendIdent(&sb, 4 * options->IdentationLevel, "{");
            int variableCount = 0;
            //ok tem a definicao completa da struct
            for (int i = 0; i < pStructUnionSpecifier->StructDeclarationList.Size; i++)
            {
                struct AnyStructDeclaration* pAnyStructDeclaration =
                    pStructUnionSpecifier->StructDeclarationList.pItems[i];
                struct StructDeclaration* pStructDeclaration =
                    AnyStructDeclaration_As_StructDeclaration(pAnyStructDeclaration);
                if (pStructDeclaration != NULL)
                {
                    struct InitDeclarator* pStructDeclarator =
                        pStructDeclaration->DeclaratorList.pHead;
                    struct StrBuilder strVariableName = STRBUILDER_INIT;
                    struct StrBuilder strPonterSizeExpr = STRBUILDER_INIT;
                    while (pStructDeclarator)
                    {
                        if (variableCount > 0)
                        {
                            StrBuilder_Append(&sb, ", ");
                        }
                        variableCount++;
                        //O padrao eh ser o inicializador do tipo
                        struct Initializer* pStructMemberInitializer =
                            pStructDeclarator->pInitializer;
                        struct PrimaryExpressionValue initializerExpression = PRIMARYEXPRESSIONVALUE_INIT;
                        StrBuilder_Clear(&strVariableName);
                        StrBuilder_Clear(&strPonterSizeExpr);
                        const char* structDeclaratorName =
                            Declarator_GetName(pStructDeclarator->pDeclarator);
                        StrBuilder_Append(&strVariableName, ".");
                        StrBuilder_Append(&strVariableName, structDeclaratorName);
                        //Este cara pode ter algo pInitializerOpt
                        bool bHasInitializers = false;
                        InstanciateInit(pSyntaxTree,
                                        options,
                                        &pStructDeclaration->SpecifierQualifierList,
                                        pStructDeclarator->pDeclarator,
                                        pStructMemberInitializer,
                                        strVariableName.c_str,
                                        &bHasInitializers,
                                        &sb);
                        PrimaryExpressionValue_Destroy(&initializerExpression);
                        pStructDeclarator = (pStructDeclarator)->pNext;
                    }
                    StrBuilder_Destroy(&strVariableName);
                    StrBuilder_Destroy(&strPonterSizeExpr);
                }
            }
            StrBuilder_AppendIdent(&sb, 4 * options->IdentationLevel, "}");
            StrBuilder_AppendFmt(strInstantiations, "#define %s ", buffer);
            StrBuilder_Append(strInstantiations, " ");
            StrBuilder_Append(strInstantiations, sb.c_str);
            StrBuilder_Append(strInstantiations, "\n");
        }
    }
    else
    {
        //error nao tem a definicao completa da struct
        StrBuilder_AppendFmt(strInstantiations, "/*incomplete type*/\n");
    }
    return  true;
}



void InstanciateNew(struct SyntaxTree* pSyntaxTree,
                    struct PrintCodeOptions* options,
                    struct SpecifierQualifierList* pSpecifierQualifierList,//<-dupla para entender o tipo
                    struct Declarator* pDeclatator,                        //<-dupla para entender o tipo
                    const char* pInitExpressionText,
                    bool bPrintStatementEnd,
                    struct StrBuilder* strInstantiations)
{
    if (!IsActive(options))
        return;
    //bool bIsPointerToObject = TPointerList_IsPointerToObject(&pDeclatator->PointerList);
    bool bIsAutoPointerToObject = PointerList_IsAutoPointerToObject(&pDeclatator->PointerList);
    //bool bIsPointer = TPointerList_IsPointer(&pDeclatator->PointerList);
    struct DeclarationSpecifier* pMainSpecifier =
        SpecifierQualifierList_GetMainSpecifier(pSpecifierQualifierList);
    if (pMainSpecifier == NULL)
    {
        //error
        return;
    }
    if (pMainSpecifier->Type == SingleTypeSpecifier_ID)
    {
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            (struct SingleTypeSpecifier*)pMainSpecifier;
        if (pSingleTypeSpecifier->TypedefName != NULL)
        {
            //bool bComplete = false;
            struct Declarator declarator = TDECLARATOR_INIT;
            //Pode ter uma cadeia de typdefs
            //ele vai entrandando em cada uma ...
            //ate que chega no fim recursivamente
            //enquanto ele vai andando ele vai tentando
            //algo com o nome do typedef
            struct DeclarationSpecifiers* pDeclarationSpecifiers =
                SymbolMap_FindTypedefFirstTarget(&pSyntaxTree->GlobalScope,
                                                 pSingleTypeSpecifier->TypedefName,
                                                 &declarator);
            if (pDeclarationSpecifiers)
            {
                /*adiciona outros modificadores*/
                for (struct Pointer* pItem = (&pDeclatator->PointerList)->pHead; pItem != NULL; pItem = pItem->pNext)
                {
                    struct Pointer* pNew = NEW((struct Pointer)POINTER_INIT);
                    Pointer_CopyFrom(pNew, pItem);
                    PointerList_PushBack(&declarator.PointerList, pNew);
                }
                //passa a informacao do tipo correto agora
                InstanciateNew(pSyntaxTree,
                               options,
                               (struct SpecifierQualifierList*)pDeclarationSpecifiers,
                               &declarator,
                               pInitExpressionText,
                               bPrintStatementEnd,
                               strInstantiations);
            }
            Declarator_Destroy(&declarator);
        }
        else
        {
            //nao eh typedef, deve ser int, double etc..
            if (bIsAutoPointerToObject)
            {
                StrBuilder_AppendFmtLn(strInstantiations, 0, "%s((void*)%s);", GetFreeStr(pSyntaxTree), pInitExpressionText);
            }
        }
    }
    else if (pMainSpecifier->Type == StructUnionSpecifier_ID)
    {
        //struct TStructUnionSpecifier* pStructUnionSpecifier =
        //  (struct TStructUnionSpecifier*)pMainSpecifier;
        //StrBuilder_AppendFmtLn(fp, 4 * options->IdentationLevel, "%s((void*)%s);", GetFreeStr(pSyntaxTree), pInitExpressionText);
    }
    else if (pMainSpecifier->Type == EnumSpecifier_ID)
    {
        struct EnumSpecifier* pEnumSpecifier =
            DeclarationSpecifier_As_EnumSpecifier(pMainSpecifier);
        pEnumSpecifier =
            SymbolMap_FindCompleteEnumSpecifier(&pSyntaxTree->GlobalScope, pEnumSpecifier->Tag);
        // StrBuilder_AppendFmtLn(fp, 4 * options->IdentationLevel, "%s((void*)%s);", GetFreeStr(pSyntaxTree), pInitExpressionText);
    }
}

void InstanciateVectorPush(struct SyntaxTree* pSyntaxTree,
                           struct PrintCodeOptions* options,
                           struct DeclarationSpecifiers* pDeclarationSpecifiers,
                           struct StrBuilder* fp)
{
    if (!IsActive(options))
        return;
    struct StrBuilder strType = STRBUILDER_INIT;
    TDeclarationSpecifiers_CodePrint(pSyntaxTree, options, pDeclarationSpecifiers, &strType);
    struct StrBuilder str = STRBUILDER_INIT;
    DeclarationSpecifiers_PrintNameMangling(pDeclarationSpecifiers, &str);
    char name[200] = { 0 };
    snprintf(name, 200, "vector_%s_push", str.c_str);
    StrBuilder_Append(fp, name);
    //StrBuilder_AppendFmtLn(strInstantiations, 0, "%s((void*)%s);", GetFreeStr(pSyntaxTree), pInitExpressionText);
    void* pv;
    bool bAlreadyInstantiated = HashMap_Lookup(&options->instantiations, name, &pv);
    if (!bAlreadyInstantiated)
    {
        void* p0;
        HashMap_SetAt(&options->instantiations, name, pv, &p0);
        StrBuilder_AppendFmt(&options->sbPreDeclaration, "static void %s(struct vector_%s * p, %s item);\n", name, str.c_str, strType.c_str);
        StrBuilder_AppendFmt(&options->sbInstantiations, "static void %s(struct vector_%s * p, %s item) {\n", name, str.c_str, strType.c_str);
        StrBuilder_AppendFmt(&options->sbInstantiations, "   if (p->size + 1 > p->capacity) {\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "       int n = p->capacity * 2;\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "       if (n == 0) n = 1; \n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "       void** pnew = p->data; \n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "       pnew = (void**)realloc(pnew, n * sizeof(void*));\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "       if (pnew) {\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "          p->data = pnew;\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "          p->capacity = n;\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "       }\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "        p->data[p->size] = item;\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "        p->size++;\n");
        StrBuilder_AppendFmt(&options->sbInstantiations, "      }\n");
        /*
        if (a->size + 1 > a->capacity)
        {
        int n = a->capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        void** pnew = a->data;
        pnew = (void**)realloc(pnew, n * sizeof(void*));
        if (pnew)
        {
            a->data = pnew;
            a->capacity = n;
        }
        }
        a->data[a->size] = value;
        a->size++;
        */
        StrBuilder_AppendFmt(&options->sbInstantiations, "}\n", name, str.c_str);
    }
    StrBuilder_Destroy(&str);
    StrBuilder_Destroy(&strType);
}

wchar_t BasicScanner_MatchChar(struct BasicScanner* scanner);


void BasicScanner_InitCore(struct BasicScanner* pBasicScanner)
{
    pBasicScanner->token = TK_BOF;
    pBasicScanner->pPrevious = NULL;
    pBasicScanner->FileIndex = -1;
    pBasicScanner->bLineStart = true;
    pBasicScanner->bMacroExpanded = false;
    StrBuilder_Init(&pBasicScanner->lexeme);
}

bool BasicScanner_Init(struct BasicScanner* pBasicScanner,
                       const char* name,
                       const char* Text)
{
    BasicScanner_InitCore(pBasicScanner);
    bool b = Stream_OpenString(&pBasicScanner->stream, Text);
    if (b)
    {
        strncpy(pBasicScanner->NameOrFullPath, name, CPRIME_MAX_PATH);
    }
    return b ? true : false;
}

bool BasicScanner_InitFile(struct BasicScanner* pBasicScanner,
                           const char* fileName)
{
    BasicScanner_InitCore(pBasicScanner);
    bool b = Stream_OpenFile(&pBasicScanner->stream, fileName);
    if (b)
    {
        strncpy(pBasicScanner->NameOrFullPath, fileName, CPRIME_MAX_PATH);
        GetFullDirS(fileName, pBasicScanner->FullDir);
    }
    return b ? true : false;
}

bool BasicScanner_Create(struct BasicScanner** pp,
                         const char* name,
                         const char* Text)
{
    bool result = false /*nomem*/;
    struct BasicScanner* p = (struct BasicScanner*)malloc(sizeof(struct BasicScanner));
    if (p)
    {
        result = BasicScanner_Init(p, name, Text);
        if (result == true)
        {
            *pp = p;
        }
        else
        {
            free(p);
        }
    }
    return result;
}

bool BasicScanner_CreateFile(const char* fileName, struct BasicScanner** pp)
{
    bool result = false /*nomem*/;
    struct BasicScanner* p = (struct BasicScanner*)malloc(sizeof(struct BasicScanner));
    if (p)
    {
        result = BasicScanner_InitFile(p, fileName);
        if (result == true)
        {
            *pp = p;
        }
        else
        {
            free(p);
        }
    }
    return result;
}


void BasicScanner_Destroy(struct BasicScanner* pBasicScanner)
{
    Stream_Destroy(&pBasicScanner->stream);
    StrBuilder_Destroy(&pBasicScanner->lexeme);
}

void BasicScanner_Delete(struct BasicScanner* pBasicScanner)
{
    if (pBasicScanner != NULL)
    {
        BasicScanner_Destroy(pBasicScanner);
        free((void*)pBasicScanner);
    }
}

struct TkPair
{
    const char* lexeme;
    enum TokenType token;
};

static struct TkPair singleoperators[] =
{
    //punctuator: one of

    {"[", TK_LEFT_SQUARE_BRACKET}, //0
    {"]", TK_RIGHT_SQUARE_BRACKET},
    {"(", TK_LEFT_PARENTHESIS},
    {")", TK_RIGHT_PARENTHESIS},
    {"{", TK_LEFT_CURLY_BRACKET},
    {"}", TK_RIGHT_CURLY_BRACKET},
    {".", TK_FULL_STOP},
    {"&", TK_AMPERSAND},
    {"*", TK_ASTERISK},
    {"+", TK_PLUS_SIGN},
    {"-", TK_HYPHEN_MINUS},
    {"~", TK_TILDE},
    {"!", TK_EXCLAMATION_MARK},
    {"%", TK_PERCENT_SIGN},
    {"<", TK_LESS_THAN_SIGN},
    {">", TK_GREATER_THAN_SIGN},
    {"^", TK_CIRCUMFLEX_ACCENT},
    {"|", TK_VERTICAL_LINE},
    {"?", TK_QUESTION_MARK},
    {":", TK_COLON},
    {";", TK_SEMICOLON},
    {"=", TK_EQUALS_SIGN},
    {",", TK_COMMA},
    {"$", TK_DOLLAR_SIGN},
    {"@", TK_COMMERCIAL_AT} //pode ser usado em macros pp-tokens
    //  {"...", TK_DOTDOTDOT},//50
    //  {"%:%:", TK_PERCENTCOLONPERCENTCOLON},
    //  {"<<=", TK_LESSLESSEQUAL},
    //{">>=", TK_GREATERGREATEREQUAL},
};

static struct TkPair doubleoperators[] =
{
    {"->", TK_ARROW},//25
    {"++", TK_PLUSPLUS},
    {"--", TK_MINUSMINUS},
    {"<<", TK_LESSLESS},
    {">>", TK_GREATERGREATER},
    {"<=", TK_LESSEQUAL},
    {">=", TK_GREATEREQUAL},
    {"==", TK_EQUALEQUAL},
    {"!=", TK_NOTEQUAL},
    {"&&", TK_ANDAND},
    {"||", TK_OROR},
    {"*=", TK_MULTIEQUAL},
    {"/=", TK_DIVEQUAL},
    {"%=", TK_PERCENT_EQUAL},
    {"+=", TK_PLUSEQUAL},
    {"-=", TK_MINUS_EQUAL},
    {"&=", TK_ANDEQUAL},
    {"^=", TK_CARETEQUAL},
    {"|=", TK_OREQUAL},
    {"##", TK_NUMBERNUMBER},
    {"<:", TK_LESSCOLON},
    {":>", TK_COLONGREATER},
    {"<%", TK_LESSPERCENT},
    {"%>", TK_PERCENTGREATER},
    {"%:", TK_PERCENTCOLON},

    {"...", TK_DOTDOTDOT},//50
    {"%:%:", TK_PERCENTCOLONPERCENTCOLON},
    {"<<=", TK_LESSLESSEQUAL},
    {">>=", TK_GREATERGREATEREQUAL}
};

static struct TkPair keywords[] =
{
    //keywords
    {"auto", TK_AUTO},
    {"break", TK_BREAK},
    {"case", TK_CASE},
    {"char", TK_CHAR},
    {"const", TK_CONST},
    {"continue", TK_CONTINUE},
    {"default", TK_DEFAULT},
    {"do", TK_DO},
    {"double", TK_DOUBLE},
    {"else", TK_ELSE},
    {"enum", TK_ENUM},
    {"extern", TK_EXTERN},
    {"float", TK_FLOAT},
    {"for", TK_FOR},
    {"goto", TK_GOTO},
    {"if", TK_IF},
    {"try", TK_TRY},
    {"defer", TK_DEFER},
    {"inline", TK_INLINE},
    {"__inline", TK__INLINE},
    {"__forceinline", TK__FORCEINLINE},
    {"int", TK_INT},
    {"long", TK_LONG},
    //
    {"__int8", TK__INT8},
    {"__int16", TK__INT16},
    {"__int32", TK__INT32},
    {"__int64", TK__INT64},
    {"__wchar_t", TK__WCHAR_T},
    {"__declspec", TK___DECLSPEC},
    //
    {"register", TK_REGISTER},
    {"restrict", TK_RESTRICT},
    {"return", TK_RETURN},
    {"throw", TK_THROW},
    {"short", TK_SHORT},
    {"signed", TK_SIGNED},
    {"sizeof", TK_SIZEOF},
    {"static", TK_STATIC},
    {"struct", TK_STRUCT},
    {"switch", TK_SWITCH},
    {"typedef", TK_TYPEDEF},
    {"union", TK_UNION},

    {"unsigned", TK_UNSIGNED},
    {"void", TK_VOID},
    {"volatile", TK_VOLATILE},
    {"while", TK_WHILE},
    {"catch", TK_CATCH},
    {"_Alignas", TK__ALIGNAS},
    {"_Alignof", TK__ALINGOF},
    {"_Atomic", TK__ATOMIC},

    {"_Bool", TK__BOOL},
    {"_Complex", TK__COMPLEX},
    {"_Generic", TK__GENERIC},
    {"_Imaginary", TK__IMAGINARY},
    {"_Noreturn", TK__NORETURN},
    {"_Static_assert", TK__STATIC_ASSERT},
    {"_Thread_local", TK__THREAD_LOCAL},
    //
    {"__asm", TK__ASM}, //visual studio
    {"__pragma", TK__PRAGMA}, //visual studio
    {"_Pragma", TK__C99PRAGMA}, //visual studio

    {"new", TK_NEW}
};
void BasicScanner_Next(struct BasicScanner* scanner);
void BasicScanner_Match(struct BasicScanner* scanner)
{
    BasicScanner_Next(scanner);
}

bool BasicScanner_MatchToken(struct BasicScanner* scanner, enum TokenType token)
{
    bool b = false;
    if (scanner->token == token)
    {
        b = true;
        BasicScanner_Match(scanner);
    }
    return b;
}


void BasicScanner_Next(struct BasicScanner* scanner)
{
    if (scanner->token == TK_MACRO_EOF ||
        scanner->token == TK_FILE_EOF)
    {
        scanner->token = TK_EOF;
        return;
    }
    //scanner->currentItem.Line = scanner->stream.Line;
    //scanner->currentItem.FileIndex = scanner->FileIndex;
    //bool bLineStart = scanner->bLineStart;
    //scanner->bLineStart = false;
    wchar_t ch = '\0';
    //Token_Reset(&scanner->currentItem);
    StrBuilder_Clear(&scanner->lexeme);
    scanner->token = TK_NONE;
    ch = scanner->stream.Character;
    wchar_t ch1 = Stream_LookAhead(&scanner->stream);
    wchar_t ch2 = Stream_LookAhead2(&scanner->stream);

    if (ch == '.' && ch1 == '.')
    {
        BasicScanner_MatchChar(scanner);
        ch = BasicScanner_MatchChar(scanner);
        if (ch != '.')
        {
            scanner->token = TK_ERROR;
        }
        BasicScanner_MatchChar(scanner);
        scanner->token = TK_DOTDOTDOT;
        return;
    }
    //procura por puncturares com 2 caracteres
    for (int i = 0; i < sizeof(doubleoperators) / sizeof(doubleoperators[0]); i++)
    {
        if (doubleoperators[i].lexeme[0] == ch &&
            doubleoperators[i].lexeme[1] == ch1)
        {
            scanner->token = doubleoperators[i].token;
            BasicScanner_MatchChar(scanner);
            BasicScanner_MatchChar(scanner);
            scanner->bLineStart = false;
            return;
        }
    }
    if (ch == '*' && ch1 == '/')
    {
        scanner->token = TK_CLOSE_COMMENT;
        ch = BasicScanner_MatchChar(scanner);
        ch = BasicScanner_MatchChar(scanner);
        return;
    }
    //procura por puncturtorscom 1 caracteres
    for (int i = 0; i < sizeof(singleoperators) / sizeof(singleoperators[0]); i++)
    {
        if (singleoperators[i].lexeme[0] == ch)
        {
            scanner->token = singleoperators[i].token;
            BasicScanner_MatchChar(scanner);
            scanner->bLineStart = false;
            return;
        }
    }
    //U'
    //u
    //L
    //Devido ao L' tem que vir antes do identificador
    //literal char
    if (ch == L'"' ||
        (ch == L'L' && ch1 == L'"') ||
        (ch == L'u' && ch1 == L'8') && ch2 == L'"')
    {
        if (ch == 'u')
        {
            ch = BasicScanner_MatchChar(scanner); //u
            ch = BasicScanner_MatchChar(scanner); //8
        }
        else if (ch == 'L')
        {
            ch = BasicScanner_MatchChar(scanner); //L
        }
        scanner->token = TK_STRING_LITERAL;
        ch = BasicScanner_MatchChar(scanner);
        for (;;)
        {
            if (ch == '\"')
            {
                ch = BasicScanner_MatchChar(scanner);
                break;
            }
            else if (ch == '\\')
            {
                //escape
                ch = BasicScanner_MatchChar(scanner);
                ch = BasicScanner_MatchChar(scanner);
            }
            else if (ch == '\0')
            {
                //oops
                scanner->token = TK_EOF;
                break;
            }
            else
            {
                ch = BasicScanner_MatchChar(scanner);
            }
        }
        scanner->bLineStart = false;
        return;
    }
    //Devido ao L' tem que vir antes do identificador
    //literal
    if (ch == L'\'' ||
        (ch == L'L' && ch1 == L'\''))
    {
        if (ch == 'L')
        {
            ch = BasicScanner_MatchChar(scanner); //L
        }
        scanner->token = TK_CHAR_LITERAL;
        ch = BasicScanner_MatchChar(scanner); //'
        if (ch == '\\')
        {
            //escape
            ch = BasicScanner_MatchChar(scanner); //
            ch = BasicScanner_MatchChar(scanner); //caractere
        }
        else
        {
            ch = BasicScanner_MatchChar(scanner);//caractere
        }
        ch = BasicScanner_MatchChar(scanner);//'
        scanner->bLineStart = false;
        return;
    }
    //Identificador
    if ((ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        ch == '_')
    {
        scanner->token = TK_IDENTIFIER;
        ch = BasicScanner_MatchChar(scanner);
        while ((ch >= 'a' && ch <= 'z') ||
               (ch >= 'A' && ch <= 'Z') ||
               (ch >= '0' && ch <= '9') ||
               ch == '_')
        {
            ch = BasicScanner_MatchChar(scanner);
        }
        //vÍ se È keywords e corrige o token
        for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
        {
            if (BasicScanner_IsLexeme(scanner, keywords[i].lexeme))
            {
                scanner->token = keywords[i].token;
                //StrBuilder_Append(&scanner->lexeme, keywords[i].lexeme);
                //Stream_Next(&scanner->stream);
                break;
            }
        }
        scanner->bLineStart = false;
        return;
    }
    //TODO binarios
    if (ch == '0' &&
        (
        (ch1 == 'x' || ch1 == 'X') || //hex
        (ch1 >= '0' && ch1 <= '9')) //octal
        )
    {
        ch = BasicScanner_MatchChar(scanner);
        if (ch == 'x' || ch == 'X')
        {
            scanner->token = TK_HEX_INTEGER;
        }
        else if (ch1 >= '0' && ch1 <= '9')
        {
            scanner->token = TK_OCTAL_INTEGER;
        }
        else
        {
            assert(false);
        }
        ch = BasicScanner_MatchChar(scanner);
        while ((ch >= '0' && ch <= '9') ||
               (ch >= 'A' && ch <= 'F') ||
               (ch >= 'a' && ch <= 'f'))
        {
            ch = BasicScanner_MatchChar(scanner);
        }
        //integer suffix
        if (ch == 'u' || ch == 'U')
        {
            ch = BasicScanner_MatchChar(scanner);
        }
        if (ch == 'l' || ch == 'L')
        {
            ch = BasicScanner_MatchChar(scanner);
            if (ch == 'l' || ch == 'L')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
            else if (ch == 'u' || ch == 'U')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
            else
            {
                //error
            }
        }
        else if (ch == 'i' || ch == 'I')
        {
            ch = BasicScanner_MatchChar(scanner);
            if (ch == '3')
            {
                ch = BasicScanner_MatchChar(scanner);
                if (ch == '2')
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
                else
                {
                    //error
                }
            }
            else if (ch == '1')
            {
                ch = BasicScanner_MatchChar(scanner);
                if (ch == '6')
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
                else
                {
                    //error
                }
            }
            else if (ch == '6')
            {
                ch = BasicScanner_MatchChar(scanner);
                if (ch == '4')
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
                else
                {
                    //error
                }
            }
            else if (ch == '8')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
        }
        return;
    }
    //numero
    if (ch >= '0' && ch <= '9')
    {
        scanner->token = TK_DECIMAL_INTEGER;
        ch = BasicScanner_MatchChar(scanner);
        while ((ch >= '0' && ch <= '9'))
        {
            ch = BasicScanner_MatchChar(scanner);
        }
        //integer suffix
        if (ch == 'u' || ch == 'U')
        {
            ch = BasicScanner_MatchChar(scanner);
        }
        if (ch == 'l' || ch == 'L')
        {
            ch = BasicScanner_MatchChar(scanner);
            if (ch == 'l' || ch == 'L')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
            else if (ch == 'u' || ch == 'U')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
            else
            {
                //error
            }
        }
        else if (ch == 'i' || ch == 'I')
        {
            ch = BasicScanner_MatchChar(scanner);
            if (ch == '3')
            {
                ch = BasicScanner_MatchChar(scanner);
                if (ch == '2')
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
                else
                {
                    //error
                }
            }
            else if (ch == '1')
            {
                ch = BasicScanner_MatchChar(scanner);
                if (ch == '6')
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
                else
                {
                    //error
                }
            }
            else if (ch == '6')
            {
                ch = BasicScanner_MatchChar(scanner);
                if (ch == '4')
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
                else
                {
                    //error
                }
            }
            else if (ch == '8')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
        }
        else
        {
            if (ch == '.')
            {
                ch = BasicScanner_MatchChar(scanner);
                scanner->token = TK_FLOAT_NUMBER;
                while ((ch >= '0' && ch <= '9'))
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
            }
            if (scanner->stream.Character == 'e' ||
                scanner->stream.Character == 'E')
            {
                ch = BasicScanner_MatchChar(scanner);
                if (ch == '-' ||
                    ch == '+')
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
                while ((ch >= '0' && ch <= '9'))
                {
                    ch = BasicScanner_MatchChar(scanner);
                }
            }
            if (ch == 'L' || ch == 'l' || ch == 'F' || ch == 'f')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
        }
        scanner->bLineStart = false;
        return;
    }
    //quebra de linha
    if (ch == '\n' || ch == L'\r')
    {
        scanner->token = TK_BREAKLINE;
        if (ch == L'\r' && ch1 == L'\n')
        {
            //so coloca \n
            Stream_Match(&scanner->stream);
            ch = scanner->stream.Character;
            ch = BasicScanner_MatchChar(scanner);
        }
        else
        {
            ch = BasicScanner_MatchChar(scanner);
            StrBuilder_Clear(&scanner->lexeme);
            //normaliza para windows?
            StrBuilder_Append(&scanner->lexeme, "\r\n");
        }
        scanner->bLineStart = true;
        return;
    }
    if (ch == '\0')
    {
        if (scanner->bMacroExpanded)
        {
            scanner->token = TK_MACRO_EOF;
        }
        else
        {
            scanner->token = TK_FILE_EOF;
        }
        scanner->bLineStart = false;
        return;
    }
    if (ch == '\f')
    {
        scanner->token = TK_SPACES;
        BasicScanner_MatchChar(scanner);
        return;
    }
    //espacos
    if (ch == ' ' || ch == '\t')
    {
        scanner->token = TK_SPACES;
        ch = BasicScanner_MatchChar(scanner);
        while (ch == ' ' || ch == '\t')
        {
            ch = BasicScanner_MatchChar(scanner);
        }
        //continua com scanner->bLineStart
        return;
    }
    if (ch < 32)
    {
        scanner->token = TK_SPACES;
    }
    //
    if (scanner->stream.Character == '#')
    {
        ch = BasicScanner_MatchChar(scanner);
        if (scanner->bLineStart)
        {
            scanner->token = TK_PREPROCESSOR;
        }
        else
        {
            scanner->token = TK_NUMBER_SIGN;
        }
        return;
    }
    //comentario de linha
    if (ch == '/')
    {
        if (ch1 == '/')
        {
            scanner->token = TK_LINE_COMMENT;
            ch = BasicScanner_MatchChar(scanner);
            ch = BasicScanner_MatchChar(scanner);
            while (ch != '\r' &&
                   ch != '\n' &&
                   ch != '\0')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
        }
        else if (ch1 == '*')
        {
            scanner->token = TK_COMMENT;
            ch = BasicScanner_MatchChar(scanner);
            ch = BasicScanner_MatchChar(scanner);
            if (ch == '@')
            {
                scanner->token = TK_OPEN_COMMENT;
                ch = BasicScanner_MatchChar(scanner);
            }
            else
            {
                for (;;)
                {
                    if (ch == '*')
                    {
                        ch = BasicScanner_MatchChar(scanner);
                        if (ch == '/')
                        {
                            ch = BasicScanner_MatchChar(scanner);
                            break;
                        }
                    }
                    else if (ch == L'\r')
                    {
                        //so coloca \n
                        Stream_Match(&scanner->stream);
                        ch = scanner->stream.Character;
                        if (ch == L'\n')
                        {
                            ch = BasicScanner_MatchChar(scanner);
                        }
                        else
                        {
                        }
                    }
                    else if (ch == L'\n')
                    {
                        ch = BasicScanner_MatchChar(scanner);
                    }
                    else
                    {
                        ch = BasicScanner_MatchChar(scanner);
                    }
                }
            }
            return;
        }
        else
        {
            scanner->token = TK_SOLIDUS;
            ch = BasicScanner_MatchChar(scanner);
        }
        return;
    }
    //junta linha
    if (ch == L'\\' &&
        (ch1 == L'\n' || ch1 == L'\r'))
    {
        //1) Whenever backslash appears at the end of
        //a line(immediately followed by the newline character), both
        //backslash and newline are deleted,
        //combining two physical source lines into one logical
        //source line.This is a single - pass operation;
        //a line ending in two backslashes followed by an empty
        //line does not combine three lines into one.
        //If a universal character name(\uXXX) is formed in this
        //phase, the behavior is undefined.
        ch = BasicScanner_MatchChar(scanner);
        if (ch == L'\r')
        {
            ch = BasicScanner_MatchChar(scanner);
            if (ch == L'\n')
            {
                ch = BasicScanner_MatchChar(scanner);
            }
        }
        else if (ch == L'\n')
        {
            BasicScanner_MatchChar(scanner);
        }
        //homogeiniza \r\n para \n
        StrBuilder_Set(&scanner->lexeme, "\\\n");
        scanner->token = TK_BACKSLASHBREAKLINE;
        scanner->bLineStart = false;
        //BasicScanner_Match(scanner);
        return;
    }
    if (ch == 2)  //peguei um
    {
        ch = BasicScanner_MatchChar(scanner);
        scanner->token = TK_MACROPLACEHOLDER;
        scanner->bLineStart = false;
        return;
    }
    if (ch == '\\')
    {
        ch = BasicScanner_MatchChar(scanner);
        scanner->token = REVERSE_SOLIDUS;
        return;
    }
    if (scanner->token == TK_ERROR)
    {
        //printf("invalid char, scanner");
        assert(false);
    }
    //assert(false);
}


enum TokenType BasicScanner_Token(struct BasicScanner* scanner)
{
    return scanner->token;
}

const char* BasicScanner_Lexeme(struct BasicScanner* scanner)
{
    return scanner->lexeme.c_str;
}

bool BasicScanner_IsLexeme(struct BasicScanner* scanner, const char* psz)
{
    return strcmp(BasicScanner_Lexeme(scanner), psz) == 0;
}

wchar_t BasicScanner_MatchChar(struct BasicScanner* scanner)
{
    StrBuilder_AppendChar(&scanner->lexeme,
                          (char)scanner->stream.Character);
    Stream_Match(&scanner->stream);
    return scanner->stream.Character;
}

//////////////////////////////////////////////

static bool IsPreprocessorTokenPhase(enum TokenType token)
/*todos estes tokens sao de uma fase anterior do parser*/
{
    return
        token == TK_SPACES ||
        token == TK_COMMENT ||
        token == TK_OPEN_COMMENT ||
        token == TK_CLOSE_COMMENT ||
        token == TK_LINE_COMMENT ||
        token == TK_BREAKLINE ||
        token == TK_BACKSLASHBREAKLINE ||
        //enum Tokens para linhas do pre processador
        token == TK_PRE_INCLUDE ||
        token == TK_PRE_PRAGMA ||
        token == TK_PRE_IF ||
        token == TK_PRE_ELIF ||
        token == TK_PRE_IFNDEF ||
        token == TK_PRE_IFDEF ||
        token == TK_PRE_ENDIF ||
        token == TK_PRE_ELSE ||
        token == TK_PRE_ERROR ||
        token == TK_PRE_LINE ||
        token == TK_PRE_UNDEF ||
        token == TK_PRE_DEFINE ||
        //fim tokens preprocessador
        token == TK_MACRO_CALL ||
        token == TK_MACRO_EOF ||
        token == TK_FILE_EOF;
}


#define List_Add(pList, pItem) \
if ((pList)->pHead == NULL) {\
    (pList)->pHead = (pItem); \
    (pList)->pTail = (pItem); \
}\
else {\
      (pList)->pTail->pNext = (pItem); \
      (pList)->pTail = (pItem); \
  }


bool IsAutoToken(enum TokenType token)
{
    return token == TK_AUTO;
}

bool IsSizeToken(enum TokenType token)
{
    // [Size]
    return token == TK_LEFT_SQUARE_BRACKET;
}

void AnyDeclaration_Delete(struct AnyDeclaration* pDeclaration);

void Declarations_Destroy(struct Declarations* p)
{
    for (int i = 0; i < p->Size; i++)
    {
        AnyDeclaration_Delete(p->pItems[i]);
    }
    free((void*)p->pItems);
}

void TDeclarations_Reserve(struct Declarations* p, int n)
{
    if (n > p->Capacity)
    {
        struct AnyDeclaration** pnew = p->pItems;
        pnew = (struct AnyDeclaration**)realloc(pnew, n * sizeof(struct AnyDeclaration*));
        if (pnew)
        {
            p->pItems = pnew;
            p->Capacity = n;
        }
    }
}
void Declarations_PushBack(struct Declarations* p, struct AnyDeclaration* pItem)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        TDeclarations_Reserve(p, n);
    }
    p->pItems[p->Size] = pItem;
    p->Size++;
}

void StructDeclarationList_Destroy(struct StructDeclarationList* p)
{
    for (int i = 0; i < p->Size; i++)
    {
        AnyStructDeclaration_Delete(p->pItems[i]);
    }
    free((void*)p->pItems);
}

void TStructDeclarationList_Reserve(struct StructDeclarationList* p, int n)
{
    if (n > p->Capacity)
    {
        struct AnyStructDeclaration** pnew = p->pItems;
        pnew = (struct AnyStructDeclaration**)realloc(pnew, n * sizeof(struct AnyStructDeclaration*));
        if (pnew)
        {
            p->pItems = pnew;
            p->Capacity = n;
        }
    }
}

void StructDeclarationList_PushBack(struct StructDeclarationList* p, struct AnyStructDeclaration* pItem)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        TStructDeclarationList_Reserve(p, n);
    }
    p->pItems[p->Size] = pItem;
    p->Size++;
}

void BlockItemList_Destroy(struct BlockItemList* p)
{
    for (int i = 0; i < p->size; i++)
    {
        BlockItem_Delete(p->data[i]);
    }
    free((void*)p->data);
}

void TBlockItemList_Reserve(struct BlockItemList* p, int n)
{
    if (n > p->capacity)
    {
        struct BlockItem** pnew = p->data;
        pnew = (struct BlockItem**)realloc(pnew, n * sizeof(struct BlockItem*));
        if (pnew)
        {
            p->data = pnew;
            p->capacity = n;
        }
    }
}

void BlockItemList_PushBack(struct BlockItemList* p, struct BlockItem* pItem)
{
    if (p->size + 1 > p->capacity)
    {
        int n = p->capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        TBlockItemList_Reserve(p, n);
    }
    p->data[p->size] = pItem;
    p->size++;
}

void CompoundStatement_Delete(struct CompoundStatement* p)
{
    if (p != NULL)
    {
        BlockItemList_Destroy(&p->BlockItemList);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        free((void*)p);
    }
}

void LabeledStatement_Delete(struct LabeledStatement* p)
{
    if (p != NULL)
    {
        Statement_Delete(p->pStatementOpt);
        Expression_Delete(p->pExpression);
        free((void*)p->Identifier);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        free((void*)p);
    }
}

void ForStatement_Delete(struct ForStatement* p)
{
    if (p != NULL)
    {
        AnyDeclaration_Delete(p->pInitDeclarationOpt);
        Expression_Delete(p->pExpression1);
        Expression_Delete(p->pExpression2);
        Expression_Delete(p->pExpression3);
        Statement_Delete(p->pStatement);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        TokenList_Destroy(&p->ClueList4);
        free((void*)p);
    }
}

void WhileStatement_Delete(struct WhileStatement* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pExpression);
        Statement_Delete(p->pStatement);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        free((void*)p);
    }
}

void DoStatement_Delete(struct DoStatement* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pExpression);
        Statement_Delete(p->pStatement);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        TokenList_Destroy(&p->ClueList4);
        free((void*)p);
    }
}

void TryBlockStatement_Delete(struct TryBlockStatement* p)
{
    if (p != NULL)
    {


        CompoundStatement_Delete(p->pCompoundStatement);
        CompoundStatement_Delete(p->pCompoundCatchStatement);

        TokenList_Destroy(&p->ClueListTry);
        TokenList_Destroy(&p->ClueListCatch);
        TokenList_Destroy(&p->ClueListLeftPar);
        TokenList_Destroy(&p->ClueListRightPar);

        free((void*)p);
    }
}


void ExpressionStatement_Delete(struct ExpressionStatement* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pExpression);
        TokenList_Destroy(&p->ClueList0);
        free((void*)p);
    }
}

void JumpStatement_Delete(struct JumpStatement* p)
{
    if (p != NULL)
    {
        free((void*)p->Identifier);
        Expression_Delete(p->pExpression);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        free((void*)p);
    }
}

void AsmStatement_Delete(struct AsmStatement* p)
{
    if (p != NULL)
    {
        TokenList_Destroy(&p->ClueList);
        free((void*)p);
    }
}

void SwitchStatement_Delete(struct SwitchStatement* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pConditionExpression);
        Statement_Delete(p->pExpression);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        free((void*)p);
    }
}

void IfStatement_Delete(struct IfStatement* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pInitialExpression);
        Expression_Delete(p->pConditionExpression);
        Statement_Delete(p->pStatement);
        Statement_Delete(p->pElseStatement);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        TokenList_Destroy(&p->ClueList4);
        TokenList_Destroy(&p->ClueList5);
        TokenList_Destroy(&p->ClueList6);
        TokenList_Destroy(&p->ClueListThrow);
        free((void*)p);
    }
}

void Statement_Delete(struct Statement* p)
{
    if (p != NULL)
    {
        switch (p->Type)
        {
            case ForStatement_ID:
                ForStatement_Delete((struct ForStatement*)p);
                break;
            case DeferStatement_ID:
                DeferStatement_Delete((struct DeferStatement*)p);
                break;
            case JumpStatement_ID:
                JumpStatement_Delete((struct JumpStatement*)p);
                break;
            case ExpressionStatement_ID:
                ExpressionStatement_Delete((struct ExpressionStatement*)p);
                break;
            case IfStatement_ID:
                IfStatement_Delete((struct IfStatement*)p);
                break;
            case TryBlockStatement_ID:
                TryBlockStatement_Delete((struct TryBlockStatement*)p);
                break;
            case WhileStatement_ID:
                WhileStatement_Delete((struct WhileStatement*)p);
                break;
            case SwitchStatement_ID:
                SwitchStatement_Delete((struct SwitchStatement*)p);
                break;
            case AsmStatement_ID:
                AsmStatement_Delete((struct AsmStatement*)p);
                break;
            case DoStatement_ID:
                DoStatement_Delete((struct DoStatement*)p);
                break;
            case LabeledStatement_ID:
                LabeledStatement_Delete((struct LabeledStatement*)p);
                break;
            case CompoundStatement_ID:
                CompoundStatement_Delete((struct CompoundStatement*)p);
                break;
            default:
                break;
        }
    }
}

void BlockItem_Delete(struct BlockItem* p)
{
    if (p != NULL)
    {
        switch (p->Type)
        {
            case ForStatement_ID:
                ForStatement_Delete((struct ForStatement*)p);
                break;
            case DeferStatement_ID:
                DeferStatement_Delete((struct DeferStatement*)p);
                break;
            case JumpStatement_ID:
                JumpStatement_Delete((struct JumpStatement*)p);
                break;
            case ExpressionStatement_ID:
                ExpressionStatement_Delete((struct ExpressionStatement*)p);
                break;
            case Declaration_ID:
                Declaration_Delete((struct Declaration*)p);
                break;
            case IfStatement_ID:
                IfStatement_Delete((struct IfStatement*)p);
                break;
            case TryBlockStatement_ID:
                TryBlockStatement_Delete((struct TryBlockStatement*)p);
                break;
            case WhileStatement_ID:
                WhileStatement_Delete((struct WhileStatement*)p);
                break;
            case SwitchStatement_ID:
                SwitchStatement_Delete((struct SwitchStatement*)p);
                break;
            case AsmStatement_ID:
                AsmStatement_Delete((struct AsmStatement*)p);
                break;
            case DoStatement_ID:
                DoStatement_Delete((struct DoStatement*)p);
                break;
            case LabeledStatement_ID:
                LabeledStatement_Delete((struct LabeledStatement*)p);
                break;
            case CompoundStatement_ID:
                CompoundStatement_Delete((struct CompoundStatement*)p);
                break;
            default:
                break;
        }
    }
}

void PrimaryExpressionValue_Destroy(struct PrimaryExpressionValue* p)
{
    free((void*)p->lexeme);
    TypeName_Destroy(&p->TypeName);
    Expression_Delete(p->pExpressionOpt);
    TokenList_Destroy(&p->ClueList0);
    TokenList_Destroy(&p->ClueList1);
}

void PrimaryExpressionValue_Delete(struct PrimaryExpressionValue* p)
{
    if (p != NULL)
    {
        PrimaryExpressionValue_Destroy(p);
        free((void*)p);
    }
}

void PrimaryExpressionLambda_Delete(struct PrimaryExpressionLambda* p)
{
    if (p != NULL)
    {
        TypeName_Destroy(&p->TypeName);

        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        free((void*)p);
    }
}

void ArgumentExpression_Delete(struct ArgumentExpression* p)
{
    if (p)
    {
        TokenList_Destroy(&p->ClueList0);
        Expression_Delete(p->pExpression);
        free(p);
    }
}

void ArgumentExpressionList_PushBack(struct ArgumentExpressionList* pArgumentExpressionList, struct ArgumentExpression* pItem)
{
    if (pArgumentExpressionList->size + 1 > pArgumentExpressionList->capacity)
    {
        int n = pArgumentExpressionList->capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        struct ArgumentExpression** pnew = pArgumentExpressionList->data;
        pnew = (struct ArgumentExpression**)realloc(pnew, n * sizeof(struct ArgumentExpression*));
        if (pnew)
        {
            pArgumentExpressionList->data = pnew;
            pArgumentExpressionList->capacity = n;
        }
    }
    pArgumentExpressionList->data[pArgumentExpressionList->size] = pItem;
    pArgumentExpressionList->size++;
}

void ArgumentExpressionList_Destroy(struct ArgumentExpressionList* pArgumentExpressionList)
{
    for (int i = 0; i < pArgumentExpressionList->size; i++)
    {
        ArgumentExpression_Delete(pArgumentExpressionList->data[i]);
    }
    free(pArgumentExpressionList->data);
}

void PostfixExpression_Delete(struct PostfixExpression* p)
{
    if (p != NULL)
    {
        ArgumentExpressionList_Destroy(&p->ArgumentExpressionList);
        TypeName_Destroy(&p->TypeName);
        free((void*)p->lexeme);
        Expression_Delete(p->pExpressionLeft);
        Expression_Delete(p->pExpressionRight);
        PostfixExpression_Delete(p->pNext);
        InitializerList_Destroy(&p->InitializerList);
        free((void*)p->Identifier);
        TypeName_Delete(p->pTypeName);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        TokenList_Destroy(&p->ClueList4);
        free((void*)p);
    }
}

void BinaryExpression_Delete(struct BinaryExpression* p)
{
    if (p != NULL)
    {
        TypeName_Destroy(&p->TypeName);
        Expression_Delete(p->pExpressionLeft);
        Expression_Delete(p->pExpressionRight);
        TokenList_Destroy(&p->ClueList0);
        free((void*)p);
    }
}

void UnaryExpressionOperator_Delete(struct UnaryExpressionOperator* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pExpressionRight);
        TypeName_Destroy(&p->TypeName);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        free((void*)p);
    }
}

void CastExpressionType_Delete(struct CastExpressionType* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pExpression);
        TypeName_Destroy(&p->TypeName);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        free((void*)p);
    }
}

void TernaryExpression_Delete(struct TernaryExpression* p)
{
    if (p != NULL)
    {
        TypeName_Destroy(&p->TypeName);
        Expression_Delete(p->pExpressionLeft);
        Expression_Delete(p->pExpressionMiddle);
        Expression_Delete(p->pExpressionRight);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        free((void*)p);
    }
}

void PrimaryExpressionLiteralItem_Delete(struct PrimaryExpressionLiteralItem* p)
{
    if (p != NULL)
    {
        free((void*)p->lexeme);
        TokenList_Destroy(&p->ClueList0);
        free((void*)p);
    }
}

void PrimaryExpressionLiteral_Delete(struct PrimaryExpressionLiteral* p)
{
    if (p != NULL)
    {
        TypeName_Destroy(&p->TypeName);
        PrimaryExpressionLiteralItemList_Destroy(&p->List);
        free((void*)p);
    }
}

struct TypeName* Expression_GetTypeName(struct Expression* p)
{
    switch (p->Type)
    {
        case BinaryExpression_ID:
            return &((struct BinaryExpression*)p)->TypeName;
        case PrimaryExpressionLambda_ID:
            return &((struct PrimaryExpressionLambda*)p)->TypeName;
        case UnaryExpressionOperator_ID:
            return &((struct UnaryExpressionOperator*)p)->TypeName;
        case CastExpressionType_ID:
            return &((struct CastExpressionType*)p)->TypeName;
        case PrimaryExpressionValue_ID:
            return &((struct PrimaryExpressionValue*)p)->TypeName;
        case PostfixExpression_ID:
            return &((struct PostfixExpression*)p)->TypeName;
        case PrimaryExpressionLiteral_ID:
            return &((struct PrimaryExpressionLiteral*)p)->TypeName;
        case TernaryExpression_ID:
            return &((struct TernaryExpression*)p)->TypeName;
        default:
            assert(false);
            break;
    }
    return NULL;
}

void DeferStatement_Delete(struct DeferStatement* p)
{
    if (p)
    {
        TokenList_Destroy(&p->ClueList0);
        Statement_Delete(p->pStatement);
    }
}

void Expression_Delete(struct Expression* p)
{
    if (p != NULL)
    {
        switch (p->Type)
        {
            case BinaryExpression_ID:
                BinaryExpression_Delete((struct BinaryExpression*)p);
                break;
            case PrimaryExpressionLambda_ID:
                PrimaryExpressionLambda_Delete((struct PrimaryExpressionLambda*)p);
                break;
            case UnaryExpressionOperator_ID:
                UnaryExpressionOperator_Delete((struct UnaryExpressionOperator*)p);
                break;
            case CastExpressionType_ID:
                CastExpressionType_Delete((struct CastExpressionType*)p);
                break;
            case PrimaryExpressionValue_ID:
                PrimaryExpressionValue_Delete((struct PrimaryExpressionValue*)p);
                break;
            case PostfixExpression_ID:
                PostfixExpression_Delete((struct PostfixExpression*)p);
                break;
            case PrimaryExpressionLiteral_ID:
                PrimaryExpressionLiteral_Delete((struct PrimaryExpressionLiteral*)p);
                break;
            case TernaryExpression_ID:
                TernaryExpression_Delete((struct TernaryExpression*)p);
                break;
            default:
                break;
        }
    }
}

void EofDeclaration_Delete(struct EofDeclaration* p)
{
    if (p != NULL)
    {
        TokenList_Destroy(&p->ClueList0);
        free((void*)p);
    }
}


void StaticAssertDeclaration_Delete(struct StaticAssertDeclaration* p)
{
    if (p != NULL)
    {
        Expression_Delete(p->pConstantExpression);
        free((void*)p->Text);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        TokenList_Destroy(&p->ClueList4);
        TokenList_Destroy(&p->ClueList5);
        free((void*)p);
    }
}

struct Enumerator* Enumerator_Clone(struct Enumerator* p)
{
    struct Enumerator* pNew = NEW((struct Enumerator)ENUMERATOR_INIT);
    pNew->bHasComma = p->bHasComma;
    pNew->pConstantExpression = NULL;
    pNew->Name = strdup(p->Name);
    return pNew;
}
void Enumerator_Delete(struct Enumerator* p)
{
    if (p != NULL)
    {
        free((void*)p->Name);
        Expression_Delete(p->pConstantExpression);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        free((void*)p);
    }
}

void EnumeratorList_Destroy(struct EnumeratorList* p)
{
    struct Enumerator* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct Enumerator* pItem = pCurrent;
        pCurrent = pCurrent->pNext;
        Enumerator_Delete(pItem);
    }
}

struct EnumSpecifier* EnumSpecifier_Clone(struct EnumSpecifier* p)
{
    struct EnumSpecifier* pNew = NEW((struct EnumSpecifier)ENUMSPECIFIER_INIT);
    pNew->Tag = p->Tag ? strdup(p->Tag) : NULL;
    for (struct Enumerator* pEnumerator = p->EnumeratorList.pHead; pEnumerator; pEnumerator = pEnumerator->pNext)
    {
        struct Enumerator* pNewEnumerator = Enumerator_Clone(pEnumerator);
        List_Add(&pNew->EnumeratorList, pNewEnumerator);
    }
    return pNew;
}

void EnumSpecifier_Delete(struct EnumSpecifier* p)
{
    if (p != NULL)
    {
        free((void*)p->Tag);
        EnumeratorList_Destroy(&p->EnumeratorList);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        free((void*)p);
    }
}

bool EnumSpecifier_IsSameTag(struct EnumSpecifier* p1, struct EnumSpecifier* p2)
{
    bool result = false;
    if (p1->Tag && p2->Tag && strcmp(p1->Tag, p2->Tag) == 0)
    {
        result = true;
    }
    return result;
}

struct UnionSetItem* UnionSetItem_Clone(struct UnionSetItem* p)
{
    struct UnionSetItem* pNew = NEW((struct UnionSetItem)TUNIONSETITEM_INIT);
    pNew->Name = strdup(p->Name);
    pNew->Token = p->Token;
    pNew->TokenFollow = p->TokenFollow;
    return pNew;
}
void UnionSetItem_Delete(struct UnionSetItem* p)
{
    if (p != NULL)
    {
        free((void*)p->Name);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        free((void*)p);
    }
}


void TUnionSet_CopyTo(struct UnionSet* from, struct UnionSet* to)
{
    for (struct UnionSetItem* p = from->pHead; p; p = p->pNext)
    {
        struct UnionSetItem* pNew = UnionSetItem_Clone(p);
        List_Add(to, pNew);
    }
}

void TUnionSet_Destroy(struct UnionSet* p)
{
    struct UnionSetItem* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct UnionSetItem* pItem = pCurrent;
        pCurrent = pCurrent->pNext;
        UnionSetItem_Delete(pItem);
    }
    TokenList_Destroy(&p->ClueList0);
    TokenList_Destroy(&p->ClueList1);
}


void TUnionSet_PushBack(struct UnionSet* pList, struct UnionSetItem* pItem)
{
    if (pList->pHead == NULL)
    {
        pList->pHead = (pItem);
    }
    else
    {
        pList->pTail->pNext = pItem;
    }
    pList->pTail = pItem;
}

struct StructUnionSpecifier* StructUnionSpecifier_Clone(struct StructUnionSpecifier* p)
{
    struct StructUnionSpecifier* pNew = NEW((struct StructUnionSpecifier)STRUCTUNIONSPECIFIER_INIT);
    pNew->Tag = p->Tag ? strdup(p->Tag) : NULL;
    pNew->Token = p->Token;
    TUnionSet_CopyTo(&p->UnionSet, &pNew->UnionSet);
    //TEM que copiar cada parta da struct! TODO
    //seia para o caso de structs anomimas
    //assert(false);
    return pNew;
}

void StructUnionSpecifier_Delete(struct StructUnionSpecifier* p)
{
    if (p != NULL)
    {
        StructDeclarationList_Destroy(&p->StructDeclarationList);
        free((void*)p->Tag);
        TUnionSet_Destroy(&p->UnionSet);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        TokenList_Destroy(&p->ClueList3);
        free((void*)p);
    }
}

void GetOrGenerateStructTagName(struct StructUnionSpecifier* p, char* out, int size)
{
    out[0] = 0;
    if (p->Tag != NULL)
    {
        strcpy(out, p->Tag);
    }
    else
    {
        struct UnionSetItem* pItem = p->UnionSet.pHead;
        if (pItem)
        {
            while (pItem)
            {
                strcat(out, pItem->Name);
                pItem = pItem->pNext;
            }
        }
        else
        {
            static int count = 0;
            sprintf(out, "_anonimous_struct_%d", count);
            count++;
        }
    }
}

bool TStructUnionSpecifier_CompareTagName(struct StructUnionSpecifier* p1, struct StructUnionSpecifier* p2)
{
    bool result = false;
    if (p1->Token == p2->Token)
    {
        if (p1->Tag && p2->Tag && strcmp(p1->Tag, p2->Tag) == 0)
        {
            result = true;
        }
    }
    return result;
}

int StrCmpNull(char const* s1, char const* s2)
{
    if (s1 == 0)
        return -1;
    return strcmp(s1, s2);
}


void TSingleTypeSpecifier_Destroy(struct SingleTypeSpecifier* p)
{
    free((void*)p->TypedefName);
    TokenList_Destroy(&p->ClueList0);
}

struct SingleTypeSpecifier* SingleTypeSpecifier_Clone(struct SingleTypeSpecifier* p)
{
    struct SingleTypeSpecifier* pNew = NEW((struct SingleTypeSpecifier)SINGLETYPESPECIFIER_INIT);
    pNew->Token2 = p->Token2;
    pNew->TypedefName = p->TypedefName ? strdup(p->TypedefName) : NULL;
    return pNew;
}

void SingleTypeSpecifier_Delete(struct SingleTypeSpecifier* p)
{
    if (p != NULL)
    {
        TSingleTypeSpecifier_Destroy(p);
        free((void*)p);
    }
}

bool SingleTypeSpecifier_Compare(struct SingleTypeSpecifier* p1, struct SingleTypeSpecifier* p2)
{
    bool result = false;
    if (p1->Token2 == p2->Token2)
    {
        if (p1->TypedefName &&
            p2->TypedefName)
        {
            if (strcmp(p1->TypedefName, p2->TypedefName) == 0)
            {
                result = true;
            }
        }
        else
        {
            result = true;
        }
    }
    return result;
}

const char* SingleTypeSpecifier_GetTypedefName(struct SingleTypeSpecifier* p)
{
    const char* result = NULL;
    if (p->Token2 == TK_IDENTIFIER)
    {
        result = p->TypedefName;
    }
    return result;
}

bool TypeSpecifier_Compare(struct TypeSpecifier* p1, struct TypeSpecifier* p2)
{
    bool result = false;
    if (p1->Type != p2->Type)
    {
        return false;
    }
    switch (p1->Type)
    {
        case StructUnionSpecifier_ID:
            result = TStructUnionSpecifier_CompareTagName((struct StructUnionSpecifier*)p1, (struct StructUnionSpecifier*)p2);
            break;
        case AtomicTypeSpecifier_ID:
            result = AtomicTypeSpecifier_Compare((struct AtomicTypeSpecifier*)p1, (struct AtomicTypeSpecifier*)p2);
            break;
        case SingleTypeSpecifier_ID:
            result = SingleTypeSpecifier_Compare((struct SingleTypeSpecifier*)p1, (struct SingleTypeSpecifier*)p2);
            break;
        case EnumSpecifier_ID:
            result = EnumSpecifier_IsSameTag((struct EnumSpecifier*)p1, (struct EnumSpecifier*)p2);
            break;
        default:
            break;
    }
    return result;
}

void TTypeSpecifier_Delete(struct TypeSpecifier* p)
{
    if (p != NULL)
    {
        switch (p->Type)
        {
            case StructUnionSpecifier_ID:
                StructUnionSpecifier_Delete((struct StructUnionSpecifier*)p);
                break;
            case AtomicTypeSpecifier_ID:
                AtomicTypeSpecifier_Delete((struct AtomicTypeSpecifier*)p);
                break;
            case SingleTypeSpecifier_ID:
                SingleTypeSpecifier_Delete((struct SingleTypeSpecifier*)p);
                break;
            case EnumSpecifier_ID:
                EnumSpecifier_Delete((struct EnumSpecifier*)p);
                break;
            default:
                break;
        }
    }
}


void Declarator_Destroy(struct Declarator* p)
{
    PointerList_Destroy(&p->PointerList);
    DirectDeclarator_Delete(p->pDirectDeclarator);
}

bool Declarator_IsAutoArray(struct Declarator* pDeclarator)
{
    return pDeclarator->pDirectDeclarator &&
        pDeclarator->pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeAutoArray;
}

void Declarator_Swap(struct Declarator* a, struct Declarator* b)
{
    struct Declarator t = *a;
    *a = *b;
    *b = t;
}

void Declarator_CopyTo(struct Declarator* from, struct Declarator* to)
{
    DirectDeclarator_Delete(to->pDirectDeclarator);
    to->pDirectDeclarator = NULL;
    if (from->pDirectDeclarator)
        to->pDirectDeclarator = DirectDeclarator_Clone(from->pDirectDeclarator);
    PointerList_CopyTo(&from->PointerList, &to->PointerList);
}

void Declarator_CopyToAbstractDeclarator(struct Declarator* from, struct Declarator* to)
{
    Declarator_CopyTo(from, to);
    if (to->pDirectDeclarator)
    {
        /*abstract declarator does not have name*/
        free(to->pDirectDeclarator->Identifier);
        to->pDirectDeclarator->Identifier = NULL;
    }
}

struct Declarator* Declarator_Clone(struct Declarator* p)
{
    struct Declarator* pNew = NEW((struct Declarator)TDECLARATOR_INIT);
    Declarator_CopyToAbstractDeclarator(p, pNew);
    return pNew;
}
void Declarator_Delete(struct Declarator* p)
{
    if (p != NULL)
    {
        Declarator_Destroy(p);
        free((void*)p);
    }
}


struct DirectDeclarator* DirectDeclarator_Clone(struct DirectDeclarator* p)
{
    struct DirectDeclarator* pNew = NEW((struct DirectDeclarator)TDIRECTDECLARATOR_INIT);
    pNew->NameMangling = p->NameMangling ? strdup(pNew->NameMangling) : NULL;
    pNew->Identifier = p->Identifier ? strdup(p->Identifier) : NULL;
    ParameterTypeList_CopyTo(&p->Parameters, &pNew->Parameters);
    pNew->DeclaratorType = p->DeclaratorType;
    if (p->pDeclarator)
    {
        pNew->pDeclarator = NEW((struct Declarator)TDECLARATOR_INIT);
        Declarator_CopyToAbstractDeclarator(p->pDeclarator, pNew->pDeclarator);
    }
    return pNew;
}


void TDeclarator_MergeTo(struct Declarator* from, struct Declarator* to)
{
    if (from->pDirectDeclarator)
    {
        DirectDeclarator_Delete(to->pDirectDeclarator);
        to->pDirectDeclarator = DirectDeclarator_Clone(from->pDirectDeclarator);
    }
    PointerList_CopyTo(&from->PointerList, &to->PointerList);
}
void TInitDeclarator_Destroy(struct InitDeclarator* p)
{
    Declarator_Delete(p->pDeclarator);
    Initializer_Delete(p->pInitializer);
    TokenList_Destroy(&p->ClueList0);
    TokenList_Destroy(&p->ClueList1);
}

void InitDeclarator_Delete(struct InitDeclarator* p)
{
    if (p != NULL)
    {
        TInitDeclarator_Destroy(p);
        free((void*)p);
    }
}


void ParameterTypeList_Destroy(struct ParameterTypeList* p)
{
    ParameterList_Destroy(&p->ParameterList);
    TokenList_Destroy(&p->ClueList0);
    TokenList_Destroy(&p->ClueList1);
}

void ParameterTypeList_CopyTo(struct ParameterTypeList* from, struct ParameterTypeList* to)
{
    to->bVariadicArgs = from->bVariadicArgs;
    to->bIsVoid = from->bIsVoid;
    for (struct Parameter* pParameter = from->ParameterList.pHead; pParameter; pParameter = pParameter->pNext)
    {
        struct Parameter* pParameterNew = Parameter_Clone(pParameter);
        List_Add(&to->ParameterList, pParameterNew);
    }
}

void ParameterTypeList_Delete(struct ParameterTypeList* p)
{
    if (p != NULL)
    {
        ParameterTypeList_Destroy(p);
        free((void*)p);
    }
}

const char* ParameterTypeList_GetFirstParameterName(struct ParameterTypeList* p)
{
    const char* name = "";
    if (p->ParameterList.pHead)
    {
        name = Declarator_GetName(&p->ParameterList.pHead->Declarator);
    }
    return name;
}

bool ParameterTypeList_HasNamedArgs(struct ParameterTypeList* p)
{
    bool result = false;
    if (p != NULL)
    {
        for (struct Parameter* pParameter = (&p->ParameterList)->pHead; pParameter != NULL; pParameter = pParameter->pNext)
        {
            const char* parameterName = Parameter_GetName(pParameter);
            if (parameterName != NULL)
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

void ParameterTypeList_GetArgsString(struct ParameterTypeList* p, struct StrBuilder* sb)
{
    if (p != NULL)
    {
        int index = 0;
        for (struct Parameter* pParameter = p->ParameterList.pHead;
             pParameter != NULL;
             pParameter = pParameter->pNext)
        {
            const char* parameterName = Parameter_GetName(pParameter);
            if (parameterName)
            {
                if (index > 0)
                {
                    StrBuilder_Append(sb, ", ");
                }
                StrBuilder_Append(sb, parameterName);
            }
            index++;
        }
    }
}

struct Parameter* ParameterTypeList_GetParameterByIndex(struct ParameterTypeList* p, int index)
{
    struct Parameter* pParameterResult = NULL;
    if (index == 0)
    {
        //A funcao ParameterTypeList_GetParameterByIndex deve retornar
        //null caso o primeiro parametro seja void. void Function(void)
        //
        if (p->ParameterList.pHead)
        {
            if (p->ParameterList.pHead->Specifiers.Size == 1 &&
                p->ParameterList.pHead->Specifiers.pData[0]->Type == SingleTypeSpecifier_ID)
            {
                struct SingleTypeSpecifier* pSingleTypeSpecifier =
                    (struct SingleTypeSpecifier*)p->ParameterList.pHead->Specifiers.pData[0];
                if (pSingleTypeSpecifier)
                {
                    if (pSingleTypeSpecifier->Token2 == TK_VOID)
                    {
                        if (p->ParameterList.pHead->Declarator.PointerList.pHead == 0)
                        {
                            return NULL;
                        }
                    }
                }
            }
        }
    }
    int indexLocal = 0;
    for (struct Parameter* pParameter = (&p->ParameterList)->pHead; pParameter != NULL; pParameter = pParameter->pNext)
    {
        if (indexLocal == index)
        {
            pParameterResult = pParameter;
            break;
        }
        indexLocal++;
    }
    return pParameterResult;
}

struct Parameter* ParameterTypeList_FindParameterByName(struct ParameterTypeList* p, const char* name)
{
    struct Parameter* pParameterResult = NULL;
    if (name)
    {
        for (struct Parameter* pParameter = p->ParameterList.pHead;
             pParameter != NULL;
             pParameter = pParameter->pNext)
        {
            //F(void) neste caso nao tem nome
            const char* parameterName = Parameter_GetName(pParameter);
            if (parameterName && strcmp(parameterName, name) == 0)
            {
                pParameterResult = pParameter;
                break;
            }
        }
    }
    return pParameterResult;
}

const char* ParameterTypeList_GetSecondParameterName(struct ParameterTypeList* p)
{
    const char* name = "";
    if (p->ParameterList.pHead &&
        p->ParameterList.pHead->pNext)
    {
        name = Declarator_GetName(&p->ParameterList.pHead->pNext->Declarator);
    }
    return name;
}

void TDirectDeclarator_Destroy(struct DirectDeclarator* p)
{
    free((void*)p->Identifier);
    free((void*)p->NameMangling);
    Declarator_Delete(p->pDeclarator);
    DirectDeclarator_Delete(p->pDirectDeclarator);
    ParameterTypeList_Destroy(&p->Parameters);
    Expression_Delete(p->pExpression);
    TokenList_Destroy(&p->ClueList0);
    TokenList_Destroy(&p->ClueList1);
    TokenList_Destroy(&p->ClueList2);
    TokenList_Destroy(&p->ClueList3);
    TokenList_Destroy(&p->ClueList4);
}


void DirectDeclarator_Delete(struct DirectDeclarator* p)
{
    if (p != NULL)
    {
        TDirectDeclarator_Destroy(p);
        free((void*)p);
    }
}

struct DeclarationSpecifier* DeclarationSpecifier_Clone(struct DeclarationSpecifier* pItem)
{
    switch (pItem->Type)
    {
        case TypeQualifier_ID:
            return (struct DeclarationSpecifier*)TTypeQualifier_Clone((struct TypeQualifier*)pItem);
        case StructUnionSpecifier_ID:
            return (struct DeclarationSpecifier*)StructUnionSpecifier_Clone((struct StructUnionSpecifier*)pItem);
        case StorageSpecifier_ID:
            return (struct DeclarationSpecifier*)StorageSpecifier_Clone((struct StorageSpecifier*)pItem);
        case AtomicTypeSpecifier_ID:
            return (struct DeclarationSpecifier*)AtomicTypeSpecifier_Clone((struct AtomicTypeSpecifier*)pItem);
        case SingleTypeSpecifier_ID:
            return (struct DeclarationSpecifier*)SingleTypeSpecifier_Clone((struct SingleTypeSpecifier*)pItem);
        case AlignmentSpecifier_ID:
            return (struct DeclarationSpecifier*)AlignmentSpecifier_Clone((struct AlignmentSpecifier*)pItem);
        case FunctionSpecifier_ID:
            return (struct DeclarationSpecifier*)FunctionSpecifier_Clone((struct FunctionSpecifier*)pItem);
        case EnumSpecifier_ID:
            return (struct DeclarationSpecifier*)EnumSpecifier_Clone((struct EnumSpecifier*)pItem);
        default:
            break;
    }
    assert(false);
    return NULL;
}

struct SpecifierQualifier* SpecifierQualifier_Clone(struct SpecifierQualifier* p);
void SpecifierQualifier_Delete(struct SpecifierQualifier* p);


struct DeclarationSpecifier* SpecifierQualifierList_GetMainSpecifier(struct SpecifierQualifierList* p)
{
    struct DeclarationSpecifier* pSpecifier = NULL;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifierQualifier = p->pData[i];
        if (pSpecifierQualifier->Type == SingleTypeSpecifier_ID ||
            pSpecifierQualifier->Type == StructUnionSpecifier_ID ||
            pSpecifierQualifier->Type == EnumSpecifier_ID)
        {
            //ATENCAO
            pSpecifier = (struct DeclarationSpecifier*)pSpecifierQualifier;
            break;
        }
    }
    return pSpecifier;
}

const char* SpecifierQualifierList_GetTypedefName(struct SpecifierQualifierList* p)
{
    const char* typedefName = NULL;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifierQualifier = p->pData[i];
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            SpecifierQualifier_As_SingleTypeSpecifier(pSpecifierQualifier);
        if (pSingleTypeSpecifier &&
            pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
        {
            typedefName = pSingleTypeSpecifier->TypedefName;
            break;
        }
    }
    return typedefName;
}

bool SpecifierQualifierList_Compare(struct SpecifierQualifierList* p1, struct SpecifierQualifierList* p2)
{
    if (p1->Size != p2->Size)
    {
        return false;
    }
    //bool bResult = false;
    for (int i = 0; i < p1->Size; i++)
    {
        if (p1->pData[i]->Type == p2->pData[i]->Type)
        {
            switch (p1->pData[i]->Type)
            {
                case SingleTypeSpecifier_ID:
                    if (!SingleTypeSpecifier_Compare((struct SingleTypeSpecifier*)p1->pData[i],
                        (struct SingleTypeSpecifier*)p2->pData[i]))
                    {
                        return false;
                    }
                    break;
                case StorageSpecifier_ID:
                    if (!StorageSpecifier_Compare((struct StorageSpecifier*)p1->pData[i],
                        (struct StorageSpecifier*)p2->pData[i]))
                    {
                        return false;
                    }
                    break;
                case TypeQualifier_ID:
                    if (!TypeQualifier_Compare((struct TypeQualifier*)p1->pData[i],
                        (struct TypeQualifier*)p2->pData[i]))
                    {
                        return false;
                    }
                    break;
                case FunctionSpecifier_ID:
                    if (!FunctionSpecifier_Compare((struct FunctionSpecifier*)p1->pData[i],
                        (struct FunctionSpecifier*)p2->pData[i]))
                    {
                        return false;
                    }
                    break;
                case StructUnionSpecifier_ID:
                    if (!TStructUnionSpecifier_CompareTagName((struct StructUnionSpecifier*)p1->pData[i],
                        (struct StructUnionSpecifier*)p2->pData[i]))
                    {
                        return false;
                    }
                    break;
                case EnumSpecifier_ID:
                    if (!EnumSpecifier_IsSameTag((struct EnumSpecifier*)p1->pData[i],
                        (struct EnumSpecifier*)p2->pData[i]))
                    {
                        return false;
                    }
                    break;
                default:
                    //assert(false);
                    break;
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool SpecifierQualifierList_IsTypedefQualifier(struct SpecifierQualifierList* p)
{
    bool bResult = false;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifierQualifier = p->pData[i];
        struct StorageSpecifier* pStorageSpecifier =
            SpecifierQualifier_As_StorageSpecifier(pSpecifierQualifier);
        if (pStorageSpecifier &&
            pStorageSpecifier->Token == TK_TYPEDEF)
        {
            bResult = true;
            break;
        }
    }
    return bResult;
}

bool SpecifierQualifierList_IsChar(struct SpecifierQualifierList* p)
{
    bool bResult = false;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifierQualifier = p->pData[i];
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            SpecifierQualifier_As_SingleTypeSpecifier(pSpecifierQualifier);
        if (pSingleTypeSpecifier &&
            pSingleTypeSpecifier->Token2 == TK_CHAR)
        {
            bResult = true;
            break;
        }
    }
    return bResult;
}


bool SpecifierQualifierList_IsAnyInteger(struct SpecifierQualifierList* p)
{
    bool bResult = false;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifierQualifier = p->pData[i];
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            SpecifierQualifier_As_SingleTypeSpecifier(pSpecifierQualifier);
        if (pSingleTypeSpecifier &&
            (pSingleTypeSpecifier->Token2 == TK_INT ||
            pSingleTypeSpecifier->Token2 == TK_SHORT ||
            pSingleTypeSpecifier->Token2 == TK_SIGNED ||
            pSingleTypeSpecifier->Token2 == TK_UNSIGNED ||
            pSingleTypeSpecifier->Token2 == TK__INT8 ||
            pSingleTypeSpecifier->Token2 == TK__INT16 ||
            pSingleTypeSpecifier->Token2 == TK__INT32 ||
            pSingleTypeSpecifier->Token2 == TK__INT64 ||
            pSingleTypeSpecifier->Token2 == TK__WCHAR_T)
            )
        {
            bResult = true;
            break;
        }
    }
    return bResult;
}


bool SpecifierQualifierList_IsAnyFloat(struct SpecifierQualifierList* p)
{
    bool bResult = false;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifierQualifier = p->pData[i];
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            SpecifierQualifier_As_SingleTypeSpecifier(pSpecifierQualifier);
        if (pSingleTypeSpecifier &&
            (pSingleTypeSpecifier->Token2 == TK_DOUBLE ||
            pSingleTypeSpecifier->Token2 == TK_FLOAT))
        {
            bResult = true;
            break;
        }
    }
    return bResult;
}

bool SpecifierQualifierList_IsBool(struct SpecifierQualifierList* p)
{
    bool bResult = false;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifierQualifier = p->pData[i];
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            SpecifierQualifier_As_SingleTypeSpecifier(pSpecifierQualifier);
        if (pSingleTypeSpecifier &&
            pSingleTypeSpecifier->Token2 == TK__BOOL)
        {
            bResult = true;
            break;
        }
    }
    return bResult;
}

const char* Declarator_GetName(struct Declarator* p)
{
    if (p == NULL)
    {
        return NULL;
    }
    struct DirectDeclarator* pDirectDeclarator = p->pDirectDeclarator;
    while (pDirectDeclarator != NULL)
    {
        if (pDirectDeclarator->Identifier != NULL &&
            pDirectDeclarator->Identifier[0] != 0)
        {
            return pDirectDeclarator->Identifier;
        }
        if (pDirectDeclarator->pDeclarator)
        {
            const char* name =
                Declarator_GetName(pDirectDeclarator->pDeclarator);
            if (name != NULL)
            {
                return name;
            }
        }
        pDirectDeclarator =
            pDirectDeclarator->pDirectDeclarator;
    }
    return NULL;
}

/*return true se tiver pelo menos 1 cara que seha auto array*/
bool HasAutoArray(struct InitDeclaratorList* p)
{
    bool bResult = false;
    struct InitDeclarator* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct InitDeclarator* pItem = pCurrent;
        if (Declarator_IsAutoArray(pItem->pDeclarator))
        {
            bResult = true;
            break;
        }
        pCurrent = pCurrent->pNext;
    }
    return bResult;
}

const char* InitDeclarator_FindName(struct InitDeclarator* p)
{
    //assert(p->pDeclarator != NULL);
    return Declarator_GetName(p->pDeclarator);
}


void TAlignmentSpecifier_Destroy(struct AlignmentSpecifier* p)
{
    free((void*)p->TypeName);
}


struct AlignmentSpecifier* AlignmentSpecifier_Clone(struct AlignmentSpecifier* p)
{
    struct AlignmentSpecifier* pNew = NEW((struct AlignmentSpecifier)ALIGNMENTSPECIFIER_INIT);
    //pNew->TypeName
    assert(false);//clone typename
    return pNew;
}

void AlignmentSpecifier_Delete(struct AlignmentSpecifier* p)
{
    if (p != NULL)
    {
        TAlignmentSpecifier_Destroy(p);
        free((void*)p);
    }
}

void StructDeclaratorList_Destroy(struct StructDeclaratorList* p)
{
    struct InitDeclarator* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct InitDeclarator* pItem = pCurrent;
        pCurrent = pCurrent->pNext;
        InitDeclarator_Delete(pItem);
    }
}

void StructDeclaratorList_Add(struct StructDeclaratorList* pList, struct InitDeclarator* pItem)
{
    if (pList->pHead == NULL)
    {
        pList->pHead = pItem;
        pList->pTail = pItem;
    }
    else
    {
        pList->pTail->pNext = pItem;
        pList->pTail = pItem;
    }
}

void TStructDeclaration_Destroy(struct StructDeclaration* p)
{
    SpecifierQualifierList_Destroy(&p->SpecifierQualifierList);
    StructDeclaratorList_Destroy(&p->DeclaratorList);
    TokenList_Destroy(&p->ClueList1);
}

void StructDeclaration_Delete(struct StructDeclaration* p)
{
    if (p != NULL)
    {
        TStructDeclaration_Destroy(p);
        free((void*)p);
    }
}

void AnyStructDeclaration_Delete(struct AnyStructDeclaration* p)
{
    if (p != NULL)
    {
        switch (p->Type)
        {
            case StaticAssertDeclaration_ID:
                StaticAssertDeclaration_Delete((struct StaticAssertDeclaration*)p);
                break;
            case StructDeclaration_ID:
                StructDeclaration_Delete((struct StructDeclaration*)p);
                break;
            default:
                break;
        }
    }
}

bool PointerList_IsAutoPointer(struct PointerList* pPointerlist)
{
    bool bIsPointer = false;
    bool bIsAuto = false;
    if (pPointerlist)
    {
        bIsPointer = (pPointerlist->pHead != NULL);
        //ForEachListItem(struct TPointer, pItem, pPointerlist)
        struct Pointer* pItem = pPointerlist->pHead;
        //for (T * var = (list)->pHead; var != NULL; var = var->pNext)
        while (pItem)
        {
            for (int i = 0; i < pItem->Qualifier.Size; i++)
            {
                struct TypeQualifier* pQualifier = pItem->Qualifier.Data[i];
                if (IsAutoToken(pQualifier->Token))
                {
                    bIsAuto = true;
                    break;
                }
            }
            if (bIsAuto && bIsPointer)
                break;
            pItem = pItem->pNext;
        }
    }
    return bIsAuto;
}

void PointerList_CopyTo(struct PointerList* from, struct PointerList* to)
{
    for (struct Pointer* p = from->pHead; p; p = p->pNext)
    {
        struct Pointer* pNew = NEW((struct Pointer)POINTER_INIT);
        Pointer_CopyFrom(pNew, p);
        PointerList_PushBack(to, pNew);
    }
}


void PointerList_Destroy(struct PointerList* p)
{
    struct Pointer* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct Pointer* pItem = pCurrent;
        pCurrent = pCurrent->pNext;
        Pointer_Delete(pItem);
    }
}

void TPointer_Destroy(struct Pointer* p)
{
    TypeQualifierList_Destroy(&p->Qualifier);
    TokenList_Destroy(&p->ClueList0);
}

void PointerList_PopFront(struct PointerList* pList)
{
    if (pList->pHead == NULL)
        return;
    struct Pointer* pFront = pList->pHead;
    pList->pHead = pList->pHead->pNext;
    Pointer_Delete(pFront);
}

void PointerList_PushBack(struct PointerList* pList, struct Pointer* pItem)
{
    if ((pList)->pHead == NULL)
    {
        (pList)->pHead = (pItem);
        (pList)->pTail = (pItem);
    }
    else
    {
        (pList)->pTail->pNext = (pItem);
        (pList)->pTail = (pItem);
    }
}

void Pointer_Delete(struct Pointer* p)
{
    if (p != NULL)
    {
        TPointer_Destroy(p);
        free((void*)p);
    }
}

void TPointerList_Printf(struct PointerList* p)
{
    for (struct Pointer* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        printf("*");
        for (int i = 0; i < pItem->Qualifier.Size; i++)
        {
            if (i > 0)
                printf(" ");
            struct TypeQualifier* pQualifier = pItem->Qualifier.Data[i];
            printf("%s", TokenToString(pQualifier->Token));
        }
    }
    printf("\n");
}

void PointerList_Swap(struct PointerList* a, struct PointerList* b)
{
    struct PointerList t = *a;
    *a = *b;
    *b = t;
}

void Pointer_CopyFrom(struct Pointer* dest, struct Pointer* src)
{
    TypeQualifierList_CopyFrom(&dest->Qualifier, &src->Qualifier);
}

bool PointerList_IsPointer(struct PointerList* pPointerlist)
{
    return pPointerlist->pHead != NULL;
}

bool PointerList_IsPointerN(struct PointerList* pPointerlist, int n)
{
    int k = 0;
    if (pPointerlist)
    {
        for (struct Pointer* pItem = pPointerlist->pHead;
             pItem != NULL;
             pItem = pItem->pNext)
        {
            k++;
        }
    }
    return k == n;
}

bool PointerList_IsPointerToObject(struct PointerList* pPointerlist)
{
    bool bResult = false;
    struct Pointer* pPointer = pPointerlist->pHead;
    if (pPointer != NULL)
    {
        if (pPointer->Qualifier.Size == 0)
        {
            pPointer = pPointer->pNext;
            if (pPointer == NULL)
            {
                bResult = true;
            }
        }
    }
    return bResult;
}

bool PointerList_IsAutoPointerToObject(struct PointerList* pPointerlist)
{
    //retorna true se tem 1 ponteiro qualificado auto
    bool bResult = false;
    struct Pointer* pPointer = pPointerlist->pHead;
    if (pPointer != NULL && pPointer->pNext == NULL)
    {
        //so tem 1 ponteiro
        //procura se tem algum qualificador auto
        for (int i = 0; i < pPointer->Qualifier.Size; i++)
        {
            if (IsAutoToken(pPointer->Qualifier.Data[i]->Token))
            {
                bResult = true;
                break;
            }
        }
    }
    return bResult;
}

bool PointerList_IsAutoPointerSizeToObject(struct PointerList* pPointerlist)
{
    bool bResult = false;
    struct Pointer* pPointer = pPointerlist->pHead;
    if (pPointer != NULL)
    {
        if (pPointer->Qualifier.Size == 2 &&
            pPointer->pNext == NULL)
        {
            bResult = (IsAutoToken(pPointer->Qualifier.Data[0]->Token) &&
                       IsSizeToken(pPointer->Qualifier.Data[1]->Token)) ||
                (IsSizeToken(pPointer->Qualifier.Data[0]->Token) &&
                 IsAutoToken(pPointer->Qualifier.Data[0]->Token));
        }
    }
    return bResult;
}

bool PointerList_IsAutoPointerToPointer(struct PointerList* pPointerlist)
{
    bool bResult = false;
    struct Pointer* pPointer = pPointerlist->pHead;
    if (pPointer != NULL)
    {
        if (pPointer->Qualifier.Size == 1 &&
            IsAutoToken(pPointer->Qualifier.Data[0]->Token))
        {
            pPointer = pPointer->pNext;
            if (pPointer != NULL)
            {
                if (pPointer->Qualifier.Size == 0)
                {
                    bResult = true;
                }
            }
        }
    }
    return bResult;
}


bool PointerList_IsAutoPointerToAutoPointer(struct PointerList* pPointerlist)
{
    bool bResult = false;
    struct Pointer* pPointer = pPointerlist->pHead;
    if (pPointer != NULL)
    {
        if (pPointer->Qualifier.Size == 1 &&
            IsAutoToken(pPointer->Qualifier.Data[0]->Token))
        {
            pPointer = pPointer->pNext;
            if (pPointer != NULL)
            {
                if (pPointer->Qualifier.Size == 1 &&
                    IsAutoToken(pPointer->Qualifier.Data[0]->Token))
                {
                    bResult = true;
                }
                else if (pPointer->Qualifier.Size == 2)
                {
                    //auto _size()
                    // _size() auto
                    bResult = IsAutoToken(pPointer->Qualifier.Data[0]->Token) ||
                        IsAutoToken(pPointer->Qualifier.Data[1]->Token);
                }
            }
        }
    }
    return bResult;
}


void TypeQualifierList_Destroy(struct TypeQualifierList* p)  /*custom*/
{
    for (int i = 0; i < p->Size; i++)
    {
        TypeQualifier_Delete(p->Data[i]);
    }
}

void TypeQualifierList_CopyFrom(struct TypeQualifierList* dest, struct TypeQualifierList* src)
{
    TypeQualifierList_Destroy(dest);
    *dest = (struct TypeQualifierList)TYPEQUALIFIERLIST_INIT;
    for (int i = 0; i < src->Size; i++)
    {
        struct TypeQualifier* pItem = TTypeQualifier_Clone(src->Data[i]);
        TypeQualifierList_PushBack(dest, pItem);
    }
}

void TypeQualifierList_PushBack(struct TypeQualifierList* p, struct TypeQualifier* pItem)
{
    if (p->Size + 1 > 4)
    {
        //nao eh p acontecer!
    }
    else
    {
        p->Data[p->Size] = pItem;
        p->Size++;
    }
}

void TTypeQualifier_Destroy(struct TypeQualifier* p)
{
    TokenList_Destroy(&p->ClueList0);
}
void TypeQualifier_Delete(struct TypeQualifier* p)
{
    if (p != NULL)
    {
        TTypeQualifier_Destroy(p);
        free((void*)p);
    }
}

struct TypeQualifier* TTypeQualifier_Clone(struct TypeQualifier* p)
{
    struct TypeQualifier* pNew = NEW((struct TypeQualifier)TYPEQUALIFIER_INIT);
    pNew->Token = p->Token;
    pNew->Type = p->Type;
    return pNew;
}


bool TypeQualifier_Compare(struct TypeQualifier* p1, struct TypeQualifier* p2)
{
    bool result = false;
    if (p1->Token == p2->Token)
    {
        result = true;
    }
    return result;
}

void TStorageSpecifier_Destroy(struct StorageSpecifier* p)
{
    TokenList_Destroy(&p->ClueList0);
}

struct StorageSpecifier* StorageSpecifier_Clone(struct StorageSpecifier* p)
{
    struct StorageSpecifier* pNew = NEW((struct StorageSpecifier)STORAGESPECIFIER_INIT);
    pNew->Token = p->Token;
    return pNew;
}

void StorageSpecifier_Delete(struct StorageSpecifier* p)
{
    if (p != NULL)
    {
        TStorageSpecifier_Destroy(p);
        free((void*)p);
    }
}

bool StorageSpecifier_Compare(struct StorageSpecifier* p1, struct StorageSpecifier* p2)
{
    bool result = false;
    if (p1->Token == p2->Token)
    {
        result = true;
    }
    return result;
}

struct AtomicTypeSpecifier* AtomicTypeSpecifier_Clone(struct AtomicTypeSpecifier* p)
{
    struct AtomicTypeSpecifier* pNew = NEW((struct AtomicTypeSpecifier)ATOMICTYPESPECIFIER_INIT);
    assert(false);
    return pNew;
}

void AtomicTypeSpecifier_Delete(struct AtomicTypeSpecifier* p)
{
    if (p != NULL)
    {
        TypeName_Destroy(&p->TypeName);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList1);
        TokenList_Destroy(&p->ClueList2);
        free((void*)p);
    }
}

bool AtomicTypeSpecifier_Compare(struct AtomicTypeSpecifier* p1, struct AtomicTypeSpecifier* p2)
{
    //bool result = false;
    //if (p1->TypeName && p2->TypeName &&
    //  strcmp(p1->TypeName, p2->TypeName) == 0)
    //{
    //  result = true;
    //}
    //so eh usado em 1 context que nao faz diferenca hj
    assert(false);
    return true;// result;
}

void SpecifierQualifierList_CopyTo(struct SpecifierQualifierList* from, struct SpecifierQualifierList* to)
{
    for (int i = 0; i < from->Size; i++)
    {
        SpecifierQualifierList_PushBack(to, SpecifierQualifier_Clone(from->pData[i]));
    }
}

void SpecifierQualifierList_Destroy(struct SpecifierQualifierList* pDeclarationSpecifiers)
{
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        SpecifierQualifier_Delete(pDeclarationSpecifiers->pData[i]);
    }
    free((void*)pDeclarationSpecifiers->pData);
}


void TSpecifierQualifierList_Reserve(struct SpecifierQualifierList* p, int n)
{
    if (n > p->Capacity)
    {
        struct SpecifierQualifier** pnew = p->pData;
        pnew = (struct SpecifierQualifier**)realloc(pnew, n * sizeof(struct SpecifierQualifier*));
        if (pnew)
        {
            p->pData = pnew;
            p->Capacity = n;
        }
    }
}

void SpecifierQualifierList_PushBack(struct SpecifierQualifierList* p, struct SpecifierQualifier* pItem)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        TSpecifierQualifierList_Reserve(p, n);
    }
    p->pData[p->Size] = pItem;
    p->Size++;
}

bool SpecifierQualifierList_CanAdd(struct SpecifierQualifierList* p, enum TokenType token)
{
    bool bResult = false;
    bool bTypeDef = false;
    bool bInt = false;
    for (int i = 0; i < p->Size; i++)
    {
        struct SpecifierQualifier* pSpecifier = p->pData[i];
        switch (pSpecifier->Type)
        {
            case SingleTypeSpecifier_ID:
            {
                struct SingleTypeSpecifier* pTSingleTypeSpecifier =
                    (struct SingleTypeSpecifier*)pSpecifier;
                switch (pTSingleTypeSpecifier->Token2)
                {
                    case TK_INT:
                        bInt = true;
                        break;
                    case TK_DOUBLE:
                        break;
                    case TK_IDENTIFIER:
                        bTypeDef = true;
                        break;
                    default:
                        //assert(false);
                        break;
                }
            }
            break;
            case StructUnionSpecifier_ID:
                //bStruct = true;
                break;
            case EnumSpecifier_ID:
                //bEnum = true;
                break;
            case StorageSpecifier_ID:
                break;
            case TypeQualifier_ID:
                break;
            case FunctionSpecifier_ID:
                break;
            case AlignmentSpecifier_ID:
                break;
            default:
                //assert(false);
                break;
        }
    }
    if (token == TK_IDENTIFIER)
    {
        if (!bTypeDef && !bInt)
        {
            //Exemplo que se quer evitar
            //typedef int X;
            //void F(int X)
            //nao pode ter nada antes
            bResult = true;
        }
    }
    else
    {
        //verificar combinacoes unsigned float etc.
        bResult = true;
    }
    return bResult;
}


struct DeclarationSpecifier* DeclarationSpecifiers_GetMainSpecifier(struct DeclarationSpecifiers* p, enum Type type)
{
    struct DeclarationSpecifier* pSpecifier = NULL;
    for (int i = 0; i < p->Size; i++)
    {
        struct DeclarationSpecifier* pSpecifierQualifier = p->pData[i];
        if (pSpecifierQualifier->Type == type)
        {
            //ATENCAO
            pSpecifier = (struct DeclarationSpecifier*)pSpecifierQualifier;
            break;
        }
    }
    return pSpecifier;
}

bool DeclarationSpecifiers_CanAddSpeficier(struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                           enum TokenType token,
                                           const char* lexeme)
{
    bool bResult = false;
    //bool bStruct = false;
    //bool bEnum = false;
    struct StructUnionSpecifier* pTStructUnionSpecifier = NULL;
    struct EnumSpecifier* pEnumSpecifier = NULL;
    bool bTypeDef = false;
    bool bOther = false;
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        struct DeclarationSpecifier* pSpecifier = pDeclarationSpecifiers->pData[i];
        switch (pSpecifier->Type)
        {
            case SingleTypeSpecifier_ID:
            {
                struct SingleTypeSpecifier* pTSingleTypeSpecifier =
                    (struct SingleTypeSpecifier*)pSpecifier;
                switch (pTSingleTypeSpecifier->Token2)
                {
                    //case TK_INT:
                    //case TK_DOUBLE:
                    //..etc...
                    //bOther = true;
                    //break;
                    case TK_IDENTIFIER:
                        bTypeDef = true;
                        break;
                    default:
                        bOther = true;
                        break;
                        ////assert(false);
                        break;
                }
            }
            break;
            case StructUnionSpecifier_ID:
                //bStruct = true;
                pTStructUnionSpecifier = (struct StructUnionSpecifier*)pSpecifier;
                break;
            case EnumSpecifier_ID:
                //bEnum = true;
                pEnumSpecifier = (struct EnumSpecifier*)pSpecifier;
                break;
            case StorageSpecifier_ID:
                break;
            case TypeQualifier_ID:
                break;
            case FunctionSpecifier_ID:
                break;
            case AlignmentSpecifier_ID:
                break;
            default:
                //assert(false);
                break;
        }
    }
    if (token == TK_IDENTIFIER)
    {
        if (pTStructUnionSpecifier)
        {
            //ja tem uma struct
            if (pTStructUnionSpecifier->Tag &&
                strcmp(pTStructUnionSpecifier->Tag, lexeme) == 0)
            {
                //typedef struct X X;
            }
            else
            {
                bResult = true;
            }
        }
        else if (pEnumSpecifier)
        {
            //ja tem uma struct
            if (pEnumSpecifier->Tag &&
                strcmp(pEnumSpecifier->Tag, lexeme) == 0)
            {
                //typedef enum X X;
            }
            else
            {
                bResult = true;
            }
        }
        else if (!bTypeDef && !bOther)
        {
            //Exemplo que se quer evitar
            //typedef int X;
            //void F(int X)
            //nao pode ter nada antes
            bResult = true;
        }
    }
    else
    {
        //verificar combinacoes unsigned float etc.
        bResult = true;
    }
    return bResult;
}

struct StructUnionSpecifier* TDeclarationSpecifiers_GetCompleteStructUnionSpecifier(struct SymbolMap* pSymbolMap,
    struct DeclarationSpecifiers* pDeclarationSpecifiers)
{
    struct StructUnionSpecifier* pStructUnionSpecifier = NULL;
    struct DeclarationSpecifier* pFirstArgSpecifier =
        DeclarationSpecifiers_GetMainSpecifier(pDeclarationSpecifiers, StructUnionSpecifier_ID);
    if (pFirstArgSpecifier == NULL)
    {
        pFirstArgSpecifier = DeclarationSpecifiers_GetMainSpecifier(pDeclarationSpecifiers, SingleTypeSpecifier_ID);
    }
    if (pFirstArgSpecifier)
    {
        pStructUnionSpecifier =
            DeclarationSpecifier_As_StructUnionSpecifier(pFirstArgSpecifier);
        if (pStructUnionSpecifier && pStructUnionSpecifier->Tag && pStructUnionSpecifier->StructDeclarationList.Size == 0)
        {
            //procura declaracao completa
            pStructUnionSpecifier =
                SymbolMap_FindCompleteStructUnionSpecifier(pSymbolMap, pStructUnionSpecifier->Tag);
        }
        else
        {
            struct Declarator declarator = TDECLARATOR_INIT;
            struct SingleTypeSpecifier* pSingleTypeSpecifier =
                DeclarationSpecifier_As_SingleTypeSpecifier(pFirstArgSpecifier);
            if (pSingleTypeSpecifier)
            {
                if (pSingleTypeSpecifier->TypedefName)
                {
                    struct DeclarationSpecifiers* p2 =
                        SymbolMap_FindTypedefTarget(pSymbolMap, pSingleTypeSpecifier->TypedefName, &declarator);
                    if (p2)
                    {
                        pStructUnionSpecifier = (struct StructUnionSpecifier*)
                            DeclarationSpecifiers_GetMainSpecifier(p2, StructUnionSpecifier_ID);
                        if (pStructUnionSpecifier &&
                            pStructUnionSpecifier->StructDeclarationList.Size == 0 &&
                            pStructUnionSpecifier->Tag != NULL)
                        {
                            pStructUnionSpecifier =
                                SymbolMap_FindCompleteStructUnionSpecifier(pSymbolMap, pStructUnionSpecifier->Tag);
                        }
                    }
                }
            }
        }
    }
    return pStructUnionSpecifier;
}

const char* DeclarationSpecifiers_GetTypedefName(struct DeclarationSpecifiers* pDeclarationSpecifiers)
{
    if (pDeclarationSpecifiers == NULL)
    {
        return NULL;
    }
    const char* typeName = NULL;
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        struct DeclarationSpecifier* pItem = pDeclarationSpecifiers->pData[i];
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            DeclarationSpecifier_As_SingleTypeSpecifier(pItem);
        if (pSingleTypeSpecifier != NULL)
        {
            if (pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
            {
                typeName = pSingleTypeSpecifier->TypedefName;
                break;
            }
        }
    }
    return typeName;
}

struct SpecifierQualifier* SpecifierQualifier_Clone(struct SpecifierQualifier* pItem)
{
    switch (pItem->Type)
    {
        case TypeQualifier_ID:
            return (struct SpecifierQualifier*)TTypeQualifier_Clone((struct TypeQualifier*)pItem);
        case StructUnionSpecifier_ID:
            return (struct SpecifierQualifier*)StructUnionSpecifier_Clone((struct StructUnionSpecifier*)pItem);
        case AtomicTypeSpecifier_ID:
            return (struct SpecifierQualifier*)AtomicTypeSpecifier_Clone((struct AtomicTypeSpecifier*)pItem);
        case SingleTypeSpecifier_ID:
            return (struct SpecifierQualifier*)SingleTypeSpecifier_Clone((struct SingleTypeSpecifier*)pItem);
        case EnumSpecifier_ID:
            return (struct SpecifierQualifier*)EnumSpecifier_Clone((struct EnumSpecifier*)pItem);
        default:
            break;
    }
    assert(false);
    return NULL;
}

void SpecifierQualifier_Delete(struct SpecifierQualifier* pItem)
{
    if (pItem != NULL)
    {
        switch (pItem->Type)
        {
            case TypeQualifier_ID:
                TypeQualifier_Delete((struct TypeQualifier*)pItem);
                break;
            case StructUnionSpecifier_ID:
                StructUnionSpecifier_Delete((struct StructUnionSpecifier*)pItem);
                break;
            case AtomicTypeSpecifier_ID:
                AtomicTypeSpecifier_Delete((struct AtomicTypeSpecifier*)pItem);
                break;
            case SingleTypeSpecifier_ID:
                SingleTypeSpecifier_Delete((struct SingleTypeSpecifier*)pItem);
                break;
            case EnumSpecifier_ID:
                EnumSpecifier_Delete((struct EnumSpecifier*)pItem);
                break;
            default:
                break;
        }
    }
}

void TDeclarationSpecifier_Delete(struct DeclarationSpecifier* pItem)
{
    if (pItem != NULL)
    {
        switch (pItem->Type)
        {
            case TypeQualifier_ID:
                TypeQualifier_Delete((struct TypeQualifier*)pItem);
                break;
            case StructUnionSpecifier_ID:
                StructUnionSpecifier_Delete((struct StructUnionSpecifier*)pItem);
                break;
            case StorageSpecifier_ID:
                StorageSpecifier_Delete((struct StorageSpecifier*)pItem);
                break;
            case AtomicTypeSpecifier_ID:
                AtomicTypeSpecifier_Delete((struct AtomicTypeSpecifier*)pItem);
                break;
            case SingleTypeSpecifier_ID:
                SingleTypeSpecifier_Delete((struct SingleTypeSpecifier*)pItem);
                break;
            case AlignmentSpecifier_ID:
                AlignmentSpecifier_Delete((struct AlignmentSpecifier*)pItem);
                break;
            case FunctionSpecifier_ID:
                FunctionSpecifier_Delete((struct FunctionSpecifier*)pItem);
                break;
            case EnumSpecifier_ID:
                EnumSpecifier_Delete((struct EnumSpecifier*)pItem);
                break;
            default:
                break;
        }
    }
}

void DeclarationSpecifiers_CopyTo(struct DeclarationSpecifiers* from,
                                  struct DeclarationSpecifiers* to)
{
    for (int i = 0; i < from->Size; i++)
    {
        struct DeclarationSpecifier* pItem = from->pData[i];
        DeclarationSpecifiers_PushBack(to, DeclarationSpecifier_Clone(pItem));
    }
}

void DeclarationSpecifiers_CopyTo_SpecifierQualifierList(struct DeclarationSpecifiers* from,
                                                         struct SpecifierQualifierList* to)
{
    for (int i = 0; i < from->Size; i++)
    {
        struct DeclarationSpecifier* pItem = from->pData[i];
        enum Type type = pItem->Type;
        switch (type)
        {
            /*copy only type qualifiers and type specifiers*/
            case TypeQualifier_ID:
            case StructUnionSpecifier_ID:
            case AtomicTypeSpecifier_ID:
            case SingleTypeSpecifier_ID:
            case EnumSpecifier_ID:
                SpecifierQualifierList_PushBack(to, (struct SpecifierQualifier*)DeclarationSpecifier_Clone(pItem));
                break;
            default:
                break;
        }
    }
}

void DeclarationSpecifiers_Destroy(struct DeclarationSpecifiers* pDeclarationSpecifiers)
{
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        TDeclarationSpecifier_Delete(pDeclarationSpecifiers->pData[i]);
    }
    free((void*)pDeclarationSpecifiers->pData);
}

void TDeclarationSpecifiers_Reserve(struct DeclarationSpecifiers* p, int n)
{
    if (n > p->Capacity)
    {
        struct DeclarationSpecifier** pnew = p->pData;
        pnew = (struct DeclarationSpecifier**)realloc(pnew, n * sizeof(struct DeclarationSpecifier*));
        if (pnew)
        {
            p->pData = pnew;
            p->Capacity = n;
        }
    }
}

void DeclarationSpecifiers_PushBack(struct DeclarationSpecifiers* p, struct DeclarationSpecifier* pItem)
{
    if (p->Size + 1 > p->Capacity)
    {
        int n = p->Capacity * 2;
        if (n == 0)
        {
            n = 1;
        }
        TDeclarationSpecifiers_Reserve(p, n);
    }
    p->pData[p->Size] = pItem;
    p->Size++;
}


struct Declarator* Declaration_FindDeclarator(struct Declaration* p, const char* name)
{
    if (p == NULL)
    {
        return NULL;
    }
    struct Declarator* pResult = NULL;
    for (struct InitDeclarator* pInitDeclarator = p->InitDeclaratorList.pHead;
         pInitDeclarator != NULL;
         pInitDeclarator = pInitDeclarator->pNext)
    {
        if (pInitDeclarator->pDeclarator &&
            pInitDeclarator->pDeclarator->pDirectDeclarator &&
            pInitDeclarator->pDeclarator->pDirectDeclarator->Identifier)
        {
            if (strcmp(pInitDeclarator->pDeclarator->pDirectDeclarator->Identifier, name) == 0)
            {
                pResult = pInitDeclarator->pDeclarator;
                break;
            }
        }
        else if (pInitDeclarator->pDeclarator &&
                 pInitDeclarator->pDeclarator->pDirectDeclarator &&
                 pInitDeclarator->pDeclarator->pDirectDeclarator->pDeclarator &&
                 pInitDeclarator->pDeclarator->pDirectDeclarator->pDeclarator->pDirectDeclarator &&
                 pInitDeclarator->pDeclarator->pDirectDeclarator->pDeclarator->pDirectDeclarator->Identifier)
        {
            /*
              This situation here:
              void (*F)(void) = 0;
              F();
            */

            if (strcmp(pInitDeclarator->pDeclarator->pDirectDeclarator->pDeclarator->pDirectDeclarator->Identifier, name) == 0)
            {
                pResult = pInitDeclarator->pDeclarator;
                break;
            }
        }

    }
    return pResult;
}


void TFunctionSpecifier_Destroy(struct FunctionSpecifier* p)
{
    TokenList_Destroy(&p->ClueList0);
}

struct FunctionSpecifier* FunctionSpecifier_Clone(struct FunctionSpecifier* p)
{
    struct FunctionSpecifier* pNew = NEW((struct FunctionSpecifier)FUNCTIONSPECIFIER_INIT);
    pNew->Token = p->Token;
    return pNew;
}

void FunctionSpecifier_Delete(struct FunctionSpecifier* p)
{
    if (p != NULL)
    {
        TFunctionSpecifier_Destroy(p);
        free((void*)p);
    }
}

bool FunctionSpecifier_Compare(struct FunctionSpecifier* p1, struct FunctionSpecifier* p2)
{
    bool result = false;
    if (p1->Token == p2->Token)
    {
        result = true;
    }
    return result;
}



bool Declaration_Is_StructOrUnionDeclaration(struct Declaration* p)
{
    bool bIsStructOrUnion = false;
    for (int i = 0; i < p->Specifiers.Size; i++)
    {
        struct DeclarationSpecifier* pItem = p->Specifiers.pData[i];
        if (DeclarationSpecifier_As_StructUnionSpecifier(pItem))
        {
            bIsStructOrUnion = true;
            break;
        }
    }
    return bIsStructOrUnion;
}

void InitDeclaratorList_Destroy(struct InitDeclaratorList* p)
{
    struct InitDeclarator* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct InitDeclarator* pItem = pCurrent;
        pCurrent = pCurrent->pNext;
        InitDeclarator_Delete(pItem);
    }
}

void Declaration_Delete(struct Declaration* p)
{
    if (p != NULL)
    {
        DeclarationSpecifiers_Destroy(&p->Specifiers);
        InitDeclaratorList_Destroy(&p->InitDeclaratorList);
        CompoundStatement_Delete(p->pCompoundStatementOpt);
        TokenList_Destroy(&p->ClueList0);
        TokenList_Destroy(&p->ClueList00);
        TokenList_Destroy(&p->ClueList001);
        TokenList_Destroy(&p->ClueList1);
        free((void*)p);
    }
}

const char* TDeclarationSpecifier_GetTypedefName(struct DeclarationSpecifiers* p)
{
    const char* typedefName = NULL;
    for (int i = 0; i < p->Size; i++)
    {
        struct DeclarationSpecifier* pSpecifier = p->pData[i];
        struct SingleTypeSpecifier* pSingleTypeSpecifier =
            DeclarationSpecifier_As_SingleTypeSpecifier(pSpecifier);
        if (pSingleTypeSpecifier &&
            pSingleTypeSpecifier->Token2 == TK_IDENTIFIER)
        {
            typedefName = pSingleTypeSpecifier->TypedefName;
        }
    }
    return typedefName;
}

const char* Parameter_GetName(struct Parameter* p)
{
    //F(void) neste caso nao tem nome
    return Declarator_GetName(&p->Declarator);
}

bool Parameter_IsAutoArray(struct Parameter* pParameter)
{
    bool b = pParameter->Declarator.pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeAutoArray;
    return b;
}

const char* Parameter_GetTypedefName(struct Parameter* p)
{
    return TDeclarationSpecifier_GetTypedefName(&p->Specifiers);
}

bool TDeclarator_IsDirectPointer(struct Declarator* p)
{
    int n = 0;
    for (struct Pointer* pPointer = p->PointerList.pHead;
         pPointer != NULL;
         pPointer = pPointer->pNext)
    {
        n++;
        if (n > 1)
        {
            break;
        }
    }
    return n == 1;
}

bool Parameter_IsDirectPointer(struct Parameter* p)
{
    return TDeclarator_IsDirectPointer(&p->Declarator);
}

struct Parameter* Parameter_Clone(struct Parameter* p)
{
    struct Parameter* pNew = NEW((struct Parameter)PARAMETER_INIT);
    Declarator_CopyToAbstractDeclarator(&p->Declarator, &pNew->Declarator);
    pNew->bHasComma = p->bHasComma;
    DeclarationSpecifiers_CopyTo(&p->Specifiers, &pNew->Specifiers);
    return pNew;
}

void Parameter_Delete(struct Parameter* p)
{
    if (p != NULL)
    {
        DeclarationSpecifiers_Destroy(&p->Specifiers);
        Declarator_Destroy(&p->Declarator);
        TokenList_Destroy(&p->ClueList0);
        free((void*)p);
    }
}

void ParameterList_Destroy(struct ParameterList* pParameterList)
{
    struct Parameter* pCurrent = pParameterList->pHead;
    while (pCurrent)
    {
        struct Parameter* p = pCurrent;
        pCurrent = pCurrent->pNext;
        Parameter_Delete(p);
    }
}

bool AnyDeclaration_Is_StructOrUnionDeclaration(struct AnyDeclaration* pAnyDeclaration)
{
    struct Declaration* pDeclaration = AnyDeclaration_As_Declaration(pAnyDeclaration);
    if (pDeclaration != NULL)
    {
        return Declaration_Is_StructOrUnionDeclaration(pDeclaration);
    }
    return false;
}

bool TDeclarationSpecifiers_IsStatic(struct DeclarationSpecifiers* pDeclarationSpecifiers)
{
    bool bResult = false;
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        struct DeclarationSpecifier* pItem = pDeclarationSpecifiers->pData[i];
        switch (pItem->Type)
        {
            case StorageSpecifier_ID:
            {
                struct StorageSpecifier* pStorageSpecifier =
                    (struct StorageSpecifier*)pItem;
                if (pStorageSpecifier->Token == TK_STATIC)
                {
                    bResult = true;
                }
            }
            break;
            default:
                //assert(false);
                break;
        }
        if (bResult)
        {
            break;
        }
    }
    return bResult;
}

bool DeclarationSpecifiers_IsTypedef(struct DeclarationSpecifiers* pDeclarationSpecifiers)
{
    bool bResult = false;
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        struct DeclarationSpecifier* pItem = pDeclarationSpecifiers->pData[i];
        switch (pItem->Type)
        {
            case StorageSpecifier_ID:
            {
                struct StorageSpecifier* pStorageSpecifier =
                    (struct StorageSpecifier*)pItem;
                if (pStorageSpecifier->Token == TK_TYPEDEF)
                {
                    bResult = true;
                }
            }
            break;
            default:
                //assert(false);
                break;
        }
        if (bResult)
        {
            break;
        }
    }
    return bResult;
}

bool AnyDeclaration_IsTypedef(struct AnyDeclaration* pDeclaration)
{
    bool bResult = false;
    switch (pDeclaration->Type)
    {
        case Declaration_ID:
        {
            struct Declaration* p = (struct Declaration*)pDeclaration;
            bResult = DeclarationSpecifiers_IsTypedef(&p->Specifiers);
        }
        break;
        default:
            //assert(false);
            break;
    }
    return bResult;
}

int AnyDeclaration_GetFileIndex(struct AnyDeclaration* pDeclaration)
{
    int result = -1;
    switch (pDeclaration->Type)
    {
        case Declaration_ID:
            result = ((struct Declaration*)pDeclaration)->FileIndex;
            break;
        case StaticAssertDeclaration_ID:
            break;
        default:
            //assert(false);
            break;
    }
    return result;
}


void AnyDeclaration_Delete(struct AnyDeclaration* pDeclaration)
{
    if (pDeclaration != NULL)
    {
        switch (pDeclaration->Type)
        {
            case StaticAssertDeclaration_ID:
                StaticAssertDeclaration_Delete((struct StaticAssertDeclaration*)pDeclaration);
                break;
            case Declaration_ID:
                Declaration_Delete((struct Declaration*)pDeclaration);
                break;
            case EofDeclaration_ID:
                EofDeclaration_Delete((struct EofDeclaration*)pDeclaration);
                break;
            default:
                break;
        }
    }
}

void TDesignator_Destroy(struct Designator* p)
{
    free((void*)p->Name);
    Expression_Delete(p->pExpression);
    TokenList_Destroy(&p->ClueList0);
    TokenList_Destroy(&p->ClueList1);
}

void Designator_Delete(struct Designator* p)
{
    if (p != NULL)
    {
        TDesignator_Destroy(p);
        free((void*)p);
    }
}

void TInitializerListType_Destroy(struct InitializerListType* p)
{
    InitializerList_Destroy(&p->InitializerList);
    TokenList_Destroy(&p->ClueList0);
    TokenList_Destroy(&p->ClueList1);
    TokenList_Destroy(&p->ClueList2);
}

void InitializerListType_Delete(struct InitializerListType* p)
{
    if (p != NULL)
    {
        TInitializerListType_Destroy(p);
        free((void*)p);
    }
}

void InitializerList_Destroy(struct InitializerList* pInitializerList)
{
    struct InitializerListItem* pCurrent = pInitializerList->pHead;
    while (pCurrent)
    {
        struct InitializerListItem* p = pCurrent;
        pCurrent = pCurrent->pNext;
        InitializerListItem_Delete(p);
    }
}

void Initializer_Delete(struct Initializer* p)
{
    if (p != NULL)
    {
        switch (p->Type)
        {
            case BinaryExpression_ID:
                BinaryExpression_Delete((struct BinaryExpression*)p);
                break;
            case PrimaryExpressionLambda_ID:
                PrimaryExpressionLambda_Delete((struct PrimaryExpressionLambda*)p);
                break;
            case UnaryExpressionOperator_ID:
                UnaryExpressionOperator_Delete((struct UnaryExpressionOperator*)p);
                break;
            case CastExpressionType_ID:
                CastExpressionType_Delete((struct CastExpressionType*)p);
                break;
            case InitializerListType_ID:
                InitializerListType_Delete((struct InitializerListType*)p);
                break;
            case PrimaryExpressionValue_ID:
                PrimaryExpressionValue_Delete((struct PrimaryExpressionValue*)p);
                break;
            case PostfixExpression_ID:
                PostfixExpression_Delete((struct PostfixExpression*)p);
                break;
            case PrimaryExpressionLiteral_ID:
                PrimaryExpressionLiteral_Delete((struct PrimaryExpressionLiteral*)p);
                break;
            case TernaryExpression_ID:
                TernaryExpression_Delete((struct TernaryExpression*)p);
                break;
            default:
                break;
        }
    }
}

void DesignatorList_Destroy(struct DesignatorList* pDesignatorList)
{
    struct Designator* pCurrent = pDesignatorList->pHead;
    while (pCurrent)
    {
        struct Designator* p = pCurrent;
        pCurrent = pCurrent->pNext;
        Designator_Delete(p);
    }
}

void DesignatorList_PushBack(struct DesignatorList* pList, struct Designator* pItem)
{
    if (pList->pHead == NULL)
    {
        pList->pHead = pItem;
    }
    else
    {
        pList->pTail->pNext = pItem;
    }
    pList->pTail = pItem;
}

void TInitializerListItem_Destroy(struct InitializerListItem* p)
{
    DesignatorList_Destroy(&p->DesignatorList);
    Initializer_Delete(p->pInitializer);
    TokenList_Destroy(&p->ClueList);
}

void InitializerListItem_Delete(struct InitializerListItem* p)
{
    if (p != NULL)
    {
        TInitializerListItem_Destroy(p);
        free((void*)p);
    }
}

struct Declaration* SyntaxTree_FindDeclaration(struct SyntaxTree* p, const char* name)
{
    struct TypePointer* pt = SymbolMap_Find(&p->GlobalScope, name);
    if (pt != NULL &&
        pt->Type == Declaration_ID)
    {
        return (struct Declaration*)pt;
    }
    return NULL;
}

struct Declaration* SyntaxTree_FindFunctionDeclaration(struct SyntaxTree* p, const char* name)
{
    struct TypePointer* pt = SymbolMap_Find(&p->GlobalScope, name);
    if (pt != NULL &&
        pt->Type == Declaration_ID)
    {
        return (struct Declaration*)pt;
    }
    return NULL;
}

//Retorna a declaracao final do tipo entrando em cada typedef.
struct Declaration* SyntaxTree_GetFinalTypeDeclaration(struct SyntaxTree* p, const char* typeName)
{
    return SymbolMap_FindTypedefDeclarationTarget(&p->GlobalScope, typeName);
}

void SyntaxTree_Destroy(struct SyntaxTree* p)
{
    Declarations_Destroy(&p->Declarations);
    FileArray_Destroy(&p->Files);
    SymbolMap_Destroy(&p->GlobalScope);
    MacroMap_Destroy(&p->Defines);
}

///////////////////////////////////////////

static bool TPostfixExpressionCore_Evaluate(struct PostfixExpression* p,
                                            int* pResult)
{
    int result = *pResult;
    if (p->pExpressionLeft)
    {
        int left;
        EvaluateConstantExpression(p->pExpressionLeft, &left);
    }
    //if (p->pInitializerList)
    {
        //falta imprimeir typename
        //TTypeName_Print*
        //b = TInitializerList_CodePrint(p->pInitializerList, b, fp);
    }
    switch (p->token)
    {
        case TK_FULL_STOP:
            //fprintf(fp, ".%s", p->Identifier);
            //assert(false);
            break;
        case TK_ARROW:
            //fprintf(fp, "->%s", p->Identifier);
            //b = true;
            //assert(false);
            break;
        case TK_LEFT_SQUARE_BRACKET:
        {
            int index;
            //fprintf(fp, "[");
            EvaluateConstantExpression(p->pExpressionRight, &index);
            //fprintf(fp, "]");
            //assert(false);
        }
        break;
        case TK_LEFT_PARENTHESIS:
        {
            EvaluateConstantExpression(p->pExpressionRight, &result);
        }
        break;
        case TK_PLUSPLUS:
            //assert(false);
            break;
        case TK_MINUSMINUS:
            //assert(false);
            break;
        default:
            //assert(false);
            break;
    }
    if (p->pNext)
    {
        int result2 = result;
        TPostfixExpressionCore_Evaluate(p->pNext, &result2);
        result = result2;
    }
    return true;
}

//Criado para avaliacao do #if
//Tem que arrumar para fazer os casts do enum
bool EvaluateConstantExpression(struct Expression* p, int* pResult)
{
    int result = -987654321;
    if (p == NULL)
    {
        return false;
    }
    bool b = false;
    switch (p->Type)
    {
        case BinaryExpression_ID:
        {
            struct BinaryExpression* pBinaryExpression =
                (struct BinaryExpression*)p;
            int left;
            b = EvaluateConstantExpression(pBinaryExpression->pExpressionLeft, &left);
            int right;
            b = EvaluateConstantExpression(pBinaryExpression->pExpressionRight, &right);
            switch (pBinaryExpression->token)
            {
                case TK_ASTERISK:
                    result = (left * right);
                    b = true;
                    break;
                case TK_PLUS_SIGN:
                    result = (left + right);
                    b = true;
                    break;
                case TK_HYPHEN_MINUS:
                    result = (left - right);
                    b = true;
                    break;
                case TK_ANDAND:
                    result = (left && right);
                    b = true;
                    break;
                case TK_OROR:
                    result = (left || right);
                    b = true;
                    break;
                case TK_NOTEQUAL:
                    result = (left != right);
                    b = true;
                    break;
                case TK_EQUALEQUAL:
                    result = (left == right);
                    b = true;
                    break;
                case TK_GREATEREQUAL:
                    result = (left >= right);
                    b = true;
                    break;
                case TK_LESSEQUAL:
                    result = (left <= right);
                    b = true;
                    break;
                case TK_GREATER_THAN_SIGN:
                    result = (left > right);
                    b = true;
                    break;
                case TK_LESS_THAN_SIGN:
                    result = (left < right);
                    b = true;
                    break;
                case TK_AMPERSAND:
                    result = (left & right);
                    b = true;
                    break;
                case TK_GREATERGREATER:
                    result = (left >> right);
                    b = true;
                    break;
                case TK_LESSLESS:
                    result = (left << right);
                    b = true;
                    break;
                case TK_VERTICAL_LINE:
                    result = (left | right);
                    b = true;
                    break;
                case TK_SOLIDUS:
                    if (right != 0)
                    {
                        result = (left / right);
                        b = true;
                    }
                    else
                    {
                        b = false;
                        //SetError
                    }
                    break;
                default:
                    //TODO ADD THE OPERADOR?
                    //assert(false);
                    b = false;
                    break;
            }
            //if (pBinaryExpression->)
        }
        break;
        case TernaryExpression_ID:
        {
            int e1, e2, e3;
            b = EvaluateConstantExpression(((struct TernaryExpression*)p)->pExpressionLeft, &e1);
            b = EvaluateConstantExpression(((struct TernaryExpression*)p)->pExpressionMiddle, &e2);
            b = EvaluateConstantExpression(((struct TernaryExpression*)p)->pExpressionRight, &e3);
            //assert(false);
        }
        break;
        case PrimaryExpressionValue_ID:
        {
            struct PrimaryExpressionValue* pPrimaryExpressionValue =
                (struct PrimaryExpressionValue*)p;
            if (pPrimaryExpressionValue->pExpressionOpt != NULL)
            {
                b = EvaluateConstantExpression(pPrimaryExpressionValue->pExpressionOpt, &result);
            }
            else
            {
                switch (pPrimaryExpressionValue->token)
                {
                    case TK_IDENTIFIER:
                        result = 0; //para macro
                        b = true;
                        break;
                    case TK_DECIMAL_INTEGER:
                        result = atoi(pPrimaryExpressionValue->lexeme);
                        b = true;
                        break;
                    case TK_HEX_INTEGER:
                        result = (int)strtol(pPrimaryExpressionValue->lexeme, NULL, 16);
                        b = true;
                        break;
                    case TK_CHAR_LITERAL:
                        if (pPrimaryExpressionValue->lexeme != NULL)
                        {
                            //vem com 'A'
                            result = pPrimaryExpressionValue->lexeme[1];
                            b = true;
                        }
                        else
                        {
                            result = 0;
                        }
                        break;
                    default:
                        b = false;
                        //assert(0);
                        break;
                }
            }
        }
        break;
        case PostfixExpression_ID:
        {
            struct PostfixExpression* pPostfixExpressionCore =
                (struct PostfixExpression*)p;
            b = TPostfixExpressionCore_Evaluate(pPostfixExpressionCore, &result);
            //assert(false);
        }
        break;
        case UnaryExpressionOperator_ID:
        {
            struct UnaryExpressionOperator* pTUnaryExpressionOperator =
                (struct UnaryExpressionOperator*)p;
            if (pTUnaryExpressionOperator->token == TK_SIZEOF)
            {
                //if (TDeclarationSpecifiers_IsTypedef(pTUnaryExpressionOperator->TypeName.SpecifierQualifierList))
                {
                    //b = TTypeQualifier_CodePrint2(&pTUnaryExpressionOperator->TypeName.qualifiers, fp);
                    //b = TTypeSpecifier_CodePrint2(pTUnaryExpressionOperator->TypeName.pTypeSpecifier, b, fp);
                    // b = TDeclarator_CodePrint(&pTUnaryExpressionOperator->TypeName.declarator, b, fp);
                }
                //else
                {
                    b = EvaluateConstantExpression(pTUnaryExpressionOperator->pExpressionRight, &result);
                }
            }
            else
            {
                int localResult;
                b = EvaluateConstantExpression(pTUnaryExpressionOperator->pExpressionRight, &localResult);
                switch (pTUnaryExpressionOperator->token)
                {
                    case TK_EXCLAMATION_MARK:
                        result = !localResult;
                        b = true;
                        break;
                    case TK_HYPHEN_MINUS:
                        result = -localResult;
                        b = true;
                        break;
                    default:
                        //assert(false);
                        break;
                }
            }
        }
        break;
        case CastExpressionType_ID:
        {
            struct CastExpressionType* pCastExpressionType =
                (struct CastExpressionType*)p;
            //b = TTypeQualifier_CodePrint2(&pCastExpressionType->TypeName.qualifiers, fp);
            //b = TTypeSpecifier_CodePrint2(pCastExpressionType->TypeName.pTypeSpecifier, b, fp);
            //b = TDeclarator_CodePrint(&pCastExpressionType->TypeName.declarator, b, fp);
            b = EvaluateConstantExpression(pCastExpressionType->pExpression, &result);
            //assert(false);
        }
        break;
        default:
            //assert(false);
            break;
    }
    //assert(result != -987654321);
    *pResult = result;
    return b;
}

struct DeclarationSpecifiers* Declaration_GetArgTypeSpecifier(struct Declaration* p, int index)
{
    struct DeclarationSpecifiers* pResult = NULL;
    struct ParameterTypeList* pArguments = TDeclaration_GetFunctionArguments(p);
    int n = 0;
    for (struct Parameter* pItem = (&pArguments->ParameterList)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        if (n == index)
        {
            pResult = &pItem->Specifiers;
            break;
        }
        n++;
    }
    return pResult;
}

struct ParameterTypeList* TDeclaration_GetFunctionArguments(struct Declaration* p)
{
    struct ParameterTypeList* pParameterTypeList = NULL;
    if (p->InitDeclaratorList.pHead != NULL)
    {
        if (p->InitDeclaratorList.pHead->pNext == NULL)
        {
            if (p->InitDeclaratorList.pHead->pDeclarator != NULL)
            {
                if (p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator)
                {
                    if (p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeFunction)
                    {
                        pParameterTypeList =
                            &p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator->Parameters;
                    }
                }
            }
        }
    }
    return pParameterTypeList;
}

const char* Declaration_GetFunctionName(struct Declaration* p)
{
    if (p == NULL)
        return NULL;
    const char* functionName = NULL;
    if (p->InitDeclaratorList.pHead != NULL)
    {
        if (p->InitDeclaratorList.pHead->pNext == NULL)
        {
            if (p->InitDeclaratorList.pHead->pDeclarator != NULL)
            {
                if (p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator)
                {
                    if (p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeFunction)
                    {
                        functionName =
                            p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator->Identifier;
                    }
                }
            }
        }
    }
    return functionName;
}

struct CompoundStatement* Declaration_Is_FunctionDefinition(struct Declaration* p)
{
    struct CompoundStatement* pCompoundStatement = NULL;
    if (p->InitDeclaratorList.pHead != NULL)
    {
        if (p->InitDeclaratorList.pHead->pNext == NULL)
        {
            if (p->InitDeclaratorList.pHead->pDeclarator != NULL)
            {
                if (p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator)
                {
                    if (p->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeFunction)
                    {
                        pCompoundStatement = p->pCompoundStatementOpt;
                    }
                }
            }
        }
    }
    return pCompoundStatement;
}

struct StructUnionSpecifier* TDeclarationSpecifiers_Find_StructUnionSpecifier(struct DeclarationSpecifiers* p)
{
    struct StructUnionSpecifier* pStructUnionSpecifier = NULL;
    for (int i = 0; i < p->Size; i++)
    {
        struct DeclarationSpecifier* pDeclarationSpecifier = p->pData[i];
        pStructUnionSpecifier =
            DeclarationSpecifier_As_StructUnionSpecifier(pDeclarationSpecifier);
        if (pStructUnionSpecifier)
        {
            break;
        }
    }
    return pStructUnionSpecifier;
}

struct StructUnionSpecifier* TParameter_Is_DirectPointerToStruct(struct SyntaxTree* pSyntaxTree, struct Parameter* pParameter)
{
    struct StructUnionSpecifier* pStructUnionSpecifier = NULL;
    if (Parameter_IsDirectPointer(pParameter))
    {
        const char* typedefName = Parameter_GetTypedefName(pParameter);
        if (typedefName != NULL)
        {
            struct Declaration* pArgType = SyntaxTree_FindDeclaration(pSyntaxTree, Parameter_GetTypedefName(pParameter));
            if (pArgType)
            {
                pStructUnionSpecifier =
                    TDeclarationSpecifiers_Find_StructUnionSpecifier(&pArgType->Specifiers);
            }
        }
    }
    return pStructUnionSpecifier;
}

void PrimaryExpressionLiteralItemList_Destroy(struct PrimaryExpressionLiteralItemList* p)
{
    struct PrimaryExpressionLiteralItem* pCurrent = p->pHead;
    while (pCurrent)
    {
        struct PrimaryExpressionLiteralItem* pItem = pCurrent;
        pCurrent = pCurrent->pNext;
        PrimaryExpressionLiteralItem_Delete(pItem);
    }
}

void PrimaryExpressionLiteralItemList_Add(struct PrimaryExpressionLiteralItemList* pList, struct PrimaryExpressionLiteralItem* pItem)
{
    if (pList->pHead == NULL)
    {
        pList->pHead = pItem;
        pList->pTail = pItem;
    }
    else
    {
        pList->pTail->pNext = pItem;
        pList->pTail = pItem;
    }
}

const char* Declaration_GetArgName(struct Declaration* p, int index)
{
    const char* argName = NULL;
    struct ParameterTypeList* pArguments = TDeclaration_GetFunctionArguments(p);
    int n = 0;
    for (struct Parameter* pItem = (&pArguments->ParameterList)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        if (n == index)
        {
            argName = Parameter_GetName(pItem);
            break;
        }
        n++;
    }
    return argName;
}


int Declaration_GetNumberFuncArgs(struct Declaration* p)
{
    struct ParameterTypeList* pArguments = TDeclaration_GetFunctionArguments(p);
    int n = 0;
    for (struct Parameter* pItem = (&pArguments->ParameterList)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        n++;
    }
    return n;
}

void TypeName_Swap(struct TypeName* a, struct TypeName* b)
{
    struct TypeName temp = *a;
    *a = *b;
    *b = temp;
}

void TypeName_CopyTo(struct TypeName* from, struct TypeName* to)
{
    to->Type = from->Type;
    SpecifierQualifierList_CopyTo(&from->SpecifierQualifierList, &to->SpecifierQualifierList);
    Declarator_CopyToAbstractDeclarator(&from->Declarator, &to->Declarator);
}

void TypeName_Destroy(struct TypeName* p)
{
    SpecifierQualifierList_Destroy(&p->SpecifierQualifierList);
    Declarator_Destroy(&p->Declarator);
}

void TypeName_Delete(struct TypeName* p)
{
    if (p != NULL)
    {
        TypeName_Destroy(p);
        free((void*)p);
    }
}

static void TSingleTypeSpecifier_PrintNameMangling(struct SingleTypeSpecifier* p,
                                                   struct StrBuilder* fp)
{
    if (p->Token2 == TK_IDENTIFIER)
    {
        StrBuilder_Append(fp, p->TypedefName);
    }
    else
    {
        StrBuilder_Append(fp, TokenToString(p->Token2));
    }
}


static void TPointer_PrintNameMangling(struct Pointer* pPointer,
                                       struct StrBuilder* fp);

static void TPointerList_PrintNameMangling(struct PointerList* p,
                                           struct StrBuilder* fp)
{
    for (struct Pointer* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        TPointer_PrintNameMangling(pItem, fp);
    }
}

void ParameterList_PrintNameMangling(struct ParameterList* p,
                                     struct StrBuilder* fp)
{
    for (struct Parameter* pItem = (p)->pHead; pItem != NULL; pItem = pItem->pNext)
    {
        Parameter_PrintNameMangling(pItem, fp);
    }
}

static void TDirectDeclarator_PrintNameMangling(struct DirectDeclarator* pDirectDeclarator,
                                                struct StrBuilder* fp)
{
    if (pDirectDeclarator == NULL)
    {
        return;
    }
    if (pDirectDeclarator->Identifier)
    {
        if (pDirectDeclarator->bOverload)
        {
            struct StrBuilder sb = STRBUILDER_INIT;
            ParameterList_PrintNameMangling(&pDirectDeclarator->Parameters.ParameterList,
                                            &sb);
            StrBuilder_Append(fp, pDirectDeclarator->Identifier);
            StrBuilder_Append(fp, sb.c_str);
            StrBuilder_Destroy(&sb);
        }
        else
        {
            //Output_Append(fp, options, pDirectDeclarator->Identifier);
        }
    }
    else  if (pDirectDeclarator->pDeclarator)
    {
        //( declarator )
        //TDeclarator_CodePrint(pSyntaxTree, options, pDirectDeclarator->pDeclarator, fp);
        //TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList1, fp);
    }
    if (pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeArray)
    {
        /*
        direct-declarator [ type-qualifier-listopt assignment-expressionopt ]
        direct-declarator [ static type-qualifier-listopt assignment-expression ]
        direct-declarator [ type-qualifier-list static assignment-expression ]
        */
        //Output_Append(fp, options, "[");
        if (pDirectDeclarator->pExpression)
        {
            //TExpression_CodePrint(pSyntaxTree, options, pDirectDeclarator->pExpression, "assignment-expression", fp);
        }
        //Output_Append(fp, options, "]");
    }
    if (pDirectDeclarator->DeclaratorType == TDirectDeclaratorTypeFunction)
    {
        //( parameter-type-list )
        //ParameterTypeList_CodePrint(pSyntaxTree, options, &pDirectDeclarator->Parameters, fp);
        //TNodeClueList_CodePrint(options, &pDirectDeclarator->ClueList3, fp);
    }
    if (pDirectDeclarator->pDirectDeclarator)
    {
        //fprintf(fp, "\"direct-declarator\":");
        //TDirectDeclarator_CodePrint(pSyntaxTree, options, pDirectDeclarator->pDirectDeclarator, fp);
    }
}



static void TDeclarator_PrintNameMangling(struct Declarator* p,
                                          struct StrBuilder* fp)
{
    TPointerList_PrintNameMangling(&p->PointerList, fp);
    TDirectDeclarator_PrintNameMangling(p->pDirectDeclarator, fp);
}


static void TTypeQualifier_PrintNameMangling(struct TypeQualifier* p,
                                             struct StrBuilder* fp)
{
    if (p->Token == TK_AUTO)
    {
        StrBuilder_Append(fp, "auto");
    }
    else if (p->Token == TK_LEFT_SQUARE_BRACKET)
    {
        StrBuilder_Append(fp, "array_size");
    }
    else
    {
        StrBuilder_Append(fp, "const");
    }
}


static void TTypeQualifierList_PrintNameMangling(struct TypeQualifierList* p,
                                                 struct StrBuilder* fp)
{
    for (int i = 0; i < p->Size; i++)
    {
        struct TypeQualifier* pItem = p->Data[i];
        TTypeQualifier_PrintNameMangling(pItem, fp);
    }
}

static void TPointer_PrintNameMangling(struct Pointer* pPointer,
                                       struct StrBuilder* fp)
{
    StrBuilder_Append(fp, "_ptr");
    TTypeQualifierList_PrintNameMangling(&pPointer->Qualifier, fp);
}

void DeclarationSpecifiers_PrintNameMangling(struct DeclarationSpecifiers* pDeclarationSpecifiers,
                                             struct StrBuilder* fp)
{
    for (int i = 0; i < pDeclarationSpecifiers->Size; i++)
    {
        struct DeclarationSpecifier* pItem = pDeclarationSpecifiers->pData[i];
        switch (pItem->Type)
        {
            case SingleTypeSpecifier_ID:
                TSingleTypeSpecifier_PrintNameMangling((struct SingleTypeSpecifier*)pItem, fp);
                break;
            case StructUnionSpecifier_ID:
            {
                struct StructUnionSpecifier* pStructUnionSpecifier = (struct StructUnionSpecifier*)pItem;
                StrBuilder_Append(fp, "_struct");
                StrBuilder_Append(fp, "_");
                StrBuilder_Append(fp, pStructUnionSpecifier->Tag);
            }
            break;
            case EnumSpecifier_ID:
                break;
            case StorageSpecifier_ID:
                break;
            case TypeQualifier_ID:
                break;
            case FunctionSpecifier_ID:
                break;
            default:
                //assert(false);
                break;
        }
    }
}

void Parameter_PrintNameMangling(struct Parameter* p, struct StrBuilder* fp)
{
    DeclarationSpecifiers_PrintNameMangling(&p->Specifiers, fp);
    TDeclarator_PrintNameMangling(&p->Declarator, fp);
    if (p->bHasComma)
    {
        StrBuilder_Append(fp, "_");
    }
}

void GetTypeName(struct Expression* pExpression, struct StrBuilder* str)
{
    struct TypeName* pTypeName = Expression_GetTypeName(pExpression);
    StrBuilder_Clear(str);
    struct PrintCodeOptions options = { 0 };
    options.Options.bCannonical = true;
    options.bInclude = true;
    TTypeName_CodePrint(NULL, &options, pTypeName, str);
}


#define _CRTDBG_MAP_ALLOC

void ParameterList_PrintNameMangling(struct ParameterList* p,
                                     struct StrBuilder* fp);


#define List_Add(pList, pItem) \
if ((pList)->pHead == NULL) {\
    (pList)->pHead = (pItem); \
    (pList)->pTail = (pItem); \
}\
else {\
      (pList)->pTail->pNext = (pItem); \
      (pList)->pTail = (pItem); \
  }

void Declarator(struct Parser* ctx, bool bAbstract, struct Declarator** ppTDeclarator2);


enum TokenType Parser_Match(struct Parser* parser, struct TokenList* listOpt);
enum TokenType Parser_MatchToken(struct Parser* parser,
    enum TokenType tk,
                                 struct TokenList* listOpt);
bool TTypeSpecifier_IsFirst(struct Parser* ctx, enum TokenType token, const char* lexeme);
void Specifier_Qualifier_List(struct Parser* ctx, struct SpecifierQualifierList* pSpecifierQualifierList);
static bool TTypeQualifier_IsFirst(enum TokenType token);




bool Declaration(struct Parser* ctx, struct AnyDeclaration** ppDeclaration);

int IsTypeName(struct Parser* ctx, enum TokenType token, const char* lexeme);


bool Parser_InitString(struct Parser* parser,
                       const char* name,
                       const char* Text)
{
    ///////
    SymbolMap_Init(&parser->GlobalScope);
    parser->pCurrentScope = &parser->GlobalScope;
    parser->bPreprocessorEvalFlag = false;
    /////////
    parser->bError = false;
    StrBuilder_Init(&parser->ErrorMessage);
    /////////
    Scanner_InitString(&parser->Scanner, name, Text);
    //sair do BOF
    struct TokenList clueList0 = { 0 };
    Parser_Match(parser, &clueList0);
    TokenList_Destroy(&clueList0);
    return true;
}

bool Parser_InitFile(struct Parser* parser, const char* fileName)
{
    parser->bPreprocessorEvalFlag = false;
    SymbolMap_Init(&parser->GlobalScope);
    parser->pCurrentScope = &parser->GlobalScope;
    parser->bError = false;
    StrBuilder_Init(&parser->ErrorMessage);
    Scanner_Init(&parser->Scanner);
#ifdef _WIN32
    /*
      quando estiver rodando dentro do command pronpt do VC++
      podemos pegar os includes da variavel de ambiente INCLUDE
    */
    unsigned long __stdcall GetEnvironmentVariableA(char* lpName, char* lpBuffer, unsigned long nSize);
#if 0
    const char* env = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.28.29910\\ATLMFC\\include;C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC\\14.28.29910\\include;C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.18362.0\\ucrt;C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.18362.0\\shared;C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.18362.0\\um;C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.18362.0\\winrt;C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.18362.0\\cppwinrt";
#else
    char env[2000];
    int n = GetEnvironmentVariableA("INCLUDE", env, sizeof(env));
    if (n > 0 && n < sizeof(env))
#endif
    {
        printf("INCLUDE variable found\n");
        const char* p = env;
        for (;;)
        {
            if (*p == '\0')
            {
                break;
            }
            char fileNameLocal[500] = { 0 };
            int count = 0;
            while (*p != '\0' && *p != ';')
            {
                fileNameLocal[count] = *p;
                p++;
                count++;
            }
            fileNameLocal[count] = 0;
            if (count > 0)
            {
                printf("INCLUDE DIR = '%s'\n", fileNameLocal);
                StrArray_Push(&parser->Scanner.IncludeDir, fileNameLocal);
            }
            if (*p == '\0')
            {
                break;
            }
            p++;
        }
    }
#endif
    Scanner_IncludeFile(&parser->Scanner, fileName, FileIncludeTypeFullPath, false);
    //sair do BOF
    struct TokenList clueList0 = { 0 };
    Parser_Match(parser, &clueList0);
    TokenList_Destroy(&clueList0);
    return true;
}

void Parser_PushFile(struct Parser* parser, const char* fileName)
{
    Scanner_IncludeFile(&parser->Scanner, fileName, FileIncludeTypeFullPath, false);
    struct TokenList clueList0 = { 0 };
    Parser_Match(parser, &clueList0);
    TokenList_Destroy(&clueList0);
}


void Parser_Destroy(struct Parser* parser)
{
    SymbolMap_Destroy(&parser->GlobalScope);
    StrBuilder_Destroy(&parser->ErrorMessage);
    Scanner_Destroy(&parser->Scanner);
}

bool Parser_HasError(struct Parser* pParser)
{
    return pParser->bError || pParser->Scanner.bError;
}

void SetWarning(struct Parser* parser, const char* fmt, ...)
{
    struct StrBuilder warningMessage = STRBUILDER_INIT;
    Scanner_GetFilePositionString(&parser->Scanner, &warningMessage);
    va_list args;
    va_start(args, fmt);
    StrBuilder_AppendFmtV(&warningMessage, fmt, args);
    va_end(args);
    printf("%s\n", warningMessage.c_str);
    StrBuilder_Destroy(&warningMessage);
}

void SetError(struct Parser* parser, const char* fmt, ...)
{
    //    //assert(false);
    if (!Parser_HasError(parser))
    {
        Scanner_GetFilePositionString(&parser->Scanner, &parser->ErrorMessage);
        parser->bError = true;
        va_list args;
        va_start(args, fmt);
        StrBuilder_AppendFmtV(&parser->ErrorMessage, fmt, args);
        va_end(args);
    }
    else
    {
        //ja esta com erro entao eh ruido...
        parser->bError = true;
    }
}


int GetCurrentLine(struct Parser* parser)
{
    if (Parser_HasError(parser))
    {
        return -1;
    }
    return Scanner_LineAt(&parser->Scanner, 0);
}

int GetFileIndex(struct Parser* parser)
{
    if (Parser_HasError(parser))
    {
        return -1;
    }
    return Scanner_FileIndexAt(&parser->Scanner, 0);
}

static void GetPosition(struct Parser* ctx, struct FilePos* pPosition)
{
    pPosition->Line = GetCurrentLine(ctx);
    pPosition->FileIndex = GetFileIndex(ctx);
}

//TODO MOVER P SCANNER
enum TokenType Parser_LookAheadToken(struct Parser* parser)
    /*
     le o token adiante relativo a phase de parser, ou seja,
     vai ignorando os tokens das fases anteriores
    */
{
    enum TokenType token = TK_ERROR;
    if (!Parser_HasError(parser))
    {
        for (int i = 1; i < 10; i++)
        {
            token = Scanner_TokenAt(&parser->Scanner, i);
            bool bActive = Scanner_IsActiveAt(&parser->Scanner, i);
            if (bActive && !IsPreprocessorTokenPhase(token))
            {
                break;
            }
        }
    }
    return token;
}

//TODO MOVER P SCANNER
const char* Parser_LookAheadLexeme(struct Parser* parser)
/*esta funcao poderia ser a mesma da de Parser_LookAheadToken*/
{
    if (Parser_HasError(parser))
    {
        return "";
    }
    const char* lexeme = NULL;
    if (!Parser_HasError(parser))
    {
        for (int i = 1; i < 10; i++)
        {
            enum TokenType token = Scanner_TokenAt(&parser->Scanner, i);
            bool bActive = Scanner_IsActiveAt(&parser->Scanner, i);
            if (bActive && !IsPreprocessorTokenPhase(token))
            {
                lexeme = Scanner_LexemeAt(&parser->Scanner, i);
                break;
            }
        }
    }
    return lexeme;
}


enum TokenType Parser_CurrentTokenType(struct Parser* parser)
{
    if (Parser_HasError(parser))
    {
        return TK_ERROR;
    }
    enum TokenType token = Scanner_TokenAt(&parser->Scanner, 0);
    if (IsPreprocessorTokenPhase(token))
    {
        SetError(parser, "!IsPreprocessorTokenPhase");
    }
    return token;
}


enum TokenType Parser_Match(struct Parser* parser, struct TokenList* listOpt)
{
    if (Parser_HasError(parser))
        return TK_ERROR;
    return FinalMatch(&parser->Scanner, listOpt);
}

enum TokenType Parser_MatchToken(struct Parser* parser,
    enum TokenType tk,
                                 struct TokenList* listOpt)
{
    if (Parser_HasError(parser))
    {
        return TK_EOF;
    }
    enum TokenType currentToken = Parser_CurrentTokenType(parser);
    if (tk != currentToken)
    {
        SetError(parser, "Unexpected token - %s", TokenToString(tk));
        return TK_EOF;
    }
    Parser_Match(parser, listOpt);
    return Parser_CurrentTokenType(parser);
}

const char* GetCompletationMessage(struct Parser* parser)
{
    const char* pMessage = "ok";
    if (Parser_HasError(parser))
    {
        if (parser->Scanner.bError)
        {
            pMessage = parser->Scanner.ErrorString.c_str;
        }
        else
        {
            pMessage = parser->ErrorMessage.c_str;
        }
    }
    return pMessage;
}

const char* Lexeme(struct Parser* parser)
{
    if (Parser_HasError(parser))
    {
        return "";
    }
    return Scanner_LexemeAt(&parser->Scanner, 0);
}

bool ErrorOrEof(struct Parser* parser)
{
    return Parser_HasError(parser) ||
        Parser_CurrentTokenType(parser) == TK_EOF;
}

void Expression(struct Parser* ctx, struct Expression**);
void CastExpression(struct Parser* ctx, struct Expression**);
void GenericSelection(struct Parser* ctx);
void ArgumentExpressionList(struct Parser* ctx, struct ArgumentExpressionList* pArgumentExpressionList);
void AssignmentExpression(struct Parser* ctx, struct Expression**);
void Initializer_List(struct Parser* ctx, struct InitializerList* pInitializerList);



bool IsFirstOfPrimaryExpression(enum TokenType token)
{
    bool bResult = false;
    switch (token)
    {
        case TK_IDENTIFIER:
        case TK_STRING_LITERAL:
        case TK_CHAR_LITERAL:
        case TK_DECIMAL_INTEGER:
        case TK_HEX_INTEGER:
        case TK_FLOAT_NUMBER:
        case TK_LEFT_PARENTHESIS:
            //////////
            //extensions
        case TK_LEFT_SQUARE_BRACKET: //lambda-expression
        /////////
        //desde que nao seja cast
        case TK__GENERIC:
            bResult = true;
            break;
        default:
            break;
    }
    return bResult;
}

void PrimaryExpressionLiteral(struct Parser* ctx, struct Expression** ppPrimaryExpression)
{
    enum TokenType token = Parser_CurrentTokenType(ctx);
    struct PrimaryExpressionLiteral* pPrimaryExpressionLiteral
        = NEW((struct PrimaryExpressionLiteral)PRIMARYEXPRESSIONLITERAL_INIT);
    *ppPrimaryExpression = (struct Expression*)pPrimaryExpressionLiteral;
    while (token == TK_STRING_LITERAL)
    {
        struct PrimaryExpressionLiteralItem* pPrimaryExpressionLiteralItem
            = NEW((struct PrimaryExpressionLiteralItem)PRIMARYEXPRESSIONLITERALITEM_INIT);
        const char* lexeme2 = Lexeme(ctx);
        pPrimaryExpressionLiteralItem->lexeme = strdup(lexeme2);
        token = Parser_Match(ctx,
                             &pPrimaryExpressionLiteralItem->ClueList0);
        PrimaryExpressionLiteralItemList_Add(&pPrimaryExpressionLiteral->List, pPrimaryExpressionLiteralItem);
    }
}

void Compound_Statement(struct Parser* ctx, struct CompoundStatement** ppStatement);

void Parameter_Type_List(struct Parser* ctx, struct ParameterTypeList* pParameterList);



void PrimaryExpression(struct Parser* ctx, struct Expression** ppPrimaryExpression)
{
    *ppPrimaryExpression = NULL;
    /*
    primary-expression:
      identifier
      constant
      char-literal
      ( expression )
      generic-selection
    */
    *ppPrimaryExpression = NULL; //out
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* lexeme = Lexeme(ctx);
    //PreprocessorTokenIndex(ctx);
    //-2 nem eh macro
    //-1 inicio de macro
    //-3 fim de macro
    if (!IsFirstOfPrimaryExpression(token))
    {
        SetError(ctx, "unexpected error IsFirstOfPrimaryExpression");
    }
    switch (token)
    {
        case TK_STRING_LITERAL:
            PrimaryExpressionLiteral(ctx, ppPrimaryExpression);
            break;
        case TK_IDENTIFIER:
        {
            struct TypePointer* pTypePointer = SymbolMap_Find(ctx->pCurrentScope, lexeme);
            if (pTypePointer == NULL)
            {
                if (!ctx->bPreprocessorEvalFlag)
                {
                    /*built in*/
                    if (strcmp(lexeme, "__FUNCTION__") == 0 ||
                        strcmp(lexeme, "destroy") == 0)
                    {
                    }
                    else
                    {
                        SetWarning(ctx, "Warning: '%s': undeclared identifier\n", lexeme);
                    }
                }
            }
            struct PrimaryExpressionValue* pPrimaryExpressionValue
                = NEW((struct PrimaryExpressionValue)PRIMARYEXPRESSIONVALUE_INIT);
            pPrimaryExpressionValue->token = token;
            pPrimaryExpressionValue->lexeme = strdup(lexeme);
            if (pTypePointer && pTypePointer->Type == Declaration_ID)
            {
                //eh uma variavel que aponta para uma declaracao
                pPrimaryExpressionValue->pDeclaration = (struct Declaration*)pTypePointer;
                /*todo se for typedef achar o tipo certo*/
                struct Declarator* pDeclaratorFrom = Declaration_FindDeclarator(pPrimaryExpressionValue->pDeclaration, lexeme);
                Declarator_CopyToAbstractDeclarator(pDeclaratorFrom, &pPrimaryExpressionValue->TypeName.Declarator);
                //Tem que apagar o nome do declator
                DeclarationSpecifiers_CopyTo_SpecifierQualifierList(&pPrimaryExpressionValue->pDeclaration->Specifiers,
                                                                    &pPrimaryExpressionValue->TypeName.SpecifierQualifierList);
            }
            if (pTypePointer && pTypePointer->Type == Parameter_ID)
            {
                //eh uma variavel que aponta para um  parametro
                pPrimaryExpressionValue->pParameter = (struct Parameter*)pTypePointer;
                Declarator_CopyToAbstractDeclarator(&pPrimaryExpressionValue->pParameter->Declarator, &pPrimaryExpressionValue->TypeName.Declarator);
                DeclarationSpecifiers_CopyTo_SpecifierQualifierList(&pPrimaryExpressionValue->pParameter->Specifiers,
                                                                    &pPrimaryExpressionValue->TypeName.SpecifierQualifierList);
            }
            Parser_Match(ctx,
                         &pPrimaryExpressionValue->ClueList0);
            *ppPrimaryExpression = (struct Expression*)pPrimaryExpressionValue;
        }
        break;
        case TK_CHAR_LITERAL:
        case TK_DECIMAL_INTEGER:
        case TK_HEX_INTEGER:
        case TK_FLOAT_NUMBER:
        {
            struct PrimaryExpressionValue* pPrimaryExpressionValue
                = NEW((struct PrimaryExpressionValue)PRIMARYEXPRESSIONVALUE_INIT);
            pPrimaryExpressionValue->token = token;
            pPrimaryExpressionValue->lexeme = strdup(Lexeme(ctx));
            Parser_Match(ctx,
                         &pPrimaryExpressionValue->ClueList0);
            *ppPrimaryExpression = (struct Expression*)pPrimaryExpressionValue;
        }
        break;
        case TK_LEFT_PARENTHESIS:
        {
            struct PrimaryExpressionValue* pPrimaryExpressionValue
                = NEW((struct PrimaryExpressionValue)PRIMARYEXPRESSIONVALUE_INIT);
            Parser_Match(ctx,
                         &pPrimaryExpressionValue->ClueList0);
            struct Expression* pExpression;
            Expression(ctx, &pExpression);
            //    //TNodeClueList_MoveToEnd(&pPrimaryExpressionValue->ClueList, &ctx->Scanner.ClueList);
            Parser_MatchToken(ctx,
                              TK_RIGHT_PARENTHESIS,
                              &pPrimaryExpressionValue->ClueList1);
            pPrimaryExpressionValue->token = token;
            pPrimaryExpressionValue->lexeme = strdup(Lexeme(ctx));
            pPrimaryExpressionValue->pExpressionOpt = pExpression;
            *ppPrimaryExpression = (struct Expression*)pPrimaryExpressionValue;
        }
        break;
        case TK__GENERIC:
            GenericSelection(ctx);
            break;
        default:
            SetError(ctx, "unexpected error");
    }
    if (*ppPrimaryExpression == NULL)
    {
        SetError(ctx, "unexpected error NULL");
    }
}

void GenericSelection(struct Parser* ctx)
{
    SetError(ctx, "_Generic not implemented");
    //_Generic
    /*
    (6.5.1.1) generic-selection:
    _Generic ( assignment-expression , generic-assoc-list )
    */
}

void GenericAssocList(struct Parser* ctx)
{
    SetError(ctx, "_Generic not implemented");
    //type-name default
    /*
    (6.5.1.1) generic-assoc-list:
    generic-association
    generic-assoc-list , generic-association
    */
}

void GenericAssociation(struct Parser* ctx)
{
    SetError(ctx, "_Generic not implemented");
    //type-name default
    /*
    (6.5.1.1) generic-association:
    type-name : assignment-expression
    default : assignment-expression
    */
}


void TypeName(struct Parser* ctx, struct TypeName* pTypeName)
{
    /*
    type-name:
    specifier-qualifier-list abstract-declaratoropt
    */
    Specifier_Qualifier_List(ctx, &pTypeName->SpecifierQualifierList);
    struct Declarator* pDeclarator = NULL;
    Declarator(ctx, true, &pDeclarator);
    if (pDeclarator)
    {
        Declarator_Swap(&pTypeName->Declarator, pDeclarator);
        Declarator_Delete(pDeclarator);
    }
}


bool HasPostfixExpressionContinuation(enum TokenType token)
{
    /*
        postfix-expression [ expression ]
        postfix-expression ( argument-expression-listopt )
        postfix-expression . identifier
        postfix-expression -> identifier
        postfix-expression ++
        postfix-expression --
    */
    switch (token)
    {
        case TK_LEFT_PARENTHESIS:
        case TK_LEFT_SQUARE_BRACKET:
        case TK_FULL_STOP:
        case TK_ARROW:
        case TK_PLUSPLUS:
        case TK_MINUSMINUS:
            return true;
    }
    return false;
}

static bool IsFunction(struct TypeName* typeName)
{
    if (typeName->Declarator.pDirectDeclarator == NULL)
        return false;

    if (typeName->Declarator.pDirectDeclarator->DeclaratorType != TDirectDeclaratorTypeFunction)
        return false;

    if (typeName->Declarator.pDirectDeclarator->pDeclarator != NULL)
    {
        if (typeName->Declarator.pDirectDeclarator->pDeclarator->PointerList.pHead != NULL)
        {
            //È um ponteiro para funcao
            return false;
        }
    }



    return true;

}


void PostfixExpressionJump(struct Parser* ctx,
                           struct TypeName *typeName, 
                           struct TokenList* tempList0,
                           struct TokenList* tempList1,
                           struct Expression** ppExpression)
{
    /*
      chamado no cast expression quando entrou no caminho errado
      entao tem que entrar em uma PostfixExpression ja sabendo que eh
      (type - name)
    */
    *ppExpression = NULL;//out

    /*
      aterriza nesta sitacao
      postfix-expression:
        ( type-name ) { initializer-list }
        ( type-name ) { initializer-list , }


        CPRIME GRAMMAR EXTENSION
        SE TYPE NAME FOR FUNCAO exemplo (void(int)) entao eh considerado
        uma literal function (antigo lambda)
        ( type-name ) compound-statement
    */

    struct PostfixExpression* pTPostfixExpressionBase = NULL;

    //struct PostfixExpression* pTPostfixExpressionCore =
        //NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);

    //pTPostfixExpressionCore->token = TK_LEFT_PARENTHESIS;
    enum TokenType token2;

    /*foram recolhidos lah fora*/
    

    if (IsFunction(typeName))
    {
        struct PrimaryExpressionLambda* pPrimaryExpressionLambda =
            NEW((struct PrimaryExpressionLambda)PRIMARYEXPRESSIONLAMBDA_INIT);
        TokenList_Swap(&pPrimaryExpressionLambda->ClueList0, tempList0);

        TypeName_Swap(&pPrimaryExpressionLambda->TypeName, typeName);
        Compound_Statement(ctx, &pPrimaryExpressionLambda->pCompoundStatement);

        token2 = Parser_CurrentTokenType(ctx);
        if (HasPostfixExpressionContinuation(token2))
        {
            pTPostfixExpressionBase =
                NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
            pTPostfixExpressionBase->pExpressionLeft = pPrimaryExpressionLambda;
            *ppExpression = (struct Expression*)pTPostfixExpressionBase;
        }
        else
        {
            *ppExpression = pPrimaryExpressionLambda;
        }

    }
    else
    {
        Parser_MatchToken(ctx, TK_LEFT_CURLY_BRACKET, NULL);

        struct PostfixExpression* pTPostfixExpressionCore =
            NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
        pTPostfixExpressionCore->pTypeName = NEW((struct TypeName)TYPENAME_INIT);
        TypeName_Swap(pTPostfixExpressionCore->pTypeName, typeName);
        TokenList_Swap(&pTPostfixExpressionCore->ClueList0, tempList0);
        TokenList_Swap(&pTPostfixExpressionCore->ClueList1, tempList1);
        
        // 
        // 
        //pTPostfixExpressionCore->pInitializerList = TInitializerList_Create();
        Initializer_List(ctx, &pTPostfixExpressionCore->InitializerList);
        //Initializer_List(ctx, pTPostfixExpressionCore->pInitializerList);
        if (Parser_CurrentTokenType(ctx) == TK_COMMA)
        {
            Parser_Match(ctx, NULL);
        }

        Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET, NULL);
        //*ppExpression = (struct Expression*)pTPostfixExpressionCore;
        //..pTPostfixExpressionBase = pTPostfixExpressionCore;


        token2 = Parser_CurrentTokenType(ctx);
        if (HasPostfixExpressionContinuation(token2))
        {
            pTPostfixExpressionBase =
                NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
            pTPostfixExpressionBase->pExpressionLeft = pTPostfixExpressionCore;
            *ppExpression = (struct Expression*)pTPostfixExpressionBase;
        }
        else
        {
            *ppExpression = pTPostfixExpressionCore;
        }

        //Parser_MatchToken(ctx, TK_LEFT_CURLY_BRACKET, &pTPostfixExpressionCore->ClueList2);
        //Initializer_List(ctx, &pTPostfixExpressionCore->InitializerList);
        //Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET, &pTPostfixExpressionCore->ClueList3);
    }
    
    //if (Parser_CurrentTokenType(ctx) == TK_COMMA)
    //{
        //Parser_Match(ctx, &pTPostfixExpressionCore->ClueList4);
    //}
    //*ppExpression = (struct Expression*)pTPostfixExpressionCore;
    
   token2 = Parser_CurrentTokenType(ctx);
    
    //if (HasPostfixExpressionContinuation)
    //struct PostfixExpression* pNext =
      //  NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
    //pTPostfixExpressionBase->pNext = pNext;
    //pTPostfixExpressionBase = pNext;

    
    while (HasPostfixExpressionContinuation(token2))
    {
        switch (token2)
        {
            case TK_LEFT_PARENTHESIS:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                //  postfix-expression ( argument-expression-listopt )
                token2 = Parser_Match(ctx,
                                      &pTPostfixExpressionBase->ClueList0);
                if (token2 != TK_RIGHT_PARENTHESIS)
                {
                    ArgumentExpressionList(ctx, &pTPostfixExpressionBase->ArgumentExpressionList);
                }
                /*verificacao dos parametros da funcao*/
                if (pTPostfixExpressionBase->pExpressionLeft != NULL)
                {
                    struct TypeName* pTypeName = Expression_GetTypeName(pTPostfixExpressionBase->pExpressionLeft);
                    if (pTypeName && pTypeName->Declarator.pDirectDeclarator)
                    {
                        bool bVariadicArgs = pTypeName->Declarator.pDirectDeclarator->Parameters.bVariadicArgs;
                        bool bIsVoid = pTypeName->Declarator.pDirectDeclarator->Parameters.bIsVoid;
                        bool bIsEmpty = pTypeName->Declarator.pDirectDeclarator->Parameters.ParameterList.pHead == NULL;
                        int sz = pTPostfixExpressionBase->ArgumentExpressionList.size;
                        int argcount = 0;
                        if (!bIsVoid)
                        {
                            for (struct Parameter* pParameter = pTypeName->Declarator.pDirectDeclarator->Parameters.ParameterList.pHead;
                                 pParameter;
                                 pParameter = pParameter->pNext)
                            {
                                argcount++;
                                if (argcount > sz)
                                {
                                    SetError(ctx, "too few arguments for call");
                                    break;
                                }
                            }
                        }
                        else
                        {
                            argcount = 0;
                        }
                        if (!bIsEmpty && !bVariadicArgs && pTPostfixExpressionBase->ArgumentExpressionList.size > argcount)
                        {
                            SetError(ctx, "too many actual parameters");
                        }
                    }
                    else
                    {
                        //destroy ainda nao foi declarado por ex
                        //assert(false);
                        //SetWarning(ctx, "function cannot be verified");
                    }
                }//
                Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS,
                                  &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_LEFT_SQUARE_BRACKET:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                // postfix-expression [ expression ]
                Parser_MatchToken(ctx, TK_LEFT_SQUARE_BRACKET,
                                  &pTPostfixExpressionBase->ClueList0);
                Expression(ctx, &pTPostfixExpressionBase->pExpressionRight);
                Parser_MatchToken(ctx, TK_RIGHT_SQUARE_BRACKET, &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_FULL_STOP:
            {
                // postfix-expression . identifier
                pTPostfixExpressionBase->token = token2;
                Parser_Match(ctx, &pTPostfixExpressionBase->ClueList0);
                pTPostfixExpressionBase->Identifier = strdup(Lexeme(ctx));
                Parser_MatchToken(ctx, TK_IDENTIFIER, &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_ARROW:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                Parser_Match(ctx, &pTPostfixExpressionBase->ClueList0);
                pTPostfixExpressionBase->Identifier = strdup(Lexeme(ctx));
                Parser_MatchToken(ctx, TK_IDENTIFIER, &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_MINUSMINUS:
            case TK_PLUSPLUS:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                Parser_Match(ctx, &pTPostfixExpressionBase->ClueList0);
            }
            break;
            default:
                break;
        }
        token2 = Parser_CurrentTokenType(ctx);
        if (HasPostfixExpressionContinuation(token2))
        {
            struct PostfixExpression* pNext =
                NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
            pTPostfixExpressionBase->pNext = pNext;
            pTPostfixExpressionBase = pNext;
        }
        else
            break;
    }
}

void PostfixExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    *ppExpression = NULL;//out
    /*
      postfix-expression:
        primary-expression
        ( type-name ) { initializer-list }
        ( type-name ) { initializer-list , }

        postfix-expression [ expression ]
        postfix-expression ( argument-expression-listopt )
        postfix-expression . identifier
        postfix-expression -> identifier
        postfix-expression ++
        postfix-expression --


         new (type-name) { initializer-list }_opt
    */
    enum TokenType token2 = Parser_CurrentTokenType(ctx);
    struct PostfixExpression* pTPostfixExpressionBase = NULL;
    if (token2 == TK_LEFT_PARENTHESIS)
    {
        const char* lookAheadlexeme = Parser_LookAheadLexeme(ctx);
        enum TokenType lookAheadToken = Parser_LookAheadToken(ctx);
        if (IsTypeName(ctx, lookAheadToken, lookAheadlexeme))
        {
            //o que parece eh que nunca cai aqui pq castexpression vem antes e depois chama o PostfixExpressionJump
            assert(false);//chamar PostfixExpressionJump?

            // ( type-name ) { initializer-list }
            struct PostfixExpression* pTPostfixExpressionCore =
                NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
            pTPostfixExpressionCore->token = TK_LEFT_PARENTHESIS;
            Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pTPostfixExpressionCore->ClueList0);
            struct TypeName typeName = TYPENAME_INIT;
            TypeName(ctx, &typeName);
            TypeName_Destroy(&typeName);
            Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pTPostfixExpressionCore->ClueList1);
            Parser_MatchToken(ctx, TK_LEFT_CURLY_BRACKET, &pTPostfixExpressionCore->ClueList2);
            Initializer_List(ctx, &pTPostfixExpressionCore->InitializerList);
            Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET, &pTPostfixExpressionCore->ClueList3);
            if (Parser_CurrentTokenType(ctx) == TK_COMMA)
            {
                Parser_Match(ctx, &pTPostfixExpressionCore->ClueList4);
            }
            *ppExpression = (struct Expression*)pTPostfixExpressionCore;
            pTPostfixExpressionBase = pTPostfixExpressionCore;
        }
        else
        {
            /*primary-expression*/
            struct Expression* pPrimaryExpression;
            PrimaryExpression(ctx, &pPrimaryExpression);
            *ppExpression = pPrimaryExpression;
            token2 = Parser_CurrentTokenType(ctx);
            if (HasPostfixExpressionContinuation(token2))
            {
                pTPostfixExpressionBase =
                    NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
                pTPostfixExpressionBase->pExpressionLeft = pPrimaryExpression;
                *ppExpression = (struct Expression*)pTPostfixExpressionBase;
            }
            else
            {
                *ppExpression = pPrimaryExpression;
            }
        }
    }
    else if (token2 == TK_NEW)
    {
        struct PostfixExpression* pTPostfixExpressionCore =
            NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
        Parser_MatchToken(ctx, TK_NEW, &pTPostfixExpressionCore->ClueList0);
        pTPostfixExpressionCore->token = TK_NEW;
        Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pTPostfixExpressionCore->ClueList0);
        assert(pTPostfixExpressionCore->pTypeName == NULL);
        pTPostfixExpressionCore->pTypeName = NEW((struct TypeName)TYPENAME_INIT);
        TypeName(ctx, pTPostfixExpressionCore->pTypeName);
        Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pTPostfixExpressionCore->ClueList1);
        enum TokenType token = Parser_CurrentTokenType(ctx);
        if (token == TK_LEFT_CURLY_BRACKET)
        {
            //optional
            Parser_MatchToken(ctx, TK_LEFT_CURLY_BRACKET, &pTPostfixExpressionCore->ClueList2);
            Initializer_List(ctx, &pTPostfixExpressionCore->InitializerList);
            Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET, &pTPostfixExpressionCore->ClueList3);
        }
        else
        {
            //equivalente a vazio (default)
        }
        *ppExpression = (struct Expression*)pTPostfixExpressionCore;
    }
    else
    {
        /*primary-expression*/
        struct Expression* pPrimaryExpression;
        PrimaryExpression(ctx, &pPrimaryExpression);
        token2 = Parser_CurrentTokenType(ctx);
        if (HasPostfixExpressionContinuation(token2))
        {
            pTPostfixExpressionBase =
                NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
            pTPostfixExpressionBase->pExpressionLeft = pPrimaryExpression;
            *ppExpression = (struct Expression*)pTPostfixExpressionBase;
        }
        else
        {
            *ppExpression = pPrimaryExpression;
        }
    }
    token2 = Parser_CurrentTokenType(ctx);
    while (HasPostfixExpressionContinuation(token2))
    {
        switch (token2)
        {
            case TK_LEFT_PARENTHESIS:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                //  postfix-expression ( argument-expression-listopt )
                token2 = Parser_Match(ctx,
                                      &pTPostfixExpressionBase->ClueList0);
                if (token2 != TK_RIGHT_PARENTHESIS)
                {
                    ArgumentExpressionList(ctx, &pTPostfixExpressionBase->ArgumentExpressionList);
                }
                /*verificacao dos parametros da funcao*/
                if (pTPostfixExpressionBase->pExpressionLeft != NULL)
                {
                    struct TypeName* pTypeName = Expression_GetTypeName(pTPostfixExpressionBase->pExpressionLeft);
                    if (pTypeName && pTypeName->Declarator.pDirectDeclarator)
                    {
                        bool bVariadicArgs = pTypeName->Declarator.pDirectDeclarator->Parameters.bVariadicArgs;
                        bool bIsVoid = pTypeName->Declarator.pDirectDeclarator->Parameters.bIsVoid;
                        bool bIsEmpty = pTypeName->Declarator.pDirectDeclarator->Parameters.ParameterList.pHead == NULL;
                        int sz = pTPostfixExpressionBase->ArgumentExpressionList.size;
                        int argcount = 0;
                        if (!bIsVoid)
                        {
                            for (struct Parameter* pParameter = pTypeName->Declarator.pDirectDeclarator->Parameters.ParameterList.pHead;
                                 pParameter;
                                 pParameter = pParameter->pNext)
                            {
                                argcount++;
                                if (argcount > sz)
                                {
                                    SetError(ctx, "too few arguments for call");
                                    break;
                                }
                            }
                        }
                        else
                        {
                            argcount = 0;
                        }
                        if (!bIsEmpty && !bVariadicArgs && pTPostfixExpressionBase->ArgumentExpressionList.size > argcount)
                        {
                            SetError(ctx, "too many actual parameters");
                        }
                    }
                    else
                    {
                        //destroy ainda nao foi declarado por ex
                        //assert(false);
                        //SetWarning(ctx, "function cannot be verified");
                    }
                }//
                Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS,
                                  &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_LEFT_SQUARE_BRACKET:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                // postfix-expression [ expression ]
                Parser_MatchToken(ctx, TK_LEFT_SQUARE_BRACKET,
                                  &pTPostfixExpressionBase->ClueList0);
                Expression(ctx, &pTPostfixExpressionBase->pExpressionRight);
                Parser_MatchToken(ctx, TK_RIGHT_SQUARE_BRACKET, &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_FULL_STOP:
            {
                // postfix-expression . identifier
                pTPostfixExpressionBase->token = token2;
                Parser_Match(ctx, &pTPostfixExpressionBase->ClueList0);
                pTPostfixExpressionBase->Identifier = strdup(Lexeme(ctx));
                Parser_MatchToken(ctx, TK_IDENTIFIER, &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_ARROW:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                Parser_Match(ctx, &pTPostfixExpressionBase->ClueList0);
                pTPostfixExpressionBase->Identifier = strdup(Lexeme(ctx));
                Parser_MatchToken(ctx, TK_IDENTIFIER, &pTPostfixExpressionBase->ClueList1);
            }
            break;
            case TK_MINUSMINUS:
            case TK_PLUSPLUS:
            {
                assert(pTPostfixExpressionBase);
                pTPostfixExpressionBase->token = token2;
                Parser_Match(ctx, &pTPostfixExpressionBase->ClueList0);
            }
            break;
            default:
                break;
        }
        token2 = Parser_CurrentTokenType(ctx);
        if (HasPostfixExpressionContinuation(token2))
        {
            struct PostfixExpression* pNext =
                NEW((struct PostfixExpression)POSTFIXEXPRESSIONCORE_INIT);
            pTPostfixExpressionBase->pNext = pNext;
            pTPostfixExpressionBase = pNext;
        }
        else
            break;
    }
}

void ArgumentExpressionList(struct Parser* ctx, struct ArgumentExpressionList* pArgumentExpressionList)
{
    /*argument-expression-list:
       assignment-expression
       argument-expression-list , assignment-expression
    */
    struct Expression* pAssignmentExpression;
    AssignmentExpression(ctx, &pAssignmentExpression);
    struct ArgumentExpression* pNew = NEW((struct ArgumentExpression)ARGUMENTEXPRESSION_INIT);
    pNew->pExpression = pAssignmentExpression;
    ArgumentExpressionList_PushBack(pArgumentExpressionList, pNew);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    while (token == TK_COMMA)
    {
        struct ArgumentExpression* pNew2 = NEW((struct ArgumentExpression)ARGUMENTEXPRESSION_INIT);
        pNew2->bHasComma = true;
        Parser_Match(ctx, &pNew2->ClueList0);
        struct Expression* pAssignmentExpressionRight;
        AssignmentExpression(ctx, &pAssignmentExpressionRight);
        pNew2->pExpression = pAssignmentExpressionRight;
        ArgumentExpressionList_PushBack(pArgumentExpressionList, pNew2);
        token = Parser_CurrentTokenType(ctx);
    }
}

static bool IsTypeQualifierToken(enum TokenType token)
{
    bool bResult = false;
    switch (token)
    {
        //type-qualifier
        case TK_CONST:
        case TK_RESTRICT:
        case TK_VOLATILE:
        case TK__ATOMIC:
            //
            bResult = true;
            break;
#ifdef LANGUAGE_EXTENSIONS
            //type-qualifier-extensions
        case TK_AUTO:
            bResult = true;
            break;
#endif
        default:
            //assert(false);
            break;
    }
    return bResult;
}

int IsTypeName(struct Parser* ctx, enum TokenType token, const char* lexeme)
{
    int bResult = false;
    if (lexeme == NULL)
    {
        return false;
    }
    switch (token)
    {
        case TK_IDENTIFIER:
            bResult = SymbolMap_IsTypeName(ctx->pCurrentScope, lexeme);
            //        bResult = DeclarationsMap_IsTypeDef(&ctx->Symbols, lexeme);
            break;
            //type-qualifier
        case TK_CONST:
        case TK_RESTRICT:
        case TK_VOLATILE:
        case TK__ATOMIC:
#ifdef LANGUAGE_EXTENSIONS
        case TK_AUTO:
#endif
            //type-specifier
        case TK_VOID:
        case TK_CHAR:
        case TK_SHORT:
        case TK_INT:
        case TK_LONG:
            //microsoft
        case TK__INT8:
        case TK__INT16:
        case TK__INT32:
        case TK__INT64:
        case TK___DECLSPEC:
        case TK__WCHAR_T:
            //
        case TK_FLOAT:
        case TK_DOUBLE:
        case TK_SIGNED:
        case TK_UNSIGNED:
        case TK__BOOL:
        case TK__COMPLEX:
        case TK_STRUCT:
        case TK_UNION:
        case TK_ENUM:
            bResult = true;
            break;
        default:
            //assert(false);
            break;
    }
    return bResult;
}

void UnaryExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    *ppExpression = NULL; //out

    /*
     unary-expression:
       postfix-expression
       ++ unary-expression
       -- unary-expression
       unary-operator cast-expression
       sizeof unary-expression
       sizeof ( type-name )
       alignof ( type-name )

      unary-operator: one of
        & * + - ~ !
    */
    enum TokenType token0 = Parser_CurrentTokenType(ctx);
    enum TokenType tokenAhead = Parser_LookAheadToken(ctx);
    const char* lookAheadlexeme2 = Parser_LookAheadLexeme(ctx);
    if (token0 == TK_NEW || IsTypeName(ctx, tokenAhead, lookAheadlexeme2))
    {
        //first para postfix-expression
        struct Expression* pPostfixExpression;
        PostfixExpression(ctx, &pPostfixExpression);
        *ppExpression = (struct Expression*)(pPostfixExpression);
        return;
    }
    else if (IsFirstOfPrimaryExpression(token0))
    {
        //primary-expression √© first para postfix-expression
        struct Expression* pPostfixExpression;
        PostfixExpression(ctx, &pPostfixExpression);
        *ppExpression = (struct Expression*)(pPostfixExpression);
        return;
    }
    switch (token0)
    {
        case TK_PLUSPLUS:
        case TK_MINUSMINUS:
        {
            struct UnaryExpressionOperator* pUnaryExpressionOperator =
                NEW((struct UnaryExpressionOperator)UNARYEXPRESSIONOPERATOR_INIT);
            Parser_Match(ctx, &pUnaryExpressionOperator->ClueList0);
            struct Expression* pUnaryExpression;
            UnaryExpression(ctx, &pUnaryExpression);
            pUnaryExpressionOperator->token = token0;
            pUnaryExpressionOperator->pExpressionRight = pUnaryExpression;
            *ppExpression = (struct Expression*)pUnaryExpressionOperator;
        }
        break;
        //unary-operator cast-expression
        case TK_AMPERSAND:
        case TK_ASTERISK:
        case TK_PLUS_SIGN:
        case TK_HYPHEN_MINUS:
        case TK_TILDE:
        case TK_EXCLAMATION_MARK:
        {
            struct UnaryExpressionOperator* pUnaryExpressionOperator =
                NEW((struct UnaryExpressionOperator)UNARYEXPRESSIONOPERATOR_INIT);
            Parser_Match(ctx, &pUnaryExpressionOperator->ClueList0);
            struct Expression* pCastExpression;
            CastExpression(ctx, &pCastExpression);
            pUnaryExpressionOperator->token = token0;
            pUnaryExpressionOperator->pExpressionRight = pCastExpression;
            *ppExpression = (struct Expression*)pUnaryExpressionOperator;
            if (token0 == TK_AMPERSAND)
            {
                TypeName_CopyTo(Expression_GetTypeName(pCastExpression), &pUnaryExpressionOperator->TypeName);
                struct Pointer* pPointer = NEW((struct Pointer)POINTER_INIT);
                //adicionar no inicio ou fim?
                PointerList_PushBack(&pUnaryExpressionOperator->TypeName.Declarator.PointerList, pPointer);
            }
            else if (token0 == TK_ASTERISK)
            {
                //remover ponteiro
                TypeName_CopyTo(Expression_GetTypeName(pCastExpression), &pUnaryExpressionOperator->TypeName);
                PointerList_PopFront(&pUnaryExpressionOperator->TypeName.Declarator.PointerList);
            }
            else
            {
                TypeName_CopyTo(Expression_GetTypeName(pCastExpression), &pUnaryExpressionOperator->TypeName);
                //TODO
                // assert(false);
            }
        }
        break;
        //////////////
        case TK_SIZEOF:
        {
            struct UnaryExpressionOperator* pUnaryExpressionOperator =
                NEW((struct UnaryExpressionOperator)UNARYEXPRESSIONOPERATOR_INIT);
            *ppExpression = (struct Expression*)pUnaryExpressionOperator;
            pUnaryExpressionOperator->token = token0;
            Parser_MatchToken(ctx, TK_SIZEOF, &pUnaryExpressionOperator->ClueList0);
            if (Parser_CurrentTokenType(ctx) == TK_LEFT_PARENTHESIS)
            {
                const char* lookAheadlexeme = Parser_LookAheadLexeme(ctx);
                enum TokenType lookAheadToken = Parser_LookAheadToken(ctx);
                if (IsTypeName(ctx, lookAheadToken, lookAheadlexeme))
                {
                    //sizeof(type-name)
                    Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pUnaryExpressionOperator->ClueList1);
                    TypeName(ctx, &pUnaryExpressionOperator->TypeName);
                    Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pUnaryExpressionOperator->ClueList2);
                }
                else
                {
                    //sizeof unary-expression
                    struct Expression* pTUnaryExpression;
                    UnaryExpression(ctx, &pTUnaryExpression);
                    pUnaryExpressionOperator->pExpressionRight = pTUnaryExpression;
                }
            }
            else
            {
                //sizeof do tipo desta expressao
                struct Expression* pTUnaryExpression;
                UnaryExpression(ctx, &pTUnaryExpression);
                pUnaryExpressionOperator->pExpressionRight = pTUnaryExpression;
            }
        }
        break;
        case TK__ALINGOF:
            //Match
            //assert(false);
            break;
        case TK_EOF:
            break;
            //TODO ver tudo que pode ser follow
        default:
            ////assert(false);
            //        SetUnexpectedError(ctx, "Assert", "");
            //aqui nao eh erro necessariamente
            break;
    }
}



void CastExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    *ppExpression = NULL; //out
    /*
    cast-expression:
    unary-expression
    ( type-name ) cast-expression
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_LEFT_PARENTHESIS)
    {
        const char* lookAheadlexeme = Parser_LookAheadLexeme(ctx);
        enum TokenType lookAheadToken = Parser_LookAheadToken(ctx);
        if (IsTypeName(ctx, lookAheadToken, lookAheadlexeme))
        {
            struct TokenList tempList0 = { 0, 0 };
            Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &tempList0);
            struct TypeName typeName = TYPENAME_INIT;
            TypeName(ctx, &typeName);
            struct TokenList tempList1 = { 0, 0 };
            token = Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &tempList1);
            if (token == TK_LEFT_CURLY_BRACKET)
            {
                PostfixExpressionJump(ctx, &typeName, &tempList0, &tempList1, ppExpression);
                /*
                   Se isso acontecer quer dizer que pode enganho achou que era
                   ( type-name ) cast-expression

                   mas na verdade era postfix-expression:

                     ( type-name ) { initializer-list }
                     ( type-name ) { initializer-list , }

                   postfix-expression:
                     primary-expression
                     ( type-name ) { initializer-list }
                     ( type-name ) { initializer-list , }
                     ...
                */

        
            }
            else
            {
                struct CastExpressionType* pCastExpressionType =
                    NEW((struct CastExpressionType)CASTEXPRESSIONTYPE_INIT);
                TokenList_Swap(&tempList0, &pCastExpressionType->ClueList0);
                TokenList_Swap(&tempList1, &pCastExpressionType->ClueList1);
                struct Expression* pCastExpression;
                CastExpression(ctx, &pCastExpression);
                TypeName_Swap(&pCastExpressionType->TypeName, &typeName);
                pCastExpressionType->pExpression = pCastExpression;
                *ppExpression = (struct Expression*)pCastExpressionType;
            }
            TypeName_Destroy(&typeName);
            TokenList_Destroy(&tempList0);
            TokenList_Destroy(&tempList1);
        }
        else
        {
            struct Expression* pUnaryExpression;
            UnaryExpression(ctx, &pUnaryExpression);
            *ppExpression = pUnaryExpression;
        }
    }
    else
    {
        struct Expression* pUnaryExpression;
        UnaryExpression(ctx, &pUnaryExpression);
        *ppExpression = pUnaryExpression;
    }
}

void MultiplicativeExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*
    (6.5.5) multiplicative-expression:
    cast-expression                                // identifier  constant  char-literal  (  _Generic ++      --     & * + - ~ !         sizeof          sizeof      alignof
    multiplicative-expression * cast-expression
    multiplicative-expression / cast-expression
    multiplicative-expression % cast-expression
    */
    struct Expression* pExpressionLeft;
    CastExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_PERCENT_SIGN:
        case TK_SOLIDUS:
        case TK_ASTERISK:
        {
            struct BinaryExpression* pBinaryExpression =
                NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            GetPosition(ctx, &pBinaryExpression->Position);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            CastExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_PERCENT_SIGN:
        case TK_SOLIDUS:
        case TK_ASTERISK:
        {
            struct BinaryExpression* pBinaryExpression =
                NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            MultiplicativeExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void AdditiveExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*
    (6.5.6) additive-expression:
    multiplicative-expression
    additive-expression + multiplicative-expression
    additive-expression - multiplicative-expression
    */
    struct Expression* pExpressionLeft;
    MultiplicativeExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_PLUS_SIGN:
        case TK_HYPHEN_MINUS:
        {
            struct BinaryExpression* pBinaryExpression =
                NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            GetPosition(ctx, &pBinaryExpression->Position);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            MultiplicativeExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_PLUS_SIGN:
        case TK_HYPHEN_MINUS:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            AdditiveExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void ShiftExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*(6.5.7) shift-expression:
    additive-expression
    shift-expression << additive-expression
    shift-expression >> additive-expression
    */
    struct Expression* pExpressionLeft;
    AdditiveExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_GREATERGREATER:
        case TK_LESSLESS:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            AdditiveExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_GREATERGREATER:
        case TK_LESSLESS:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            ShiftExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void RelationalExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*
    (6.5.8) relational-expression:
    shift-expression
    relational-expression < shift-expression
    relational-expression > shift-expression
    relational-expression <= shift-expression
    relational-expression >= shift-expression
    */
    struct Expression* pExpressionLeft;
    ShiftExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_LESS_THAN_SIGN:
        case TK_GREATER_THAN_SIGN:
        case TK_GREATEREQUAL:
        case TK_LESSEQUAL:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            ShiftExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_LESS_THAN_SIGN:
        case TK_GREATER_THAN_SIGN:
        case TK_GREATEREQUAL:
        case TK_LESSEQUAL:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            RelationalExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void EqualityExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*(6.5.9) equality-expression:
    relational-expression
    equality-expression == relational-expression
    equality-expression != relational-expression
    */
    struct Expression* pExpressionLeft;
    RelationalExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_EQUALEQUAL:
        case TK_NOTEQUAL:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            RelationalExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_EQUALEQUAL:
        case TK_NOTEQUAL:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            EqualityExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void AndExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*(6.5.10) AND-expression:
    equality-expression
    AND-expression & equality-expression
    */
    struct Expression* pExpressionLeft;
    EqualityExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_AMPERSAND:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            EqualityExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_AMPERSAND:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            AndExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void ExclusiveOrExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*
    (6.5.11) exclusive-OR-expression:
    AND-expression
    exclusive-OR-expression ^ AND-expression
    */
    struct Expression* pExpressionLeft;
    AndExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_CIRCUMFLEX_ACCENT:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            AndExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_CIRCUMFLEX_ACCENT:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            ExclusiveOrExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void InclusiveOrExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*
    (6.5.12) inclusive-OR-expression:
    exclusive-OR-expression
    inclusive-OR-expression | exclusive-OR-expression
    */
    struct Expression* pExpressionLeft;
    ExclusiveOrExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_VERTICAL_LINE:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            ExclusiveOrExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_VERTICAL_LINE:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            InclusiveOrExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void LogicalAndExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*
    (6.5.13) logical-AND-expression:
    inclusive-OR-expression
    logical-AND-expression && inclusive-OR-expression
    */
    struct Expression* pExpressionLeft;
    InclusiveOrExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_ANDAND:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            GetPosition(ctx, &pBinaryExpression->Position);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            InclusiveOrExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_ANDAND:
        {
            struct BinaryExpression* pBinaryExpression =
                NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            GetPosition(ctx, &pBinaryExpression->Position);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            LogicalAndExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void LogicalOrExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*(6.5.14) logical-OR-expression:
    logical-AND-expression
    logical-OR-expression || logical-AND-expression
    */
    struct Expression* pExpressionLeft;
    LogicalAndExpression(ctx, &pExpressionLeft);
    *ppExpression = pExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_OROR:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            GetPosition(ctx, &pBinaryExpression->Position);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            LogicalAndExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
    token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_OROR:
        {
            struct BinaryExpression* pBinaryExpression = NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            GetPosition(ctx, &pBinaryExpression->Position);
            pBinaryExpression->token = token;
            pBinaryExpression->pExpressionLeft = *ppExpression;
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            struct Expression* pExpressionRight;
            LogicalOrExpression(ctx, &pExpressionRight);
            pBinaryExpression->pExpressionRight = pExpressionRight;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void ConditionalExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*(6.5.15) conditional-expression:
    logical-OR-expression
    logical-OR-expression ? expression : conditional-expression
    */
    struct Expression* pLogicalOrExpressionLeft;
    LogicalOrExpression(ctx, &pLogicalOrExpressionLeft);
    *ppExpression = pLogicalOrExpressionLeft;
    if (Parser_CurrentTokenType(ctx) == TK_QUESTION_MARK)
    {
        struct TernaryExpression* pTernaryExpression =
            NEW((struct TernaryExpression)TERNARYEXPRESSION_INIT);
        Parser_Match(ctx, &pTernaryExpression->ClueList0);
        struct Expression* pTExpression;
        Expression(ctx, &pTExpression);
        Parser_MatchToken(ctx, TK_COLON, &pTernaryExpression->ClueList1);
        struct Expression* pConditionalExpressionRight;
        ConditionalExpression(ctx, &pConditionalExpressionRight);
        pTernaryExpression->token = TK_QUESTION_MARK;
        pTernaryExpression->pExpressionLeft = pLogicalOrExpressionLeft;
        pTernaryExpression->pExpressionMiddle = pTExpression;
        pTernaryExpression->pExpressionRight = pConditionalExpressionRight;
        *ppExpression = (struct Expression*)pTernaryExpression;
    }
}

void AssignmentExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    /*(6.5.16) assignment-expression:
    conditional-expression
    unary-expression assignment-operator assignment-expression

    (6.5.16) assignment-operator: one of
    = *= /= %= += -= <<= >>= &= ^= |=
    */
    //N√£o sei se eh  conditional-expression ou
    //unary-expression
    //Mas a conditional-expression faz tambem a
    //unary-expression
    struct Expression* pConditionalExpressionLeft;
    ConditionalExpression(ctx, &pConditionalExpressionLeft);
    *ppExpression = pConditionalExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_EQUALS_SIGN:
        case TK_MULTIEQUAL:
        case TK_DIVEQUAL:
        case TK_PERCENT_EQUAL:
        case TK_PLUSEQUAL:
        case TK_MINUS_EQUAL:
        case TK_LESSLESSEQUAL:
        case TK_GREATERGREATEREQUAL:
        case TK_ANDEQUAL:
        case TK_CARETEQUAL:
        case TK_OREQUAL:
        {
            struct BinaryExpression* pBinaryExpression =
                NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
            Parser_Match(ctx, &pBinaryExpression->ClueList0);
            //Significa que o anterior deve ser do tipo  unary-expression
            //embora tenhamos feito o parser de conditional-expression
            //se nao for √© erro.
            struct Expression* pAssignmentExpressionRight;
            AssignmentExpression(ctx, &pAssignmentExpressionRight);
            GetPosition(ctx, &pBinaryExpression->Position);
            pBinaryExpression->pExpressionLeft = *ppExpression;
            pBinaryExpression->pExpressionRight = pAssignmentExpressionRight;
            pBinaryExpression->token = token;
            *ppExpression = (struct Expression*)pBinaryExpression;
        }
        break;
        default:
            //√â apenas conditional-expression
            break;
    }
}

void Expression(struct Parser* ctx, struct Expression** ppExpression)
{
    *ppExpression = NULL; //out
    /*
    (6.5.17) expression:
    assignment-expression
    expression , assignment-expression
    */
    struct Expression* pAssignmentExpressionLeft;
    AssignmentExpression(ctx, &pAssignmentExpressionLeft);
    *ppExpression = pAssignmentExpressionLeft;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_COMMA)
    {
        struct Expression* pAssignmentExpressionRight;
        Parser_Match(ctx, NULL);
        Expression(ctx, &pAssignmentExpressionRight);
        struct BinaryExpression* pBinaryExpression =
            NEW((struct BinaryExpression)BINARYEXPRESSION_INIT);
        GetPosition(ctx, &pBinaryExpression->Position);
        pBinaryExpression->pExpressionLeft = *ppExpression;
        pBinaryExpression->pExpressionRight = pAssignmentExpressionRight;
        pBinaryExpression->token = TK_COMMA;
        *ppExpression = (struct Expression*)pBinaryExpression;
    }
}

void ConstantExpression(struct Parser* ctx, struct Expression** ppExpression)
{
    *ppExpression = NULL; //out
    /*
    (6.6) constant-expression:
    conditional-expression
    */
    ConditionalExpression(ctx, ppExpression);
}


///////////////////////////////////////////////////////////////////////////////


void Designator(struct Parser* ctx, struct Designator* pDesignator);
void Designator_List(struct Parser* ctx, struct DesignatorList* pDesignatorList);
void Designation(struct Parser* ctx, struct DesignatorList* pDesignatorList);
void Initializer_List(struct Parser* ctx, struct InitializerList* pInitializerList);
bool Statement(struct Parser* ctx, struct Statement** ppStatement);
void Compound_Statement(struct Parser* ctx, struct CompoundStatement** ppStatement);
void Parameter_Declaration(struct Parser* ctx, struct Parameter* pParameterDeclaration);
bool Declaration(struct Parser* ctx, struct AnyDeclaration** ppDeclaration);
void Type_Qualifier_List(struct Parser* ctx, struct TypeQualifierList* pQualifiers);
void Declaration_Specifiers(struct Parser* ctx, struct DeclarationSpecifiers* pDeclarationSpecifiers);
void Type_Specifier(struct Parser* ctx, struct TypeSpecifier** ppTypeSpecifier);
bool Type_Qualifier(struct Parser* ctx, struct TypeQualifier* pQualifier);
void Initializer(struct Parser* ctx, struct Initializer** ppInitializer);


void Expression_Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    /*
    expression-statement:
       expression_opt;
    */
    struct ExpressionStatement* pExpression = NEW((struct ExpressionStatement)EXPRESSIONSTATEMENT_INIT);
    *ppStatement = (struct Statement*)pExpression;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token != TK_SEMICOLON)
    {
        Expression(ctx, &pExpression->pExpression);
    }
    Parser_MatchToken(ctx, TK_SEMICOLON, &pExpression->ClueList0);
}

void Selection_Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    /*
    selection-statement:
    if ( expression ) statement
    if ( expression ) statement else statement
    switch ( expression ) statement
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_IF:
        {
            struct IfStatement* pIfStatement = NEW((struct IfStatement)IFSTATEMENT_INIT);
            *ppStatement = (struct Statement*)pIfStatement;
            Parser_Match(ctx, &pIfStatement->ClueList0);
            Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pIfStatement->ClueList1);
            struct SymbolMap BlockScope = SYMBOLMAP_INIT;
            BlockScope.pPrevious = ctx->pCurrentScope;
            ctx->pCurrentScope = &BlockScope;
            //primeira expressao do if
            bool bHasDeclaration = Declaration(ctx, &pIfStatement->pInitDeclarationOpt);
            if (bHasDeclaration)
            {
                token = Parser_CurrentTokenType(ctx);
                //Esta eh a 2 expressao do if a que tem a condicao a declaracao ja comeu 1
                Expression(ctx, &pIfStatement->pConditionExpression);
                token = Parser_CurrentTokenType(ctx);
                if (token == TK_SEMICOLON)
                {
                    //TEM DEFER
                    Parser_MatchToken(ctx, TK_SEMICOLON, &pIfStatement->ClueList2);
                    Expression(ctx, &pIfStatement->pDeferExpression);
                }
                Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pIfStatement->ClueList4);
            }
            else /*if normal*/
            {
                struct Expression* pExpression = NULL;
                Expression(ctx, &pExpression);
                token = Parser_CurrentTokenType(ctx);

                if (token == TK_SEMICOLON)
                {
                    Parser_Match(ctx, &pIfStatement->ClueList2);
                    pIfStatement->pInitialExpression = pExpression;
                    Expression(ctx, &pIfStatement->pConditionExpression);
                }
                else if (token == TK_RIGHT_PARENTHESIS)
                {
                    //Parser_Match(ctx, &pIfStatement->ClueList2);
                    pIfStatement->pConditionExpression = pExpression;
                }
                else {
                    //error
                }

                token = Parser_CurrentTokenType(ctx);
                if (token == TK_SEMICOLON)
                {
                    //TEM DEFER
                    Parser_MatchToken(ctx, TK_SEMICOLON, &pIfStatement->ClueList3);
                    Expression(ctx, &pIfStatement->pDeferExpression);
                }
                Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pIfStatement->ClueList4);
            }


            Statement(ctx, &pIfStatement->pStatement);
            token = Parser_CurrentTokenType(ctx);
            if (token == TK_ELSE)
            {
                Parser_Match(ctx, &pIfStatement->ClueList3);
                Statement(ctx, &pIfStatement->pElseStatement);
            }



            ctx->pCurrentScope = BlockScope.pPrevious;
        }
        break;
        case TK_SWITCH:
        {
            struct SwitchStatement* pSelectionStatement = NEW((struct SwitchStatement)SWITCHSTATEMENT_INIT);
            *ppStatement = (struct Statement*)pSelectionStatement;
            Parser_Match(ctx, &pSelectionStatement->ClueList0);
            Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pSelectionStatement->ClueList1);
            Expression(ctx, &pSelectionStatement->pConditionExpression);
            Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pSelectionStatement->ClueList2);
            Statement(ctx, &pSelectionStatement->pExpression);
        }
        break;
        default:
            //assert(false);
            break;
    }
}

void Defer_Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_DEFER:
        {
            struct DeferStatement* pDeferStatement = NEW((struct DeferStatement)DEFERSTATEMENT_INIT);
            *ppStatement = (struct Statement*)pDeferStatement;
            Parser_Match(ctx, &pDeferStatement->ClueList0);
            Statement(ctx, &pDeferStatement->pStatement);
        }
        break;
    }
}
void Jump_Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    /*
    jump-statement:
    goto identifier ;
    continue ;
    break ;
    return expressionopt ;
    */
    //jump-statement
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_GOTO:
        {
            struct JumpStatement* pJumpStatement = NEW((struct JumpStatement)JUMPSTATEMENT_INIT);
            pJumpStatement->token = token;
            *ppStatement = (struct Statement*)pJumpStatement;
            Parser_Match(ctx, &pJumpStatement->ClueList0);
            pJumpStatement->Identifier = strdup(Lexeme(ctx));
            Parser_MatchToken(ctx, TK_IDENTIFIER, &pJumpStatement->ClueList1);
            Parser_MatchToken(ctx, TK_SEMICOLON, &pJumpStatement->ClueList2);
        }
        break;
        case TK_CONTINUE:
        {
            struct JumpStatement* pJumpStatement = NEW((struct JumpStatement)JUMPSTATEMENT_INIT);
            pJumpStatement->token = token;
            *ppStatement = (struct Statement*)pJumpStatement;
            Parser_Match(ctx, &pJumpStatement->ClueList0);
            Parser_MatchToken(ctx, TK_SEMICOLON, &pJumpStatement->ClueList2);
        }
        break;
        case TK_BREAK:
        {
            struct JumpStatement* pJumpStatement = NEW((struct JumpStatement)JUMPSTATEMENT_INIT);
            pJumpStatement->token = token;
            *ppStatement = (struct Statement*)pJumpStatement;
            Parser_Match(ctx, &pJumpStatement->ClueList0);
            Parser_MatchToken(ctx, TK_SEMICOLON, &pJumpStatement->ClueList2);
        }
        break;
        case TK_RETURN:
        {
            struct JumpStatement* pJumpStatement = NEW((struct JumpStatement)JUMPSTATEMENT_INIT);
            pJumpStatement->token = token;
            *ppStatement = (struct Statement*)pJumpStatement;
            token = Parser_Match(ctx, &pJumpStatement->ClueList0);
            if (token != TK_SEMICOLON)
            {
                Expression(ctx, &pJumpStatement->pExpression);
            }
            Parser_MatchToken(ctx, TK_SEMICOLON, &pJumpStatement->ClueList2);
        }
        break;
        case TK_THROW:
        {
            struct JumpStatement* pJumpStatement = NEW((struct JumpStatement)JUMPSTATEMENT_INIT);
            pJumpStatement->token = token;
            *ppStatement = (struct Statement*)pJumpStatement;
            token = Parser_Match(ctx, &pJumpStatement->ClueList0);
            if (token != TK_SEMICOLON)
            {
                Expression(ctx, &pJumpStatement->pExpression);
            }
            Parser_MatchToken(ctx, TK_SEMICOLON, &pJumpStatement->ClueList2);
        }
        break;

        default:
            //assert(false);
            break;
    }
}

void Iteration_Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    /*
    iteration-statement:
    while ( expression ) statement
    do statement while ( expression ) ;
    for ( expressionopt ; expressionopt ; expressionopt ) statement
    for ( declaration expressionopt ; expressionopt ) statement
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {

        case TK_WHILE:
        {
            struct WhileStatement* pWhileStatement = NEW((struct WhileStatement)WHILESTATEMENT_INIT);
            *ppStatement = (struct Statement*)pWhileStatement;
            Parser_Match(ctx, &pWhileStatement->ClueList0);
            Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pWhileStatement->ClueList1);
            Expression(ctx, &pWhileStatement->pExpression);
            Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pWhileStatement->ClueList2);
            Statement(ctx, &pWhileStatement->pStatement);
        }
        break;
        case TK_TRY:
        {
            struct TryBlockStatement* pTryBlockStatement = NEW((struct TryBlockStatement)TRYBLOCKSTATEMENT_INIT);
            *ppStatement = (struct Statement*)pTryBlockStatement;
            Parser_Match(ctx, &pTryBlockStatement->ClueListTry);
            Compound_Statement(ctx, &pTryBlockStatement->pCompoundStatement);
            token = Parser_CurrentTokenType(ctx);
            if (token == TK_CATCH)
            {
                Parser_MatchToken(ctx, TK_CATCH, &pTryBlockStatement->ClueListCatch);
                Compound_Statement(ctx, &pTryBlockStatement->pCompoundCatchStatement);
            }
            else
            {
                //sem catch
            }
        }
        break;
        case TK_DO:
        {
            struct DoStatement* pDoStatement = NEW((struct DoStatement)DOSTATEMENT_INIT);
            *ppStatement = (struct Statement*)pDoStatement;
            Parser_Match(ctx, &pDoStatement->ClueList0); //do
            Statement(ctx, &pDoStatement->pStatement);
            token = Parser_CurrentTokenType(ctx);

            Parser_MatchToken(ctx, TK_WHILE, &pDoStatement->ClueList1); //while
            Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pDoStatement->ClueList2); //(
            Expression(ctx, &pDoStatement->pExpression);
            Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pDoStatement->ClueList3); //)
            Parser_MatchToken(ctx, TK_SEMICOLON, &pDoStatement->ClueList4); //;

        }
        break;
        case TK_FOR:
        {
            struct ForStatement* pIterationStatement = NEW((struct ForStatement)FORSTATEMENT_INIT);
            *ppStatement = (struct Statement*)pIterationStatement;
            Parser_Match(ctx, &pIterationStatement->ClueList0);
            token = Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pIterationStatement->ClueList1);
            //primeira expressao do for
            if (token != TK_SEMICOLON)
            {
                //
                //for (expressionopt; expressionopt; expressionopt) statement
                //for (declaration expressionopt; expressionopt) statement
                bool bHasDeclaration = Declaration(ctx, &pIterationStatement->pInitDeclarationOpt);
                if (bHasDeclaration)
                {
                    token = Parser_CurrentTokenType(ctx);
                    if (token != TK_SEMICOLON)
                    {
                        //Esta eh a 2 expressao do for, a declaracao ja comeu 1
                        Expression(ctx, &pIterationStatement->pExpression2);
                        Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList2);
                    }
                    else
                    {
                        //segunda expressao vazia
                        Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList2);
                    }
                }
                else
                {
                    token = Parser_CurrentTokenType(ctx);
                    if (token != TK_SEMICOLON)
                    {
                        //primeira expressao do for
                        Expression(ctx, &pIterationStatement->pExpression1);
                        Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList2);
                    }
                    token = Parser_CurrentTokenType(ctx);
                    if (token != TK_SEMICOLON)
                    {
                        //segunda expressao do for
                        Expression(ctx, &pIterationStatement->pExpression2);
                        Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList3);
                    }
                    else
                    {
                        //segunda expressao vazia
                        Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList3);
                    }
                }
            }
            else
            {
                //primeira expressao do for vazia
                Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList2);
                token = Parser_CurrentTokenType(ctx);
                if (token != TK_SEMICOLON)
                {
                    //Esta eh a 2 expressao do for, a declaracao ja comeu 1
                    Expression(ctx, &pIterationStatement->pExpression2);
                    Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList3);
                }
                else
                {
                    //segunda expressao do for vazia tb
                    Parser_MatchToken(ctx, TK_SEMICOLON, &pIterationStatement->ClueList3);
                }
            }
            token = Parser_CurrentTokenType(ctx);
            //terceira expressao do for
            if (token != TK_RIGHT_PARENTHESIS)
            {
                Expression(ctx, &pIterationStatement->pExpression3);
            }
            Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pIterationStatement->ClueList4);
            Statement(ctx, &pIterationStatement->pStatement);
        }
        break;
        default:
            //assert(false);
            break;
    }
}


void Labeled_Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    /*
    labeled-statement:
    identifier : statement (ver Labeled_StatementLabel)
    case constant-expression : statement
    default : statement
    */
    struct LabeledStatement* pLabeledStatement = NEW((struct LabeledStatement)LABELEDSTATEMENT_INIT);
    *ppStatement = (struct Statement*)pLabeledStatement;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    pLabeledStatement->token = token;
    if (token == TK_IDENTIFIER)
    {
        //aqui nao eh um tipo
        assert(pLabeledStatement->Identifier == 0);
        pLabeledStatement->Identifier = strdup(Lexeme(ctx));
        Parser_Match(ctx, &pLabeledStatement->ClueList0);
        Parser_MatchToken(ctx, TK_COLON, &pLabeledStatement->ClueList1);
        Statement(ctx, &pLabeledStatement->pStatementOpt);
    }
    else if (token == TK_CASE)
    {
        Parser_Match(ctx, &pLabeledStatement->ClueList0);
        ConstantExpression(ctx, &pLabeledStatement->pExpression);
        Parser_MatchToken(ctx, TK_COLON, &pLabeledStatement->ClueList1);
        Statement(ctx, &pLabeledStatement->pStatementOpt);
    }
    else if (token == TK_DEFAULT)
    {
        Parser_Match(ctx, &pLabeledStatement->ClueList0);
        Parser_MatchToken(ctx, TK_COLON, &pLabeledStatement->ClueList1);
        Statement(ctx, &pLabeledStatement->pStatementOpt);
    }
}

void Asm_Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    /*
    __asm assembly-instruction ;opt
    __asm { assembly-instruction-list };opt
    */
    struct AsmStatement* pAsmStatement = NEW((struct AsmStatement)ASMSTATEMENT_INIT);
    *ppStatement = (struct Statement*)pAsmStatement;
    Parser_MatchToken(ctx, TK__ASM, NULL);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_LEFT_CURLY_BRACKET)
    {
        Parser_Match(ctx, NULL);
        for (; ;)
        {
            if (ErrorOrEof(ctx))
            {
                break;
            }
            token = Parser_CurrentTokenType(ctx);
            if (token == TK_RIGHT_CURLY_BRACKET)
            {
                Parser_Match(ctx, NULL);
                break;
            }
            Parser_Match(ctx, NULL);
        }
    }
    else
    {
        //sem ;
        //    __asm int 0x2c
        //chato
        token = Parser_CurrentTokenType(ctx);
        for (; ;)
        {
            if (ErrorOrEof(ctx))
            {
                break;
            }
            token = Parser_CurrentTokenType(ctx);
            if (token == TK_RIGHT_CURLY_BRACKET)
            {
                //__asm mov al, 2   __asm mov dx, 0xD007   __asm out dx, al
                //chute na verdade..
                //dificil saber aonde termina
                //https://msdn.microsoft.com/en-us/library/45yd4tzz.aspx
                break;
            }
            if (token == TK_SEMICOLON)
            {
                break;
            }
            Parser_Match(ctx, NULL);
        }
    }
    //opcional
    token = Parser_CurrentTokenType(ctx);
    if (token == TK_SEMICOLON)
    {
        Parser_Match(ctx, NULL);
    }
}

bool Statement(struct Parser* ctx, struct Statement** ppStatement)
{
    //assert(*ppStatement == NULL);
    if (Parser_HasError(ctx))
    {
        return false;
    }
    bool bResult = false;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* lexeme = Lexeme(ctx);
    switch (token)
    {
        case TK__ASM:
            bResult = true;
            Asm_Statement(ctx, ppStatement);
            break;
        case TK_LEFT_CURLY_BRACKET:
        {
            bResult = true;
            Compound_Statement(ctx, ppStatement);
        }
        break;
        case TK_CASE:
        case TK_DEFAULT:
            bResult = true;
            Labeled_Statement(ctx, ppStatement);
            break;
        case TK_IF:
        case TK_SWITCH:
            bResult = true;
            Selection_Statement(ctx, ppStatement);
            break;

        case TK_TRY:
            bResult = true;
            Iteration_Statement(ctx, ppStatement);
            break;

            //case TK_ELSE:
            ////assert(false);
            //Ele tem que estar fazendo os statement do IF!
            //bResult = true;
            //Parser_Match(ctx, NULL); //else
            //poderia retornar uma coisa so  p dizer q eh else
            //Statement(ctx, obj);
            //break;
            //iteration-statement

        case TK_WHILE:
        case TK_FOR:
        case TK_DO:
            bResult = true;
            Iteration_Statement(ctx, ppStatement);
            break;
            //jump-statement
        case TK_DEFER:
            bResult = true;
            Defer_Statement(ctx, ppStatement);
            break;
        case TK_GOTO:
        case TK_CONTINUE:
        case TK_BREAK:
        case TK_THROW:
        case TK_RETURN:
            bResult = true;
            Jump_Statement(ctx, ppStatement);
            break;
            //lista de first para express√µes
            //expression-statement
        case TK_LEFT_SQUARE_BRACKET://lamda todo isprimeiryfirst
        case TK_LEFT_PARENTHESIS:
        case TK_SEMICOLON:
        case TK_DECIMAL_INTEGER:
        case TK_FLOAT_NUMBER:
        case TK_STRING_LITERAL:
            //unary
        case TK_PLUSPLUS:
        case TK_MINUSMINUS:
        case TK_SIZEOF:
            //unary-operator
        case TK_AMPERSAND:
        case TK_ASTERISK:
        case TK_PLUS_SIGN:
        case TK_HYPHEN_MINUS:
        case TK_TILDE:
        case TK_EXCLAMATION_MARK:
#ifdef LANGUAGE_EXTENSIONS
            //unary-operator-extension
        case TK_ANDAND: //&&
#endif
            bResult = true;
            Expression_Statement(ctx, ppStatement);
            break;
        case TK_IDENTIFIER:
            if (IsTypeName(ctx, TK_IDENTIFIER, lexeme))
            {
                //√â uma declaracao
            }
            else
            {
                if (Parser_LookAheadToken(ctx) == TK_COLON)
                {
                    //era um label..
                    Labeled_Statement(ctx, ppStatement);
                }
                else
                {
                    Expression_Statement(ctx, ppStatement);
                }
                bResult = true;
            }
            break;
        case TK_INLINE:
        case TK__INLINE: //microscoft
        case TK__NORETURN:
        case TK__ALIGNAS:
            //type-qualifier
        case TK_CONST:
        case TK_RESTRICT:
        case TK_VOLATILE:
        case TK__ATOMIC:
        case TK_TYPEDEF:
        case TK_EXTERN:
        case TK_STATIC:
        case TK__THREAD_LOCAL:
        case TK_AUTO:
        case TK_REGISTER:
        case TK_VOID:
        case TK_CHAR:
        case TK_SHORT:
        case TK_INT:
        case TK_LONG:
            //microsoft
        case TK__INT8:
        case TK__INT16:
        case TK__INT32:
        case TK__INT64:
        case TK___DECLSPEC:
        case TK__WCHAR_T:
            /////////
        case TK_FLOAT:
        case TK_DOUBLE:
        case TK_SIGNED:
        case TK_UNSIGNED:
        case TK__BOOL:
        case TK__COMPLEX:
        case TK_STRUCT:
        case TK_UNION:
        case TK_ENUM:
        case TK__STATIC_ASSERT:
            bResult = false;
            break;
        default:
            SetError(ctx, "unexpected error");
            //bResult = true;
            //SetType(pStatement, "expression-statement");
            //Expression_Statement(ctx, pStatement);
            break;
    }
    return bResult;
}

void Block_Item(struct Parser* ctx, struct BlockItem** ppBlockItem)
{
    /*
    block-item:
    declaration
    statement
    */
    *ppBlockItem = NULL;
    struct Statement* pStatement = NULL;
    if (Statement(ctx, &pStatement))
    {
        *ppBlockItem = (struct BlockItem*)pStatement;
        //assert(*ppBlockItem != NULL);
    }
    else
    {
        struct AnyDeclaration* pDeclaration;
        Declaration(ctx, &pDeclaration);
        *ppBlockItem = (struct BlockItem*)pDeclaration;
        //assert(*ppBlockItem != NULL);
    }
}

void Block_Item_List(struct Parser* ctx, struct BlockItemList* pBlockItemList)
{
    for (; ;)
    {
        if (ErrorOrEof(ctx))
            break;
        struct BlockItem* pBlockItem = NULL;
        Block_Item(ctx, &pBlockItem);
        BlockItemList_PushBack(pBlockItemList, pBlockItem);
        enum TokenType token = Parser_CurrentTokenType(ctx);
        if (token == TK_RIGHT_CURLY_BRACKET)
        {
            //terminou
            break;
        }
        if (ErrorOrEof(ctx))
            break;
    }
}


void Compound_Statement(struct Parser* ctx, struct CompoundStatement** ppStatement)
{
    /*
    compound-statement:
    { block-item-listopt }
    */
    struct CompoundStatement* pCompoundStatement = NEW((struct CompoundStatement)COMPOUNDSTATEMENT_INIT);
    *ppStatement = (struct CompoundStatement*)pCompoundStatement;
    struct SymbolMap BlockScope = SYMBOLMAP_INIT;
    BlockScope.pPrevious = ctx->pCurrentScope;
    ctx->pCurrentScope = &BlockScope;
    Parser_MatchToken(ctx, TK_LEFT_CURLY_BRACKET, &pCompoundStatement->ClueList0);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token != TK_RIGHT_CURLY_BRACKET)
    {
        Block_Item_List(ctx, &pCompoundStatement->BlockItemList);
    }
    Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET, &pCompoundStatement->ClueList1);
    //SymbolMap_Print(ctx->pCurrentScope);
    ctx->pCurrentScope = BlockScope.pPrevious;
    SymbolMap_Destroy(&BlockScope);
}

void Struct_Or_Union(struct Parser* ctx,
                     struct StructUnionSpecifier* pStructUnionSpecifier)
{
    /*
    struct-or-union:
    struct
    union
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK_STRUCT:
            pStructUnionSpecifier->Token = token;
            Parser_Match(ctx, &pStructUnionSpecifier->ClueList0);
            break;
        case TK_UNION:
            pStructUnionSpecifier->Token = token;
            Parser_Match(ctx, &pStructUnionSpecifier->ClueList0);
            break;
        default:
            //assert(false);
            break;
    }
}

void Static_Assert_Declaration(struct Parser* ctx, struct StaticAssertDeclaration* pStaticAssertDeclaration)
{
    /*
    static_assert-declaration:
    _Static_assert ( constant-expression , char-literal ) ;
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK__STATIC_ASSERT)
    {
        Parser_Match(ctx, &pStaticAssertDeclaration->ClueList0);
        Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pStaticAssertDeclaration->ClueList1);
        ConstantExpression(ctx,
                           &pStaticAssertDeclaration->pConstantExpression);
        Parser_MatchToken(ctx, TK_COMMA, &pStaticAssertDeclaration->ClueList2);
        free(pStaticAssertDeclaration->Text);
        pStaticAssertDeclaration->Text = strdup(Lexeme(ctx));
        Parser_MatchToken(ctx, TK_STRING_LITERAL, &pStaticAssertDeclaration->ClueList3);
        Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pStaticAssertDeclaration->ClueList4);
        Parser_MatchToken(ctx, TK_SEMICOLON, &pStaticAssertDeclaration->ClueList5);
    }
}

void Specifier_Qualifier_List(struct Parser* ctx, struct SpecifierQualifierList* pSpecifierQualifierList)
{
    /*specifier-qualifier-list:
    type-specifier specifier-qualifier-listopt
    type-qualifier specifier-qualifier-listopt
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* lexeme = Lexeme(ctx);
    if (TTypeSpecifier_IsFirst(ctx, token, lexeme))
    {
        if (SpecifierQualifierList_CanAdd(pSpecifierQualifierList, token))
        {
            struct TypeSpecifier* pTypeSpecifier = NULL;
            Type_Specifier(ctx, &pTypeSpecifier);
            if (pTypeSpecifier != NULL)
            {
                //ATENCAO
                SpecifierQualifierList_PushBack(pSpecifierQualifierList, (struct SpecifierQualifier*)pTypeSpecifier);
            }
        }
        else
        {
            SetError(ctx, "invalid specifier-qualifier-list");
        }
    }
    else if (TTypeQualifier_IsFirst(token))
    {
        struct TypeQualifier* pTypeQualifier = NEW((struct TypeQualifier)TYPEQUALIFIER_INIT);
        Type_Qualifier(ctx, pTypeQualifier);
        SpecifierQualifierList_PushBack(pSpecifierQualifierList, TypeQualifier_As_SpecifierQualifier(pTypeQualifier));
    }
    else
    {
        SetError(ctx, "internal error 01 %s", TokenToString(token));
    }
    token = Parser_CurrentTokenType(ctx);
    lexeme = Lexeme(ctx);
    if (TTypeQualifier_IsFirst(token))
    {
        Specifier_Qualifier_List(ctx, pSpecifierQualifierList);
    }
    else if (TTypeSpecifier_IsFirst(ctx, token, lexeme))
    {
        if (SpecifierQualifierList_CanAdd(pSpecifierQualifierList, token))
        {
            /*
            typedef int X;
            void F(int X ); //X vai ser variavel e nao tipo
            */
            Specifier_Qualifier_List(ctx, pSpecifierQualifierList);
        }
    }
}


void Struct_Declarator(struct Parser* ctx,
                       struct InitDeclarator** ppTDeclarator2)
{
    /**
    struct-declarator:
    declarator
    declaratoropt : constant-expression
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_COLON)
    {
        //AST TODO
        ////TNodeClueList_MoveToEnd(&ppTDeclarator2->ClueList, &ctx->Scanner.ClueList);
        Parser_Match(ctx, NULL);// &ppTDeclarator2->ClueList);
        struct Expression* p = NULL;
        ConstantExpression(ctx, &p);
        Expression_Delete(p);
    }
    else
    {
        struct InitDeclarator* pInitDeclarator = NEW((struct InitDeclarator)INITDECLARATOR_INIT);
        *ppTDeclarator2 = pInitDeclarator;
        Declarator(ctx, false, &pInitDeclarator->pDeclarator);
        token = Parser_CurrentTokenType(ctx);
        if (token == TK_COLON)
        {
            Parser_Match(ctx, &pInitDeclarator->ClueList0);
            struct Expression* p = NULL;
            ConstantExpression(ctx, &p);
            Expression_Delete(p);
        }
#ifdef LANGUAGE_EXTENSIONS
        if (token == TK_EQUALS_SIGN)
        {
            Parser_Match(ctx, &pInitDeclarator->ClueList1); //_defval ou =
            Initializer(ctx, &pInitDeclarator->pInitializer);
            token = Parser_CurrentTokenType(ctx);
        }
#endif
    }
}

void Struct_Declarator_List(struct Parser* ctx,

                            struct StructDeclaratorList* pStructDeclarationList)
{
    /*
    struct-declarator-list:
    struct-declarator
    struct-declarator-list , struct-declarator
    */
    struct InitDeclarator* pTDeclarator2 = NULL;// TDeclarator_Create();
    Struct_Declarator(ctx, &pTDeclarator2);
    StructDeclaratorList_Add(pStructDeclarationList, pTDeclarator2);
    for (; ;)
    {
        if (ErrorOrEof(ctx))
            break;
        enum TokenType token = Parser_CurrentTokenType(ctx);
        if (token == TK_COMMA)
        {
            //Tem mais
            Parser_Match(ctx, &pTDeclarator2->ClueList0);
            //ANNOTATED AQUI TEM O COMENTARIO /*= 1*/
            Struct_Declarator_List(ctx, pStructDeclarationList);
        }
        else if (token == TK_SEMICOLON)
        {
            //em ctx cluelist
            //ANNOTATED AQUI TEM O COMENTARIO /*= 1*/
            break;
        }
        else
        {
            if (token == TK_RIGHT_CURLY_BRACKET)
            {
                SetError(ctx, "syntax error: missing ';' before '}'");
            }
            else
            {
                SetError(ctx, "syntax error: expected ',' or ';'");
            }
            break;
        }
    }
}

void Struct_Declaration(struct Parser* ctx,
                        struct AnyStructDeclaration** ppStructDeclaration)
{
    /**
    struct-declaration:
    specifier-qualifier-list struct-declarator-listopt ;
    static_assert-declaration
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token != TK__STATIC_ASSERT)
    {
        struct StructDeclaration* pStructDeclarationBase = NEW((struct StructDeclaration)STRUCTDECLARATION_INIT);
        *ppStructDeclaration = (struct AnyStructDeclaration*)pStructDeclarationBase;
        Specifier_Qualifier_List(ctx,
                                 &pStructDeclarationBase->SpecifierQualifierList);
        token = Parser_CurrentTokenType(ctx);
        if (token != TK_SEMICOLON)
        {
            Struct_Declarator_List(ctx,
                                   &pStructDeclarationBase->DeclaratorList);
            Parser_MatchToken(ctx, TK_SEMICOLON, &pStructDeclarationBase->ClueList1);
            //TODO AQUI TEM O COMENTARIO /*= 1*/
        }
        else
        {
            Parser_MatchToken(ctx, TK_SEMICOLON, &pStructDeclarationBase->ClueList1);
        }
    }
    else
    {
        struct StaticAssertDeclaration* pStaticAssertDeclaration = NEW((struct StaticAssertDeclaration)STATICASSERTDECLARATION_INIT);
        *ppStructDeclaration = (struct AnyStructDeclaration*)pStaticAssertDeclaration;
        Static_Assert_Declaration(ctx, pStaticAssertDeclaration);
    }
}

void Struct_Declaration_List(struct Parser* ctx,
                             struct StructDeclarationList* pStructDeclarationList)
{
    /*
    struct-declaration-list:
    struct-declaration
    struct-declaration-list struct-declaration
    */
    if (ErrorOrEof(ctx))
    {
        return;
    }
    struct AnyStructDeclaration* pStructDeclaration = NULL;
    Struct_Declaration(ctx, &pStructDeclaration);
    StructDeclarationList_PushBack(pStructDeclarationList, pStructDeclaration);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token != TK_RIGHT_CURLY_BRACKET)
    {
        //Tem mais?
        Struct_Declaration_List(ctx, pStructDeclarationList);
    }
}

void UnionSetItem(struct Parser* ctx, struct UnionSet* p)
{
    /*
    _union-set-item:
    struct Identifier
    union Identifier
    Identifier
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* lexeme = Lexeme(ctx);
    struct UnionSetItem* pItem = NEW((struct UnionSetItem)TUNIONSETITEM_INIT);
    if (token == TK_IDENTIFIER)
    {
        pItem->Name = strdup(lexeme);
        Parser_Match(ctx, &pItem->ClueList1);
        TUnionSet_PushBack(p, pItem);
    }
    else if (token == TK_STRUCT ||
             token == TK_UNION)
    {
        Parser_Match(ctx, &pItem->ClueList0);
        pItem->Name = strdup(lexeme);
        Parser_MatchToken(ctx, TK_IDENTIFIER, &pItem->ClueList1);
        TUnionSet_PushBack(p, pItem);
    }
    else
    {
        SetError(ctx, "invalid token for union set");
    }
}

void UnionSetList(struct Parser* ctx, struct UnionSet* p)
{
    /*
    _union-set-list:
    _union-set-item
    _union-set-item | _union-set-list
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    UnionSetItem(ctx, p);
    token = Parser_CurrentTokenType(ctx);
    if (token == TK_VERTICAL_LINE)
    {
        p->pTail->TokenFollow = token;
        Parser_Match(ctx, &p->pTail->ClueList2);
        UnionSetList(ctx, p);
    }
}

void UnionSet(struct Parser* ctx, struct UnionSet* pUnionSet)
{
    /*
    _union-set:
    < _union-set-list >
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    //const char* lexeme = Lexeme(ctx);
    if (token == TK_LESS_THAN_SIGN)
    {
        Parser_Match(ctx, &pUnionSet->ClueList0);
        UnionSetList(ctx, pUnionSet);
        Parser_MatchToken(ctx, TK_GREATER_THAN_SIGN,
                          &pUnionSet->ClueList1);
    }
}

void Struct_Or_Union_Specifier(struct Parser* ctx,
                               struct StructUnionSpecifier* pStructUnionSpecifier)
{
    /*
    struct-or-union-specifier:
    struct-or-union identifieropt { struct-declaration-list }
    struct-or-union identifier
    */
    /*
    struct-or-union-specifier:
    struct-or-union )opt identifieropt { struct-declaration-list }
    struct-or-union )opt identifier
    */
    //aqui teria que ativar o flag
    Struct_Or_Union(ctx, pStructUnionSpecifier);//TODO
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* lexeme = Lexeme(ctx);
    if (token == TK_LESS_THAN_SIGN)
    {
        UnionSet(ctx, &pStructUnionSpecifier->UnionSet);
        token = Parser_CurrentTokenType(ctx);
    }
    token = Parser_CurrentTokenType(ctx);
    lexeme = Lexeme(ctx);
    if (token == TK_IDENTIFIER)
    {
        //ANNOTATED AQUI TEM O COMENTARIO /*Box | Circle*/ antes nome da struct
        assert(pStructUnionSpecifier->Tag == 0);
        pStructUnionSpecifier->Tag = strdup(lexeme);
        Parser_Match(ctx, &pStructUnionSpecifier->ClueList1);
    }
    if (pStructUnionSpecifier->Tag != NULL)
    {
        SymbolMap_SetAt(ctx->pCurrentScope, pStructUnionSpecifier->Tag, (struct TypePointer*)pStructUnionSpecifier);
    }
    token = Parser_CurrentTokenType(ctx);
    if (token == TK_LEFT_CURLY_BRACKET)
    {
        //ANNOTATED AQUI TEM O COMENTARIO /*Box | Circle*/ antes do {
        Parser_Match(ctx, &pStructUnionSpecifier->ClueList2);
        Struct_Declaration_List(ctx,
                                &pStructUnionSpecifier->StructDeclarationList);
        Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET,
                          &pStructUnionSpecifier->ClueList3);
    }
    else
    {
        //struct X *
        // SetError2(ctx, "expected name or {", "");
    }
}

void Enumeration_Constant(struct Parser* ctx,
                          struct Enumerator* pEnumerator2)
{
    const char* lexeme = Lexeme(ctx);
    assert(pEnumerator2->Name == 0);
    pEnumerator2->Name = strdup(lexeme);
    Parser_MatchToken(ctx, TK_IDENTIFIER, &pEnumerator2->ClueList0);
}

bool EnumeratorC(struct Parser* ctx, struct Enumerator* pEnumerator2)
{
    /*
    enumerator:
    enumeration-constant
    enumeration-constant = constant-expression
    */
    bool bValueAssigned = false;
    Enumeration_Constant(ctx, pEnumerator2);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_EQUALS_SIGN)
    {
        Parser_Match(ctx, &pEnumerator2->ClueList1);
        ConstantExpression(ctx, &pEnumerator2->pConstantExpression);
        bValueAssigned = true;
    }
    return bValueAssigned;
}

void Enumerator_List(struct Parser* ctx,
                     struct EnumeratorList* pEnumeratorList2)
{
    if (ErrorOrEof(ctx))
    {
        return;
    }
    /*
    enumerator-list:
    enumerator
    enumerator-list , enumerator
    */
    struct Enumerator* pEnumerator2 = NEW((struct Enumerator)ENUMERATOR_INIT);
    List_Add(pEnumeratorList2, pEnumerator2);
    EnumeratorC(ctx, pEnumerator2);
    SymbolMap_SetAt(ctx->pCurrentScope, pEnumerator2->Name, (struct TypePointer*)pEnumerator2);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    //tem mais?
    if (token == TK_COMMA)
    {
        Parser_Match(ctx, &pEnumerator2->ClueList2);
        token = Parser_CurrentTokenType(ctx);
        pEnumerator2->bHasComma = true;
        if (token != TK_RIGHT_CURLY_BRACKET)
        {
            Enumerator_List(ctx, pEnumeratorList2);
        }
    }
}

void Enum_Specifier(struct Parser* ctx, struct EnumSpecifier* pEnumSpecifier2)
{
    /*
    enum-specifier:
      enum identifier_opt { enumerator-list }
      enum identifier_opt { enumerator-list, }
      enum identifier
    */
    Parser_MatchToken(ctx, TK_ENUM, &pEnumSpecifier2->ClueList0);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_IDENTIFIER) /*identificador optional*/
    {
        const char* lexeme = Lexeme(ctx);
        assert(pEnumSpecifier2->Tag == 0);
        pEnumSpecifier2->Tag = strdup(lexeme);
        token = Parser_Match(ctx, &pEnumSpecifier2->ClueList1);
        SymbolMap_SetAt(ctx->pCurrentScope, pEnumSpecifier2->Tag, (struct TypePointer*)pEnumSpecifier2);
        if (token == TK_LEFT_CURLY_BRACKET)
        {
            Parser_Match(ctx, &pEnumSpecifier2->ClueList1);
            Enumerator_List(ctx, &pEnumSpecifier2->EnumeratorList);
            Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET, &pEnumSpecifier2->ClueList3);
        }
    }
    else if (token == TK_LEFT_CURLY_BRACKET)
    {
        token = Parser_Match(ctx, &pEnumSpecifier2->ClueList1);
        Enumerator_List(ctx, &pEnumSpecifier2->EnumeratorList);
        Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET, &pEnumSpecifier2->ClueList3);
    }
    else
    {
        SetError(ctx, "expected enum name or {");
    }
}

bool TFunctionSpecifier_IsFirst(enum TokenType token)
{
    /*
    function-specifier:
    inline
    _Noreturn
    */
    bool bResult = false;
    switch (token)
    {
        case TK_INLINE:
        case TK__INLINE://microsoft
        case TK__FORCEINLINE://microsoft
        case TK__NORETURN:
            bResult = true;
            break;
        default:
            break;
    }
    return bResult;
}

void Function_Specifier(struct Parser* ctx,
                        struct FunctionSpecifier* pFunctionSpecifier)
{
    /*
    function-specifier:
    inline
    _Noreturn
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        case TK__INLINE://microsoft
        case TK__FORCEINLINE://microsoft
        case TK_INLINE:
        case TK__NORETURN:
            pFunctionSpecifier->Token = token;
            Parser_Match(ctx, &pFunctionSpecifier->ClueList0);
            break;
        default:
            break;
    }
}

bool TStorageSpecifier_IsFirst(enum TokenType token)
{
    bool bResult = false;
    /*
    storage-class-specifier:
    typedef
    extern
    static
    _Thread_local
    auto
    register
    */
    switch (token)
    {
        case TK___DECLSPEC: //microsoft
        case TK_TYPEDEF:
        case TK_EXTERN:
        case TK_STATIC:
        case TK__THREAD_LOCAL:
        case TK_AUTO:
        case TK_REGISTER:
            bResult = true;
            break;
            //TODO
            //‚ÄÉ__declspec
            //https://docs.microsoft.com/pt-br/cpp/c-language/summary-of-declarations?view=msvc-160
        default:
            break;
    }
    return bResult;
}

void Storage_Class_Specifier(struct Parser* ctx,

                             struct StorageSpecifier* pStorageSpecifier)
{
    /*
    storage-class-specifier:
    typedef
    extern
    static
    _Thread_local
    auto
    register
    */
    /*
     VISUAL C++
    __declspec****( extended-decl-modifier-seq )
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        //https://docs.microsoft.com/en-us/cpp/cpp/declspec?view=msvc-160
        //Microsoft extension
        case TK___DECLSPEC:
            pStorageSpecifier->Token = token;
            Parser_Match(ctx, NULL);
            int count = 0;
            for (;;)
            {
                token = Parser_CurrentTokenType(ctx);
                switch (token)
                {
                    case TK_RIGHT_PARENTHESIS:
                        count--;
                        break;
                    case TK_LEFT_PARENTHESIS:
                        count++;
                        break;
                }
                Parser_Match(ctx, NULL);
                if (count == 0 || Parser_HasError(ctx))
                    break; //last parentesis closed
            }
            break;
        case TK_TYPEDEF:
        case TK_EXTERN:
        case TK_STATIC:
        case TK__THREAD_LOCAL:
        case TK_AUTO:
        case TK_REGISTER:
            pStorageSpecifier->Token = token;
            Parser_Match(ctx, &pStorageSpecifier->ClueList0);
            break;
        default:
            break;
    }
}

void Parameter_List(struct Parser* ctx,
                    struct ParameterList* pParameterList)
{
    /*
    parameter-list:
      parameter-declaration
      parameter-list, parameter-declaration
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    struct Parameter* pParameter = NEW((struct Parameter)PARAMETER_INIT);
    List_Add(pParameterList, pParameter);
    Parameter_Declaration(ctx, pParameter);
    //Tem mais?
    token = Parser_CurrentTokenType(ctx);
    if (token == TK_COMMA)
    {
        //a virgula fica no anterior
        pParameter->bHasComma = true;
        Parser_Match(ctx, &pParameter->ClueList0);
        token = Parser_CurrentTokenType(ctx);
        if (token != TK_DOTDOTDOT)
        {
            Parameter_List(ctx, pParameterList);
        }
    }
}

void Parameter_Declaration(struct Parser* ctx,
                           struct Parameter* pParameterDeclaration)
{
    /*
    parameter-declaration:
    declaration-specifiers declarator
    declaration-specifiers abstract-declaratoropt
    */
    Declaration_Specifiers(ctx, &pParameterDeclaration->Specifiers);
    struct Declarator* pDeclarator = NULL;
    Declarator(ctx, true, &pDeclarator);
    if (pDeclarator)
    {
        Declarator_Swap(&pParameterDeclaration->Declarator, pDeclarator);
        Declarator_Delete(pDeclarator);
    }
}

void Parameter_Type_List(struct Parser* ctx,
                         struct ParameterTypeList* pParameterList)
{
    /*
    parameter-type-list:
      parameter-list
      parameter-list , ...
    */
    Parameter_List(ctx, &pParameterList->ParameterList);
    if (pParameterList->ParameterList.pHead &&
        pParameterList->ParameterList.pHead->pNext == NULL)
    {
        if (pParameterList->ParameterList.pHead->Declarator.PointerList.pHead == 0)
        {
            if (pParameterList->ParameterList.pHead->Specifiers.Size == 1)
            {
                if (pParameterList->ParameterList.pHead->Specifiers.pData[0]->Type == SingleTypeSpecifier_ID)
                {
                    struct SingleTypeSpecifier* pSingleTypeSpecifier =
                        (struct SingleTypeSpecifier*)pParameterList->ParameterList.pHead->Specifiers.pData[0];
                    pParameterList->bIsVoid = pSingleTypeSpecifier->Token2 == TK_VOID;
                }
            }
        }
    }
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_DOTDOTDOT)
    {
        pParameterList->bVariadicArgs = true;
        Parser_Match(ctx, &pParameterList->ClueList1);
    }
}


void Direct_Declarator(struct Parser* ctx, bool bAbstract, struct DirectDeclarator** ppDeclarator2)
{
    *ppDeclarator2 = NULL; //out
    /////////////////////////////////////////////////////////////////////////
    //ignoring fastcall etc...
    //This is not the correct position...
    //https://docs.microsoft.com/pt-br/cpp/c-language/summary-of-declarations?view=msvc-160
    enum TokenType token = Parser_CurrentTokenType(ctx);
    //////////////////////////////////////////////////////////////////////////////

    /*
    direct-declarator:
      identifier
      ( declarator )
      direct-declarator [ type-qualifier-listopt assignment-expressionopt ]
      direct-declarator [ static type-qualifier-listopt assignment-expression ]
      direct-declarator [ type-qualifier-list static assignment-expression ]
      direct-declarator [ type-qualifier-listopt * ]
      direct-declarator ( parameter-type-list )
      direct-declarator ( identifier-listopt )
    */

    /*
      direct-abstract-declarator:
       ( abstract-declarator )
       direct-abstract-declarator_opt [ type-qualifier-listopt    assignment-expressionopt ]
       direct-abstract-declarator_opt [ static type-qualifier-listopt assignment-expression ]
       direct-abstract-declarator_opt [ type-qualifier-list static assignment-expression ]
       direct-abstract-declarator_opt [ * ]
       direct-abstract-declarator_opt ( parameter-type-listopt )
    */

    struct DirectDeclarator* pDirectDeclarator = NULL;
    if (ErrorOrEof(ctx))
        return;
    switch (token)
    {
        case TK_LEFT_PARENTHESIS:
        {
            pDirectDeclarator = NEW((struct DirectDeclarator)TDIRECTDECLARATOR_INIT);
            token = Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pDirectDeclarator->ClueList0);

            if (token == TK_ASTERISK || token == TK_IDENTIFIER)
            {
                pDirectDeclarator->DeclaratorType = TDirectDeclaratorTypeDeclarator;
                Declarator(ctx, bAbstract, &pDirectDeclarator->pDeclarator);
            }
            else
            {
                if (bAbstract)
                {
                    pDirectDeclarator->DeclaratorType = TDirectDeclaratorTypeFunction;
                    if (token != TK_RIGHT_PARENTHESIS)
                        Parameter_Type_List(ctx, &pDirectDeclarator->Parameters.ParameterList);
                }
                else
                {
                    SetError(ctx, "Missing declarator name");
                }
            }


            Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pDirectDeclarator->ClueList1);
        }
        break;
        case TK_IDENTIFIER:
        {
            //identifier
            pDirectDeclarator = NEW((struct DirectDeclarator)TDIRECTDECLARATOR_INIT);
            //Para indicar que eh uma identificador
            pDirectDeclarator->DeclaratorType = TDirectDeclaratorTypeIdentifier;
            const char* lexeme = Lexeme(ctx);
            pDirectDeclarator->Identifier = strdup(lexeme);
            pDirectDeclarator->Position.Line = GetCurrentLine(ctx);
            pDirectDeclarator->Position.FileIndex = GetFileIndex(ctx);
            Parser_Match(ctx, &pDirectDeclarator->ClueList0);
        }
        break;
        default:
            ////assert(false);
            break;
    }
    if (pDirectDeclarator == NULL)
    {
        //Por enquanto esta funcao esta sendo usada para
        //abstract declarator que nao tem nome.
        //vou criar aqui por enquanto um cara vazio
        pDirectDeclarator = NEW((struct DirectDeclarator)TDIRECTDECLARATOR_INIT);
        pDirectDeclarator->Identifier = strdup("");
        pDirectDeclarator->Position.Line = GetCurrentLine(ctx);
        pDirectDeclarator->Position.FileIndex = GetFileIndex(ctx);
        //Para indicar que eh uma identificador
        pDirectDeclarator->DeclaratorType = TDirectDeclaratorTypeIdentifier;
        //Quando tiver abstract declarator vai ser
        //bug cair aqui
    }
    *ppDeclarator2 = pDirectDeclarator;
    for (;;)
    {
        //assert(pDirectDeclarator != NULL);
        token = Parser_CurrentTokenType(ctx);
        switch (token)
        {
            case TK_LEFT_PARENTHESIS:
                /*
                direct-declarator ( parameter-type-list )
                direct-declarator ( identifier-listopt )
                */
                //      pDirectDeclarator->token = token;
                //      //assert(pDirectDeclarator->pParametersOpt == NULL);
                //      pDirectDeclarator->pParametersOpt = TParameterList_Create();
                token = Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pDirectDeclarator->ClueList2);
                //Para indicar que eh uma funcao
                pDirectDeclarator->DeclaratorType = TDirectDeclaratorTypeFunction;
                if (token != TK_RIGHT_PARENTHESIS)
                {
                    //opt
                    Parameter_Type_List(ctx, &pDirectDeclarator->Parameters);
                }
                token = Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pDirectDeclarator->ClueList3);
                if (token == TK_IDENTIFIER && strcmp(Lexeme(ctx), "overload") == 0)
                {
                    /*
                      int f() overload;
                      int f() overload, a;
                    */
                    struct StrBuilder sb = STRBUILDER_INIT;
                    /*ainda nao tem o retorno specifiers no  name mangling*/
                    /*esta declaracao fica no codeprint*/
                    StrBuilder_Append(&sb, pDirectDeclarator->Identifier);
                    ParameterList_PrintNameMangling(&pDirectDeclarator->Parameters.ParameterList, &sb);
                    Parser_MatchToken(ctx, TK_IDENTIFIER, &pDirectDeclarator->ClueList4);
                    pDirectDeclarator->bOverload = true;
                    pDirectDeclarator->NameMangling = sb.c_str; /*moved*/
                }
                break;
            case TK_LEFT_SQUARE_BRACKET:
                /*
                direct-declarator [ type-qualifier-listopt assignment-expressionopt ]
                direct-declarator [ static type-qualifier-listopt assignment-expression ]
                direct-declarator [ type-qualifier-list static assignment-expression ]
                direct-declarator [ type-qualifier-listopt * ]
                */
                ////assert(pDirectDeclarator->pParametersOpt == NULL);
                //pDirectDeclarator->pParametersOpt = TParameterList_Create();
                //Para indicar que eh um array
                pDirectDeclarator->DeclaratorType = TDirectDeclaratorTypeArray;
                token = Parser_MatchToken(ctx, TK_LEFT_SQUARE_BRACKET, &pDirectDeclarator->ClueList2);
                if (token == TK_STATIC)
                {
                }
                else if (token == TK_AUTO)
                {
                    //int a[auto];
                    pDirectDeclarator->DeclaratorType = TDirectDeclaratorTypeAutoArray;
                    Parser_MatchToken(ctx, TK_AUTO, &pDirectDeclarator->ClueList3);
                }
                else
                {
                    if (token != TK_RIGHT_SQUARE_BRACKET)
                    {
                        //assert(pDirectDeclarator->pExpression == NULL);
                        AssignmentExpression(ctx, &pDirectDeclarator->pExpression);
                    }
                    else
                    {
                        //array vazio √© permitido se for o ultimo cara da struct
                        //struct X { int ElementCount;  int Elements[]; };
                    }
                }
                Parser_MatchToken(ctx, TK_RIGHT_SQUARE_BRACKET, &pDirectDeclarator->ClueList3);
                break;
            default:
                //assert(false);
                break;
        }
        token = Parser_CurrentTokenType(ctx);
        if (token != TK_LEFT_PARENTHESIS && token != TK_LEFT_SQUARE_BRACKET)
        {
            break;
        }
        else
        {
            struct DirectDeclarator* pDirectDeclaratorNext = NEW((struct DirectDeclarator)TDIRECTDECLARATOR_INIT);
            pDirectDeclarator->pDirectDeclarator = pDirectDeclaratorNext;
            pDirectDeclarator = pDirectDeclaratorNext;
        }
    }
    token = Parser_CurrentTokenType(ctx);
    if (token == TK_LEFT_PARENTHESIS)
    {
        //tem mais
        struct DirectDeclarator* pDirectDeclaratorNext = NULL;
        Direct_Declarator(ctx, bAbstract, &pDirectDeclaratorNext);
        pDirectDeclarator->pDirectDeclarator = pDirectDeclaratorNext;
    }
    else if (!bAbstract && token == TK_IDENTIFIER)
    {
        //tem mais
        struct DirectDeclarator* pDirectDeclaratorNext = NULL;
        Direct_Declarator(ctx, bAbstract, &pDirectDeclaratorNext);
        pDirectDeclarator->pDirectDeclarator = pDirectDeclaratorNext;
    }
}

static bool TTypeQualifier_IsFirst(enum TokenType token)
{
    bool bResult = false;
    switch (token)
    {
        case TK_CONST:
        case TK_RESTRICT:
        case TK_VOLATILE:
        case TK__ATOMIC:
            bResult = true;
            break;
#ifdef LANGUAGE_EXTENSIONS
            //type-qualifier-extensions
        case TK_LEFT_SQUARE_BRACKET:
        case TK_AUTO:
            bResult = true;
            break;
#endif
        default:
            break;
    }
    return bResult;
}

bool Type_Qualifier(struct Parser* ctx, struct TypeQualifier* pQualifier)
{
    /*
    type-qualifier:
    const
    restrict
    volatile
    _Atomic
    */
    //extensions
    /*
    auto
    _size(identifier)
    _size(int)
    */
    bool bResult = false;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    //const char* lexeme = Lexeme(ctx);
    switch (token)
    {
        case TK_CONST:
        case TK_RESTRICT:
        case TK_VOLATILE:
        case TK__ATOMIC:
            pQualifier->Token = token;
            Parser_Match(ctx, &pQualifier->ClueList0);
            bResult = true;
            break;
#ifdef LANGUAGE_EXTENSIONS
        case TK_AUTO:
            pQualifier->Token = token;
            Parser_Match(ctx, &pQualifier->ClueList0);
            bResult = true;
            break;
#endif
        default:
            break;
    }
    return bResult;
}

void Type_Qualifier_List(struct Parser* ctx,
                         struct TypeQualifierList* pQualifiers)
{
    /*
    type-qualifier-list:
      type-qualifier
      type-qualifier-list type-qualifier
    */
    struct TypeQualifier* pTypeQualifier = NEW((struct TypeQualifier)TYPEQUALIFIER_INIT);
    Type_Qualifier(ctx, pTypeQualifier);
    TypeQualifierList_PushBack(pQualifiers, pTypeQualifier);
    if (IsTypeQualifierToken(Parser_CurrentTokenType(ctx)))
    {
        Type_Qualifier_List(ctx, pQualifiers);
    }
}


void Pointer(struct Parser* ctx, struct PointerList* pPointerList)
{
    /*
    pointer:
    * type-qualifier-listopt
    * type-qualifier-listopt pointer
    */
    struct Pointer* pPointer = NEW((struct Pointer)POINTER_INIT);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_ASTERISK)
    {
        //ANNOTATED AQUI VAI TER AUTO SIZEOF
        PointerList_PushBack(pPointerList, pPointer);
        Parser_Match(ctx, &pPointer->ClueList0);
    }
    else
    {
        //Erro
        SetError(ctx, "pointer error");
    }
    token = Parser_CurrentTokenType(ctx);
    //Opcional
    if (IsTypeQualifierToken(token))
    {
        Type_Qualifier_List(ctx, &pPointer->Qualifier);
    }
    token = Parser_CurrentTokenType(ctx);
    //Tem mais?
    if (token == TK_ASTERISK)
    {
        Pointer(ctx, pPointerList);
    }
}

static bool IsVCAtributte(const char* lexeme)
{
    //https://docs.microsoft.com/pt-br/cpp/c-language/summary-of-declarations?view=msvc-160
    if (strcmp(lexeme, "__asm") == 0 ||
        strcmp(lexeme, "__based") == 0 ||
        strcmp(lexeme, "__cdecl") == 0 ||
        strcmp(lexeme, "__clrcall") == 0 ||
        strcmp(lexeme, "__fastcall") == 0 ||
        strcmp(lexeme, "__inline") == 0 ||
        strcmp(lexeme, "__stdcall") == 0 ||
        strcmp(lexeme, "__thiscall") == 0 ||
        strcmp(lexeme, "__vectorcall") == 0)
    {
        return true;
    }
    return false;
}

static void AtributtesOptions(struct Parser* ctx)
{
    /////////////////////////////////////////////////////////////////////////
    //ignoring fastcall etc...
    //This is not the correct position...
    //https://docs.microsoft.com/pt-br/cpp/c-language/summary-of-declarations?view=msvc-160
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* lexeme = Lexeme(ctx);
    while (token == TK_IDENTIFIER && IsVCAtributte(lexeme))
    {
        token = Parser_Match(ctx, NULL);
        lexeme = Lexeme(ctx);
    }
    //////////////////////////////////////////////////////////////////////////////
}

void Declarator(struct Parser* ctx, bool bAbstract, struct Declarator** ppTDeclarator2)
{
    /*
    declarator:
      pointer_opt direct-declarator
    */
    *ppTDeclarator2 = NULL; //out
    AtributtesOptions(ctx);
    struct Declarator* pDeclarator = NEW((struct Declarator)TDECLARATOR_INIT);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_ASTERISK)
    {
        Pointer(ctx, &pDeclarator->PointerList);
    }
    AtributtesOptions(ctx);
    Direct_Declarator(ctx, bAbstract, &pDeclarator->pDirectDeclarator);
    *ppTDeclarator2 = pDeclarator;
}


bool TAlignmentSpecifier_IsFirst(enum TokenType token)
{
    /*
    alignment - specifier:
    _Alignas(type - name)
    _Alignas(constant - expression)
    */
    return (token == TK__ALIGNAS);
}

bool Alignment_Specifier(struct Parser* ctx,
                         struct AlignmentSpecifier* pAlignmentSpecifier)
{
    bool bResult = false;
    /*
    alignment - specifier:
    _Alignas(type - name)
    _Alignas(constant - expression)
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK__ALIGNAS)
    {
        Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, NULL);
        //assert(false);//TODO
        Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, NULL);
        bResult = true;
    }
    return bResult;
}


bool TTypeSpecifier_IsFirst(struct Parser* ctx, enum TokenType token, const char* lexeme)
{
    /*
    type-specifier:
    void
    char
    short
    int
    long
    float
    double
    signed
    unsigned
    _Bool
    _Complex
    atomic-type-specifier
    struct-or-union-specifier
    enum-specifier
    typedef-name
    */
    bool bResult = false;
    switch (token)
    {
        case TK_VOID:
        case TK_CHAR:
        case TK_SHORT:
        case TK_INT:
        case TK_LONG:
            //microsoft
        case TK__INT8:
        case TK__INT16:
        case TK__INT32:
        case TK__INT64:
        case TK__WCHAR_T:
            /////
        case TK_FLOAT:
        case TK_DOUBLE:
        case TK_SIGNED:
        case TK_UNSIGNED:
        case TK__BOOL:
        case TK__COMPLEX:
        case TK__ATOMIC:
        case TK_STRUCT:
        case TK_UNION:
        case TK_ENUM:
            bResult = true;
            break;
        case TK_IDENTIFIER:
            bResult = IsTypeName(ctx, TK_IDENTIFIER, lexeme);
            break;
        default:
            break;
    }
    return bResult;
}


void AtomicTypeSpecifier(struct Parser* ctx,
                         struct TypeSpecifier** ppTypeSpecifier)
{
    //assert(false); //tODO criar struct TAtomicTypeSpecifier
    /*
    atomic-type-specifier:
    _Atomic ( type-name )
    */
    struct AtomicTypeSpecifier* pAtomicTypeSpecifier = NEW((struct AtomicTypeSpecifier)ATOMICTYPESPECIFIER_INIT);
    *ppTypeSpecifier = AtomicTypeSpecifier_As_TypeSpecifier(pAtomicTypeSpecifier);
    Parser_MatchToken(ctx, TK__ATOMIC, &pAtomicTypeSpecifier->ClueList0);
    Parser_MatchToken(ctx, TK_LEFT_PARENTHESIS, &pAtomicTypeSpecifier->ClueList1);
    TypeName(ctx, &pAtomicTypeSpecifier->TypeName);
    Parser_MatchToken(ctx, TK_RIGHT_PARENTHESIS, &pAtomicTypeSpecifier->ClueList2);
}

void Type_Specifier(struct Parser* ctx, struct TypeSpecifier** ppTypeSpecifier)
{
    /*
    type-specifier:
    void
    char
    short
    int
    long
    float
    double
    signed
    unsigned
    _Bool
    _Complex
    atomic-type-specifier
    struct-or-union-specifier
    enum-specifier
    typedef-name
    */
    //bool bResult = false;
    const char* lexeme = Lexeme(ctx);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    switch (token)
    {
        //type - specifier
        case TK_VOID:
        case TK_CHAR:
        case TK_SHORT:
        case TK_INT:
        case TK_LONG:
            //microsoft
        case TK__INT8:
        case TK__INT16:
        case TK__INT32:
        case TK__INT64:
            //case TK___DECLSPEC:
        case TK__WCHAR_T:
            /////////
        case TK_FLOAT:
        case TK_DOUBLE:
        case TK_SIGNED:
        case TK_UNSIGNED:
        case TK__BOOL:
        case TK__COMPLEX:
        {
            struct SingleTypeSpecifier* pSingleTypeSpecifier = NEW((struct SingleTypeSpecifier)SINGLETYPESPECIFIER_INIT);
            pSingleTypeSpecifier->Token2 = token;
            //bResult = true;
            Parser_Match(ctx, &pSingleTypeSpecifier->ClueList0);
            *ppTypeSpecifier = (struct TypeSpecifier*)pSingleTypeSpecifier;
        }
        break;
        //atomic-type-specifier
        case TK__ATOMIC:
            //bResult = true;
            AtomicTypeSpecifier(ctx, ppTypeSpecifier);
            break;
        case TK_STRUCT:
        case TK_UNION:
        {
            //assert(*ppTypeSpecifier == NULL);
            //bResult = true;
            struct StructUnionSpecifier* pStructUnionSpecifier = NEW((struct StructUnionSpecifier)STRUCTUNIONSPECIFIER_INIT);
            *ppTypeSpecifier = (struct TypeSpecifier*)pStructUnionSpecifier;
            Struct_Or_Union_Specifier(ctx, pStructUnionSpecifier);
        }
        break;
        case TK_ENUM:
        {
            struct EnumSpecifier* pEnumSpecifier2 = NEW((struct EnumSpecifier)ENUMSPECIFIER_INIT);
            *ppTypeSpecifier = (struct TypeSpecifier*)pEnumSpecifier2;
            Enum_Specifier(ctx, pEnumSpecifier2);
        }
        break;
        case TK_IDENTIFIER:
        {
            int bIsTypedef = IsTypeName(ctx, TK_IDENTIFIER, lexeme);
            if (bIsTypedef)
            {
                struct SingleTypeSpecifier* pSingleTypeSpecifier = NEW((struct SingleTypeSpecifier)SINGLETYPESPECIFIER_INIT);
                if (bIsTypedef == 2 /*struct*/)
                    pSingleTypeSpecifier->Token2 = TK_STRUCT;
                else if (bIsTypedef == 3 /*union*/)
                    pSingleTypeSpecifier->Token2 = TK_UNION;
                else if (bIsTypedef == 4 /*enum*/)
                    pSingleTypeSpecifier->Token2 = TK_ENUM;
                else
                    pSingleTypeSpecifier->Token2 = token;
                pSingleTypeSpecifier->TypedefName = strdup(lexeme);
                //bResult = true;
                Parser_Match(ctx, &pSingleTypeSpecifier->ClueList0);
                *ppTypeSpecifier = (struct TypeSpecifier*)pSingleTypeSpecifier;
            }
            else
            {
                //assert(false); //temque chegar aqui limpo ja
                SetError(ctx, "internal error 2");
            }
        }
        break;
        default:
            break;
    }
    //token = Parser_CurrentToken(ctx);
    //if (token == TK_VERTICAL_LINE)
    //{
    //criar uma lista
    //}
}

bool Declaration_Specifiers_IsFirst(struct Parser* ctx, enum TokenType token, const char* lexeme)
{
    /*
    declaration-specifiers:
    storage-class-specifier declaration-specifiersopt
    type-specifier          declaration-specifiersopt
    type-qualifier          declaration-specifiersopt
    function-specifier      declaration-specifiersopt
    alignment-specifier     declaration-specifiersopt
    */
    bool bResult =
        TStorageSpecifier_IsFirst(token) ||
        TTypeSpecifier_IsFirst(ctx, token, lexeme) ||
        TTypeQualifier_IsFirst(token) ||
        TFunctionSpecifier_IsFirst(token) ||
        TFunctionSpecifier_IsFirst(token);
    return bResult;
}

void Declaration_Specifiers(struct Parser* ctx,
                            struct DeclarationSpecifiers* pDeclarationSpecifiers)
{
    /*
    declaration-specifiers:
    storage-class-specifier declaration-specifiersopt
    type-specifier          declaration-specifiersopt
    type-qualifier          declaration-specifiersopt
    function-specifier      declaration-specifiersopt
    alignment-specifier     declaration-specifiersopt
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* lexeme = Lexeme(ctx);
    if (TStorageSpecifier_IsFirst(token))
    {
        struct StorageSpecifier* pStorageSpecifier = NEW((struct StorageSpecifier)STORAGESPECIFIER_INIT);
        Storage_Class_Specifier(ctx, pStorageSpecifier);
        DeclarationSpecifiers_PushBack(pDeclarationSpecifiers, StorageSpecifier_As_DeclarationSpecifier(pStorageSpecifier));
    }
    else if (TTypeSpecifier_IsFirst(ctx, token, lexeme))
    {
        if (DeclarationSpecifiers_CanAddSpeficier(pDeclarationSpecifiers,
            token,
            lexeme))
        {
            struct TypeSpecifier* pTypeSpecifier = NULL;
            Type_Specifier(ctx, &pTypeSpecifier);
            //ATENCAO
            DeclarationSpecifiers_PushBack(pDeclarationSpecifiers, (struct DeclarationSpecifier*)pTypeSpecifier);
        }
        else
        {
            SetError(ctx, "double typedef");
        }
    }
    else if (TTypeQualifier_IsFirst(token))
    {
        struct TypeQualifier* pTypeQualifier = NEW((struct TypeQualifier)TYPEQUALIFIER_INIT);
        Type_Qualifier(ctx, pTypeQualifier);
        //ATENCAO
        DeclarationSpecifiers_PushBack(pDeclarationSpecifiers, (struct DeclarationSpecifier*)TypeQualifier_As_SpecifierQualifier(pTypeQualifier));
    }
    else if (TFunctionSpecifier_IsFirst(token))
    {
        struct FunctionSpecifier* pFunctionSpecifier = NEW((struct FunctionSpecifier)FUNCTIONSPECIFIER_INIT);
        Function_Specifier(ctx, pFunctionSpecifier);
        DeclarationSpecifiers_PushBack(pDeclarationSpecifiers, FunctionSpecifier_As_DeclarationSpecifier(pFunctionSpecifier));
    }
    else if (TAlignmentSpecifier_IsFirst(token))
    {
        //assert(false);
        //struct TAlignmentSpecifier* pAlignmentSpecifier = TAlignmentSpecifier_Create();
        //List_Add(pDeclarationSpecifiers, TAlignmentSpecifier_As_TDeclarationSpecifier(pAlignmentSpecifier));
    }
    else
    {
        SetError(ctx, "internal error 3");
    }
    token = Parser_CurrentTokenType(ctx);
    lexeme = Lexeme(ctx);
    //Tem mais?
    if (Declaration_Specifiers_IsFirst(ctx, token, lexeme))
    {
        if (DeclarationSpecifiers_CanAddSpeficier(pDeclarationSpecifiers,
            token,
            lexeme))
        {
            Declaration_Specifiers(ctx, pDeclarationSpecifiers);
        }
    }
}

void Initializer(struct Parser* ctx,
                 struct Initializer** ppInitializer)
{
    //assert(*ppInitializer == NULL);
    /*
    initializer:
      assignment-expression
      { initializer-list }
      { initializer-list , }
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_LEFT_CURLY_BRACKET)
    {
        struct InitializerListType* pTInitializerList = NEW((struct InitializerListType)INITIALIZERLISTTYPE_INIT);
        *ppInitializer = (struct Initializer*)pTInitializerList;
        Parser_Match(ctx, &pTInitializerList->ClueList1);
        Initializer_List(ctx, &pTInitializerList->InitializerList);
        Parser_MatchToken(ctx, TK_RIGHT_CURLY_BRACKET,
                          &pTInitializerList->ClueList2);
    }
    else
    {
        struct Expression* pExpression = NULL;
        AssignmentExpression(ctx, &pExpression);
        *ppInitializer = (struct Initializer*)pExpression;
    }
}

void Initializer_List(struct Parser* ctx, struct InitializerList* pInitializerList)
{
    /*
    initializer-list:
      designation_opt initializer
      initializer-list , designation_opt initializer
    */
    for (; ;)
    {
        if (ErrorOrEof(ctx))
            break;
        enum TokenType token = Parser_CurrentTokenType(ctx);
        if (token == TK_RIGHT_CURLY_BRACKET)
        {
            //Empty initializer
            break;
        }
        struct InitializerListItem* pTInitializerListItem = NEW((struct InitializerListItem)INITIALIZERLISTITEM_INIT);
        List_Add(pInitializerList, pTInitializerListItem);
        if (token == TK_LEFT_SQUARE_BRACKET ||
            token == TK_FULL_STOP)
        {
            Designation(ctx, &pTInitializerListItem->DesignatorList);
        }
        Initializer(ctx, &pTInitializerListItem->pInitializer);
        token = Parser_CurrentTokenType(ctx);
        if (token == TK_COMMA)
        {
            Parser_Match(ctx, &pTInitializerListItem->ClueList);
        }
        else
        {
            break;
        }
    }
}

void Designation(struct Parser* ctx, struct DesignatorList* pDesignatorList)
{
    /*
    designation:
    designator-list =
    */
    Designator_List(ctx, pDesignatorList);
    Parser_MatchToken(ctx, TK_EQUALS_SIGN, NULL);//tODO
}

void Designator_List(struct Parser* ctx, struct DesignatorList* pDesignatorList)
{
    // http://www.drdobbs.com/the-new-c-declarations-initializations/184401377
    /*
    designator-list:
      designator
      designator-list designator
    */
    struct Designator* pDesignator = NEW((struct Designator)DESIGNATOR_INIT);
    Designator(ctx, pDesignator);
    List_Add(pDesignatorList, pDesignator);
    for (; ;)
    {
        if (ErrorOrEof(ctx))
            break;
        enum TokenType token = Parser_CurrentTokenType(ctx);
        if (token == TK_LEFT_SQUARE_BRACKET ||
            token == TK_FULL_STOP)
        {
            struct Designator* pDesignatorNew = NEW((struct Designator)DESIGNATOR_INIT);
            Designator(ctx, pDesignatorNew);
            List_Add(pDesignatorList, pDesignatorNew);
        }
        else
        {
            break;
        }
    }
}

void Designator(struct Parser* ctx, struct Designator* p)
{
    /*
    designator:
      [ constant-expression ]
      . identifier
    */
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK_LEFT_SQUARE_BRACKET)
    {
        Parser_Match(ctx, &p->ClueList0);
        ConstantExpression(ctx, &p->pExpression);
        Parser_Match(ctx, &p->ClueList1);
        Parser_MatchToken(ctx, TK_RIGHT_SQUARE_BRACKET, NULL);
    }
    else if (token == TK_FULL_STOP)
    {
        Parser_Match(ctx, &p->ClueList0);
        p->Name = strdup(Lexeme(ctx));
        Parser_MatchToken(ctx, TK_IDENTIFIER, &p->ClueList1);
    }
}

void Init_Declarator(struct Parser* ctx,
                     struct InitDeclarator** ppDeclarator2)
{
    /*
    init-declarator:
     declarator
     declarator = initializer
    */
    struct InitDeclarator* pInitDeclarator = NEW((struct InitDeclarator)INITDECLARATOR_INIT);
    Declarator(ctx, false, &pInitDeclarator->pDeclarator);
    enum TokenType token = Parser_CurrentTokenType(ctx);
    const char* declaratorName = InitDeclarator_FindName(pInitDeclarator);
    if (declaratorName)
    {
        //Fica em um contexto que vive so durante a declaracao
        //depois eh substituido
        SymbolMap_SetAt(ctx->pCurrentScope, declaratorName, (struct TypePointer*)pInitDeclarator);
    }
    //Antes do =
    *ppDeclarator2 = pInitDeclarator;
    if (token == TK_EQUALS_SIGN)
    {
        //assert(*ppDeclarator2 != NULL);
        Parser_Match(ctx, &pInitDeclarator->ClueList0);
        Initializer(ctx, &pInitDeclarator->pInitializer);
        ////TNodeClueList_MoveToEnd(&pInitDeclarator->ClueList, &ctx->Scanner.ClueList);
    }
}



void Init_Declarator_List(struct Parser* ctx,
                          struct InitDeclaratorList* pInitDeclaratorList)
{
    enum TokenType token = Parser_CurrentTokenType(ctx);
    /*
    init-declarator-list:
      init-declarator
      init-declarator-list , init-declarator
    */
    struct InitDeclarator* pInitDeclarator = NULL;
    Init_Declarator(ctx, &pInitDeclarator);
    List_Add(pInitDeclaratorList, pInitDeclarator);
    //Tem mais?
    token = Parser_CurrentTokenType(ctx);
    if (token == TK_COMMA)
    {
        Parser_Match(ctx, &pInitDeclarator->ClueList0);
        Init_Declarator_List(ctx, pInitDeclaratorList);
    }
}

void Parse_Declarations(struct Parser* ctx, struct Declarations* declarations);


void PopBack(struct TokenList* clueList)
{
    if (clueList->pHead &&
        clueList->pHead->pNext == clueList->pTail)
    {
        Token_Delete(clueList->pTail);
        clueList->pTail = clueList->pHead;
        clueList->pHead->pNext = 0;
    }
}

bool HasCommentedKeyword(struct TokenList* clueList, const char* keyword)
{
    bool bResult = false;
    struct Token* pCurrent = clueList->pTail;
    if (pCurrent &&
        pCurrent->token == TK_COMMENT)
    {
        bResult = strncmp(pCurrent->lexeme.c_str + 2, keyword, strlen(keyword)) == 0;
    }
    return bResult;
}


bool  Declaration(struct Parser* ctx,
                  struct AnyDeclaration** ppDeclaration)
{
    /*
    declaration:
      declaration-specifiers;
      declaration-specifiers init-declarator-list ;
      static_assert-declaration
    */
    bool bHasDeclaration = false;
    enum TokenType token = Parser_CurrentTokenType(ctx);
    if (token == TK__STATIC_ASSERT)
    {
        struct StaticAssertDeclaration* pStaticAssertDeclaration = NEW((struct StaticAssertDeclaration)STATICASSERTDECLARATION_INIT);
        *ppDeclaration = (struct AnyDeclaration*)pStaticAssertDeclaration;
        Static_Assert_Declaration(ctx, pStaticAssertDeclaration);
        bHasDeclaration = true;
    }
    else
    {
        struct Declaration* pFuncVarDeclaration = NEW((struct Declaration)DECLARATION_INIT);
        if (token == TK_SEMICOLON)
        {
            //declaracao vazia como ;
            bHasDeclaration = true;
            //Match(ctx);
        }
        else
        {
            if (Declaration_Specifiers_IsFirst(ctx, Parser_CurrentTokenType(ctx), Lexeme(ctx)))
            {
                Declaration_Specifiers(ctx, &pFuncVarDeclaration->Specifiers);
                bHasDeclaration = true;
            }
        }
        if (bHasDeclaration)
        {
            *ppDeclaration = (struct AnyDeclaration*)pFuncVarDeclaration;
            pFuncVarDeclaration->FileIndex = GetFileIndex(ctx);
            pFuncVarDeclaration->Line = GetCurrentLine(ctx);
            ////assert(pFuncVarDeclaration->FileIndex >= 0);
            token = Parser_CurrentTokenType(ctx);
            if (token == TK_SEMICOLON)
            {
                Parser_Match(ctx, &pFuncVarDeclaration->ClueList1);
            }
            else
            {
                //Pega os parametros das funcoes mas nao usa
                //se nao for uma definicao de funcao
                //////////////////////
                /////vou criar um escopo para declarators
                // int* p = malloc(sizeof p);
                //                        ^
                //                       p esta no contexto
                // mas nao tem toda declaracao
                struct SymbolMap BlockScope = SYMBOLMAP_INIT;
                BlockScope.pPrevious = ctx->pCurrentScope;
                ctx->pCurrentScope = &BlockScope;
                //Agora vem os declaradores que possuem os ponteiros
                Init_Declarator_List(ctx, &pFuncVarDeclaration->InitDeclaratorList);
                ctx->pCurrentScope = BlockScope.pPrevious;
                SymbolMap_Destroy(&BlockScope);
                ////////////////////////
                token = Parser_CurrentTokenType(ctx);
                //colocar os declaradores nos simbolos
                //agora ele monta a tabela com a declaracao toda
                for (struct InitDeclarator* pInitDeclarator = pFuncVarDeclaration->InitDeclaratorList.pHead;
                     pInitDeclarator != NULL;
                     pInitDeclarator = pInitDeclarator->pNext)
                {
                    const char* declaratorName = InitDeclarator_FindName(pInitDeclarator);
                    if (declaratorName != NULL)
                    {
                        SymbolMap_SetAt(ctx->pCurrentScope, declaratorName, (struct TypePointer*)pFuncVarDeclaration);
                    }
                    //ctx->
                }
                if (token == TK_LEFT_CURLY_BRACKET)
                {
                    //Ativa o escopo dos parametros
                    //Adiconar os parametros em um escopo um pouco a cima.
                    struct SymbolMap BlockScope2 = SYMBOLMAP_INIT;
                    struct InitDeclarator* pDeclarator3 =
                        pFuncVarDeclaration->InitDeclaratorList.pHead;
                    for (struct Parameter* pParameter = pDeclarator3->pDeclarator->pDirectDeclarator->Parameters.ParameterList.pHead;
                         pParameter != NULL;
                         pParameter = pParameter->pNext)
                    {
                        const char* parameterName = Declarator_GetName(&pParameter->Declarator);
                        if (parameterName != NULL)
                        {
                            SymbolMap_SetAt(&BlockScope2, parameterName, (struct TypePointer*)pParameter);
                        }
                        else
                        {
                            //parametro sem nome
                        }
                    }
                    BlockScope2.pPrevious = ctx->pCurrentScope;
                    ctx->pCurrentScope = &BlockScope2;
                    //SymbolMap_Print(ctx->pCurrentScope);
                    /*
                    6.9.1) function-definition:
                    declaration-specifiers declarator declaration-listopt compound-statement
                    */
                    struct CompoundStatement* pStatement;
                    Compound_Statement(ctx, &pStatement);
                    //TODO cast
                    ctx->pCurrentScope = BlockScope2.pPrevious;
                    SymbolMap_Destroy(&BlockScope2);
                    pFuncVarDeclaration->pCompoundStatementOpt = (struct CompoundStatement*)pStatement;
                }
                else
                {
                    //ANNOTATED AQUI TEM O COMENTARIO  antes do ;
                    Parser_MatchToken(ctx, TK_SEMICOLON, &pFuncVarDeclaration->ClueList1);
                }
            }
            // StrBuilder_Swap(&pFuncVarDeclaration->PreprocessorAndCommentsString,
            // &ctx->Scanner.PreprocessorAndCommentsString);
        }
        else
        {
            Declaration_Delete(pFuncVarDeclaration);
        }
    }
    return bHasDeclaration;
}


void Parse_Declarations(struct Parser* ctx, struct Declarations* declarations)
{
    int declarationIndex = 0;
    while (!ErrorOrEof(ctx))
    {
        struct AnyDeclaration* pDeclarationOut = NULL;
        bool bHasDecl = Declaration(ctx, &pDeclarationOut);
        if (bHasDecl)
        {
            Declarations_PushBack(declarations, pDeclarationOut);
            declarationIndex++;
        }
        else
        {
            if (Parser_CurrentTokenType(ctx) == TK_EOF)
            {
                //ok
                Parser_Match(ctx, NULL);
            }
            else
            {
                //nao ter mais declaracao nao eh erro
                //SetError(ctx, "declaration expected");
            }
            break;
        }
        if (Parser_CurrentTokenType(ctx) == TK_EOF)
        {
            break;
        }
        if (Parser_HasError(ctx))
            break;
    }
    if (Parser_CurrentTokenType(ctx) == TK_EOF)
    {
        struct EofDeclaration* pEofDeclaration = NEW((struct EofDeclaration)EOFDECLARATION_INIT);
        //ok
        Parser_Match(ctx, &pEofDeclaration->ClueList0);
        Declarations_PushBack(declarations, (struct AnyDeclaration*)pEofDeclaration);
    }
}

void Parser_Main(struct Parser* ctx, struct Declarations* declarations)
{
    Parse_Declarations(ctx, declarations);
}


bool BuildSyntaxTreeFromFile(const char* filename,
                             const char* configFileName /*optional*/,
                             struct SyntaxTree* pSyntaxTree)
{
    bool bResult = false;
    struct Parser parser;
    const bool bHasConfigFile = configFileName != NULL && configFileName[0] != '\0';
    if (bHasConfigFile)
    {
        //opcional
        char fullConfigFilePath[CPRIME_MAX_PATH] = { 0 };
        GetFullPathS(configFileName, fullConfigFilePath);
        Parser_InitFile(&parser, fullConfigFilePath);
        Parser_Main(&parser, &pSyntaxTree->Declarations);
        //apaga declaracoes eof por ex
        Declarations_Destroy(&pSyntaxTree->Declarations);
        pSyntaxTree->Declarations = (struct Declarations)TDECLARATIONS_INIT;
        //Some com o arquivo de configclea
        TokenList_Clear(&parser.Scanner.PreviousList);
        BasicScannerStack_Pop(&parser.Scanner.stack);
        //Some com o arquivo de config
    }
    char fullFileNamePath[CPRIME_MAX_PATH] = { 0 };
    GetFullPathS(filename, fullFileNamePath);
    if (filename != NULL)
    {
        if (!bHasConfigFile)
        {
            Parser_InitFile(&parser, fullFileNamePath);
        }
        else
        {
            Parser_PushFile(&parser, fullFileNamePath);
        }
        Parser_Main(&parser, &pSyntaxTree->Declarations);
    }
    TFileMapToStrArray(&parser.Scanner.FilesIncluded, &pSyntaxTree->Files);
    printf("%s\n", GetCompletationMessage(&parser));
    SymbolMap_Swap(&parser.GlobalScope, &pSyntaxTree->GlobalScope);
    if (Parser_HasError(&parser))
    {
        Scanner_PrintDebug(&parser.Scanner);
    }
    MacroMap_Swap(&parser.Scanner.Defines2, &pSyntaxTree->Defines);
    bResult = !Parser_HasError(&parser);
    Parser_Destroy(&parser);
    return bResult;
}


bool BuildSyntaxTreeFromString(const char* sourceCode,
                               struct SyntaxTree* pSyntaxTree)
{
    bool bResult = false;
    struct Parser parser;
    Parser_InitString(&parser, "source", sourceCode);
    Parser_Main(&parser, &pSyntaxTree->Declarations);
    TFileMapToStrArray(&parser.Scanner.FilesIncluded, &pSyntaxTree->Files);
    printf("%s\n", GetCompletationMessage(&parser));
    SymbolMap_Swap(&parser.GlobalScope, &pSyntaxTree->GlobalScope);
    if (Parser_HasError(&parser))
    {
        Scanner_PrintDebug(&parser.Scanner);
    }
    MacroMap_Swap(&parser.Scanner.Defines2, &pSyntaxTree->Defines);
    bResult = !Parser_HasError(&parser);
    Parser_Destroy(&parser);
    return bResult;
}


#define _CRTDBG_MAP_ALLOC


int Compile(const char* configFileName,
            const char* inputFileName,
            const char* outputFileName,
            struct OutputOptions* options);

char* CompileText(int type, int bNoImplicitTag, const char* input);



#define STDIOH \
"typedef int FILE;\n"\
"typedef int size_t;\n"\
"typedef void* va_list;\n"\
"int remove(const char *filename);\n"\
"int rename(const char *old, const char *news);\n"\
"FILE *tmpfile(void);\n"\
"char *tmpnam(char *s);\n"\
"int fclose(FILE *stream);\n"\
"int fflush(FILE *stream);\n"\
"FILE *fopen(const char * restrict filename,const char * restrict mode);\n"\
"FILE *freopen(const char * restrict filename,const char * restrict mode,FILE * restrict stream);\n"\
"void setbuf(FILE * restrict stream,char * restrict buf);\n"\
"int setvbuf(FILE * restrict stream,char * restrict buf, int mode, size_t size);\n"\
"int fprintf(FILE * restrict stream,const char * restrict format, ...);\n"\
"int fscanf(FILE * restrict stream, const char * restrict format, ...);\n"\
"int printf(const char * restrict format, ...);\n"\
"int scanf(const char * restrict format, ...);\n"\
"int snprintf(char * restrict s, size_t n, const char * restrict format, ...);\n"\
"int sprintf(char * restrict s, const char * restrict format, ...);\n"\
"int sscanf(const char * restrict s, const char * restrict format, ...);\n"\
"int vfprintf(FILE * restrict stream,const char * restrict format, va_list arg);\n"\
"int vfscanf(FILE * restrict stream,const char * restrict format, va_list arg);\n"\
"int vprintf(const char * restrict format, va_list arg);\n"\
"int vscanf(const char * restrict format, va_list arg);\n"

#define STDLIB \
"typedef int size_t;\n"\
"typedef int wchar_t;\n"\
"#define NULL 0\n"\
"#define EXIT_FAILURE 1\n"\
"#define EXIT_SUCCESS 0\n"\
"double atof(const char *nptr);\n"\
"int atoi(const char *nptr);\n"\
"long int atol(const char *nptr);\n"\
"long long int atoll(const char *nptr);\n"\
"double strtod(const char * restrict nptr,char ** restrict endptr);\n"\
"float strtof(const char * restrict nptr,char ** restrict endptr);\n"\
"long double strtold(const char * restrict nptr,char ** restrict endptr);\n"\
"long int strtol(const char * restrict nptr,char ** restrict endptr, int base);\n"\
"long long int strtoll(const char * restrict nptr,char ** restrict endptr, int base);\n"\
"unsigned long int strtoul(const char * restrict nptr,char ** restrict endptr, int base);\n"\
"unsigned long long int strtoull(const char * restrict nptr,char ** restrict endptr, int base);\n"\
"int rand(void);\n"\
"void srand(unsigned int seed);\n"\
"void *aligned_alloc(size_t alignment, size_t size);\n"\
"void *calloc(size_t nmemb, size_t size);\n"\
"void free(void *ptr);\n"\
"void *malloc(size_t size);\n"\
"void *realloc(void *ptr, size_t size);\n"\
"_Noreturn void abort(void);\n"\
"int atexit(void (*func)(void));\n"\
"int at_quick_exit(void (*func)(void));\n"\
"_Noreturn void exit(int status);\n"\
"_Noreturn void _Exit(int status);\n"\
"char *getenv(const char *name);\n"\
"_Noreturn void quick_exit(int status);\n"\
"int system(const char *string);\n"

#define STRING \
"typedef int size_t;\n"\
"#define NULL 0\n"\
"typedef int rsize_t;\n"\
"void *memcpy(void * restrict s1,const void * restrict s2, size_t n);\n"\
"void *memmove(void *s1, const void *s2, size_t n);\n"\
"char *strcpy(char * restrict s1, const char * restrict s2);\n"\
"char *strncpy(char * restrict s1,const char * restrict s2, size_t n);\n"\
"char *strcat(char * restrict s1,const char * restrict s2);\n"\
"char *strncat(char * restrict s1,const char * restrict s2, size_t n);\n"\
"int memcmp(const void *s1, const void *s2, size_t n);\n"\
"int strcmp(const char *s1, const char *s2);\n"\
"int strcoll(const char *s1, const char *s2);\n"\
"int strncmp(const char *s1, const char *s2, size_t n);\n"\
"size_t strxfrm(char * restrict s1,const char * restrict s2, size_t n);\n"\
"void *memchr(const void *s, int c, size_t n);\n"\
"char *strchr(const char *s, int c);\n"\
"size_t strcspn(const char *s1, const char *s2);\n"\
"char *strpbrk(const char *s1, const char *s2);\n"\
"char *strrchr(const char *s, int c);\n"\
"size_t strspn(const char *s1, const char *s2);\n"\
"char *strstr(const char *s1, const char *s2);\n"\
"char *strtok(char * restrict s1,const char * restrict s2);\n"\
"void *memset(void *s, int c, size_t n);\n"\
"char *strerror(int errnum);\n"\
"size_t strlen(const char *s);\n"\
"typedef int errno_t;\n"\
"errno_t memcpy_s(void * restrict s1, rsize_t s1max,const void * restrict s2, rsize_t n);\n"\
"errno_t memmove_s(void *s1, rsize_t s1max,const void *s2, rsize_t n);\n"\
"errno_t strcpy_s(char * restrict s1,rsize_t s1max,const char * restrict s2);\n"\
"errno_t strncpy_s(char * restrict s1,rsize_t s1max,const char * restrict s2, rsize_t n);\n"\
"errno_t strcat_s(char * restrict s1,rsize_t s1max,const char * restrict s2);\n"\
"errno_t strncat_s(char * restrict s1,rsize_t s1max,const char * restrict s2,rsize_t n);\n"\
"char *strtok_s(char * restrict s1,rsize_t * restrict s1max,\n"\
"const char * restrict s2,char ** restrict ptr);\n"\
"errno_t memset_s(void *s, rsize_t smax, int c, rsize_t n)\n"\
"errno_t strerror_s(char *s, rsize_t maxsize,errno_t errnum);\n"\
"size_t strerrorlen_s(errno_t errnum);\n"\
"\n"

#define STDBOOL \
"//\n"\
"// stdbool.h\n"\
"#ifndef _STDBOOL\n"\
"#define _STDBOOL\n"\
"\n"\
"#define __bool_true_false_are_defined 1\n"\
"\n"\
"#ifndef __cplusplus\n"\
"\n"\
"#define bool  _Bool\n"\
"#define false 0\n"\
"#define true  1\n"\
"\n"\
"#endif /* __cplusplus */\n"\
"\n"\
"#endif /* _STDBOOL */\n"

#define STDERRNO \
"#pragma once\n"\
"\n"\
"extern int errno;\n"\
"\n"\
"// Error codes\n"\
"#define EPERM           1\n"\
"#define ENOENT          2\n"\
"#define ESRCH           3\n"\
"#define EINTR           4\n"\
"#define EIO             5\n"\
"#define ENXIO           6\n"\
"#define E2BIG           7\n"\
"#define ENOEXEC         8\n"\
"#define EBADF           9\n"\
"#define ECHILD          10\n"\
"#define EAGAIN          11\n"\
"#define ENOMEM          12\n"\
"#define EACCES          13\n"\
"#define EFAULT          14\n"\
"#define EBUSY           16\n"\
"#define EEXIST          17\n"\
"#define EXDEV           18\n"\
"#define ENODEV          19\n"\
"#define ENOTDIR         20\n"\
"#define EISDIR          21\n"\
"#define ENFILE          23\n"\
"#define EMFILE          24\n"\
"#define ENOTTY          25\n"\
"#define EFBIG           27\n"\
"#define ENOSPC          28\n"\
"#define ESPIPE          29\n"\
"#define EROFS           30\n"\
"#define EMLINK          31\n"\
"#define EPIPE           32\n"\
"#define EDOM            33\n"\
"#define EDEADLK         36\n"\
"#define ENAMETOOLONG    38\n"\
"#define ENOLCK          39\n"\
"#define ENOSYS          40\n"\
"#define ENOTEMPTY       41\n"\
"\n"

char* CompileText(int type, int bNoImplicitTag, const char* input)
{
    struct EmulatedFile* files[] =
    {
        &(struct EmulatedFile)
        {
            "c:/stdio.h", STDIOH
        },
        &(struct EmulatedFile) {"c:/stdlib.h", STDLIB},
        &(struct EmulatedFile) {"c:/string.h", STRING},
        &(struct EmulatedFile) {"c:/stdbool.h", STDBOOL},
        &(struct EmulatedFile) {"c:/errno.h", STDERRNO},
        NULL
    };
    s_emulatedFiles = files;
    char* output = NULL;
    struct OutputOptions options2 = OUTPUTOPTIONS_INIT;
    options2.Target = type == 0 ? CompilerTarget_Preprocessed : CompilerTarget_C99;
    struct SyntaxTree pSyntaxTree = SYNTAXTREE_INIT;
    if (BuildSyntaxTreeFromString(input, &pSyntaxTree))
    {
        struct StrBuilder sb = STRBUILDER_INIT;
        StrBuilder_Reserve(&sb, 500);
        if (options2.Target == CompilerTarget_Preprocessed)
        {
            PrintPreprocessedToString2(&sb, input, NULL);
        }
        else
        {
            SyntaxTree_PrintCodeToString(&pSyntaxTree, &options2, &sb);
        }
        output = sb.c_str;
    }
    s_emulatedFiles = 0;
    return output;
}

int Compile(const char* configFileName,
            const char* inputFileName,
            const char* outputFileName,
            struct OutputOptions* options)
{
    int bSuccess = 0;
    struct SyntaxTree pSyntaxTree = SYNTAXTREE_INIT;
    clock_t tstart = clock();
    printf("Parsing...\n");
    if (BuildSyntaxTreeFromFile(inputFileName, configFileName, &pSyntaxTree))
    {
        bSuccess = 1;
        char drive[CPRIME_MAX_DRIVE];
        char dir[CPRIME_MAX_DIR];
        char fname[CPRIME_MAX_FNAME];
        char ext[CPRIME_MAX_EXT];
        SplitPath(inputFileName, drive, dir, fname, ext); // C4996
        printf("Generating code for %s...\n", inputFileName);
        if (outputFileName[0] != '\0')
        {
            SyntaxTree_PrintCodeToFile(&pSyntaxTree, options, outputFileName);
        }
        else
        {
            char outc[CPRIME_MAX_PATH] = { 0 };
            //gera em cima do proprio arquivo
            MakePath(outc, drive, dir, fname, ext);
            SyntaxTree_PrintCodeToFile(&pSyntaxTree, options, outc);
        }
        clock_t tend = clock();
        printf("Completed in %d second(s)\n", (int)((tend - tstart) / CLOCKS_PER_SEC));
    }
    SyntaxTree_Destroy(&pSyntaxTree);
    return bSuccess;
}

