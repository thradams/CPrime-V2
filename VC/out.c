#include "cloud_server.h"
#include "sha256.h"
#include "osstream.h"
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#include <stdlib.h>
#endif


#include <errno.h>
#include <assert.h>

#include "fs.h"
#include "stringex.h"
#include <ctype.h>
#include "json.h"
#include "smtp.h"
#include "cloud_messages.h"
#include "pagarme.h"
#include "async.h"
#include "http.h"
#include "fs.h"
#include "http_request.h"
#include "logs.h"

#define MESSAGE(LANG, X) ((LANG) == LANG_PT ? X)


/*
  Variavel global com o path, é escrita uma vez na vida
  no main e pode ser usada por diversas threads*/
static char s_RootPath[300];

//extern char s_GlobalPagarmeAPIKey[sizeof "ak_live_h1lndMoqTCmvFKPsxAv4PieSGt6i7g"];

#define ARRAY_AND_SIZE(x) (x), (sizeof(x)/sizeof((x)[0]))

static struct CloudServer s_cloudServer;

static bool IsValidPassword(const char* password)
{
    if (password == NULL)
        return false;
#ifndef _DEBUG
    int count = 0;
    while (*password != 0 && count < MAX_PASSWORD_SIZE)
    {
        password++;
        count++;
    }
    return count >= MIN_PASSWORD_SIZE && count < MAX_PASSWORD_SIZE;
#else
    return true;
#endif
}

static bool IsValidEmail(const char* email)
{
    if (email == NULL)
        return false;
    const char* p = email;
    int count = 0;
    //primeira parte até o @
    for (; *p && count < MAX_EMAIL_SIZE; count++, p++)
    {
        if (isdigit(*p) ||
            isalpha(*p) ||
            *p == '.' ||
            *p == '_')
        {
            /*ok*/
        }
        else
            break;
    }
    if (*p == '@')
    {
        count++;
        p++;
    }
    else
    {
        /*nunca achou o @*/
        return false;
    }
    //segunda parte depois do @
    for (; *p && count < MAX_EMAIL_SIZE; count++, p++)
    {
        if (isdigit(*p) ||
            isalpha(*p) ||
            *p == '-' ||
            *p == '.')
        {
            /*ok*/
        }
        else
            break;
    }
    return *p == 0;
}

void Cards_Destroy(struct Cards* cards);
void Projects_Destroy(struct Projects* projects);

void User_Delete(struct User* user)
{
    if (user)
    {
        Cards_Destroy(&user->cards);
        Projects_Destroy(&user->projects);
        free(user->name);
        free(user->contry);
        free(user->phoneNumber);
        free(user->documentNumber);
        free(user->zipcode);
        free(user->address);
        free(user->address_complement1);
        free(user->address_complement2);
        free(user->city);
        free(user->estate);
        free(user->estateid);

        free(user);
    }
}

static void User_ExchangePublic(struct Data* data, void* p)
{
    struct User* x = (struct User*)p;
    data->ExchangeStaticText(data, "email", x->Email, sizeof(x->Email));
    data->ExchangeText(data, "name", &x->name);
    data->ExchangeText(data, "address", &x->address);
    data->ExchangeText(data, "document_number", &x->documentNumber);
    data->ExchangeText(data, "contry", &x->contry);
    data->ExchangeText(data, "city", &x->city);
    data->ExchangeText(data, "estate", &x->estate);
    data->ExchangeText(data, "estateid", &x->estateid);
    data->ExchangeText(data, "phone", &x->phoneNumber);
    data->ExchangeText(data, "zipcode", &x->zipcode);
    data->ExchangeText(data, "address_complement1", &x->address_complement1);
    data->ExchangeText(data, "address_complement2", &x->address_complement2);
    data->ExchangeBool(data, "email_confirmed", &x->bEmailConfirmed);
}

static void User_ExchangePrivate(struct Data* data, void* p)
{
    struct User* x = (struct User*)p;
    data->ExchangeStaticText(data, "hash", x->hash, sizeof x->hash);
    User_ExchangePublic(data, p);
}

void MakeUserPasswordHash(const char* user,
                          const char* password,
                          char hashstring[PASSWORD_HASH_STRING_SIZE])
{
    char userpass[MAX_EMAIL_SIZE + MAX_PASSWORD_SIZE + 2] = { 0 };
    if (snprintf(userpass, sizeof(userpass), "%s:%s", user, password) < (int)sizeof(userpass))
    {
        uint8_t hash[SHA256_BYTES];
        sha256(userpass, strlen(userpass), hash);
        //passar para string em hexa
        for (int j = 0; j < SHA256_BYTES; j++)
        {
            snprintf(&hashstring[j * 2], 3, "%02x", (unsigned int)hash[j]);
        }
    }
}

void User_SetHash(struct User* pUser, const char* user, const char* password)
{
    MakeUserPasswordHash(user, password, pUser->hash);
}

char* User_SaveToJson(struct User* pUser)
{
    char* json = JsonSaveToString(User_ExchangePublic, pUser);
    return json;
}

int User_SetJson(struct User* pUser, const char* json, struct error* error)
{
    return JsonLoadFromString(json, User_ExchangePublic, pUser, error);
}

