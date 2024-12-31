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
#define REMOTE_PATH_START "im gonna change that, that sucks to port my code"
#define SSH_PATH "/home/local_username/.ssh/id_rsa"
#define LOCAL_PATH "videos/"
#define SEED 721077     // funny magic



void* downloadFromServer(void *arg)
{
    setlocale(LC_ALL, "fr_FR.UTF-8");
    char command[255];
    char *remote_path = (char*)arg;

    snprintf(command, sizeof(command), "scp -i %s \"%s@%s:%s%s\" %s", SSH_PATH, REMOTE_NAME, REMOTE_IP, REMOTE_PATH_START, remote_path, LOCAL_PATH);
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

    if ((int)bitrate >= 5600000) return 6000000;

    return (int)bitrate + 400000;
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


void *writeToFifo(void *arg) {
    int fifo_fd = open("video_fifo", O_WRONLY);
    if (fifo_fd == -1) {
        perror("Erreur lors de l'ouverture du FIFO");
        exit(1);
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -re -i \"%s%s\" -c copy -f mpegts -", LOCAL_PATH, (char *)arg);
    FILE *ffmpeg = popen(cmd, "r");
    if (!ffmpeg) {
        perror("Erreur lors de l'exécution de ffmpeg");
        exit(1);
    }

    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), ffmpeg)) > 0) {
        write(fifo_fd, buffer, bytesRead);
    }

    pclose(ffmpeg);
    close(fifo_fd);
    return NULL;
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
        fprintf(fichier, "on va éclater %s sur %s dans \n%02ld:%02ld:%02ld", opponent, game, hours, minutes, seconds);
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


int main(void) {
    setlocale(LC_ALL, "fr_FR.UTF-8");

    char **playlist = getAllFiles();
    char *video = playlist[chooseVideo(size_of_double_array(playlist))];
    pthread_t downloadThreadId;
    pthread_create(&downloadThreadId, NULL, downloadFromServer, (void *)video);
    pthread_join(downloadThreadId, NULL);
    mkfifo("video_fifo", 0666);

    char ffmpegCmd[1024];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
        "exec 1>/dev/null 2>/dev/null; "
        "ffmpeg -re -i video_fifo "
        "-vf \"drawtext=textfile=./tmp.time:reload=60:x=0:y=1040:fontsize=20:fontcolor=white\" "
        "-af \"highpass=f=200,lowpass=f=3000\" "
        "-c:v h264_qsv -preset veryslow -global_quality 1 "
        "-maxrate 6000k -bufsize 9000k "
        "-c:a aac -b:a 128k -f flv rtmp://live.twitch.tv/app/%s > /dev/null 2>&1",
        STREAM_KEY);
    

    pthread_t ffmpegThreadId;
    pthread_create(&ffmpegThreadId, NULL, cmdRunInThread, (void *)ffmpegCmd);
    pthread_t timerThdId;
    pthread_create(&timerThdId, NULL, ffmpegTimer, NULL);
    handleAPI();
    
    pthread_t writerThread;
    char tmp[256];

    FILE *fichier = fopen("next.data", "r");
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);

    long timeleft = convert_to_timestamp(tmp);

    while (1) {
        // Diffuser la vidéo actuelle
        pthread_create(&writerThread, NULL, writeToFifo, (void *)video);

        // Télécharger la prochaine vidéo
        char *nextVideo = playlist[chooseVideo(size_of_double_array(playlist))];
        pthread_create(&downloadThreadId, NULL, downloadFromServer, (void *)nextVideo);
        snprintf(tmp, sizeof(tmp), "python3 handle_name_changing.py -u \"%s\" -r", video);
        system(tmp);


        pthread_join(downloadThreadId, NULL);
        pthread_join(writerThread, NULL);


        remove(video);
        video = nextVideo;
        if (timeleft - (long)time(NULL) < 600)
        {
            sleep(600);
            fgets(tmp, sizeof(tmp), fichier);
            timeleft = convert_to_timestamp(tmp);
            sleep((long)time(NULL) - timeleft);
        }
    }

    return 0;
}