#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cjson/cJSON.h>
#include <dirent.h>

#define STREAM_KEY "your twitch channel stream key"
#define REMOTE_IP "127.0.0.1"
#define REMOTE_NAME "your remote server username"
#define REMOTE_PATH_START "where are your videos stored"
#define SSH_PATH "/home/local_username/.ssh/id_rsa"
#define LOCAL_PATH "videos/"
#define SEED 721077     // what could that number be




char **getAllFiles()
{
    struct dirent *de;
    DIR *dr = opendir(LOCAL_PATH);
    
    char **lines = NULL;
    int count = 0;

    while ((de = readdir(dr)) != NULL) {
        //printf("de->d_name = %s, %d\n", de->d_name, (int)strlen(de->d_name));
        if (strlen(de->d_name) <= 3)     continue;
        char *line = malloc(strlen(de->d_name) + 1);
        if (!line) {
            fprintf(stderr, "Erreur : allocation mémoire échouée\n");
            exit(EXIT_FAILURE);
        }
        strcpy(line, de->d_name);

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


    closedir(dr);
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



long convert_to_timestamp(char *datetime) {     // converts YYYY-MM-DDThh:mm:ss.000Z to timestamp
    struct tm tm = {0};
    char temp[20];

    strncpy(temp, datetime, 19);
    temp[19] = '\0';

    sscanf(temp, "%d-%d-%dT%d:%d:%d",
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;

    time_t timestamp = timegm(&tm);
    if (timestamp == -1) {
        fprintf(stderr, "Erreur : Conversion en timestamp échouée.\n");
        return -1;
    }

    return (long)timestamp;
}



void *cmdRunInThread(void *str)
{
    const char *cmd = (const char *)str;
    system(cmd);
    return NULL;
}


void *handleAPI()
{
    system("curl https://api2.kametotv.fr/karmine/events -o api.json");

    FILE *fichier = fopen("api.json", "r");
    char tmp[16384];
    fread(tmp, 1, sizeof(tmp), fichier); 
    cJSON *json = cJSON_Parse(tmp); 
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            printf("Error: %s\n", error_ptr); 
        } 
        cJSON_Delete(json); 
        return NULL; 
    }
    cJSON *upcomming = cJSON_GetArrayItem(json, 0);

    cJSON *opponent = cJSON_GetObjectItemCaseSensitive(upcomming, "team_name_exterieur");
    cJSON *game = cJSON_GetObjectItemCaseSensitive(upcomming, "competition_name");
    cJSON *start = cJSON_GetObjectItemCaseSensitive(upcomming, "start");
    cJSON *end = cJSON_GetObjectItemCaseSensitive(upcomming, "end");
    cJSON *channel = cJSON_GetObjectItemCaseSensitive(upcomming, "streamLink");
    
    fichier = fopen("next.data", "w");
    fprintf(fichier, "%s\n%s\n%s\n%s\n%s\n", opponent->valuestring, game->valuestring, start->valuestring, end->valuestring, channel->valuestring);
    fclose(fichier);
    cJSON_Delete(json);

    return NULL;
}


int main(void) {
    restart:
    setlocale(LC_ALL, "fr_FR.UTF-8");

    char **playlist = getAllFiles();                                             
    char *video = playlist[chooseVideo(size_of_double_array(playlist))];
    unlink("video_fifo");
    mkfifo("video_fifo", 0666);

    char ffmpegCmd[1024];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
        "ffmpeg -loglevel debug -re -i video_fifo "
        "-c:v copy -bufsize 18000k "
        "-c:a copy -f flv rtmp://live.twitch.tv/app/%s > logs.txt 2>&1",
        STREAM_KEY);
    

    pthread_t ffmpegThreadId;
    pthread_create(&ffmpegThreadId, NULL, cmdRunInThread, (void *)ffmpegCmd);
    handleAPI();
    
    char tmp[256];

    FILE *fichier = fopen("next.data", "r");
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);

    long timeleft = convert_to_timestamp(tmp);
    long timestart = (long)time(NULL);
    char *nextVideo = video;

    int fifo_fd = open("video_fifo", O_WRONLY);

    while ((long)time(NULL) - timestart < 18000) {
        printf("%s\n", video);
        
        char cmdFIFO[1024];
        if (video != NULL)
            snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -re -i \"%s%s\" -c copy -f mpegts -", LOCAL_PATH, video);
        else 
            snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -re -i placeholder.mp4 -c copy -f mpegts -");

        while (video == nextVideo)
            nextVideo = playlist[chooseVideo(size_of_double_array(playlist))];
        snprintf(tmp, sizeof(tmp), "python3 handle_twitchAPI.py -u \"%s\" -re", video);
        system(tmp);

        FILE *ffmpeg = popen(cmdFIFO, "r");
        char buffer[4096];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), ffmpeg)) > 0) {
            write(fifo_fd, buffer, bytesRead);
        }
        pclose(ffmpeg);

        
        video = nextVideo;


        if (timeleft - (long)time(NULL) < 600)
        {
            printf("next match will be starting soon, closing stream...\n");
            wait_again:
            char ending[26];
            char next[26];
            char streamer[64];
            sleep(timeleft - (long)time(NULL));                             // sleep until match start
            fgets(ending, sizeof(ending), fichier);                         // gets timestamp of match ending
            timeleft = convert_to_timestamp(ending);
            fgets(streamer, sizeof(streamer), fichier);                     // gets streamer to raid
            fclose(fichier);                                                // end of file is reached, closing file
            snprintf(tmp, sizeof(tmp), "python3 handle_twitchAPI.py -ra %s", streamer);       //stream isn't stopped yet
            system(tmp);                                               // raid streamer
            printf("closing stream\n");                                     // close stream
            system("echo q > ./video_fifo");

            sleep(timeleft - (long)time(NULL));                             // sleep until match ending
            handleAPI();                                                    // refresh api data
            fichier = fopen("next.data", "r");
            fgets(tmp, sizeof(tmp), fichier);
            fgets(tmp, sizeof(tmp), fichier);
            fgets(next, sizeof(next), fichier);                             // gets timestamp of next match
            timeleft = convert_to_timestamp(next);
            close(fifo_fd);
            if (timeleft - (long)time(NULL) < 3600)  goto wait_again;       // if next match starts in less than an hour, wait again
            else                                     goto restart;          // else, restart the stream
        }
    }

    printf("Twitch limit of 48h is almost reached, closing stream\n");
    system("echo q > ./video_fifo");
    printf("resetting stream...\n");
    close(fifo_fd);
    goto restart;

    return 0;
}
