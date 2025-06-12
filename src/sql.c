#include "../include/sql.h"
#include "../include/utils.h"






sqlite3 *init_db(char *db_path)
{
    char justCreatedDb = !fileExists(db_path);
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);

    if (rc) 
    {
        log4c(2, "Can't open database: %s", sqlite3_errmsg(db));
        return NULL;
    } 
    else 
    {
        log4c(0, "successfully opened SQL table");
        if (justCreatedDb)
        {
            char req[] = "CREATE TABLE videos(uid INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, date TEXT, yt_id TEXT, game TEXT, localpath TEXT, duration INTEGER)";
            char *err_msg = NULL;
            int res = sqlite3_exec(db, req, 0, 0, &err_msg);
            if (res != SQLITE_OK) 
            {
                log4c(2, "SQL error: %s", err_msg);
                sqlite3_free(err_msg);
            } 
            else 
                log4c(0, "successfully created SQL table");
        }
        return db;
    }
}


int sql_callback_monores(void *res, int argc __attribute__((unused)), char **argv, char **azColName __attribute__((unused))) {
    char **added = (char **)res;
    *added = realloc(*added, strlen(argv[0]) + 1);
    strcpy(*added, argv[0]);

    return 0;
}


int sql_callback_doubleres(void *res, int argc __attribute__((unused)), char **argv, char **azColName __attribute__((unused))) {
    char ***added = (char ***)res;
    int added_size = 0;
    
    if (*added)
        while((*added)[added_size])
            added_size++;

    *added = realloc(*added, (added_size+2) * sizeof(char *));
    (*added)[added_size+1] = NULL;
    if(argv[0])
    {
        (*added)[added_size] = malloc(sizeof(char) * (strlen(argv[0]) + 1));
        strcpy((*added)[added_size], argv[0]);
    }
    else
        (*added)[added_size] = NULL;
    
    (*added)[added_size+1] = NULL;

    return 0;
}


int sql_callback_tridires(void *res, int argc, char **argv, char **azColName __attribute__((unused))) {
    char ****added = (char ****)res;
    int added_size = 0;
    
    if (*added)
        while((*added)[added_size])
            added_size++;

    *added = realloc(*added, (added_size+2) * sizeof(char **));
    (*added)[added_size+1] = NULL;
    (*added)[added_size] = malloc(sizeof(char *) * (argc+1));
    for (int i = 0; i < argc; i++)
    {
        if (argv[i])
        {
            (*added)[added_size][i] = malloc(strlen(argv[i]) + 1);
            strcpy((*added)[added_size][i], argv[i]);
        }
        else
        {
            (*added)[added_size][i] = malloc(5);
            strcpy((*added)[added_size][i], "null");
        }  
    }
    (*added)[added_size][argc] = NULL;
    return 0;
}


int sql_callback_multires(void *res, int argc, char **argv, char **azColName __attribute__((unused))) {
    video_db **added = (video_db **)res;
    int added_size = 0;
    
    if (*added)
        while((*added)[added_size].uid)
            added_size++;

    *added = realloc(*added, (added_size+2) * sizeof(video_db));
    (*added)[added_size+1].uid = 0;
    if(argv[0] && argc == 7)
    {
        (*added)[added_size].uid = atoi(argv[0]);
        (*added)[added_size].duration = atoi(argv[6]);
        (*added)[added_size].title = strdup(argv[1]);
        strcpy((*added)[added_size].date, argv[2]);
        strcpy((*added)[added_size].date, argv[3]);
        if (argv[4])    (*added)[added_size].game = strdup(argv[4]);
            else        (*added)[added_size].game = NULL;
        
        if(argv[5])     (*added)[added_size].localpath = strdup(argv[5]);
            else        (*added)[added_size].localpath = NULL;
    }

    return 0;
}