int User_DeleteDir(struct User* pUser, struct error* error)
{
    
    if (1) /*try*/
    {
        char userpath[ELIPSE_CLOUD_MAX_PATH];
        if (snprintf(userpath, sizeof userpath, "%sUsers\\%s\\user.json", s_RootPath, pUser->Email) >= (int)sizeof(userpath))
        {
            seterror(error, "internal error");
            /*throw*/ goto _catch_label1;
        }
        if (remove(userpath) != 0)
        {
            seterror(error, "cannot remove file %s", get_posix_error_message(errno));
            /*throw*/ goto _catch_label1;
        }
        dirname(userpath);
        if (rmdir(userpath) != 0)
        {
            seterror(error, "cannot remove dir %s", get_posix_error_message(errno));
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
    return error->code;
}

int User_SaveToFile(struct User* pUser, struct error* error)
{
    char userpath[ELIPSE_CLOUD_MAX_PATH];
    if (snprintf(userpath, sizeof userpath, "%sUsers\\%s\\user.json", s_RootPath, pUser->Email) < (int)sizeof(userpath))
    {
        JsonSaveToFile(userpath, User_ExchangePrivate, pUser, error);
    }
    else
    {
        /*too big*/
        seterror(error, "internal error %d", __LINE__);
    }
    return error->code;
}

//lint -sem(User_LoadFromFile, @P == 1n)
struct User* User_LoadFromFile(const char* path, struct error* error)
{
    struct User* p = NULL;
    
    if (1) /*try*/
    {
        p = calloc(1, sizeof * p);
        if (p == NULL)
        {
            /*throw*/ goto _catch_label1;
        }
        if (JsonLoadFromFile(path, User_ExchangePrivate, p, error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
        User_Delete(p);
        p = NULL;
    }
    return p;
}

int Users_Push(struct Users* p, struct User* pitem /*sink*/)
{
    int er = 0;
    
    if (1) /*try*/
    {
        if (p->size + 1 > p->capacity)
        {
            int n = (p->capacity == 0) ? 2 : p->capacity * 2;
            void* pnew = realloc(p->data, n * sizeof(p->data[0]));
            if (pnew == NULL)
            {
                er = ENOMEM;
                /*throw*/ goto _catch_label1;
            }
            p->data = pnew;
            p->capacity = n;
        }
        p->data[p->size] = pitem;
        p->size++;
    }
    else /*catch*/ _catch_label1:
    {
        User_Delete(pitem);
    }
    return er;
}

static int Cards_LoadFromDir(struct User* pUser, struct error* error);
static int Projects_LoadFromDir(struct User* pUser, struct error* error);

int Users_LoadFromDir(struct Users* users, struct error* error)
{
    DIR* dir = NULL;
    struct User* pUser = NULL;
    
    if (1) /*try*/
    {
        char path[ELIPSE_CLOUD_MAX_PATH];

        if (string_join(path, sizeof(path), s_RootPath, "Users") < 0)
        {
            seterror(error, "buffer too small");
            /*throw*/ goto _catch_label1;
        }

        dir = opendir(path);
        if (dir == NULL)
        {
            seterror(error, "Could not open the users dir");
            /*throw*/ goto _catch_label1;
        }

        struct dirent* dp;
        while ((dp = readdir(dir)) != NULL)
        {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            {
                /* skip self and parent */
                continue;
            }

            char userjson[ELIPSE_CLOUD_MAX_PATH];
            if (snprintf(userjson, sizeof userjson, "%sUsers/%s/user.json", s_RootPath, dp->d_name) >= (int)sizeof(userjson))
            {
                seterror(error, "buffer too small");
                /*throw*/ goto _catch_label1;
            }


            if (dp->d_type & DT_DIR)
            {
                struct error localerror = { 0 };
                assert(pUser == NULL);
                pUser = User_LoadFromFile(userjson, &localerror);
                if (pUser == NULL)
                {
                    seterror(error, "error loading user '%s' %s", dp->d_name, localerror.message);
                    /*throw*/ goto _catch_label1;
                }

                if (Cards_LoadFromDir(pUser, &localerror) != 0)
                {
                    seterror(error, "error loading cards from user '%s'", pUser->Email);
                    /*throw*/ goto _catch_label1;
                }

                if (Projects_LoadFromDir(pUser, &localerror) != 0)
                {
                    seterror(error, "error loading projects from user '%s' : %s", pUser->Email, localerror.message);
                    /*throw*/ goto _catch_label1;
                }
                Users_Push(users, pUser /*sink*/);
                pUser = NULL; /*MOVED*/
            }
        }

    }
    else /*catch*/ _catch_label1:
    {
    }

    if (pUser)
    {
        User_Delete(pUser);
    }

    if (dir != NULL)
    {
        closedir(dir);
    }

    return error->code;
}

struct User* Users_FindUserByPasswordHash(struct Users* users, const char* hash)
{
    assert(users != NULL);
    assert(hash != NULL);
    for (int i = 0; i < users->size; i++)
    {
        if (strcmp(hash, users->data[i]->hash) == 0)
        {
            return users->data[i];
        }
    }
    return NULL;
}

int Users_FindUserIndexByEmail(struct Users* users, const char* userName)
{
    assert(userName != NULL);
    for (int i = 0; i < users->size; i++)
    {
        if (strcmp(userName, users->data[i]->Email) == 0)
        {
            return i;
        }
    }
    return -1;
}

struct User* Users_FindUserByEmail(struct Users* users, const char* userName)
{
    assert(userName != NULL);
    for (int i = 0; i < users->size; i++)
    {
        if (strcmp(userName, users->data[i]->Email) == 0)
        {
            return users->data[i];
        }
    }
    return NULL;
}

int Users_RemoveUserByEmail(struct Users* users, const char* userName)
{
    int index = Users_FindUserIndexByEmail(users, userName);
    if (index >= 0)
    {
        struct User* pUser = users->data[0];
        User_Delete(pUser);

        if (users->size - index - 1 > 0)
        {
            memmove(users->data + index,
                    users->data + index + 1,
                    sizeof(users->data[0]) * (users->size - index - 1));
        }
        users->size--;
    }
    return index >= 0 ? 0 : 1;
}

void Users_Destroy(struct Users* p)
{
    for (int i = 0; i < p->size; i++)
    {
        User_Delete(p->data[i]);
    }
    free(p->data);
}

static void Project_Exchange(struct Data* data, void* p)
{
    struct Project* pProject = p;
    data->ExchangeText(data, "name", &pProject->name);
    //data->ExchangeText(data, "appname", &pMobilePlan->appName);
    //data->ExchangeInt(data, "num_users", &pMobilePlan->numUsers);
    //data->ExchangeInt(data, "num_tags", &pMobilePlan->numTags);
    //data->ExchangeText(data, "plan_name", &pMobilePlan->planName);
}

static void Plan_Exchange(struct Data* data, void* p)
{
    struct Plan* pMobilePlan = p;
    data->ExchangeStaticText(data, "id", pMobilePlan->id, sizeof pMobilePlan->id);
    data->ExchangeText(data, "appname", &pMobilePlan->appName);
    data->ExchangeStaticText(data, "product_name", pMobilePlan->productName, sizeof pMobilePlan->productName);

    data->ExchangeText(data, "current_plan_name", &pMobilePlan->current_planName);
    data->ExchangeText(data, "new_plan_name", &pMobilePlan->new_planName);
}

void Card_Delete(struct Card* p)
{
    if (p)
    {
        free(p);
    }
}

static void Card_Exchange(struct Data* data, void* p)
{
    struct Card* x = (struct Card*)p;
    data->ExchangeStaticText(data, "id", x->id, sizeof(x->id));

    data->ExchangeStaticText(data, "pagarme_id", x->pagarme_id, sizeof(x->pagarme_id));
    data->ExchangeStaticText(data, "date_created", x->date_created, sizeof(x->date_created));
    data->ExchangeStaticText(data, "date_updated", x->date_updated, sizeof(x->date_updated));
    data->ExchangeStaticText(data, "first_digits", x->first_digits, sizeof(x->first_digits));
    data->ExchangeStaticText(data, "last_digits", x->last_digits, sizeof(x->last_digits));
    data->ExchangeStaticText(data, "expiration_date", x->expiration_date, sizeof(x->expiration_date));
    data->ExchangeStaticText(data, "holder_name", x->holder_name, sizeof(x->holder_name));
}

int Card_SaveToFile(struct User* pUser, struct Card* pCard, struct error* error)
{
    char cardpath[ELIPSE_CLOUD_MAX_PATH];
    if (snprintf(cardpath, sizeof cardpath, "%susers\\%s\\cards\\%s.json", s_RootPath, pUser->Email, pCard->id) < (int)sizeof(cardpath))
    {
        JsonSaveToFile(cardpath, Card_Exchange, pCard, error);
    }
    else
    {
        /*too big*/
        seterror(error, "internal error %d", __LINE__);
    }
    return error->code;
}

struct Card* Card_LoadFromFile(const char* fileName, struct error* error)
{
    struct Card* pCard = NULL;
    
    if (1) /*try*/
    {
        pCard = calloc(1, sizeof * pCard);
        if (pCard == NULL)
        {
            seterror(error, "out of mem");
            /*throw*/ goto _catch_label1;
        }

        if (JsonLoadFromFile(fileName, Card_Exchange, pCard, error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
        Card_Delete(pCard);
        pCard = NULL;
    }

    return pCard;
}

int Cards_Push(struct Cards* p, struct Card* pitem)
{
    int errcode = 0;
    
    if (1) /*try*/
    {
        if (p->size + 1 > p->capacity)
        {
            int n = p->capacity * 2;

            if (n == 0)
                n = 1;

            void* pnew = realloc(p->data, n * sizeof(p->data[0]));

            if (pnew == NULL)
            {
                errcode = ENOMEM;
                /*throw*/ goto _catch_label1;
            }

            p->data = pnew;
            p->capacity = n;
        }
        p->data[p->size] = pitem; /*MOVED*/
        pitem = NULL;
        p->size++;
    }
    else /*catch*/ _catch_label1:
    {
        Card_Delete(pitem);
        pitem = NULL;
    }

    return errcode;
}

void Cards_Destroy(struct Cards* cards)
{
    for (int i = 0; i < cards->size; i++)
    {
        Card_Delete(cards->data[i]);
    }
    free(cards->data);
}

void Plans_Destroy(struct Plans* plans);
void Project_Delete(struct Project* pProject)
{
    if (pProject)
    {
        Plans_Destroy(&pProject->plans);
        free(pProject->name);
        free(pProject);
    }
}

struct Plan* Project_FindPlan(struct Project* pProject, const char* id)
{
    struct Plan* p = NULL;
    for (int i = 0; i < pProject->plans.size; i++)
    {
        if (strcmp(pProject->plans.data[i]->id, id) == 0)
        {
            p = pProject->plans.data[i];
            break;
        }
    }
    return p;
}

int Projects_Push(struct Projects* p, struct Project* pitem)
{
    assert(pitem->name != NULL);

    int errcode = 0;
    
    if (1) /*try*/
    {
        if (p->size + 1 > p->capacity)
        {
            int n = p->capacity * 2;

            if (n == 0)
                n = 1;

            void* pnew = realloc(p->data, n * sizeof(p->data[0]));

            if (pnew == NULL)
            {
                errcode = ENOMEM;
                /*throw*/ goto _catch_label1;
            }

            p->data = pnew;
            p->capacity = n;
        }
        p->data[p->size] = pitem;
        pitem = NULL; /*MOVED*/
        p->size++;
    }
    else /*catch*/ _catch_label1:
    {
        Project_Delete(pitem);
        pitem = NULL;
    }

    return errcode;
}
void Projects_Destroy(struct Projects* projects)
{
    for (int i = 0; i < projects->size; i++)
    {
        Project_Delete(projects->data[i]);
    }
    free(projects->data);
}

static void Plan_Delete(struct Plan* pPlan)
{
    if (pPlan)
    {
        free(pPlan->appName);
        free(pPlan->current_planName);
        free(pPlan->new_planName);
        free(pPlan);
    }
}

void Plans_Destroy(struct Plans* plans)
{
    for (int i = 0; i < plans->size; i++)
    {
        Plan_Delete(plans->data[i]);
    }
    free(plans->data);
}

int Plans_Push(struct Plans* p, struct Plan* pitem)
{
    int errcode = 0;
    
    if (1) /*try*/
    {
        if (p->size + 1 > p->capacity)
        {
            int n = p->capacity * 2;

            if (n == 0)
                n = 1;

            void* pnew = realloc(p->data, n * sizeof(p->data[0]));

            if (pnew == NULL)
            {
                errcode = ENOMEM;
                /*throw*/ goto _catch_label1;
            }

            p->data = pnew;
            p->capacity = n;
        }
        p->data[p->size] = pitem;
        p->size++;
    }
    else /*catch*/ _catch_label1:
    {
        Plan_Delete(pitem);
    }

    return errcode;
}

static void Card_PagarmeExchange(struct Data* data, void* p)
{
    /*
    leitura do json enviado pelo pagarme faz a leitura do
    'id' como sendo pagarme_id
    */
    struct Card* x = (struct Card*)p;
    data->ExchangeStaticText(data, "id", x->pagarme_id, sizeof(x->pagarme_id));

    data->ExchangeStaticText(data, "date_created", x->date_created, sizeof(x->date_created));
    data->ExchangeStaticText(data, "date_updated", x->date_updated, sizeof(x->date_updated));
    data->ExchangeStaticText(data, "first_digits", x->first_digits, sizeof(x->first_digits));
    data->ExchangeStaticText(data, "last_digits", x->last_digits, sizeof(x->last_digits));
    data->ExchangeStaticText(data, "expiration_date", x->expiration_date, sizeof(x->expiration_date));
    data->ExchangeStaticText(data, "holder_name", x->holder_name, sizeof(x->holder_name));
}

int Plan_SaveToFile(struct User* pUser, struct Project* pProject, struct Plan* pPlan, struct error* error)
{
    char cardpath[ELIPSE_CLOUD_MAX_PATH];
    if (snprintf(cardpath, sizeof cardpath, "%susers\\%s\\projects\\%s\\plans\\%s.json", s_RootPath, pUser->Email, pProject->name, pPlan->id) < (int)sizeof(cardpath))
    {
        JsonSaveToFile(cardpath, Plan_Exchange, pPlan, error);
    }
    else
    {
        /*too big*/
        seterror(error, "internal error %d", __LINE__);
    }
    return error->code;
}

struct Project* Project_LoadFromFile(const char* fileName, struct error* error)
{
    struct Project* pProject = calloc(1, sizeof * pProject);
    if (pProject)
    {
        if (JsonLoadFromFile(fileName, Project_Exchange, pProject, error) != 0)
        {
            Project_Delete(pProject);
            pProject = NULL;
        }
    }
    return pProject;
}

struct Plan* Plan_LoadFromFile(const char* fileName, struct error* error)
{
    struct Plan* pPlan = calloc(1, sizeof * pPlan);
    if (pPlan)
    {
        JsonLoadFromFile(fileName, Plan_Exchange, pPlan, error);
    }
    return pPlan;
}

static int Plans_LoadFromDir(struct User* pUser, struct Project* pProject, struct error* error);

static int Projects_LoadFromDir(struct User* pUser, struct error* error)
{
    DIR* dir = NULL;
    struct Project* pProject = NULL;

    

    if (1) /*try*/
    {
        char path[ELIPSE_CLOUD_MAX_PATH];
        snprintf(path, sizeof(path), "%susers/%s/projects", s_RootPath, pUser->Email);
        dir = opendir(path);
        if (dir == NULL)
        {
            seterror(error, "Could not open the project dir '%s'", path);
            /*throw*/ goto _catch_label1;
        }

        struct dirent* dp;
        while ((dp = readdir(dir)) != NULL)
        {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            {
                /* skip self and parent */
                continue;
            }

            char obj_file_name[ELIPSE_CLOUD_MAX_PATH];
            snprintf(obj_file_name, sizeof(obj_file_name), "%susers/%s/projects/%s/project.json", s_RootPath, pUser->Email, dp->d_name);

            if (dp->d_type & DT_DIR)
            {
                assert(pProject == NULL);
                struct error localerror = { 0 };
                pProject = Project_LoadFromFile(obj_file_name, &localerror);
                if (pProject == NULL)
                {
                    seterror(error, "error loading project '%s' %s", dp->d_name, localerror.message);
                    /*throw*/ goto _catch_label1;
                }

                if (Plans_LoadFromDir(pUser, pProject, error) != 0)
                {
                    /*throw*/ goto _catch_label1;
                }
                Projects_Push(&pUser->projects, pProject /*moved*/);
                pProject = NULL;
            }
        }
    }
    else /*catch*/ _catch_label1:
    {
    }

    if (pProject != NULL)
    {
        /*pendurado*/
        Project_Delete(pProject);
    }

    if (dir)
    {
        closedir(dir);
    }

    return error->code;

}

static int Cards_LoadFromDir(struct User* pUser, struct error* error)
{
    char path[ELIPSE_CLOUD_MAX_PATH];
    snprintf(path, sizeof(path), "%susers/%s/cards", s_RootPath, pUser->Email);
    DIR* dir = opendir(path);
    if (dir == NULL)
    {
        int errcode = errno;
        seterror(error, "Could not open the users dir '%s' %d", path, errcode);
        return errcode;
    }
    struct dirent* dp;
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            /* skip self and parent */
            continue;
        }
        char obj_file_name[ELIPSE_CLOUD_MAX_PATH];
        snprintf(obj_file_name, sizeof(obj_file_name), "%susers/%s/cards/%s", s_RootPath, pUser->Email, dp->d_name);
        if (dp->d_type & DT_DIR)
        {

        }
        else
        {
            struct error localerror = { 0 };
            struct Card* pCard = Card_LoadFromFile(obj_file_name, &localerror);
            if (pUser)
            {
                Cards_Push(&pUser->cards, pCard /*moved*/);
            }
            else
            {
                seterror(error, "error loading card '%s' %s", dp->d_name, localerror.message);
            }
        }
        if (error->code != 0)
            break;
    }
    closedir(dir);
    return error->code;
}

static int Plans_LoadFromDir(struct User* pUser, struct Project* pProject, struct error* error)
{
    char path[ELIPSE_CLOUD_MAX_PATH];
    snprintf(path, sizeof(path), "%susers/%s/projects/%s/plans", s_RootPath, pUser->Email, pProject->name);
    DIR* dir = opendir(path);
    if (dir == NULL)
    {
        int errcode = errno;
        seterror(error, "Could not open the users dir '%s' %d", path, errcode);
        return errcode;
    }
    struct dirent* dp;
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            /* skip self and parent */
            continue;
        }
        char obj_file_name[ELIPSE_CLOUD_MAX_PATH];
        snprintf(obj_file_name, sizeof(obj_file_name), "%susers/%s/projects/%s/plans/%s", s_RootPath, pUser->Email, pProject->name, dp->d_name);
        if (dp->d_type & DT_DIR)
        {

        }
        else
        {
            struct error localerror = { 0 };
            struct Plan* pPlan = Plan_LoadFromFile(obj_file_name, &localerror);
            if (pUser)
            {
                Plans_Push(&pProject->plans, pPlan /*moved*/);
            }
            else
            {
                seterror(error, "error loading card '%s' %s", dp->d_name, localerror.message);
            }
        }
        if (error->code != 0)
            break;
    }
    closedir(dir);
    return error->code;
}

struct Project* User_FindProjectByName(struct User* pUser, const char* projectName)
{
    struct Project* p = 0;
    for (int i = 0; i < pUser->projects.size; i++)
    {
        if (strcasecmp(pUser->projects.data[i]->name, projectName) == 0)
        {
            p = pUser->projects.data[i];
            break;
        }
    }
    return p;
}

static void ForgotPasswordHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)) != 0)
        {
            seterror(&response->error, "Invalid email.");
            /*throw*/ goto _catch_label1;
        }
        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, email);
        if (pUser == NULL)
        {
            seterror(&response->error, "User not found");
            /*throw*/ goto _catch_label1;
        }
        /*criamos um novo token para mudar passord*/
        NewUUID(pUser->activationtoken);
        char link[500];
        if (snprintf(link, sizeof link, "http://localhost/index.html?code=%s&username=%s#PasswordReset", pUser->activationtoken, pUser->Email) >= (int)sizeof(link))
        {
            seterror(&response->error, "internal error");
            /*throw*/ goto _catch_label1;
        }
        char email_body[1000];
        if (string_format(email_body, sizeof email_body, MESSAGE(request->AcceptLanguage, IDS_FORGOTPASS_CLOUDADMIN_MESSAGE), pUser->Email, link) >= (int)sizeof(email_body))
        {
            seterror(&response->error, "internal error");
            /*throw*/ goto _catch_label1;
        }
        SendMailAsync(&s_cloudServer.smtp,
                      pUser->Email,
                      MESSAGE(request->AcceptLanguage, IDS_FORGOTPASS_SUBJECT),
                      email_body);
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void SendActivationEmail(enum Language lang, struct User* pUser)
{
    if (pUser->bEmailConfirmed)
    {
        /*já foi confirmado*/
        return;
    }
    /*calcula tamanho*/
    int n = string_format(NULL, 0, MESSAGE(lang, IDS_ACTIVATION_MAIL), "http://localhost", pUser->activationtoken);
    if (n > 0)
    {
        char* email_body = malloc(n * sizeof(char));
        if (email_body)
        {
            n = string_format(email_body, n, MESSAGE(lang, IDS_ACTIVATION_MAIL), "http://localhost", pUser->activationtoken);
            SendMailAsync(&s_cloudServer.smtp,
                          pUser->Email,
                          MESSAGE(lang, IDS_ELIPSE_MOBILE_WELCOME),
                          email_body);
            free(email_body);
        }
    }
}

