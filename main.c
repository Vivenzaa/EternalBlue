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
#define DEFAULT_REFRESH_TOKEN "jhpxs3j3pop5spqme9subykt9lvg36i9jlsop783uy966uwkjm"
#define GOOGLE_API_KEY "AIzaSyA_mlDBPL3nTGUiIyonwF6HCnKeHnWdanM"
#define SEED 0xb00b5


void *download_videos(void* bool)
{
    char **videos = file_lines("/var/tmp/Karmine/recentVids");
    remove("/var/tmp/Karmine/recentVids");
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
        log2file(tmp);
        write_metadata(videotmp, fullLink);
        free(fullLink);
        char moov[512];
        snprintf(moov, sizeof(moov), "mv /tmp/Karmine/30784230304235.mp4 \"./%s%s\"", LOCAL_PATH, videotmp);
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
    system("rm -rf /tmp/Karmine/");         system("mkdir /tmp/Karmine");
    system("rm -rf /var/tmp/Karmine");      system("mkdir /var/tmp/Karmine");
    short current_time_offset = 0;
restart:
    current_time_offset = get_utc_offset() * 3600;
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

    long timestart = (long)time(NULL) + current_time_offset;
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


    while (((long)time(NULL) + current_time_offset) - timestart < 157000) {
        ffdata data = {video, &fifo_fd};
        pthread_t ffThr;
        pthread_create(&ffThr, NULL, ffwrite, (void *)&data);
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
        cJSON_AddNumberToObject(json, "videoStartTime", (double)time(NULL) + current_time_offset);                  //
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
        
        char **KCAPIDatas = malloc(sizeof(char *) * 3);
            KarmineAPI_handle(0, &KCAPIDatas);
        char **old_KCAPIDatas = malloc(sizeof(KCAPIDatas));
            memcpy(old_KCAPIDatas, KCAPIDatas, sizeof(char *));

        
        
        long start = convert_to_timestamp(KCAPIDatas[0]);
        long ending = convert_to_timestamp(KCAPIDatas[1]);


        if (start <= (long)get_video_duration(nextVideo, LOCAL_PATH) + (long)endtime + current_time_offset)     // if next match starts before next video ends (with one more minute)
        {
            pthread_join(ffThr, NULL);
            ffdata data = {"./placeholder.mp4", &fifo_fd};
            pthread_create(&ffThr, NULL, ffwrite, (void *)&data);                                               // wait until current video ends
            log2file("next match will be starting soon, raiding next stream...");
            
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
        wait_again:
            ending = convert_to_timestamp(KCAPIDatas[1]);                       // on cherche la fin de l'event
            if (ending < (long)time(NULL) + current_time_offset)              // si on a dépassé la fin
            {
                KarmineAPI_handle(0, &KCAPIDatas);                              // refresh api
                if(memcmp(KCAPIDatas, old_KCAPIDatas, sizeof(char *)) == 0)     // si l'api renvoie les memes valeurs   
                {
                    KarmineAPI_handle(1, &KCAPIDatas);                          // on prends juste l'event suivant
                    ending = convert_to_timestamp(KCAPIDatas[1]);               // on cherche la fin de cet event
                    if (ending < (long)time(NULL) + current_time_offset)      // si c'est dépassé aussi
                    {
                        free(old_KCAPIDatas);
                        free(KCAPIDatas);
                        goto restart;                                           // bah la c'est bourbier on recommence à 0
                    }
                }
            }

            if (ending - (long)time(NULL) - current_time_offset > 20000)      // si il y a plus de 6h d'attente c'est bizarre, on restart
            {
                log2file("Waiting time is above 5h30, assuming an error was made, restarting...");
                free(old_KCAPIDatas);
            }
                
            if (ending - ((long)time(NULL) + current_time_offset) > 0)
            {
                snprintf(tmp, sizeof(tmp), "Waiting %02ld:%02ld:%02ld until next match, estimated next loop starts at %02ld:%02ld:%02ld...", 
                    ((ending - time(NULL) - current_time_offset)/3600) % 24, ((ending - time(NULL))/60)%60, (ending - time(NULL))%60,
                    (ending/3600) % 24, (ending/60)%60, ending%60);
                log2file(tmp);
                sleep(ending - ((long)time(NULL) + current_time_offset) + 10);                             // sleep until match ending
            }
            else
            {
                log2file("Waiting time is negative, restarting...");
                free(old_KCAPIDatas);
                free(KCAPIDatas);
                goto restart;
            }
            
            
            
            KarmineAPI_handle(0, &KCAPIDatas);
            start = convert_to_timestamp(KCAPIDatas[0]);
            long nextVidDuration = (long)get_video_duration(nextVideo, LOCAL_PATH) + current_time_offset + (long)time(NULL);
            
            snprintf(tmp, sizeof(tmp), "start : %ld\tnextVidDuration : %ld", start, nextVidDuration);
            log2file(tmp);

            if (start < nextVidDuration)  
            {
                snprintf(tmp, sizeof(tmp), "Choosing not the restart the stream right now, next match starts at %02ld:%02ld:%02ld while next video would end at %02ld:%02ld:%02ld",
                        (start / 3600) % 24, (start/60)%60, start%60,
                        (nextVidDuration / 3600) % 24, ((nextVidDuration)/60)%60, nextVidDuration%60);
                log2file(tmp);
                goto wait_again;
            }
            else
            {
                log2file("restarting stream...");
                free(old_KCAPIDatas);
                free(KCAPIDatas);
                sleep(3);
                goto restart;          // else, restart the stream
            }
                
        }
        video = nextVideo;
        free(old_KCAPIDatas);
        free(KCAPIDatas);
        pthread_join(ffThr, NULL);

    }
    log2file("Twitch limit of 48h is almost reached, resetting stream...");
    close(fifo_fd);
    sleep(5);
goto restart;

    return 0;
}



