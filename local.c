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
#include <ctype.h>
#include <libavformat/avformat.h>

#define STREAM_KEY "live_1219233412_x9qHLfK4nDukOO8SFaiRNjqivyFuGh"
#define LOCAL_PATH "videos/"
#define CHANNEL_NAME "Kc_Replays"
#define BOT_ID "kjnngccbj9fcde0r6jc3kv6jyzofqh"
#define BOT_SECRET "ee4iflzar1zoeafxlyacxc2fk4cnwx"
#define DEFAULT_REFRESH_TOKEN "svs3ozdcp34txn80ons3iztukfj1uitembkzobnn6xfoyrbei0"
#define GOOGLE_API_KEY "AIzaSyA_mlDBPL3nTGUiIyonwF6HCnKeHnWdanM"
#define SEED 0xb00b5


typedef struct ffdata
{
    char *videopath;
    int *fifo;
}ffdata;



void log2file(char *toWrite)
{
    time_t current_time;
    struct tm * time_info;
    char timeString[9];
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);

    FILE *fichier = fopen("consolelog.out", "a");
    fprintf(fichier, "[%s] %s\n", timeString, toWrite);
    fclose(fichier);
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
        char tmp[strlen(ptr) + (apNb * 3) + 1];
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
        tmp[strlen(ptr) + apNb] = '\0';
        char *aaa = malloc(strlen(tmp) + 1);
        strcpy(aaa, tmp);
        printf("%s\n", aaa);
        return aaa;
    }
    return ptr;
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
                log2file(tmp);
                return i;
            }
                
            j++;
        }
        j = 0;
        i++;
    }
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "Couln't find game for title %s, defaulting to \"Special Event\"...", title);        // try getting game from YT videos's data
    log2file(tmp);
    return 0;
}


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
        log2file("Erreur : Impossible d'ouvrir le fichier dans lequel écrire les métadonnées");
        exit(1);
    }

    log2file("write_metadata: writing metadata on file...");
    avformat_alloc_output_context2(&out_ctx, NULL, NULL, output);
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream *in_stream = fmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(out_ctx, NULL);
        if (!out_stream) {
            log2file("Erreur : Impossible de copier le flux");
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
            log2file("write_metadata : Impossible d'ouvrir le fichier de sortie");
            avformat_close_input(&fmt_ctx);
            avformat_free_context(out_ctx);
            exit(1);
        }
    }


    if (avformat_write_header(out_ctx, NULL) < 0) {
        log2file("write_metadata : Impossible d'écrire l'en-tête");
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
            log2file("write_metadata : Impossible d'écrire un paquet");
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
    log2file("write_metadata: successfully added metadata to file...");
}


char **getAllFiles()
{
    struct dirent *de;
    DIR *dr = opendir(LOCAL_PATH);
    
    char **lines = NULL;
    int count = 0;

    while ((de = readdir(dr)) != NULL) {
        if (strlen(de->d_name) <= 3)     continue;
        char *line = malloc(strlen(de->d_name) + 1);
        if (!line) {
            log2file("getAllFiles : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        strcpy(line, de->d_name);

        char **temp = realloc(lines, sizeof(char *) * (count + 1));
        if (!temp) {
            log2file("getAllFiles : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        lines = temp;

        lines[count++] = line;
    }
    
    char **temp = realloc(lines, sizeof(char *) * (count + 1));
    if (!temp) {
        log2file("getAllFiles : allocation mémoire échouée");
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
            log2file("file_lines : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        strcpy(line, buffer);
        line[strlen(line) - 1] = '\0';

        char **temp = realloc(lines, sizeof(char *) * (count + 1));
        if (!temp) {
            log2file("file_lines : allocation mémoire échouée");
            exit(EXIT_FAILURE);
        }
        lines = temp;

        lines[count++] = line;
    }
    
    char **temp = realloc(lines, sizeof(char *) * (count + 1));
    if (!temp) {
        log2file("file_lines : allocation mémoire échouée");
        exit(EXIT_FAILURE);
    }
    lines = temp;
    lines[count++] = NULL;

    return lines;
}


unsigned int chooseVideo(unsigned int x)       // x is the len of file list, NOT the last index of filelist
{
    unsigned int tmp = (unsigned int)time(NULL) * SEED * 0x5f3759df;   // what the fuck?
    return tmp % x;   
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
        log2file("convert_to_timestamp : Conversion en timestamp échouée.");
        return -1;
    }

    return (long)timestamp;
}


void *cmdRunInThread(void *str)
{
    log2file("launching main ffmpeg");
    const char *cmd = (const char *)str;
    system(cmd);
    log2file("main ffmpeg has ended");
    return NULL;
}


void *ffwrite(void *data)
{
    ffdata *pathfifo = (ffdata *)data;
    int *fifo_fd = pathfifo->fifo;
    
    char cmdFIFO[strlen("ffmpeg -re -i \"\" -c copy -f mpegts -") + 
                 strlen(LOCAL_PATH) +
                 strlen(pathfifo->videopath) + 1];
        
    snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -re -i \"%s%s\" -c copy -f mpegts -", LOCAL_PATH, pathfifo->videopath); 

    FILE *ffmpeg = popen(cmdFIFO, "r");
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), ffmpeg)) > 0) {
        write(*fifo_fd, buffer, bytesRead);
    }
    pclose(ffmpeg);

    return NULL;
}


