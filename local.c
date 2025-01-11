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
#include <libavformat/avformat.h>

#define STREAM_KEY "your twitch channel stream key"
#define LOCAL_PATH "videos/"
#define SEED 721077     // what could that number be



char *get_metadata(char *filename)
{
    AVFormatContext *fmt_ctx = NULL;
    avformat_open_input(&fmt_ctx, filename, NULL, NULL);
    AVDictionaryEntry *tag = NULL;

    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (strcmp(tag->key, "comment") == 0) {
            char *toReturn = malloc(sizeof(char *) * strlen(tag->value));
            strcpy(toReturn, tag->value);
            avformat_close_input(&fmt_ctx);   
            return toReturn;
        }
    }
    return 0;
}


void write_metadata(char *filename, char *toWrite)
{
    char *output = "30784230304235.mp4";
    AVFormatContext *fmt_ctx = NULL;
    AVFormatContext *out_ctx = NULL;

    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Erreur : Impossible d'ouvrir le fichier %s\n", filename);
        exit(1);
    }

    avformat_alloc_output_context2(&out_ctx, NULL, NULL, output);
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream *in_stream = fmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(out_ctx, NULL);
        if (!out_stream) {
            fprintf(stderr, "Erreur : Impossible de copier le flux\n");
            avformat_close_input(&fmt_ctx);
            avformat_free_context(out_ctx);
            exit(1);
        }
        avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        out_stream->time_base = in_stream->time_base;
    }
    av_dict_set(&out_ctx->metadata, "comment", toWrite, 0);


    if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_ctx->pb, output, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Erreur : Impossible d'ouvrir le fichier de sortie\n");
            avformat_close_input(&fmt_ctx);
            avformat_free_context(out_ctx);
            exit(1);
        }
    }


    if (avformat_write_header(out_ctx, NULL) < 0) {
        fprintf(stderr, "Erreur : Impossible d'écrire l'en-tête\n");
        avformat_close_input(&fmt_ctx);
        avformat_free_context(out_ctx);
        exit(1);
    }


    AVPacket pkt;
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        AVStream *in_stream = fmt_ctx->streams[pkt.stream_index];
        AVStream *out_stream = out_ctx->streams[pkt.stream_index];

        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        if (av_interleaved_write_frame(out_ctx, &pkt) < 0) {
            fprintf(stderr, "Erreur : Impossible d'écrire un paquet\n");
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(out_ctx);

    avformat_close_input(&fmt_ctx);
    if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_ctx->pb);
    }

    avformat_close_input(&fmt_ctx);

    remove(filename);
    rename(output, filename);
}


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


char **file_lines(char *filename) {
    FILE *fichier = fopen(filename, "r");
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
    printf("launching main ffmpeg");
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


int get_undownloaded_videos()
{
    char tmp[256];
    char video[128];
    snprintf(tmp, sizeof(tmp), "find %s -printf '%%T+ %%p\n' | sort -r | head > ltsvd.tmp", LOCAL_PATH);
    system(tmp);
    strcpy(tmp, "\0");
    FILE *lst = fopen("ltsvd.tmp", "r");
    do
    {
        fgets(tmp, sizeof(tmp), lst);
    }   
    while (strlen(tmp) < 31 + strlen(LOCAL_PATH) + 2);

    if (tmp[strlen(tmp) - 1] == '\n')
        tmp[strlen(tmp) - 1] = '\0';
    
    fclose(lst);
    remove("ltsvd.tmp");
    for (int i = 31; tmp[i] != '\n' && tmp[i] != '\0' && tmp[i] != EOF; i++)
    {
        video[i - 31] = tmp[i];
        if (tmp[i + 1] == '\0')
        {
            video[i-30] = '\0';
            break;
        }
    }
    char *link = get_metadata(video);

    const char *base_url = "https://www.youtube.com/watch?v=";
    char url[64];
    strcpy(url, base_url);
    int i = 1;
    while (i < 9999)
    {
        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies ./cooking.txt --flat-playlist \"https://www.youtube.com/@KarmineCorpVOD/videos\" --print \"%%(id)s\" --playlist-end %d > recentVids.txt", i);
        printf("fetching %d video...\n", i);
        system(tmp);

        FILE *tmpVids = fopen("recentVids.txt", "r");
        for (int j = 0; j<i; j++)   
            fgets(tmp, sizeof(tmp), tmpVids);

        tmp[strlen(tmp) - 1] = '\0';
        strcat(url, tmp);
        if(strcmp(url, link) == 0)
        {
            if (i == 1)
                return 0;
            snprintf(tmp, sizeof(tmp), "yt-dlp --cookies ./cooking.txt --flat-playlist \"https://www.youtube.com/@KarmineCorpVOD/videos\" --print \"%%(id)s\" --playlist-end %d > recentVids.txt", i - 1);
            system(tmp);
            return i-1;
        }
        else
            strcpy(url, base_url);

        i++;
    }  

    return 0;
}


