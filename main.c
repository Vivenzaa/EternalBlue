#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cjson/cJSON.h>

#define STREAM_KEY "your twitch channel stream key"
#define REMOTE_IP "127.0.0.1"
#define REMOTE_NAME "your remote server username"
#define REMOTE_PATH_START "where are your videos stored"
#define SSH_PATH "/home/local_username/.ssh/id_rsa"
#define LOCAL_PATH "videos/"
#define SEED 721077     // what could that number be



char **getAllUrls()
{
    printf("fetching all videos...\n");
    system("yt-dlp --flat-playlist \"https://www.youtube.com/@KarmineCorpVOD/videos\" --print \"%%(id)s\" > recentVids.txt");

    FILE *fichier = fopen("recentVids.txt", "r");
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
    fclose(fichier);
    remove("recentVids.txt");
    
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


void *downloadVideo(void *vUrl)
{

    char *url = (char *)vUrl;
    char tmp[strlen(url) + strlen("yt-dlp --cookies ./cooking.txt -f 299 ") + 1];
    strcpy(tmp, "yt-dlp --cookies ./cooking.txt -f 299 ");
    strcat(tmp, url);
    system(tmp);

    snprintf(tmp, sizeof(tmp), "yt-dlp --cookies ./cooking.txt -f 299 --print filename %s > %s", url, url);
    system(tmp);

    FILE *fichier = fopen(url, "r");
    fgets(tmp, sizeof(tmp), fichier);
    char *oldFilename = malloc(sizeof(char *) * (strlen(tmp) + 1));
    strcpy(oldFilename, tmp);
    oldFilename[strlen(oldFilename) - 1] = '\0';
    tmp[strlen(tmp) - strlen(url) - 8] = '\0';
    strcat(tmp, ".mp4");
    rename(oldFilename, tmp);
    printf("%s\t%s\n", oldFilename, tmp);

    char moov[strlen(tmp) + strlen(LOCAL_PATH) + 7];
    snprintf(moov, sizeof(moov), "mv \"%s\" %s", tmp, LOCAL_PATH);
    system(moov);
    remove(url);

    return 0;
}


char *getvideoPath(char *url)
{
    char tmp[strlen("yt-dlp --cookies ./cooking.txt -f 299 --print filename  > ") + (2 * strlen(url)) + 1];
    snprintf(tmp, sizeof(tmp), "yt-dlp --cookies ./cooking.txt -f 299 --print filename %s > %s", url, url);
    system(tmp);

    FILE *fichier = fopen(url, "r");
    fgets(tmp, sizeof(tmp), fichier);
    tmp[strlen(tmp) - strlen(url) - 8] = '\0';
    strcat(tmp, ".mp4");
    char *toReturn = malloc(sizeof(char *) * strlen(tmp));
    strcpy(toReturn, tmp);

    return toReturn;
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


void put_spaces(char *str) {

   size_t length = strlen(str);
    size_t new_length = 0;
    char buffer[64] = {0};

    for (size_t i = 0; i < length; i++) {
        if (i > 0 && isupper(str[i]) && 
            (!isupper(str[i - 1]) || isspace(str[i - 1]))) {
            buffer[new_length++] = ' ';
        }

        buffer[new_length++] = str[i];
    }

    buffer[new_length] = '\0';
    strcpy(str, buffer);

}


void *cmdRunInThread(void *str)
{
    const char *cmd = (const char *)str;
    system(cmd);
    return NULL;
}


void* ffmpegTimer(void *)
{
    setlocale(LC_ALL, "fr_FR.UTF-8");
    FILE *fichier = fopen("next.data", "r");
    char tmp[64];
    char opponent[32];
    char game[32];
    fgets(opponent, sizeof(opponent), fichier);
    fgets(game, sizeof(game), fichier);
    fgets(tmp, sizeof(tmp), fichier);
    fclose(fichier);

    put_spaces(game);                       // formatting
    opponent[strlen(opponent) - 1] = '\0';  // removing the
    game[strlen(game) - 1] = '\0';          // \n character

    long tmpstmp = convert_to_timestamp(tmp);
    fichier = fopen("tmp.time.tmp", "w");
    long hours = (tmpstmp - (long)time(NULL)) / 3600;
    long minutes = ((tmpstmp - (long)time(NULL)) % 3600) / 60;
    long seconds = (tmpstmp - (long)time(NULL)) % 60;
    fprintf(fichier, "%ld:%ld:%ld\n", hours, minutes, seconds);
    fclose(fichier);
    rename("tmp.time.tmp", "tmp.time");
    while (1)
    {
        hours =     (tmpstmp - (long)time(NULL)) / 3600;
        minutes =  ((tmpstmp - (long)time(NULL)) % 3600) / 60;
        seconds =   (tmpstmp - (long)time(NULL)) % 60;
        
        fichier = fopen("tmp.time.tmp", "w");
        fprintf(fichier, "Prochaine victoire contre %s sur %s dans \n%02ld:%02ld:%02ld", opponent, game, hours, minutes, seconds);
        fflush(fichier);
        fclose(fichier);
        rename("tmp.time.tmp", "tmp.time");

        sleep(1);
    }
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


int get_video_duration(char *video)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ffprobe -show_entries format=duration \"./%s%s\" > drtn.tmp", LOCAL_PATH, video);
    system(cmd);

    FILE *fichier = fopen("drtn.tmp", "r");
    fgets(cmd, sizeof(cmd), fichier);
    fgets(cmd, sizeof(cmd), fichier);
    char duration[6];

    int i = 9;
    while(cmd[i] != '.')
    {   
        duration[i-9] = cmd[i];
        i++;
    }
    duration[6] = '\0';

    fclose(fichier);
    remove("drtn.tmp");

    return atoi(duration);
}


int main(void) {
    restart:
    setlocale(LC_ALL, "fr_FR.UTF-8");

    char **playlist = getAllUrls();
    char *videoUrl = playlist[chooseVideo(size_of_double_array(playlist))];
    pthread_t downloadThreadId;
    pthread_create(&downloadThreadId, NULL, downloadVideo, (void *)videoUrl);
    pthread_join(downloadThreadId, NULL);
    unlink("video_fifo");
    mkfifo("video_fifo", 0666);

    char ffmpegCmd[1024];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
        "ffmpeg -loglevel debug -re -i video_fifo "
        "-vf \"drawtext=textfile=./tmp.time:reload=60:x=0:y=1040:fontfile=/usr/share/fonts/truetype/freefont/FreeSansBold.ttf:fontsize=20:fontcolor=white\" "
        "-c:v h264_qsv -preset veryslow -global_quality 1 "
        "-b:v 5000k -maxrate 6100k -bufsize 18000k "
        "-c:a copy -f flv rtmp://live.twitch.tv/app/%s > logs.txt 2>&1",
        STREAM_KEY);
    

    pthread_t ffmpegThreadId;
    pthread_t timerThdId;
    pthread_create(&ffmpegThreadId, NULL, cmdRunInThread, (void *)ffmpegCmd);
    pthread_create(&timerThdId, NULL, ffmpegTimer, NULL);
    handleAPI();
    
    char tmp[256];

    FILE *fichier = fopen("next.data", "r");
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);

    long timeleft = convert_to_timestamp(tmp);
    long timestart = (long)time(NULL);
    char *nextVideoUrl = videoUrl;

    int fifo_fd = open("video_fifo", O_WRONLY);

    while ((long)time(NULL) - timestart < 154000) {
        char *videoPath = getvideoPath(videoUrl);
        char *videoTitle = videoPath;
        videoTitle[strlen(videoTitle) - 4] = '\0';
        printf("%s\n", videoTitle);

        system("python3 handle_twitchAPI.py -ov");
        FILE *monitoring = fopen("mntrdata.tmp", "r");
        char isOnline[9];
        char viewerCount[6];

        fgets(isOnline, sizeof(isOnline), monitoring);
        fgets(viewerCount, sizeof(viewerCount), monitoring);
        isOnline[strlen(isOnline) - 1] = '\0';
        fclose(monitoring);
        remove("mntrdata.tmp");

        monitoring = fopen("monitoring.json", "w");
        cJSON *json = cJSON_CreateObject();

        cJSON_AddStringToObject(json, "status", isOnline);
        cJSON_AddStringToObject(json, "videoTitle", videoTitle);
        cJSON_AddNumberToObject(json, "videoDuration", (double)get_video_duration(videoPath));
        cJSON_AddNumberToObject(json, "videoStartTime", (double)time(NULL));
        cJSON_AddNumberToObject(json, "streamStartTime", (double)timestart);
        cJSON_AddNumberToObject(json, "viewers", (double)atoi(viewerCount));

        char *json_str = cJSON_Print(json);
        fputs(json_str, monitoring);
        fclose(monitoring);
        cJSON_free(json_str);
        cJSON_Delete(json);

        
        char cmdFIFO[1024];
        if (videoPath != NULL)
            snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -re -i \"%s%s\" -c copy -f mpegts -", LOCAL_PATH, videoPath);
        else 
            snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -re -i placeholder.mp4 -c:v libx264 -f mpegts -");

        while (videoUrl == nextVideoUrl)
            nextVideoUrl = playlist[chooseVideo(size_of_double_array(playlist))];
        pthread_create(&downloadThreadId, NULL, downloadVideo, (void *)nextVideoUrl);     // Téléchargement de la prochaine
        snprintf(tmp, sizeof(tmp), "python3 handle_twitchAPI.py -u \"%s\" -re", videoPath);
        system(tmp);

        FILE *ffmpeg = popen(cmdFIFO, "r");
        char buffer[4096];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), ffmpeg)) > 0) {
            write(fifo_fd, buffer, bytesRead);
        }
        pclose(ffmpeg);

        snprintf(tmp, sizeof(tmp), "rm -f \"%s%s\"", LOCAL_PATH, videoUrl);
        system(tmp);
        videoUrl = nextVideoUrl;


        if (timeleft - (long)time(NULL) < 600)
        {
            char *nextVideoPath = nextVideoUrl;
            nextVideoPath[strlen(nextVideoPath) - 4] = '\0';
            snprintf(tmp, sizeof(tmp), "rm -f \"%s%s\"", LOCAL_PATH, nextVideoPath);    // removes video that was supposed to be played
            system(tmp);

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

    char *nextVideoPath = nextVideoUrl;
    nextVideoPath[strlen(nextVideoPath) - 4] = '\0';
    snprintf(tmp, sizeof(tmp), "rm -f \"%s%s\"", LOCAL_PATH, nextVideoPath);
    system(tmp);
    printf("Twitch limit of 48h is almost reached, closing stream\n");
    system("echo q > ./video_fifo");
    printf("resetting stream...\n");
    close(fifo_fd);
    goto restart;

    return 0;
}