static void CreateAccountHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    /*
     Handlers só fazem verificação de tamanho de input e não
     das caracteristicas.
    */
    enum Language lang = request->AcceptLanguage;
    struct User* pUser = NULL;
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char email[MAX_EMAIL_SIZE];
        char password[MAX_PASSWORD_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)) != 0 ||
            !IsValidEmail(email))
        {
            seterror(&response->error, MESSAGE(lang, IDS_INVALID_EMAIL));
            /*throw*/ goto _catch_label1;
        }
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(password)) != 0 ||
            !IsValidPassword(password))
        {
            seterror(&response->error, MESSAGE(lang, IDS_INVALID_PASSWORD));
            /*throw*/ goto _catch_label1;
        }
        char plan[MAX_PLAN_NAME_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(plan)) != 0)
        {
            seterror(&response->error, "invalid plan");
            /*throw*/ goto _catch_label1;
        }
        struct User* pUserLocal = Users_FindUserByEmail(&s_cloudServer.users, email);
        if (pUserLocal != NULL)
        {
            /*caso usuario ja existe*/
            seterror(&response->error, MESSAGE(lang, IDS_USER_ALREADY_EXIST));
            /*throw*/ goto _catch_label1;
        }
        char userpath[ELIPSE_CLOUD_MAX_PATH];
        if (snprintf(userpath, sizeof userpath, "%sUsers\\%s", s_RootPath, email) >= (int)sizeof(userpath))
        {
            /*erro interno do calculo errado dos tamanhos*/
            seterror(&response->error, "Internal error : email path too big.");
            /*throw*/ goto _catch_label1;
        }
        assert(pUser == NULL);
        pUser = calloc(1, sizeof * pUser);
        if (pUser == NULL)
        {
            seterror(&response->error, "out of memory");
            /*throw*/ goto _catch_label1;
        }
        assert(pUser != NULL);
        if (mkdir(userpath, 0700) != 0)
        {
            seterror(&response->error, "Internal error : email path already exists?");
            /*throw*/ goto _catch_label1;
        }
        NewUUID(pUser->activationtoken);
        snprintf(pUser->Email, sizeof pUser->Email, "%s", email);
        User_SetHash(pUser, email, password);
        if (User_SaveToFile(pUser, &response->error) != 0)
        {
            /*nao vou enviar os detalhes deste error*/
            clearerror(&response->error);
            seterror(&response->error, "Internal error");
            /*throw*/ goto _catch_label1;
        }
        if (Users_Push(&s_cloudServer.users, pUser) != 0)
        {
            seterror(&response->error, "Internal error");
            /*throw*/ goto _catch_label1;
        }
        SendActivationEmail(lang, pUser);
        pUser = 0; /*movido no Users_Push*/
    }
    else /*catch*/ _catch_label1:
    {
    }
    /*caso nao tenha sido movido*/
    User_Delete(pUser);
}

