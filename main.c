#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>

#define STREAM_KEY "your twitch channel stream key"
#define REMOTE_IP "127.0.0.1"
#define REMOTE_NAME "your remote server username"
#define REMOTE_PATH_START "im gonna change that, that sucks to port my code"
#define SSH_PATH "/home/local_username/.ssh/id_rsa"
#define LOCAL_PATH "videos/"
#define SEED 721077     // funny magic




void* downloadFromServer(void *arg)
{
    setlocale(LC_ALL, "");
    char command[255];
    char *remote_path = (char*)arg;

    snprintf(command, sizeof(command), "scp -i %s \"%s@%s:%s%s\" %s", SSH_PATH, REMOTE_NAME, REMOTE_IP, REMOTE_PATH_START, remote_path, LOCAL_PATH);

    printf("executing %s\n", command);
    system(command);
    pthread_exit(NULL);
}


char **getAllFiles()
{
    FILE *fichier = fopen("files.txt", "r");
    char **lines = NULL;
    char buffer[256];
    int count = 0;

    while (fgets(buffer, 256, fichier)) {
        char *line = malloc(strlen(buffer) + 1);
        if (!line) {
            fprintf(stderr, "Erreur : allocation mémoire échouée\n");
            exit(EXIT_FAILURE);
        }
        strcpy(line, buffer);
        line[strlen(line) - 1] = '\0';

        char **temp = realloc(lines, sizeof(char *) * (count + 1));
        if (!temp) {
            fprintf(stderr, "Erreur : allocation mémoire échouée\n");
            exit(EXIT_FAILURE);
        }
        lines = temp;

        lines[count++] = line;
    }
    
    char **temp = realloc(lines, sizeof(char *) * (count + 1));
    if (!temp) {
        fprintf(stderr, "Erreur : allocation mémoire échouée\n");
        exit(EXIT_FAILURE);
    }
    lines = temp;
    lines[count++] = NULL;

    return lines;
}


unsigned short chooseVideo(unsigned short x)       // x is the len of file list, NOT the last index of filelist
{
    unsigned short tmp = (unsigned int)time(NULL) * SEED * 0x5f3759df;   // what the fuck?
    return tmp % x;   
}


unsigned int size_of_double_array(char **array) {
    unsigned int size = 0;
    while (array[size] != NULL) {
        size++;
    }
    return size;
}


int get_bitrate(const char *filename) {
    char command[512];
    char temp_file[] = "ffprobe_output.txt";
    FILE *fp;
    double bitrate = -1;

    snprintf(command, sizeof(command),
             "ffprobe -v error -select_streams v:0 -show_entries format=bit_rate -of default=noprint_wrappers=1:nokey=1 \"%s\" > %s",
             filename, temp_file);

    if (system(command) != 0) {
        fprintf(stderr, "Erreur lors de l'exécution de ffprobe.\n");
        return -1;
    }

    fp = fopen(temp_file, "r");
    if (fp == NULL) {
        perror("Erreur lors de l'ouverture du fichier temporaire");
        return -1;
    }

    if (fscanf(fp, "%lf", &bitrate) != 1) {
        fprintf(stderr, "Impossible de lire le bitrate depuis ffprobe.\n");
        bitrate = -1;
    }

    fclose(fp);
    remove(temp_file);

    return (int)bitrate;
}


int main(void)
{
    setlocale(LC_ALL, "");
    char **playlist = getAllFiles();

    char *video = playlist[chooseVideo(size_of_double_array(playlist))];
    char *current_video = video;
    char cmd[512];
    char tmp[256];
    pthread_t downloadThrId;
    pthread_create(&downloadThrId, NULL, downloadFromServer, (void*)video);
    pthread_join(downloadThrId, NULL);
    
    while(1)
    {
        current_video = video;
        printf("Now Streaming %s\n", current_video);
        snprintf(cmd, sizeof(cmd), 
            "ffmpeg "
            "-re "
            "-hwaccel_output_format qsv "
            "-i \"%s\" "
            "-c:v h264_qsv "
            "-preset veryslow "
            "-b:v %d -maxrate %d "
            "-bufsize 9000k "
            "-c:a aac "
            "-b:a 128k "
            "-f flv "
            "rtmp://live.twitch.tv/app/%s",
            current_video, get_bitrate(current_video), get_bitrate(current_video), STREAM_KEY);
        

        
        while (video == current_video)
            video = playlist[chooseVideo(size_of_double_array(playlist))];
        pthread_create(&downloadThrId, NULL, downloadFromServer, (void*)video);
        snprintf(tmp, sizeof(cmd), "python3 handle_name_changing.py -u \"%s\" -r", current_video);
        system(tmp);
        system(cmd);

        remove(current_video);
    }
    
    return 0;
}