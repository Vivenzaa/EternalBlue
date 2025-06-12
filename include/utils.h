#ifndef utils_H
#define utils_H

typedef struct ffdata
{
    char *videopath;
    int *fifo;
}ffdata;

typedef struct {
    char major;
    char minor;
    char patch;
} ver_t;

extern const ver_t version;
extern char logDepth;

char get_utc_offset();
char fileExists(char *path);
char *curl_filename(char *ptr);
char **file_lines(char *filename);
int askSave_env_infos(char *filepath);
int get_env_infos(char * restrict filepath, char * restrict stream_key, char * restrict bot_id, char * restrict bot_secret, char * restrict refresh_token, char * restrict gapi_key, char * restrict lpath, char * restrict channel);
int get_undownloaded_videos(char *google_api_key);
int get_video_duration(char * restrict video);
int getGame(char *title, char ***wordlist);
long convert_to_timestamp(char *datetime);
long SizeOfFile(char *path);
unsigned int chooseVideo(unsigned int x, int seed);       // x is the len of file list, NOT the last index of filelist
unsigned int size_of_double_array(char **array);
void get_env_filepath(char *buffer, unsigned int size);
void itos(int N, char *str);
void log4c(char logtype, char *base, ...);
void recur_free(char **tab);
void recur_tabcpy(char **dest, char **src, int size);
void *cmdRunInThread(void *str);
char recurFreeN(void *array, int n);     // where n is the array's depth


#endif