void *handleAPI()
{
    system("curl https://api2.kametotv.fr/karmine/events -o api.json");
    log2file("Fetching api data of La Prestigieuse...");
    FILE *fichier = fopen("api.json", "r");
    char tmp[16384];
    fread(tmp, 1, sizeof(tmp), fichier); 
    cJSON *json = cJSON_Parse(tmp); 
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log2file((char *)error_ptr); 
        } 
        cJSON_Delete(json); 
        return NULL; 
    }
    cJSON *upcomming = cJSON_GetArrayItem(json, 0);
    cJSON *start    = cJSON_GetObjectItemCaseSensitive(upcomming, "start");
    cJSON *end      = cJSON_GetObjectItemCaseSensitive(upcomming, "end");
    cJSON *channel  = cJSON_GetObjectItemCaseSensitive(upcomming, "streamLink");
    
    fichier = fopen("next.data", "w");
    fprintf(fichier, "%s\n%s\n%s\n", start->valuestring, end->valuestring, channel->valuestring);
    fclose(fichier);
    cJSON_Delete(json);
    return NULL;
}


char YTAPI_Get_Recent_Videos(unsigned int max)          // Creates file of form yt_id;yt_title sorted by most recent video
{
    char tmp[2048*max];
    snprintf(tmp, sizeof(tmp), "curl -X GET \"https://www.googleapis.com/youtube/v3/search?"
        "part=snippet&channelId=UCApmTE4So9oX7sPkDGgSpFQ&maxResults=%d&order=date&type=video&key=%s\" > response.json", 
        max, GOOGLE_API_KEY);
    system(tmp);

    FILE *fichier = fopen("response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("response.json");


    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log2file((char *)error_ptr); 
            return -1;
        } 
        cJSON_Delete(json); 
        return -1; 
    }
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        snprintf(tmp, sizeof(tmp), "YTAPI_Get_Recent_Videos: Request failed: %s", error->valuestring);
        log2file(tmp);
        cJSON_Delete(json);
        return 0;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "items");
    for(unsigned int i = 0; i < max; i++)
    {
        cJSON *item = cJSON_GetArrayItem(data, i);  
        cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
        cJSON *videoId = cJSON_GetObjectItemCaseSensitive(id, "videoId");

        fichier = fopen("recentVids", "a");
        fprintf(fichier, "%s\n", videoId->valuestring);
        fclose(fichier);
    }
    
    
    cJSON_Delete(json);
    return 0;
}


char *YTAPI_Get_Video_Name(char *videoId)
{
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "curl -X GET \"https://www.googleapis.com/youtube/v3/videos?part=snippet&id=%s&key=%s\" > response.json", 
        videoId, GOOGLE_API_KEY);
    system(tmp);

    FILE *fichier = fopen("response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("response.json");


    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log2file((char *)error_ptr); 
            return NULL;
        } 
        cJSON_Delete(json); 
        return NULL; 
    }
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        snprintf(tmp, sizeof(tmp), "YTAPI_Get_Video_Name: Request failed: %s", error->valuestring);
        log2file(tmp);
        cJSON_Delete(json);
        return NULL;
    }

    cJSON *data =       cJSON_GetObjectItemCaseSensitive(json, "items");
    cJSON *item =       cJSON_GetArrayItem(data, 0);
    cJSON *snippet =    cJSON_GetObjectItemCaseSensitive(item, "snippet");
    cJSON *title =      cJSON_GetObjectItemCaseSensitive(snippet, "title");

    char *toReturn = malloc(sizeof(char *) * strlen(title->valuestring));
    strcpy(toReturn, title->valuestring);

    cJSON_Delete(json);
    return toReturn;

}