static void ResendVerificationHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)) != 0)
        {
            seterror(&response->error, "invalid argument");
            /*throw*/ goto _catch_label1;
        }
        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, email);
        if (pUser == NULL)
        {
            seterror(&response->error, "Invalid email");
            /*throw*/ goto _catch_label1;
        }
        if (pUser->bEmailConfirmed)
        {
            seterror(&response->error, "This email has already been confirmed.");
            /*throw*/ goto _catch_label1;
        }
        SendActivationEmail(request->AcceptLanguage, pUser);
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void SaveUserAppTask(enum TASK_ACTION action, struct Capture* pCapture)
{
    /*roda no handler actor, entao eh seguro usar s_cloudServer*/
    struct HttpResponse* response = pCapture->arg1.pHttpResponse;
    const char* appname = pCapture->arg2.pstring;
    const char* email = pCapture->arg3.pstring;
    assert(pCapture->arg4.pstring == NULL);

    if (action == TASK_RUN)
    {
        
        if (1) /*try*/
        {
            struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, email);
            if (pUser == NULL)
                /*throw*/ goto _catch_label1;
            struct error error = { 0 };
            User_SaveToFile(pUser, &error);
        }
        else /*catch*/ _catch_label1:
        {
        }
    }
    free((void*)appname);
    free((void*)email);
    HttpResponse_Sink(response);
}

