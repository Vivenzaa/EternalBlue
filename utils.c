#include    "utils.h"
#include    "APIs.h"    
#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>


void itos(int N, char *str) 
{
    int i = 0;
    int sign = N;

    if (N < 0)
        N = -N;

    while (N > 0) 
    {
        str[i++] = N % 10 + '0';
      	N /= 10;
    } 

    if (sign < 0) 
        str[i++] = '-';
    
    str[i] = '\0';

    for (int j = 0, k = i - 1; j < k; j++, k--) 
    {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}


void log4c(char *base, ...)
{
    char *prev = base;
    char *current = strchr(base, '%');
    char *word = NULL;
    char *payload_str = calloc(1, 1);
    int payload_len = 1;
    char timeString[20];        // 18 is enough for 64bit systems but somehow my 32bit server needs 20 ??
    
    time_t current_time;
    va_list list;
    struct tm * time_info;
    va_start(list, base);
    
    while(current)
    {
        word = malloc(current - prev + 1);
        char flag[3];

        strncpy(word, prev, current - prev);
        strncpy(flag, current, 2);

        word[current - prev] = '\0';
        flag[2] = '\0';

        payload_len += strlen(word);
        payload_str = realloc(payload_str, payload_len + 1);
        strcat(payload_str, word);


        switch(flag[1])
        {
            case 's':
                char *buffer = va_arg(list, char *);
                payload_len += strlen(buffer);
                payload_str = realloc(payload_str, payload_len + 1);
                strcat(payload_str, buffer);
                break;

            case 'd':
                char buffd[11];    //max int -> 2,147,483,647 and max uint has 10 characters too
                itos(va_arg(list, int), buffd);
                payload_len += strlen(buffd);
                payload_str = realloc(payload_str, payload_len + 1);
                strcat(payload_str, buffd);
                break;
            
            default:
                printf("error -->%s<-- here, this flag is not supported...\n", flag);
                break;
            // case 0 -> expands to 02d ?
        }

        prev = current + strlen(flag);
        current = strchr(prev, '%');
        free(word);
    }

    if (prev != base + strlen(base))
    {
        payload_len += strlen(prev);
        payload_str = realloc(payload_str, payload_len + 1);
        strcat(payload_str, prev);
    }

    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timeString, sizeof(timeString), "%x %H:%M:%S", time_info);

    FILE *fichier = fopen("/var/tmp/Karmine/console.log", "a");
    fprintf(fichier, "[%s] %s\n", timeString, payload_str);
    
    va_end(list);
    free(payload_str);
    fclose(fichier);
}


char get_utc_offset() 
{
    time_t now = time(NULL); 
    struct tm utc_time;
    struct tm local_time;

    gmtime_r(&now, &utc_time);
    localtime_r(&now, &local_time);

    char offset = local_time.tm_hour - utc_time.tm_hour;

    if (local_time.tm_yday < utc_time.tm_yday) {
        offset -= 24;
    } else if (local_time.tm_yday > utc_time.tm_yday) {
        offset += 24;
    }

    return offset;
}


long SizeOfFile(char *path)
{
    FILE *fichier = fopen(path, "r");
    if(fichier == NULL)
        return 0;
    fseek(fichier, 0, SEEK_END);
    long toReturn = ftell(fichier);
    fclose(fichier);
    return toReturn + 1;        // pour stocker charactères de fin de chaine
}


char *curl_filename(char *ptr)
{
    int apNb = 0;
    for(unsigned long i = 0; i < strlen(ptr); i++)
    {
        if (ptr[i] == '\'')
            apNb++;
    }

    if (apNb)
    {
        int j = 0;
        char tmp[strlen(ptr) + (apNb * 3) + 1];         // apNb multiplié par 3 car ' -> '\''
        for(unsigned long i = 0; i < strlen(ptr); i++)
        {
            if (ptr[i] == '\'')
            {
                tmp[i + j] = '\'';
                tmp[i + j + 1] = '\\';
                tmp[i + j + 2] = '\'';
                j += 3;
            }
            tmp[i + j] = ptr[i];
        }
        tmp[strlen(ptr) + (apNb * 3)] = '\0';
        char *final = malloc(strlen(tmp) + 1);
        strcpy(final, tmp);
        return final;
    }
    return NULL;
}