int get_video_duration(char *video)             // returns timestamp of video duration
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
    char *link = get_metadata(video);       // form : https://www.youtube.com/watch?v=xxxxxxxxxxx
    link = (link + strlen(link) - 11);      // form : xxxxxxxxxxx


    int i = 1;
    log2file("get_undownloaded_videos: fetching most recent videos...");
    while (i < 9999)
    {
        YTAPI_Get_Recent_Videos(i);
        FILE *tmpVids = fopen("recentVids", "r");
        for (int j = 0; j<i; j++)   
            fgets(tmp, sizeof(tmp), tmpVids);
        fclose(tmpVids);
        remove("recentVids");


        tmp[11] = '\0';
        if(strcmp(tmp, link) == 0)
        {
            if (i == 1)
                return 0;
            YTAPI_Get_Recent_Videos(i - 1);
            return i-1;
        }

        i++;
    }  

    return 0;
}


void *download_videos(void* bool)
{
    char **videos = file_lines("recentVids");
    remove("recentVids");
    char tmp[256];
    for (int i = size_of_double_array(videos) - 1; i >= 0; i--)
    {
        snprintf(tmp, sizeof(tmp), "Downloading %s...", videos[i]);
        log2file(tmp);

        snprintf(tmp, sizeof(tmp), "mkdir %s", videos[i]);
        system(tmp);

        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies-from-browser firefox -r 5000000 -f 299+140 %s -P ./%s/", videos[i], videos[i]);   // -r 6MB
        system(tmp);

        char *videoTitle = YTAPI_Get_Video_Name(videos[i]);
        snprintf(tmp, sizeof(tmp), "Successfully downloaded %s : %s", videos[i], videoTitle);
        log2file(tmp);

        
        char videotmp[strlen(videoTitle) + 5];
        strcpy(videotmp, videoTitle);
        strcat(videotmp, ".mp4");

        snprintf(tmp, sizeof(tmp), "mv %s/* ./%s", videos[i], videotmp);
        system(tmp);
        snprintf(tmp, sizeof(tmp), "rmdir %s", videos[i]);
        system(tmp);

        char *fullLink = malloc(strlen("https://www.youtube.com/watch?v=") + strlen(videos[i]) + 1);
        strcpy(fullLink, "https://www.youtube.com/watch?v=");
        strcat(fullLink, videos[i]);

        write_metadata(videotmp, fullLink);
        char moov[512];
        snprintf(moov, sizeof(moov), "mv \"%s\" %s", videotmp, LOCAL_PATH);
        system(moov);
    }

    log2file("Successfully downloaded all recent videos.");
    int *a = (int *)bool;
    *a = 1;
    bool = (void *)a;

    return bool;
}


void revoke_access_token(char *access)
{
    char tmp[
        strlen("curl -X POST 'https://id.twitch.tv/oauth2/revoke' -H 'Content-Type: application/x-www-form-urlencoded' -d 'client_id=&token='") +
        strlen(BOT_ID) +
        strlen(access) + 1];

    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://id.twitch.tv/oauth2/revoke' "
    "-H 'Content-Type: application/x-www-form-urlencoded' -d 'client_id=%s&token=%s'", BOT_ID, access);
    system(tmp);

    snprintf(tmp, sizeof(tmp), "Revoked token %s", access);
    log2file(tmp);
}


char **refresh_access_token(char *refresh_token)
{
    char tmp[16384];
    snprintf(tmp, sizeof(tmp), 
    "curl -X POST https://id.twitch.tv/oauth2/token -H 'Content-Type: application/x-www-form-urlencoded' " 
    "-d 'grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s' -o response.json", refresh_token, BOT_ID, BOT_SECRET);
    system(tmp);


    FILE *fichier = fopen("response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier); 


    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL)
            log2file((char *)error_ptr); 
        
        fclose(fichier);
        cJSON_Delete(json); 
        return NULL; 
    }
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        snprintf(tmp, sizeof(tmp), "request failed with code %d", error->valueint);
        log2file(tmp);
        return 0;
    }
    cJSON *access = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    cJSON *expire = cJSON_GetObjectItemCaseSensitive(json, "expires_in");
    cJSON *refresh = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");

    
    char **buffer = malloc(4 * sizeof(char*));
    for (int i =0 ; i < 4; ++i)
        buffer[i] = malloc(64 * sizeof(char));
    

    strcpy(buffer[0], access->valuestring);
    strcpy(buffer[1], refresh->valuestring);
    sprintf(buffer[2], "%d", expire->valueint);
    buffer[3] = NULL;
    char **toReturn = buffer;

    fclose(fichier);
    remove("response.json");
    cJSON_Delete(json);
    return toReturn;
}


