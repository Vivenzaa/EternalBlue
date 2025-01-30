#include    "APIs.h"
#include    "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cjson/cJSON.h>
#include <time.h>

#define STREAM_KEY "live_1219233412_x9qHLfK4nDukOO8SFaiRNjqivyFuGh"
#define LOCAL_PATH "videos/"
#define CHANNEL_NAME "Kc_Replays"
#define BOT_ID "kjnngccbj9fcde0r6jc3kv6jyzofqh"
#define BOT_SECRET "ee4iflzar1zoeafxlyacxc2fk4cnwx"
#define DEFAULT_REFRESH_TOKEN "svs3ozdcp34txn80ons3iztukfj1uitembkzobnn6xfoyrbei0"
#define GOOGLE_API_KEY "AIzaSyA_mlDBPL3nTGUiIyonwF6HCnKeHnWdanM"
#define SEED 0xb00b5


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

        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies-from-browser firefox -r 3000000 -f 299+140 %s -P ./%s/", videos[i], videos[i]);   // -r 6MB
        system(tmp);

        char *videoTitle = YTAPI_Get_Video_Name(videos[i], GOOGLE_API_KEY);
        snprintf(tmp, sizeof(tmp), "Successfully downloaded %s : %s", videos[i], videoTitle);
        log2file(tmp);

        
        char videotmp[strlen(videoTitle) + 5];
        strcpy(videotmp, videoTitle);
        strcat(videotmp, ".mp4");

        snprintf(tmp, sizeof(tmp), "mv %s/* \"./%s\"", videos[i], videotmp);
        system(tmp);
        snprintf(tmp, sizeof(tmp), "rmdir %s", videos[i]);
        system(tmp);

        char *fullLink = malloc(strlen("https://www.youtube.com/watch?v=") + strlen(videos[i]) + 1);
        strcpy(fullLink, "https://www.youtube.com/watch?v=");
        strcat(fullLink, videos[i]);

        snprintf(tmp, sizeof(tmp), "writing metadata to %s", videotmp);
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