int getGame(char *title, char ***wordlist)
{
    int i = 0;
    int j = 0;
    char *isFound;
    char titleLowered[strlen(title) + 1];
    strcpy(titleLowered, title);
    for (long unsigned int i = 0; i < strlen(titleLowered); i++) {
        titleLowered[i] = tolower(titleLowered[i]);
    }
    while (wordlist[i])
    {
        while(wordlist[i][j])
        {
            isFound = strstr(titleLowered, wordlist[i][j]);
            if (isFound)
            {
                char tmp[128];
                snprintf(tmp, sizeof(tmp), "found game: %s in title %s", wordlist[i][0], title);
                log4c(tmp);
                return i;
            }
            j++;
        }
        j = 0;
        i++;
    }
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "Couln't find game for title %s, defaulting to \"Special Event\"...", title);        // try getting game from YT videos's data
    log4c(tmp);
    return 0;
}


char *get_metadata(char *filename)
{
    {char tmp[256]; snprintf(tmp, sizeof(tmp), "fetching metadatas of %s...", filename); log4c(tmp);}
    AVFormatContext *fmt_ctx = NULL;
    avformat_open_input(&fmt_ctx, filename, NULL, NULL);
    AVDictionaryEntry *tag = NULL;

    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (!strcmp(tag->key, "comment")) {
            char *toReturn = malloc(strlen(tag->value) + 1);
            strcpy(toReturn, tag->value);
            avformat_close_input(&fmt_ctx);   
            return toReturn;
        }
    }
    {char tmp[256]; snprintf(tmp, sizeof(tmp), "couldn't get metadatas of %s...", filename); log4c(tmp);}
    return 0;
}


void write_metadata(char *filename, char *toWrite)
{
    char *output = "/tmp/Karmine/30784230304235.mp4";
    AVFormatContext *fmt_ctx = NULL;
    AVFormatContext *out_ctx = NULL;

    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        log4c("Erreur : Impossible d'ouvrir le fichier dans lequel écrire les métadonnées");
        exit(1);
    }

    log4c("write_metadata: writing metadata on file...");
    avformat_alloc_output_context2(&out_ctx, NULL, NULL, output);
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream *in_stream = fmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(out_ctx, NULL);
        if (!out_stream) {
            log4c("Erreur : Impossible de copier le flux");
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
            log4c("write_metadata : Impossible d'ouvrir le fichier de sortie");
            avformat_close_input(&fmt_ctx);
            avformat_free_context(out_ctx);
            exit(1);
        }
    }


    if (avformat_write_header(out_ctx, NULL) < 0) {
        log4c("write_metadata : Impossible d'écrire l'en-tête");
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
            log4c("write_metadata : Impossible d'écrire un paquet");
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
    log4c("write_metadata: successfully added metadata to file...");
}