char get_stream_info(char *access)
{
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "curl -X GET 'https://api.twitch.tv/helix/streams?user_login=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-id: %s' -o response.json", CHANNEL_NAME, access, BOT_ID);
    system(tmp);


    FILE *fichier = fopen("response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);

    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log2file((char *)error_ptr); 
            return -1;
        } 
        cJSON_Delete(json); 
        return -1; 
    }
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        snprintf(tmp, sizeof(tmp), "Request failed: %s", error->valuestring);
        log2file(tmp);
        cJSON_Delete(json);
        return 0;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
    cJSON *item = cJSON_GetArrayItem(data, 0);

    fclose(fichier);
    remove("response.json");
    fichier = fopen("mntr.data", "w");
    char *isOnline = "offline";
    cJSON *vwCount = cJSON_GetObjectItemCaseSensitive(item, "viewer_count");
    if (vwCount)
        isOnline = "online";
    else
    {
        fputs("offline\n0", fichier);
        fclose(fichier);
        cJSON_Delete(json);
        return 0;
    }

    int toReturn = vwCount->valueint;
    snprintf(tmp, sizeof(tmp), "%s\n%d", isOnline, toReturn);
    fputs(tmp, fichier);
    fclose(fichier);
    cJSON_Delete(json);
    return 1;
}


void update_stream_info(char *streamerId, char *access, int gameId, char *title)
{
    char tmp[1024];
    char titleRediff[strlen(title) + 12];
    strcpy(titleRediff, title);
    titleRediff[strlen(title) - 4] = '\0';
    strcat(titleRediff, " - [REDIFFUSION]");
    snprintf(tmp, sizeof(tmp), "curl -X PATCH 'https://api.twitch.tv/helix/channels?broadcaster_id=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -H 'Content-Type: application/json' "
    "--data-raw '{\"game_id\":\"%d\", \"title\":\"%s\", \"broadcaster_language\":\"fr\",  \"tags\":[\"247Stream\", \"botstream\", \"Français\", \"KarmineCorp\"]}' -o response.json", 
    streamerId, access, BOT_ID, gameId, titleRediff);
    system(tmp);

    FILE *fichier = fopen("response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("response.json");
    cJSON *json = cJSON_Parse(tmp);
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        snprintf(tmp, sizeof(tmp), "Request failed to update stream informations: %d: %s", error->valueint, error->valuestring);
        log2file(tmp);
    }
    else
    {
        snprintf(tmp, sizeof(tmp), "Updated stream infos to game_id: %d and title %s", gameId, titleRediff);
        log2file(tmp);
    }
    cJSON_Delete(json);
}


char *get_streamer_id(char *access, char *streamer_login)
{
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "curl -X GET 'https://api.twitch.tv/helix/users?login=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o response.json", streamer_login, access, BOT_ID);
    system(tmp);

    FILE *fichier = fopen("response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);

    cJSON *json = cJSON_Parse(tmp);
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        printf("request failed : %s\n", error->valuestring);
        return NULL;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
    cJSON *item = cJSON_GetArrayItem(data, 0);

    fclose(fichier);
    cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
    if(id == NULL)
        return NULL;
    char *toReturn = malloc(sizeof(id->valuestring));
    strcpy(toReturn, id->valuestring);
    remove("response.json");
    cJSON_Delete(json);
    return toReturn;
}


char *raid(char *access, char *fromId, char *toId)
{
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://api.twitch.tv/helix/raids?from_broadcaster_id=%s&to_broadcaster_id=%s' "
        "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o response.json", fromId, toId, access, BOT_ID);
    system(tmp);

    FILE *fichier = fopen("response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);

    cJSON *json = cJSON_Parse(tmp);
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        cJSON *msg = cJSON_GetObjectItemCaseSensitive(json, "message");
        char *toReturn = malloc(sizeof(msg->valuestring));
        strcpy(toReturn, msg->valuestring);
        
        cJSON_Delete(json);
        remove("response.json");
        return toReturn;
    }
    return NULL;
}