int main(void) {
    setlocale(LC_ALL, "fr_FR.UTF-8");
    char **tokensInfos = TTV_API_refresh_access_token(DEFAULT_REFRESH_TOKEN, BOT_ID, BOT_SECRET);
    system("rm -rf /tmp/Karmine/");      system("mkdir /tmp/Karmine");
    system("rm -rf /var/tmp/Karmine");  system("mkdir /var/tmp/Karmine");
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
    char **playlist = getAllFiles(LOCAL_PATH);                                           
    char *video = playlist[chooseVideo(size_of_double_array(playlist), SEED)];
    mkfifo("/tmp/Karmine/video_fifo", 0666);


    char ffmpegCmd[189];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
        "ffmpeg -loglevel debug -re -i /tmp/Karmine/video_fifo "
        "-c copy -bufsize 18000k "
        "-f flv rtmp://live.twitch.tv/app/%s > /var/tmp/Karmine/ff.log 2>&1",
        STREAM_KEY);

    char tmp[256];

    long timestart = (long)time(NULL) + 3600*get_utc_offset();
    char *nextVideo = video;

    pthread_t ffmpegThreadId;
    pthread_create(&ffmpegThreadId, NULL, cmdRunInThread, (void *)ffmpegCmd);

    int fifo_fd = open("/tmp/Karmine/video_fifo", O_WRONLY);

    system("pip install yt-dlp -U --break-system-packages");
    int isDownloaded = 0;
    if (get_undownloaded_videos(LOCAL_PATH, GOOGLE_API_KEY))
    {
        pthread_t downloaderThr;
        pthread_create(&downloaderThr, NULL, download_videos, (void *)&isDownloaded);
    }


    while (((long)time(NULL) + 3600*get_utc_offset()) - timestart < 157000) {
        ffdata data = {video, &fifo_fd};
        pthread_t ffThr;
        pthread_create(&ffThr, NULL, ffwrite, (void *)&data);
        int currentVideoEndtime = (int)time(NULL)+ get_video_duration(video, LOCAL_PATH);
        tokensInfos = TTV_API_refresh_access_token(tokensInfos[1], BOT_ID, BOT_SECRET);
        TTV_API_update_stream_info(TTV_API_get_streamer_id(tokensInfos[0], CHANNEL_NAME, BOT_ID), tokensInfos[0], CATEGORY_IDS[getGame(video, WORDLIST)], curl_filename(video), BOT_ID);
        if(isDownloaded)
        {
            isDownloaded = 0;
            playlist = getAllFiles(LOCAL_PATH);
        }
        int endtime = get_video_duration(video, LOCAL_PATH) + (int)time(NULL);
        snprintf(tmp, sizeof(tmp), "Now playing %s, estimated end time : %02d:%02d:%02d", video, ((endtime + 3600)/3600)%24, (endtime/60)%60, endtime%60);
        log2file(tmp);
        

        log2file("web_monitoring: getting self stream infos...");
        TTV_API_get_stream_info(tokensInfos[0], CHANNEL_NAME, BOT_ID);
        FILE *streamInfos = fopen("/var/tmp/Karmine/mntr.data", "r");
        char isOnline[8];
        char vwCount[6];
        fgets(isOnline, sizeof(isOnline), streamInfos);
            isOnline[strlen(isOnline) - 1] = '\0';
        fgets(tmp, sizeof(tmp), streamInfos);
        fgets(vwCount, sizeof(vwCount), streamInfos);
        fclose(streamInfos);
        remove("/var/tmp/Karmine/mntr.data");
        /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/ 
                                                                                                                    //
        streamInfos = fopen("/var/www/html/monitoring.json", "w");                                                  //
        cJSON *json = cJSON_CreateObject();                                                                         //
                                                                                                                    //
        cJSON_AddStringToObject(json, "status", isOnline);                                                          //
        cJSON_AddStringToObject(json, "videoTitle", video);                                                         //
        cJSON_AddNumberToObject(json, "videoDuration", (double)get_video_duration(video, LOCAL_PATH));              //
        cJSON_AddNumberToObject(json, "videoStartTime", (double)time(NULL) + 3600*get_utc_offset());                //
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
            nextVideo = playlist[chooseVideo(size_of_double_array(playlist), SEED)];
        
        char **KCAPIDatas = KarmineAPI_handle();
        
        long start = convert_to_timestamp(KCAPIDatas[0]);
        long ending = 0;
        
        if (start <= (long)get_video_duration(nextVideo, LOCAL_PATH) + currentVideoEndtime)     // if next match starts before next video ends (with one more minute)
        {
            pthread_join(ffThr, NULL);
            ffdata data = {"./placeholder.mp4", &fifo_fd};
            pthread_create(&ffThr, NULL, ffwrite, (void *)&data);                                               // wait until current video ends
            log2file("next match will be starting soon, raiding next stream...");
            wait_again:
            
            ending = convert_to_timestamp(KCAPIDatas[1]);
            snprintf(tmp, sizeof(tmp), "attempting to raid %s...", KCAPIDatas[2]);
            log2file(tmp);
            char *raidValue = TTV_API_raid(tokensInfos[0], TTV_API_get_streamer_id(tokensInfos[0], CHANNEL_NAME, BOT_ID), TTV_API_get_streamer_id(tokensInfos[0], KCAPIDatas[2], BOT_ID), BOT_ID);
            
            if(raidValue)
                snprintf(tmp, sizeof(tmp), "Failed to raid %s: %s", KCAPIDatas[2], raidValue);
            else
                snprintf(tmp, sizeof(tmp), "Successfully raided %s", KCAPIDatas[2]);
            
            log2file(tmp);
            log2file("Attempting to close stream...");
            pthread_join(ffThr, NULL);
            system("echo q > /tmp/Karmine/video_fifo");
            close(fifo_fd);

            snprintf(tmp, sizeof(tmp), "Waiting %02ld:%02ld:%02ld until next match, estimated next loop starts at %02ld:%02ld:%02ld...", 
                    ((ending - time(NULL))/3600) % 24, ((ending - time(NULL))/60)%60, (ending - time(NULL))%60,
                    (ending/3600) % 24, (ending/60)%60, ending%60);
            log2file(tmp);
            if (ending - ((long)time(NULL) + 3600*get_utc_offset()) > 0)
                sleep(ending - ((long)time(NULL) + 3600*get_utc_offset()));                             // sleep until match ending

            KCAPIDatas = KarmineAPI_handle();      
            start = convert_to_timestamp(KCAPIDatas[0]);
            long nextVidDuration = (long)get_video_duration(nextVideo, LOCAL_PATH) + 3600*get_utc_offset() + (long)time(NULL);
            
            if (start < nextVidDuration)  
            {
                snprintf(tmp, sizeof(tmp), "Choosing not the restart the stream right now, next match starts at %02ld:%02ld:%02ld while next video would end at %02ld:%02ld:%02ld",
                        (start / 3600) % 24, (start/60)%60, start%60,
                        (nextVidDuration / 3600) % 24, ((nextVidDuration)/60)%60, nextVidDuration%60);
                fifo_fd = open("/tmp/Karmine/video_fifo", O_WRONLY);      // if next match starts in less time than next video duration, wait again
                goto wait_again;
            }
            else
            {
                log2file("restarting stream...");
                goto restart;          // else, restart the stream
            }
                
        }
        video = nextVideo;
        pthread_join(ffThr, NULL);
    }
    log2file("Twitch limit of 48h is almost reached, resetting stream...");
    close(fifo_fd);
    sleep(5);
goto restart;

    return 0;
}



/*
    changer random par /dev/random (ok) || hash logs
    passer web monitoring vers node js

    un jour : 
        ajouter des arguments au programme
            - --enable-chatbot
            - --enable-monitoring-server
            - --full-fflog
            - --exec-mod (NULL = main, debug(no system()))
            

        coder le terrifiant chatbot twitch
            - commencer par faire communiquer 2 programmes ensemble de façon efficace
            - faire un serveur https pour prendre les requêtes twitch (nodeJS ?)
            - se demerder jsp jme suis pas encore renseigné
            - IMPORTANT : if (message == Cheap viewers on *)    perma_ban(user);
            - ajouter des arguments :
                - -access ACCESS_TOKEN
                - -refresh REFRESH_TOKEN

*/
 