#ifndef SQL_H
#define SQL_H

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    int uid;
    char *title;
    char date[21];
    char yt_id[12];
    char *game;
    char *localpath;
    int duration;
} video_db;


sqlite3 *init_db(char *db_path);
int sql_callback_monores(void *res, int argc __attribute__((unused)), char **argv, char **azColName __attribute__((unused)));
int sql_callback_doubleres(void *res, int argc __attribute__((unused)), char **argv, char **azColName __attribute__((unused)));
int sql_callback_tridires(void *res, int argc, char **argv, char **azColName __attribute__((unused)));
int sql_callback_multires(void *res, int argc, char **argv, char **azColName __attribute__((unused)));

#endif