static void CreateMobileAppHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    struct error error = { 0 };
    char* response_str = 0;
    char* response2_str = 0;
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char appname[100];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(appname)))
        {
            seterror(&error, u8"invalid app name");
            /*throw*/ goto _catch_label1;
        }
        struct http_request http_request;
        http_request.url = "http://localhost:8080/api/login";
        http_request.method = "POST";
        http_request.headers = "X-Csrf-Token:0";
        http_request.content_type = "application/x-www-form-urlencoded";
        http_request.content = "&persistent=false&id=&version=1.5.160&encryption=acddd3306198ba9a108c80c775a705a901e259ee";
        if (http_request_send_sync(&http_request, &response_str, &error) != 0)
            /*throw*/ goto _catch_label1;
        scanner =(struct JsonScanner){
        
            0}
        ;
        scanner.json = response_str;
        JsonScanner_Match(&scanner);//
        JsonScanner_Match(&scanner);//{
        JsonScanner_Match(&scanner);//""
        JsonScanner_Match(&scanner);//:
        http_request.url = "http://localhost:8080/api/create-app";
        http_request.method = "POST";
        char headers[100] = { 0 };
        snprintf(headers, sizeof headers, "X-Csrf-Token: %.*s", scanner.LexemeByteSize, &scanner.json[scanner.LexemeByteStart]);
        http_request.headers = headers;
        http_request.content_type = "application/x-www-form-urlencoded";
        char content[300] = { 0 };
        snprintf(content, sizeof content, "&app=%s&copyImages=true&template=Empty&run=true", appname);
        http_request.content = content;
        if (http_request_send_sync(&http_request, &response2_str, &error) != 0)
            /*throw*/ goto _catch_label1;
        /*como estamos executando fora do actor do server,
         vamos enviar uma mensagem para salver nele
        */
        struct Capture capture = { 0 };
        capture.arg2.pstring = strdup(appname);
        capture.arg3.pstring = strdup(request->session.email);
        capture.arg1.pHttpResponse = response;
        response = 0; /*MOVED*/
        RunAtHandlerActor(SaveUserAppTask, &capture /*sink*/);
    }
    else /*catch*/ _catch_label1:
    {
    }
    free(response2_str);
    free(response_str);
}

static void SaveUserDeletedAppTask(enum TASK_ACTION action, struct Capture* pCapture)
{
    struct HttpResponse* response = pCapture->arg1.pHttpResponse;
    const char* appname = pCapture->arg2.pstring;
    const char* email = pCapture->arg3.pstring;
    if (action == TASK_RUN)
    {
        
        if (1) /*try*/
        {
            struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, email);
            if (pUser == NULL)
                /*throw*/ goto _catch_label1;
            struct error error = { 0 };
            User_SaveToFile(pUser, &error);
        }
        else /*catch*/ _catch_label1:
        {
        }
    }
    HttpResponse_Sink(response);
    free((void*)appname);
    free((void*)email);
}

static void GetPlansHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };

        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)))
        {
            seterror(&response->error, "invalid email");
            /*throw*/ goto _catch_label1;
        }

        char projectName[100];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(projectName)))
        {
            seterror(&response->error, u8"invalid project name");
            /*throw*/ goto _catch_label1;
        }

        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            /*considerando que ele tem sessao...deveria existir*/
            seterror(&response->error, "internal error");
            /*throw*/ goto _catch_label1;
        }

        struct Project* pProject = User_FindProjectByName(pUser, projectName);
        if (pProject == NULL)
        {
            seterror(&response->error, "project '%s' not found", projectName);
            /*throw*/ goto _catch_label1;
        }

        struct osstream oss = { 0 };
        ss_printf(&oss, "[");
        for (int i = 0; i < pProject->plans.size; i++)
        {
            char* json = JsonSaveToString(Plan_Exchange, pProject->plans.data[i]);
            if (json)
            {
                if (i > 0)
                {
                    ss_printf(&oss, ",");
                }

                ss_printf(&oss, "%s", json);
                free(json);
            }
        }
        ss_printf(&oss, "]");
        response->Content = oss.c_str; /*moved*/
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void GetProjectsHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            seterror(&response->error, u8"Internal server error");
            /*throw*/ goto _catch_label1;
        }

        int count = 0;
        struct osstream os = { 0 };
        ss_printf(&os, "[");
        char path[ELIPSE_CLOUD_MAX_PATH];
        snprintf(path, sizeof(path), "%susers/%s/projects", s_RootPath, pUser->Email);
        DIR* dir = opendir(path);
        if (dir == NULL)
        {
            int errcode = errno;
            log_error("Could not open the users dir '%s' %d", path, errcode);
            seterror(&response->error, "Internal server error");
            /*throw*/ goto _catch_label1;
        }
        struct dirent* dp;
        while ((dp = readdir(dir)) != NULL)
        {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            {
                /* skip self and parent */
                continue;
            }
            if (count != 0)
            {
                ss_printf(&os, ",");
            }
            count++;

            ss_printf(&os, "\"%s\"", dp->d_name); /*nome de projeto nao tem nada de especial*/
        }
        closedir(dir);
        ss_printf(&os, "]");
        response->Content = os.c_str;

    }
    else /*catch*/ _catch_label1:
    {
    }
}

static bool IsValidProjectName(const char* name)
{
    int index = 0;
    if ((name[index] >= 'A' && name[index] <= 'Z') ||
        (name[index] >= 'a' && name[index] <= 'z') ||
        name[index] == '_')
    {
        index++;
        while ((name[index] >= 'A' && name[index] <= 'Z') ||
               (name[index] >= 'a' && name[index] <= 'z') ||
               (name[index] >= '0' && name[index] <= '9') ||
               name[index] == '_')
        {
            index++;
        }

        if (name[index] == '\0')
        {
            return true;
        }
    }

    return false;
}