int main(void) {
    setlocale(LC_ALL, "fr_FR.UTF-8");
    char **tokensInfos = refresh_access_token(DEFAULT_REFRESH_TOKEN);
    long longTmp = atol(tokensInfos[2]);
    sprintf(tokensInfos[2], "%ld", (long)time(NULL) + longTmp);
restart:

    char ***WORDLIST = malloc(sizeof(char **) * 9);
    WORDLIST[0] = (char *[]){"kcx", NULL};
    WORDLIST[1] = (char *[]){"rocket league", "buts", "spacestation", "flip", "spin", "rlcs", "exotiik", "zen", "rl ", " rl", "moist esport", "complexity gaming", NULL};
    WORDLIST[2] = (char *[]){"tft", "canbizz", NULL};
    WORDLIST[3] = (char *[]){"valorant", "vct", "vcl", "vctemea", "redbullhomeground", "game changers", "karmine corp gc", "joueuses", "female", "nelo", "filles", "ninou", "ze1sh", "féminine", "shin", "matriix", NULL};
    WORDLIST[4] = (char *[]){"lol", "league of legends", "div2lol", "emeamasters", "emea masters", "redbull league of its own", "lfl", "lec", "lck", "eu masters", "academy", "karmine corp blue", "movistar riders", "aegis", "bk rog", "eumasters", "ldlc", "vitality bee", "kcb", "hantera", "t1", "faker", "vitality.bee", "cdf", "coupe de france", "botlane", "gold", "eum", "caliste", "saken", NULL};
    WORDLIST[5] = (char *[]){"trackmania", "otaaaq", "bren", NULL};
    WORDLIST[6] = (char *[]){"kurama", NULL};
    WORDLIST[7] = (char *[]){"fncs", NULL};
    WORDLIST[8] = NULL;
    const int CATEGORY_IDS[] = {509663, 30921, 513143, 516575, 21779, 687129551, 504461, 33214};
    char **playlist = getAllFiles();                                             
    char *video = playlist[chooseVideo(size_of_double_array(playlist))];
    unlink("video_fifo");
    mkfifo("video_fifo", 0666);


    char ffmpegCmd[180];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
        "ffmpeg -loglevel debug -re -i video_fifo "
        "-c copy -bufsize 18000k "
        "-f flv rtmp://live.twitch.tv/app/%s > fflog.out 2>&1",
        STREAM_KEY);

    char tmp[256];

    long timestart = (long)time(NULL);
    char *nextVideo = video;

    pthread_t ffmpegThreadId;
    pthread_create(&ffmpegThreadId, NULL, cmdRunInThread, (void *)ffmpegCmd);

    int fifo_fd = open("video_fifo", O_WRONLY);

    system("pip install yt-dlp -U --break-system-packages");
    int isDownloaded = 0;
    if (get_undownloaded_videos())
    {
        pthread_t downloaderThr;
        pthread_create(&downloaderThr, NULL, download_videos, (void *)&isDownloaded);
    }


    while ((long)time(NULL) - timestart < 154000) {
        ffdata data = {video, &fifo_fd};
        pthread_t ffThr;
        pthread_create(&ffThr, NULL, ffwrite, (void *)&data);
        update_stream_info(get_streamer_id(tokensInfos[0], CHANNEL_NAME), tokensInfos[0], CATEGORY_IDS[getGame(video, WORDLIST)], curl_filename(video));
        if(isDownloaded)
        {
            isDownloaded = 0;
            playlist = getAllFiles();
        }
        int endtime = get_video_duration(video) + (int)time(NULL);
        snprintf(tmp, sizeof(tmp), "Now playing %s, estimated end time : %02d:%02d:%02d", video, ((endtime + 3600)/3600)%24, (endtime/60)%60, endtime%60);
        log2file(tmp);
        

        if (atol(tokensInfos[2]) >= (long)time(NULL) + 60)
        {
            revoke_access_token(tokensInfos[0]);
            tokensInfos = refresh_access_token(tokensInfos[1]);
            log2file("refreshed token");
            longTmp = atol(tokensInfos[2]);
            sprintf(tokensInfos[2], "%ld", (long)time(NULL) + longTmp);
        }
        log2file("getting stream info");
        get_stream_info(tokensInfos[0]);
        FILE *streamInfos = fopen("mntr.data", "r");
        char isOnline[8];
        char vwCount[6];
        fgets(isOnline, sizeof(isOnline), streamInfos);
            isOnline[strlen(isOnline) - 1] = '\0';
        fgets(tmp, sizeof(tmp), streamInfos);
        fgets(vwCount, sizeof(vwCount), streamInfos);
        fclose(streamInfos);
        remove("mntr.data");
        /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/ 
                                                                                                                    //
        streamInfos = fopen("/var/www/html/monitoring.json", "w");                                                  //
        cJSON *json = cJSON_CreateObject();                                                                         //
                                                                                                                    //
        cJSON_AddStringToObject(json, "status", isOnline);                                                          //
        cJSON_AddStringToObject(json, "videoTitle", video);                                                         //
        cJSON_AddNumberToObject(json, "videoDuration", (double)get_video_duration(video));                          //
        cJSON_AddNumberToObject(json, "videoStartTime", (double)time(NULL));                                        //
        cJSON_AddNumberToObject(json, "streamStartTime", (double)timestart);                                        //
        cJSON_AddNumberToObject(json, "viewers", (double)atoi(vwCount));                                            //
                                                                                                                    //
        char *json_str = cJSON_Print(json);                                                                         //
        fputs(json_str, streamInfos);                                                                               //
        fclose(streamInfos);                                                                                        //
        cJSON_free(json_str);                                                                                       //
        cJSON_Delete(json);                                                                                         //
                                                                                                                    //
        /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/

        while (video == nextVideo)
            nextVideo = playlist[chooseVideo(size_of_double_array(playlist))];
        video = nextVideo;
        
        handleAPI();
        FILE *fichier = fopen("next.data", "r");
        fgets(tmp, sizeof(tmp), fichier);
        long timeleft = convert_to_timestamp(tmp);


        if (timeleft - (long)time(NULL) <= (long)get_video_duration(video) + 60)     // if next match starts before next video ends (with one more minute)
        {
            log2file("next match will be starting soon, raiding next stream...");
            wait_again:
            char ending[26];
            char next[26];
            char streamer[64];
            fgets(ending, sizeof(ending), fichier);                         // gets timestamp of match ending
            timeleft = convert_to_timestamp(ending);
            fgets(streamer, sizeof(streamer), fichier);                     // gets streamer to raid
                streamer[strlen(streamer) - 1] = '\0';
            fclose(fichier);                                                // end of file is reached, closing file
            snprintf(tmp, sizeof(tmp), "attempting to raid %s...\n", streamer);
            log2file(tmp);
            char *raidValue = raid(tokensInfos[0], get_streamer_id(tokensInfos[0], CHANNEL_NAME), get_streamer_id(tokensInfos[0], streamer));
            if(raidValue)
            {
                snprintf(tmp, sizeof(tmp), "Failed to raid %s: %s", streamer, raidValue);
                log2file(tmp);
            }
            else
            {
                snprintf(tmp, sizeof(tmp), "Successfully raided %s", streamer);
                log2file(tmp);
            }
            sleep(5);
            log2file("Attempting to close stream...");
            system("echo q > ./video_fifo");

            sleep(timeleft - (long)time(NULL));                             // sleep until match ending
            snprintf(tmp, sizeof(tmp), "Waiting %02ld:%02ld:%02ld until next match, estimated next loop starts in %02ld:%02ld:%02ld...", 
                (timeleft - (long)time(NULL))/3600, ((timeleft - (long)time(NULL))/60)%60, (timeleft - (long)time(NULL))%60, 
                timeleft/3600, (timeleft/60)%60, timeleft%60);
            log2file(tmp);
            handleAPI();                                                    // refresh api data
            fichier = fopen("next.data", "r");
            fgets(next, sizeof(next), fichier);                             // gets timestamp of next match
            timeleft = convert_to_timestamp(next);
            close(fifo_fd);
            if (timeleft - (long)time(NULL) < (long)get_video_duration(video))  
                                                    goto wait_again;       // if next match starts in less time than next video duration, wait again
            else                                    goto restart;          // else, restart the stream
        }

        pthread_join(ffThr, NULL);
    }
    log2file("Twitch limit of 48h is almost reached, resetting stream...");
    close(fifo_fd);
    sleep(5);
goto restart;

    return 0;
}