void *download_videos(void* bool)
{
    char **videos = file_lines("recentVids.txt");
    char tmp[256];
    for (int i = size_of_double_array(videos) - 1; i >= 0; i--)
    {
        printf("%s\n", videos[i]);
        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies ./cooking.txt -f 299 %s", videos[i]);
        system(tmp);

        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies ./cooking.txt -f 299 --print filename %s > %s", videos[i], videos[i]);
        system(tmp);
    
        FILE *fichier = fopen(videos[i], "r");
        fgets(tmp, sizeof(tmp), fichier);
        char *oldFilename = malloc(sizeof(char *) * ((strlen(tmp) + 1)));
        strcpy(oldFilename, tmp);
        oldFilename[strlen(oldFilename) - 1] = '\0';
        tmp[strlen(tmp) - strlen(videos[i]) - 8] = '\0';
        strcat(tmp, ".mp4");
        rename(oldFilename, tmp);
        printf("%s\t%s\n", oldFilename, tmp);

        char *fullLink = malloc(strlen("https://www.youtube.com/watch?v=") + strlen(videos[i]) + 3);
        strcpy(fullLink, "https://www.youtube.com/watch?v=");
        strcat(fullLink, videos[i]);

        fclose(fichier);
        write_metadata(tmp, fullLink);
        char moov[512];
        snprintf(moov, sizeof(moov), "mv \"%s\" %s", tmp, LOCAL_PATH);
        system(moov);
        remove(videos[i]);
    }

    int *a = (int *)bool;
    *a = 1;
    bool = (void *)a;

    return bool;
}



int main(void) {
    restart:
    setlocale(LC_ALL, "fr_FR.UTF-8");

    char **playlist = getAllFiles();                                             
    char *video = playlist[chooseVideo(size_of_double_array(playlist))];
    unlink("video_fifo");
    mkfifo("video_fifo", 0666);

    char ffmpegCmd[256];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
        "ffmpeg -loglevel debug -re -i video_fifo "
        "-c:v copy -bufsize 18000k "
        "-c:a copy -f flv rtmp://live.twitch.tv/app/%s > logs.txt 2>&1",
        STREAM_KEY);

    handleAPI();
    char tmp[256];

    /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/
    FILE *fichier = fopen("next.data", "r");
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);
    fgets(tmp, sizeof(tmp), fichier);
    /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/

    long timeleft = convert_to_timestamp(tmp);
    long timestart = (long)time(NULL);
    char *nextVideo = video;

    pthread_t ffmpegThreadId;
    pthread_create(&ffmpegThreadId, NULL, cmdRunInThread, (void *)ffmpegCmd);

    int fifo_fd = open("video_fifo", O_WRONLY);
    int isDownloaded = 0;

    while ((long)time(NULL) - timestart < 154000) {
        if (get_undownloaded_videos())
        {
            pthread_t downloaderThr;
            pthread_create(&downloaderThr, NULL, download_videos, (void *)&isDownloaded);
        }
        if(isDownloaded)
        {
            isDownloaded = 0;
            playlist = getAllFiles();
        }
            
        printf("%s\n", video);

        /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/
        system("python3 handle_twitchAPI.py -ov");                                                                  //
        FILE *monitoring = fopen("mntrdata.tmp", "r");                                                              //
        char isOnline[9];                                                                                           //
        char viewerCount[6];                                                                                        //
                                                                                                                    //
        fgets(isOnline, sizeof(isOnline), monitoring);                                                              //
        fgets(viewerCount, sizeof(viewerCount), monitoring);                                                        //
        isOnline[strlen(isOnline) - 1] = '\0';                                                                      //
        fclose(monitoring);                                                                                         //
        remove("mntrdata.tmp");                                                                                     //
                                                                                                                    //
        monitoring = fopen("/var/www/html/monitoring.json", "w");                                                   //
        cJSON *json = cJSON_CreateObject();                                                                         //
                                                                                                                    //
        cJSON_AddStringToObject(json, "status", isOnline);                                                          //
        cJSON_AddStringToObject(json, "videoTitle", video);                                                         //
        cJSON_AddNumberToObject(json, "videoDuration", (double)get_video_duration(video));                          //
        cJSON_AddNumberToObject(json, "videoStartTime", (double)time(NULL));                                        //
        cJSON_AddNumberToObject(json, "streamStartTime", (double)timestart);                                        //
        cJSON_AddNumberToObject(json, "viewers", (double)atoi(viewerCount));                                        //
                                                                                                                    //
        char *json_str = cJSON_Print(json);                                                                         //
        fputs(json_str, monitoring);                                                                                //
        fclose(monitoring);                                                                                         //
        cJSON_free(json_str);                                                                                       //
        cJSON_Delete(json);                                                                                         //
                                                                                                                    //
        /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/

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

    printf("Twitch limit of 48h is almost reached, resetting stream...\n");
    close(fifo_fd);
    goto restart;

    return 0;
}