int Project_SaveToFile(struct User* pUser, struct Project* pProject, struct error* error)
{
    
    if (1) /*try*/
    {
        /*vamos criar pasta projeto*/

        char path[ELIPSE_CLOUD_MAX_PATH];
        if (snprintf(path, sizeof path, "%susers\\%s\\projects\\%s", s_RootPath, pUser->Email, pProject->name) >= (int)sizeof(path))
        {
            seterror(error, "path too long");
            /*throw*/ goto _catch_label1;
        }

        int errorcode = mkdir(path, 0700);
        if (errorcode != 0)
        {
            seterror(error, "mkdir failed");
            /*throw*/ goto _catch_label1;
        }
        
        /*ja vou deixar criada pasta plans*/
        if (snprintf(path, sizeof path, "%susers\\%s\\projects\\%s\\plans", s_RootPath, pUser->Email, pProject->name) >= (int)sizeof(path))
        {
            seterror(error, "path too long");
            /*throw*/ goto _catch_label1;
        }
        
        errorcode = mkdir(path, 0700);
        if (errorcode != 0)
        {
            seterror(error, "mkdir failed");
            /*throw*/ goto _catch_label1;
        }


        if (snprintf(path, sizeof path, "%susers\\%s\\projects\\%s\\project.json", s_RootPath, pUser->Email, pProject->name) >= (int)sizeof(path))
        {
            seterror(error, "path too long");
            /*throw*/ goto _catch_label1;
        }

        if (JsonSaveToFile(path, Project_Exchange, pProject, error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
    return error->code;
}

static void CreateProjectHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    struct Project* pProject = NULL;
    struct error error = { 0 };
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char name[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(name)) &&
            IsValidProjectName(name))
        {
            seterror(&response->error, u8"invalid name");
            /*throw*/ goto _catch_label1;
        }


        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            seterror(&response->error, "user '%s' not found", request->session.email);
            /*throw*/ goto _catch_label1;
        }


        pProject = calloc(1, sizeof * pProject);
        if (pProject == NULL)
        {
            seterror(&response->error, "out of mem");
            /*throw*/ goto _catch_label1;
        }
        pProject->name = strdup(name);
        if (pProject->name == NULL)
        {
            seterror(&response->error, "out of mem");
            /*throw*/ goto _catch_label1;
        }

        if (Project_SaveToFile(pUser, pProject, &error) != 0)
        {
            log_error("%s", error.message);
            seterror(&response->error, "internal error saving project");
            /*throw*/ goto _catch_label1;
        }
        //ver se nome nao eh usado.

        Projects_Push(&pUser->projects, pProject);
        pProject = NULL; /*moved*/        
    }
    else /*catch*/ _catch_label1:
    {
    }

    if (pProject != NULL)
    {
        Project_Delete(pProject);
    }
}

static void AddEditMobilePlanHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)))
        {
            seterror(&response->error, u8"invalid email");
            /*throw*/ goto _catch_label1;
        }

        char planname[100];
        //TODO IsValidPlanName
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(planname)))
        {
            seterror(&response->error, u8"invalid plan name");
            /*throw*/ goto _catch_label1;
        }

        char planId[UUID_STRING_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(planId)))
        {
            seterror(&response->error, u8"invalid id");
            /*throw*/ goto _catch_label1;
        }


        char projectName[100];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(projectName)) ||
            !IsValidProjectName(projectName))
        {
            seterror(&response->error, "invalid project name");
            /*throw*/ goto _catch_label1;
        }



        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            seterror(&response->error, "user '%s' not found", request->session.email);
            /*throw*/ goto _catch_label1;
        }

        struct Project* pProject = User_FindProjectByName(pUser, projectName);
        if (pProject == NULL)
        {
            seterror(&response->error, "project '%s' not found", projectName);
            /*throw*/ goto _catch_label1;
        }

        //novo plano
        if (planId[0] == '\0')
        {
            struct Plan* pPlan = calloc(1, sizeof * pPlan);
            if (pPlan == NULL)
            {
                seterror(&response->error, "out of mem");
                /*throw*/ goto _catch_label1;
            }
            NewUUID(pPlan->id);
            pPlan->current_planName = NULL;
            pPlan->new_planName = strdup(planname);


            struct error localerror = { 0 };
            if (Plan_SaveToFile(pUser, pProject, pPlan, &localerror) != 0)
            {
                log_error("error saving plan %s", localerror.message);
                /*throw*/ goto _catch_label1;
            }

            if (Plans_Push(&pProject->plans, pPlan /*sink*/) != 0)
            {
                /*throw*/ goto _catch_label1;
            }
        }
        else  //edita plano
        {
            struct Plan* pPlan = Project_FindPlan(pProject, planId);
            if (pPlan == NULL)
            {
                seterror(&response->error, "plan '%s' not found in project %s", planId, projectName);
                /*throw*/ goto _catch_label1;
            }
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void DeleteMobileAppHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    char* responsestr = 0;
    char* response2 = 0;
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char appname[100];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(appname)))
        {
            seterror(&response->error, u8"invalid app name");
            /*throw*/ goto _catch_label1;
        }
        struct http_request http_request;
        http_request.url = "http://localhost:8080/api/login";
        http_request.method = "POST";
        http_request.headers = "X-Csrf-Token:0";
        http_request.content_type = "application/x-www-form-urlencoded";
        http_request.content = "&persistent=false&id=&version=1.5.160&encryption=acddd3306198ba9a108c80c775a705a901e259ee";
        if (http_request_send_sync(&http_request, &responsestr, &response->error) != 0)
            /*throw*/ goto _catch_label1;
        scanner =(struct JsonScanner){
        
            0}
        ;
        scanner.json = responsestr;
        JsonScanner_Match(&scanner);//
        JsonScanner_Match(&scanner);//{
        JsonScanner_Match(&scanner);//""
        JsonScanner_Match(&scanner);//:
        http_request.url = "http://localhost:8080/api/create-app";
        http_request.method = "POST";
        char headers[100] = { 0 };
        snprintf(headers, sizeof headers, "X-Csrf-Token: %.*s", scanner.LexemeByteSize, &scanner.json[scanner.LexemeByteStart]);
        http_request.headers = headers;
        http_request.content_type = "application/x-www-form-urlencoded";
        char content[300] = { 0 };
        snprintf(content, sizeof content, "&app=%s&copyImages=true&template=Empty&run=true", appname);
        http_request.content = content;
        if (http_request_send_sync(&http_request, &response2, &response->error) != 0)
            /*throw*/ goto _catch_label1;
        /*como estamos executando fora do actor do server,
         vamos enviar uma mensagem para salver nele
        */
        struct Capture capture = { 0 };
        capture.arg2.pstring = strdup(appname);
        capture.arg3.pstring = strdup(request->session.email);
        capture.arg1.pHttpResponse = response;
        response = 0; /*MOVED*/
        RunAtHandlerActor(SaveUserDeletedAppTask, &capture);
    }
    else /*catch*/ _catch_label1:
    {
    }
    free(response2);
    free(response);
}

