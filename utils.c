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
#include <sys/stat.h>


const ver_t version = {1, 1, 0};


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
                log4c("found game: %s in title %s", wordlist[i][0], title);
                return i;
            }
            j++;
        }
        j = 0;
        i++;
    }
    log4c("Couln't find game for title %s, defaulting to \"Special Event\"...", title);
    return 0;
}


char *get_metadata(char *filename)
{
    log4c("fetching metadatas of %s...", filename);
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    avformat_open_input(&fmt_ctx, filename, NULL, NULL);

    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (!strcmp(tag->key, "comment")) {
            char *toReturn = malloc(strlen(tag->value) + 1);
            strcpy(toReturn, tag->value);
            avformat_close_input(&fmt_ctx);   
            return toReturn;
        }
    }
    log4c("couldn't get metadatas of %s...", filename);
    return 0;
}


void write_metadata(char * restrict filename, char * restrict toWrite)  // thx to ChatGPT for like 90% of this function
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
        log4c("convert_to_timestamp : Conversion en timestamp échouée. str given : %s, after processing : %s", datetime, temp);
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


int get_video_duration(char * restrict video, char * restrict local_path)             // returns timestamp of video duration
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ffprobe -hide_banner -show_entries format=duration \"./%s%s\" > /tmp/Karmine/drtn.tmp", local_path, video);
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


int get_undownloaded_videos(char * restrict local_path, char * restrict google_api_key)
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


void init_array_cheat(char *array, unsigned int size)
{
    if (size < 5)
    {
        perror("array size must be > 5");
        exit(1);
    }
    array[size - 5] = 98;
    array[size - 4] = 111;
    array[size - 3] = 111;
    array[size - 2] = 98;
    array[size - 1] = 115;
}


int get_cheat_array_size(char *array)       // can be optimized (search method)
{
    char searchedValues[] = {98, 111, 111, 98, 115, 0};
    int i = 0;
    int j = 0;
    unsigned int security = UINT32_MAX;

    
    while (security)
    {
        if (array[i] == searchedValues[j])
        {
            j++;
            if (!searchedValues[j])
                return i+1;
        }

        security--;
        i++;
    }

    return 0;
}


void get_env_filepath(char *buffer, unsigned int size) {
    char path[512];
    const char *home = getenv("HOME");
    if (!home) {
        log4c("Erreur : variable d'environnement HOME non définie");
        exit(1);
    }

    snprintf(path, sizeof(path), "%s%s", home, "/.config/Karmine");
    mkdir(path, 0700);
    snprintf(buffer, size, "%s%s/%s", home, "/.config/Karmine", ".env");
}


int get_env_infos(char * restrict filepath, char * restrict stream_key, char * restrict bot_id, char * restrict bot_secret, char * restrict refresh_token, char * restrict gapi_key, char * restrict lpath, char * restrict channel) {
    FILE *file = fopen(filepath, "r");
    if (!file) return 0;

    char line[256];
    char *vars[] = {stream_key, bot_id, bot_secret, refresh_token, gapi_key, lpath, channel};

    log4c("Fetching API keys from config file...");
    for (int i = 0; i < 7; i++)
    {
        if(!fgets(line, sizeof(line), file)) return 0;
        strncpy(vars[i], line + 9, get_cheat_array_size(vars[i]) + 1);
        vars[i][strlen(vars[i]) - 1] = '\0';
    }

    fclose(file);
    return 1;
}


int askSave_env_infos(char *filepath) {
    char stream_key[128], bot_id[128], bot_secret[128], refresh_token[128], gapi_key[128], lpath[128], channel[128];

    printf("Entrez votre clé de stream Twitch : ");
    if (!fgets(stream_key, sizeof(stream_key), stdin)) return 0;
    stream_key[strcspn(stream_key, "\r\n")] = 0;

    printf("Entrez votre BOT ID Twitch : ");
    if (!fgets(bot_id, sizeof(bot_id), stdin)) return 0;
    bot_id[strcspn(bot_id, "\r\n")] = 0;

    printf("Entrez votre BOT SECRET Twitch : ");
    if (!fgets(bot_secret, sizeof(bot_secret), stdin)) return 0;
    bot_secret[strcspn(bot_secret, "\r\n")] = 0;

    printf("Entrez votre refresh token Twitch : ");
    if (!fgets(refresh_token, sizeof(refresh_token), stdin)) return 0;
    refresh_token[strcspn(refresh_token, "\r\n")] = 0;

    printf("Entrez votre clé API Google : ");
    if (!fgets(gapi_key, sizeof(gapi_key), stdin)) return 0;
    gapi_key[strcspn(gapi_key, "\r\n")] = 0;

    printf("Entrez votre dossier ou seront stockées les videos : ");
    if (!fgets(lpath, sizeof(lpath), stdin)) return 0;
    lpath[strcspn(lpath, "\r\n")] = 0;

    printf("Entrez le nom de votre chaine Twitch : ");
    if (!fgets(channel, sizeof(channel), stdin)) return 0;
    channel[strcspn(channel, "\r\n")] = 0;

    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Erreur lors de l'écriture du fichier .env");
        return 0;
    }

    fprintf(file, "TTV_LIVE=%s\n", stream_key);
    fprintf(file, "TTV_BTID=%s\n", bot_id);
    fprintf(file, "TTV_SECR=%s\n", bot_secret);
    fprintf(file, "TTV__REF=%s\n", refresh_token);
    fprintf(file, "GAPI_KEY=%s\n", gapi_key);
    fprintf(file, "LOCAL__P=%s\n", lpath);
    fprintf(file, "TTV_NAME=%s\n", channel);
    fclose(file);

    log4c("Clés API enregistrées dans %s\n", filepath);
    return 1;
}