char **getAllFiles(char *local_path)
{
    struct dirent *de;
    DIR *dr = opendir(local_path);
    
    char **lines = NULL;
    int count = 0;

    while ((de = readdir(dr)) != NULL) {
        if (strlen(de->d_name) <= 3)     continue;
        char *line = malloc(strlen(de->d_name) + 1);
        if (!line) {
            log4c("getAllFiles : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        strcpy(line, de->d_name);

        char **temp = realloc(lines, sizeof(char *) * (count + 1));
        if (!temp) {
            log4c("getAllFiles : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        lines = temp;

        lines[count++] = line;
    }
    
    char **temp = realloc(lines, sizeof(char *) * (count + 1));
    if (!temp) {
        log4c("getAllFiles : allocation mémoire échouée");
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
            log4c("file_lines : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        strcpy(line, buffer);
        line[strlen(line) - 1] = '\0';

        char **temp = realloc(lines, sizeof(char *) * (count + 1));
        if (!temp) {
            log4c("file_lines : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        lines = temp;

        lines[count++] = line;
    }
    
    char **temp = realloc(lines, sizeof(char *) * (count + 1));
    if (!temp) {
        log4c("file_lines : allocation mémoire échouée");
        exit(EXIT_FAILURE);
    }
    lines = temp;
    lines[count++] = NULL;

    return lines;
}


unsigned int chooseVideo(unsigned int x, int seed)       // x is the len of file list, NOT the last index of filelist
{
    unsigned int tmp;
    int fichier = open("/dev/random", O_RDONLY);
    read(fichier, &tmp, sizeof(tmp));
    close(fichier);
    
    return (tmp * seed) % x;   
}


unsigned int size_of_double_array(char **array) {
    unsigned int size = 0;
    while (array[size] != NULL)
        size++;
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
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "convert_to_timestamp : Conversion en timestamp échouée. str given : %s, after processing : %s", datetime, temp);
        log4c(tmp);
        return -1;
    }

    return (long)timestamp;
}


void *cmdRunInThread(void *str)
{
    log4c("launching main ffmpeg");
    const char *cmd = (const char *)str;
    system(cmd);
    log4c("main ffmpeg has ended");
    return NULL;
}


int get_video_duration(char *video, char *local_path)             // returns timestamp of video duration
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ffprobe -show_entries format=duration \"./%s%s\" > /tmp/Karmine/drtn.tmp", local_path, video);
    system(cmd);

    FILE *fichier = fopen("/tmp/Karmine/drtn.tmp", "r");
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
    remove("/tmp/Karmine/drtn.tmp");

    return atoi(duration);
}


int get_undownloaded_videos(char *local_path, char *google_api_key)
{
    char tmp[256];
    char video[128];
    snprintf(tmp, sizeof(tmp), "find %s -printf '%%T+ %%p\n' | sort -r | head > /tmp/Karmine/ltsvd.tmp", local_path);
    system(tmp);
    strcpy(tmp, "\0");
    FILE *lst = fopen("/tmp/Karmine/ltsvd.tmp", "r");
    do
    {
        fgets(tmp, sizeof(tmp), lst);
    }   
    while (strlen(tmp) < 31 + strlen(local_path) + 2);

    if (tmp[strlen(tmp) - 1] == '\n')
        tmp[strlen(tmp) - 1] = '\0';
    
    fclose(lst);

    remove("/tmp/Karmine/ltsvd.tmp");
    for (int i = 31; tmp[i] != '\n' && tmp[i] != '\0' && tmp[i] != EOF; i++)
    {
        video[i - 31] = tmp[i];
        if (tmp[i + 1] == '\0')
        {
            video[i-30] = '\0';
            break;
        }
    }
    char *fullLink = get_metadata(video);       // form : https://www.youtube.com/watch?v=xxxxxxxxxxx
    char link[12];
    //link = (link + strlen(link) - 11);      // form : xxxxxxxxxxx
    strcpy(link, fullLink + strlen(fullLink) - 11);
    free(fullLink);

    int i = 1;
    log4c("get_undownloaded_videos: fetching most recent videos...");
    while (i < 9999)
    {
        if (YTAPI_Get_Recent_Videos(i, google_api_key) != 0)
        {
            log4c("Couldn't get url list, aborting download...");
            return 0;
        }
        FILE *tmpVids = fopen("/tmp/Karmine/recentVids", "r");
        for (int j = 0; j<i; j++)   
            fgets(tmp, sizeof(tmp), tmpVids);
        fclose(tmpVids);
        remove("/tmp/Karmine/recentVids");


        tmp[11] = '\0';
        if(!strcmp(tmp, link))
        {
            if (i == 1)
                return 0;
                
            YTAPI_Get_Recent_Videos(i - 1, google_api_key);
            return i-1;
        }

        i++;
    } 
    return 0;
}


void recur_free(char **tab)
{
    for (int i = 0; i < 256; i++)
    {
        if (tab[i] == NULL)
            break;
        free(tab[i]);
    }  
    free(tab);
}


void recur_tabcpy(char **dest, char **src, int size) {
    for (int i = 0; i < size; i++) {
        if (src[i] == NULL)
        {
            dest[i] = NULL;
            continue;
        }
        dest[i] = strdup(src[i]); // Alloue et copie la chaîne
        if (dest[i] == NULL) {
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            exit(EXIT_FAILURE);
        }
    }
}