static void DeleteAccountHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            seterror(&response->error, "user not found");
            /*throw*/ goto _catch_label1;
        }


        if (Users_RemoveUserByEmail(&s_cloudServer.users, request->session.email) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
        if (User_DeleteDir(pUser, &response->error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void SaveUserCardHandler_Continuation(enum TASK_ACTION action, struct Capture* pCapture)
{
    /*executado pelo actor*/
    const char* email = pCapture->arg1.pstring;
    const char* json = pCapture->arg2.pstring;
    if (action == TASK_RUN)
    {
        struct error error = { 0 };
        
        if (1) /*try*/
        {
            struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, email);
            if (pUser == NULL)
            {
                seterror(&error, "user not found");
                /*throw*/ goto _catch_label1;
            }

            struct Card card = { 0 };
            NewUUID(card.id);
            if (JsonLoadFromString(json, Card_PagarmeExchange, &card, &error) != 0)
            {
                /*throw*/ goto _catch_label1;
            }

            if (Card_SaveToFile(pUser, &card, &error) != 0)
            {
                /*throw*/ goto _catch_label1;
            }
        }
        else /*catch*/ _catch_label1:
        {
            log_error("error saving user card %s", error.message);
        }
    }
    free((void*)email);
    free((void*)json);
    //free((void*)cardnumber);
}

static void GetCardsHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    struct osstream oss = { 0 };
    
    if (1) /*try*/
    {
        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            /*considerando que ele tem sessao...deveria existir*/
            seterror(&response->error, "internal error");
            /*throw*/ goto _catch_label1;
        }

        ss_printf(&oss, "[");
        for (int i = 0; i < pUser->cards.size; i++)
        {
            char* json = JsonSaveToString(Card_Exchange, pUser->cards.data[i]);
            if (json)
            {
                if (i > 0)
                {
                    ss_printf(&oss, ",");
                }

                ss_printf(&oss, "%s", json);
                free(json);
            }
            else
            {
                seterror(&response->error, "unexpected");
                /*throw*/ goto _catch_label1;
            }
        }
        ss_printf(&oss, "]");
        response->Content = oss.c_str; /*MOVED*/
        oss.c_str = NULL;
    }
    else /*catch*/ _catch_label1:
    {
    }

    if (oss.c_str)
    {
        /*caso nao tenha sido movido*/
        free(oss.c_str);
    }
}

static void SaveUserCardHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    struct JsonScanner scanner = { .json=  request->Content };
    
    if (1) /*try*/
    {
        char cardnumber[100];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(cardnumber)) != 0)
        {
            seterror(&response->error, "invalid card number");
            /*throw*/ goto _catch_label1;
        }
        char cardcvv[5];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(cardcvv)) != 0)
        {
            seterror(&response->error, "invalid card cvv");
            /*throw*/ goto _catch_label1;
        }
        char expirationdate[100];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(expirationdate)) != 0)
        {
            seterror(&response->error, "invalid card expiration date");
            /*throw*/ goto _catch_label1;
        }
        char cardownername[100];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(cardownername)) != 0)
        {
            seterror(&response->error, "invalid card owner");
            /*throw*/ goto _catch_label1;
        }
        char* json = 0;
        if (PagarMeAddCardSync(cardnumber, cardcvv, expirationdate, cardownername, &json, &response->error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
        /*como estamos executando fora do actor do server,
          vamos enviar uma mensagem para salver nele
        */
        struct Capture capture = { 0 };
        capture.arg1.pstring = strdup(request->session.email);
        capture.arg2.pstring = json;
        RunAtHandlerActor(SaveUserCardHandler_Continuation, &capture);
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void SaveUserHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            seterror(&response->error, "user not found - unexpected");
            /*throw*/ goto _catch_label1;
        }
        if (User_SetJson(pUser, request->Content, &response->error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }

        if (User_SaveToFile(pUser, &response->error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void GetUserHandler(struct HttpRequest* request, struct HttpResponse* response)
{

    

    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };

        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)) != 0)
        {
            seterror(&response->error, "invalid email");
            /*throw*/ goto _catch_label1;
        }

        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, request->session.email);
        if (pUser == NULL)
        {
            /*considerando que ele tem sessao...deveria existir*/
            seterror(&response->error, "user not found - unexpected");
            /*throw*/ goto _catch_label1;
        }

        char* json = User_SaveToJson(pUser);
        if (json == NULL)
        {
            seterror(&response->error, "internal error generating user json");
            /*throw*/ goto _catch_label1;
        }
        response->Content = json;
        response->ResponseContentType = RESPONSE_CONTENT_JSON;
        json = 0; /*moved*/
        free(json);
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void ResetPasswordWithTokenHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    struct JsonScanner scanner = { .json=  request->Content };
    
    if (1) /*try*/
    {
        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)) != 0)
        {
            seterror(&response->error, "invalid email");
            /*throw*/ goto _catch_label1;
        }
        char newpassword[MAX_PASSWORD_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(newpassword)) != 0 ||
            !IsValidPassword(newpassword))
        {
            seterror(&response->error, "invalid new password");
            /*throw*/ goto _catch_label1;
        }
        char token[UUID_STRING_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(token)) != 0)
        {
            seterror(&response->error, "invalid token");
            /*throw*/ goto _catch_label1;
        }
        struct User* pUser = Users_FindUserByEmail(&s_cloudServer.users, email);
        if (pUser == NULL)
        {
            seterror(&response->error, "User not found");
            /*throw*/ goto _catch_label1;
        }
        if (strcmp(pUser->activationtoken, token) != 0)
        {
            seterror(&response->error, "incorrect token");
            /*throw*/ goto _catch_label1;
        }
        /*Verificar validade*/
        User_SetHash(pUser, email, newpassword);
        User_SaveToFile(pUser, &response->error);
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void ChangePasswordHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)) != 0 ||
            !IsValidEmail(email))
        {
            seterror(&response->error, MESSAGE(request->AcceptLanguage, IDS_INVALID_EMAIL));
            /*throw*/ goto _catch_label1;
        }

        char oldpassword[MAX_PASSWORD_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(oldpassword)) != 0)
        {
            seterror(&response->error, MESSAGE(request->AcceptLanguage, IDS_INVALID_PASSWORD));
            /*throw*/ goto _catch_label1;
        }

        char newpassword[MAX_PASSWORD_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(newpassword)) != 0 ||
            !IsValidPassword(newpassword))
        {
            seterror(&response->error, MESSAGE(request->AcceptLanguage, IDS_INVALID_PASSWORD));
            /*throw*/ goto _catch_label1;
        }
        /*vamos usar email da sessao para garantia de so trocar dele mesmo*/
        char hashstring[PASSWORD_HASH_STRING_SIZE];
        MakeUserPasswordHash(request->session.email, oldpassword, hashstring);

        struct User* pUser = Users_FindUserByPasswordHash(&s_cloudServer.users, hashstring);
        if (pUser == NULL)
        {
            seterror(&response->error, "old password is invalid");
            /*throw*/ goto _catch_label1;
        }
        User_SetHash(pUser, email, newpassword);
        if (User_SaveToFile(pUser, &response->error) != 0)
        {
            log_error("unexpected %s %d", __func__, __LINE__);
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void ActivateAccountHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };
        char token[UUID_STRING_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(token)) != 0)
        {
            seterror(&response->error, "invalid token");
            /*throw*/ goto _catch_label1;
        }
        bool bActivated = false;
        for (int i = 0; i < s_cloudServer.users.size; i++)
        {
            struct User* pUser = s_cloudServer.users.data[i];
            if (strcmp(pUser->activationtoken, token) == 0)
            {
                pUser->bEmailConfirmed = true;
                pUser->activationtoken[0] = 0;
                User_SaveToFile(pUser, &response->error);
                char sessiontoken[UUID_STRING_SIZE];
                CreateSession(pUser->Email, sessiontoken);
                char json[500];
                if (json_snprintf(json, sizeof json, "{ 'email' :'%s', 'token' : '%s', 'isAdmin' : false }", pUser->Email, sessiontoken) >= (int)sizeof(json))
                {
                    seterror(&response->error, "internal error.");
                    /*throw*/ goto _catch_label1;
                }
                bActivated = true;
                break;
            }
        }
        if (!bActivated)
        {
            seterror(&response->error, "Invalid token");
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void LogoutHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    DeleteSession(request->session.session_token);
    clearerror(&response->error);
}

