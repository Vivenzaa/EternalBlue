#ifndef utils_H
#define utils_H

typedef struct ffdata
{
    char *videopath;
    int *fifo;
}ffdata;

void itos(int N, char *str);
void log4c(char *base, ...);
char get_utc_offset();
long SizeOfFile(char *path);
char *curl_filename(char *ptr);
int getGame(char *title, char ***wordlist);
char *get_metadata(char *filename);
void write_metadata(char * restrict filename, char * restrict toWrite);
char **getAllFiles(char *local_path);
char **file_lines(char *filename);
unsigned int chooseVideo(unsigned int x, int seed);       // x is the len of file list, NOT the last index of filelist
unsigned int size_of_double_array(char **array);
long convert_to_timestamp(char *datetime);
void *cmdRunInThread(void *str);
int get_video_duration(char * restrict video, char * restrict local_path);
int get_undownloaded_videos(char * restrict local_path, char * restrict google_api_key);
void recur_free(char **tab);
void recur_tabcpy(char **dest, char **src, int size);

#endif