/*
    ligne 184 à fix pour pas avoir à utiliser de fichier temporaire intermediaire

    POUR LES PREDICTIONS :
        - on fait une prédiction à chaque début de match sur qui va gagner, et on fetch le resultat dans karmine api pour avoir le résultat de la prédiction


    changer get_app_token pour seulement renvoyer le token, il sera simplement refresh à tous les restart donc blc
    passer web monitoring vers node js
    mettre en place une console par fichier externe pour piloter un peu ce qu'il se passe malgré nohup
    changer les response.json par des fichiers à nom unique (pour éviter les potentiels conflits)
    fix l'interdependance des 2 .h

    un jour : 
        ajouter des arguments au programme
            - --enable-chatbot
            - --enable-monitoring-server
            - --full-fflog
            - --log-to-console
            - --exec-mod (NULL = main, debug(no system()))
            - --log-current-token 
            - --log-retention x hours
            

        coder le terrifiant chatbot twitch
            - commencer par faire communiquer 2 programmes ensemble de façon efficace
            - se demerder jsp jme suis pas encore renseigné
            - IMPORTANT : if (message == Cheap viewers on *)    perma_ban(user);
            - ajouter des arguments :
                - -access ACCESS_TOKEN
                - -refresh REFRESH_TOKEN

            - idées chatbot :
                - !random -> donne une phrase random (#KCWIN, fuck Karnage, prochain match dans hh:mm:ss, ...)
                - !next -> donne hh:mm:ss avant le prochain match
                - !(game) -> prochain match sur (game)
                - !resultats -> affiche tous les resultats des dernières 24h (ou le plus récent)
                - !resultats (jeux) -> affiche les 3 derniers resultats d'un jeu


            - prendre :
                - stream.online
                - stream.offline
                - channel.update
                - channel.follow
                - channel.chat.message
                - channel.chat.message_delete
                - channel.chat_settings.update
                - channel.chat.user_message_hold
                - channel.subscribe
                - channel.ban
                - channel.prediction.* (qui win, ajuster le scope je crois)
                - channel.vip.add
                - channel.vip.remove


            - point à faire gaffe :
                - fermer toutes les sub à la fermeture du programme (trouver un moyen pour les kill)
                - faire gaffe à app_token (refresh toutes les 48h histoire de dire)
                

*/
 