static void LoginHandler(struct HttpRequest* request, struct HttpResponse* response)
{
    
    if (1) /*try*/
    {
        struct JsonScanner scanner = { .json=  request->Content };

        char email[MAX_EMAIL_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(email)) != 0)
        {
            seterror(&response->error, "invalid email");
            /*throw*/ goto _catch_label1;
        }

        char password[MAX_PASSWORD_SIZE];
        if (ReadStringFromJsonArray(&scanner, ARRAY_AND_SIZE(password)) != 0)
        {
            seterror(&response->error, "invalid password");
            /*throw*/ goto _catch_label1;
        }

        char hashstring[PASSWORD_HASH_STRING_SIZE];
        MakeUserPasswordHash(email, password, hashstring);
        struct User* pUserFound = Users_FindUserByPasswordHash(&s_cloudServer.users, hashstring);
        if (pUserFound == NULL)
        {
            seterror(&response->error, MESSAGE(request->AcceptLanguage, IDS_INVALID_MAIL_OR_PASSWORD));
            /*throw*/ goto _catch_label1;
        }

        char sessiontoken[UUID_STRING_SIZE];
        if (CreateSession(pUserFound->Email, sessiontoken) != 0)
        {
            seterror(&response->error, "Internal error, creating session.");
            /*throw*/ goto _catch_label1;
        }

        assert(response->Content == NULL);
        response->Content = malloc(sizeof(char) * 200);
        if (response->Content == NULL)
        {
            seterror(&response->error, "out of mem");
            /*throw*/ goto _catch_label1;
        }

        response->ResponseContentType = RESPONSE_CONTENT_JSON;
        if (json_snprintf(response->Content, 200, "{ 'token' : '%s', 'isAdmin': false }", sessiontoken) >= 200)
        {
            free(response->Content);
            response->Content = 0;
            seterror(&response->error, "Internal error, insuficient space.");
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
}

static void ExchangeConfig(struct Data* data, void* p)
{
    p;//lint !e523 !e522
    data->ExchangeStaticText(data, "port", s_cloudServer.httpConfig.port, sizeof s_cloudServer.httpConfig.port);
    data->ExchangeStaticText(data, "smtp_server", s_cloudServer.smtp.server, sizeof s_cloudServer.smtp.server);
    data->ExchangeStaticText(data, "smtp_port", s_cloudServer.smtp.port, sizeof s_cloudServer.smtp.port);
    data->ExchangeStaticText(data, "smtp_authentication_method", s_cloudServer.smtp.authentication_method, sizeof s_cloudServer.smtp.authentication_method);
    data->ExchangeStaticText(data, "smtp_connection_security", s_cloudServer.smtp.connection_security, sizeof s_cloudServer.smtp.connection_security);
    data->ExchangeStaticText(data, "smtp_user", s_cloudServer.smtp.user, sizeof s_cloudServer.smtp.user);
    data->ExchangeStaticText(data, "smtp_password", s_cloudServer.smtp.password, sizeof s_cloudServer.smtp.password);
    /*
    objetivo aqui é ler uma vez na vida e ser threadsafe (ready only)
    */
    data->ExchangeStaticText(data, "pagarme_key", s_GlobalPagarmeAPIKey, sizeof s_GlobalPagarmeAPIKey);
}

int LoadConfig(struct error* error)
{
    
    if (1) /*try*/
    {
        char path[ELIPSE_CLOUD_MAX_PATH];
        if (string_join(path, sizeof(path), s_RootPath, "config.json") < 0)
        {
            seterror(error, "buffer too small join load config");
            /*throw*/ goto _catch_label1;
        }

        struct error localerror = { 0 };
        if (JsonLoadFromFile(path, ExchangeConfig, NULL, &localerror) != 0)
        {
            seterror(error, "Cannot read config.json '%s' : %s", path, localerror.message);
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
    }

    return error->code;
}

int CloudServer_Init(const char* rootPath, struct error* error)
{
    memset(&s_cloudServer, 0, sizeof(s_cloudServer));

    

    if (1) /*try*/
    {
        if (string_copy(s_RootPath, sizeof s_RootPath, rootPath) != 0)
        {
            seterror(error, "rootpath too big");
            /*throw*/ goto _catch_label1;
        }

        static struct Entry entries[] =
        {
            {"/api/login",                   LoginHandler, .bSessionTokenNotRequired=  true},
            {"/api/forgotpassword",          ForgotPasswordHandler, .bSessionTokenNotRequired=  true},
            {"/api/create-account",          CreateAccountHandler, .bSessionTokenNotRequired=  true},
            {"/api/activate-account",        ActivateAccountHandler, .bSessionTokenNotRequired=  true},
            {"/api/reset-password",          ResetPasswordWithTokenHandler, .bSessionTokenNotRequired=  true},
            /*
              Todas que não são publicas exigem(automaticamente)um token de login
            */
            {"/api/logout",                    LogoutHandler},
            {"/api/change-password",           ChangePasswordHandler},
            {"/api/resend-email-verification", ResendVerificationHandler},
            {"/api/get-user",                  GetUserHandler},
            {"/api/save-user",                 SaveUserHandler},
            {"/api/save-card",                 SaveUserCardHandler, .bSpareThread=  1},
            {"/api/get-cards",                 GetCardsHandler},
            {"/api/delete-account",            DeleteAccountHandler},
            {"/api/create-mobile-app",         CreateMobileAppHandler, .bSpareThread=  1},
            {"/api/delete-mobile-app",         DeleteMobileAppHandler, .bSpareThread=  1},
            {"/api/get-plans",                 GetPlansHandler},
            {"/api/edit-mobile-plan",          AddEditMobilePlanHandler},
            {"/api/create-project",            CreateProjectHandler},
            {"/api/get-projects",              GetProjectsHandler}
        };

        if (LoadConfig(error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }

        if (Users_LoadFromDir(&s_cloudServer.users, error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }

        s_cloudServer.httpConfig.handler = HttpDefaultHandler;
        s_cloudServer.httpConfig.security = SECURITY_VERSION_NONE;
        s_cloudServer.httpConfig.routes = entries;
        s_cloudServer.httpConfig.routesize = sizeof(entries) / sizeof(entries[0]);

        char htmlRootPath[500];
        if (string_join(htmlRootPath, sizeof htmlRootPath, s_RootPath, "html/") <= 0)
        {
            /*throw*/ goto _catch_label1;
        }

        if (HttpInit(&s_cloudServer.httpConfig, htmlRootPath, error) != 0)
        {
            /*throw*/ goto _catch_label1;
        }
    }
    else /*catch*/ _catch_label1:
    {
    }
    return error->code;
}

void CloudServer_Destroy()
{
    Users_Destroy(&s_cloudServer.users);
    HttpDestroy();
}
