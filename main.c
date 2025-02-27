#include "APIs.h"
#include "utils.h"
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
// #include <signal.h>

#define STREAM_KEY "live_1219233412_x9qHLfK4nDukOO8SFaiRNjqivyFuGh"
#define LOCAL_PATH "videos/"
#define CHANNEL_NAME "Kc_Replays"
#define BOT_ID "kjnngccbj9fcde0r6jc3kv6jyzofqh"
#define BOT_SECRET "ee4iflzar1zoeafxlyacxc2fk4cnwx"
#define DEFAULT_REFRESH_TOKEN "jhpxs3j3pop5spqme9subykt9lvg36i9jlsop783uy966uwkjm"
#define GOOGLE_API_KEY "AIzaSyA_mlDBPL3nTGUiIyonwF6HCnKeHnWdanM"
#define SEED 0xb00b5

void *download_videos(void *bool)
{
    char **videos = file_lines("/var/tmp/Karmine/recentVids");
    remove("/var/tmp/Karmine/recentVids");
    char tmp[256];
    for (int i = size_of_double_array(videos) - 1; i >= 0; i--)
    {
        snprintf(tmp, sizeof(tmp), "Downloading %s...", videos[i]);
        log2file(tmp);

        snprintf(tmp, sizeof(tmp), "mkdir ./%s", videos[i]);
        system(tmp);

        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies-from-browser firefox -r 3000000 -f 299+140 %s%s -P ./%s/", "https://www.youtube.com/watch?v=" + (32 * (videos[i][0] != '-')), videos[i], videos[i]);
        system(tmp);

        char *videoTitle = YTAPI_Get_Video_Name(videos[i], GOOGLE_API_KEY);
        snprintf(tmp, sizeof(tmp), "Successfully downloaded %s : %s", videos[i], videoTitle);
        log2file(tmp);

        char videotmp[strlen(videoTitle) + 5];
        strcpy(videotmp, videoTitle);
        strcat(videotmp, ".mp4");

        snprintf(tmp, sizeof(tmp), "mv ./%s/* \"./%s\"", videos[i], videotmp);
        system(tmp);
        snprintf(tmp, sizeof(tmp), "rmdir ./%s", videos[i]);
        system(tmp);

        char *fullLink = malloc(32 + strlen(videos[i]) + 1);
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

    recur_free(videos);
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

    char cmdFIFO[strlen("ffmpeg -hide_banner -re -i \"\" -c copy -f mpegts -") +
                 strlen(LOCAL_PATH) +
                 strlen(pathfifo->videopath) + 1];

    snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -hide_banner -re -i \"%s%s\" -c copy -f mpegts -", LOCAL_PATH, pathfifo->videopath);

    FILE *ffmpeg = popen(cmdFIFO, "r");
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), ffmpeg)) > 0)
    {
        write(*fifo_fd, buffer, bytesRead);
    }
    pclose(ffmpeg);

    return NULL;
}

void handleArgs(char **argv, int argc, unsigned long *settings, char **knownArgs)
{
    for (int i = 1; i < argc; i++)
    {
        for (unsigned int ii = 0; ii < size_of_double_array(knownArgs); ii++)
        {
            if (!strcmp(argv[i], knownArgs[ii]))
            {
                if (ii >= 6 && ii < size_of_double_array(knownArgs))
                {
                    if (i + 1 >= argc)
                        break;
                    settings[ii] = atoi(argv[i + 1]);
                    i++;
                    break;
                }
                settings[ii] = 1;
                break;
            }
        }
    }
}

void intHandler()
{
    printf("signal\n");
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "fr_FR.UTF-8");
    system("rm -rf /tmp/Karmine/");
    system("mkdir /tmp/Karmine");
    system("rm -rf /var/tmp/Karmine");
    system("mkdir /var/tmp/Karmine");
    short current_time_offset = 0;
    char ***WORDLIST = malloc(sizeof(char **) * 9);
    WORDLIST[0] = (char *[]){"kcx", NULL};
    WORDLIST[1] = (char *[]){"rocket league", "buts", "spacestation", "spin", "rlcs", "exotiik", "zen", "rl ", " rl", "moist esport", "complexity gaming", NULL};
    WORDLIST[2] = (char *[]){"tft", "canbizz", NULL};
    WORDLIST[3] = (char *[]){"valorant", "vct", "vcl", "vctemea", "redbullhomeground", "game changers", "karmine corp gc", "joueuses", "female", "nelo", "filles", "ninou", "ze1sh", "féminine", "shin", "matriix", NULL};
    WORDLIST[4] = (char *[]){"lol", "league of legends", "div2lol", "emeamasters", "emea masters", "redbull league of its own", "lfl", "lec", "lck", "eu masters", "academy", "karmine corp blue", "movistar riders", "aegis", "bk rog", "eumasters", "ldlc", "vitality bee", "kcb", "hantera", "t1", "faker", "vitality.bee", "cdf", "coupe de france", "botlane", "gold", "eum", "caliste", "saken", NULL};
    WORDLIST[5] = (char *[]){"trackmania", "otaaaq", "bren", NULL};
    WORDLIST[6] = (char *[]){"kurama", NULL};
    WORDLIST[7] = (char *[]){"fncs", NULL};
    WORDLIST[8] = NULL;
    const int CATEGORY_IDS[] = {509663, 30921, 513143, 516575, 21779, 687129551, 504461, 33214};
    // isCurrentlyHandled{   True,      False ,                      True,                  True,          False,                  False,           False,         False,         False       False}
    char *knownArgs[] = {"--help", "--enable-chatbot", "--enable-monitoring-server", "--full-fflog", "--log-current-token", "--keep-after-read", "-loglevel", "-logretention", "-dlspeed", "-delay"};
    unsigned long *settings = calloc(sizeof(char *), sizeof(knownArgs) / sizeof(char *));
    handleArgs(argv, argc, settings, knownArgs);
    char *streamer = malloc(8);

    // signal(SIGINT, intHandler);
    // signal(SIGTERM, intHandler);

    if (settings[0])
    {
        printf("list of args : \n"
               "\t--help :                         affiche cette page\n"
               "\t--enable-chatbot :               enables twitch chatbot, prediction handling etc\n"
               "\t--enable-monitoring-server :     enables monitoring via https url\n"
               "\t--full-fflog :                   allow creation of ff.log\n"
               "\t--log-current-token              UNSECURE : logs current access and app token in console.log\n"
               "\t--log-retention x hours          how many time before logs completely delete\n"
               "\t--keep-after-read                keeps videos on the machine after downloading/reading them\n"
               "\t-loglevel [0,1,2]                how precise you want logs to be :\n"
               "\t    0: no logs at all\n"
               "\t    1: only infos\n"
               "\t    2: debug, log everything\n"
               "\t--dlspeed                        specifies the download speed in bytes/s\n");
        exit(0);
    }

    char **tokensInfos = TTV_API_refresh_access_token(DEFAULT_REFRESH_TOKEN, BOT_ID, BOT_SECRET);
restart:
    current_time_offset = get_utc_offset() * 3600;

    char **playlist = getAllFiles(LOCAL_PATH);
    char *video = playlist[chooseVideo(size_of_double_array(playlist), SEED)];
    mkfifo("/tmp/Karmine/video_fifo", 0666);

    char *WantsFFLogs = malloc(strlen("/var/tmp/Karmine/ff.log") * settings[3] + strlen("/dev/null") * (!settings[3]) + 1);
    if (settings[3])
        strcpy(WantsFFLogs, "/var/tmp/Karmine/ff.log");
    else
        strcpy(WantsFFLogs, "/dev/null");
    char ffmpegCmd[179 + strlen(WantsFFLogs)];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
             "ffmpeg -hide_banner -loglevel debug -re -i /tmp/Karmine/video_fifo "
             "-c copy -bufsize 18000k "
             "-f flv rtmp://live.twitch.tv/app/%s > %s 2>&1",
             STREAM_KEY, WantsFFLogs);

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
    while (((long)time(NULL) + current_time_offset) - timestart < 157000)
    {
        ffdata data = {video, &fifo_fd};
        pthread_t ffThr;
        pthread_create(&ffThr, NULL, ffwrite, (void *)&data);
        tokensInfos = TTV_API_refresh_access_token(tokensInfos[1], BOT_ID, BOT_SECRET);
        TTV_API_update_stream_info(TTV_API_get_streamer_id(tokensInfos[0], CHANNEL_NAME, BOT_ID), tokensInfos[0], CATEGORY_IDS[getGame(video, WORDLIST)], curl_filename(video), BOT_ID);
        if (isDownloaded)
        {
            isDownloaded = 0;
            playlist = getAllFiles(LOCAL_PATH);
        }
        int endtime = get_video_duration(video, LOCAL_PATH) + (int)time(NULL);
        snprintf(tmp, sizeof(tmp), "Now playing %s, estimated end time : %02d:%02d:%02d", video, ((endtime + 3600) / 3600) % 24, (endtime / 60) % 60, endtime % 60);
        log2file(tmp);

        if (settings[2])
        {
            /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/
            log2file("web_monitoring: getting self stream infos...");                                      //
            TTV_API_get_stream_info(tokensInfos[0], CHANNEL_NAME, BOT_ID);                                 //
            FILE *streamInfos = fopen("/var/tmp/Karmine/mntr.data", "r");                                  //
            char isOnline[8];                                                                              //
            char vwCount[6];                                                                               //
            fgets(isOnline, sizeof(isOnline), streamInfos);                                                //
            isOnline[strlen(isOnline) - 1] = '\0';                                                         //
            fgets(tmp, sizeof(tmp), streamInfos);                                                          //
            fgets(vwCount, sizeof(vwCount), streamInfos);                                                  //
            fclose(streamInfos);                                                                           //
            remove("/var/tmp/Karmine/mntr.data");                                                          //
                                                                                                           //
            streamInfos = fopen("/var/www/html/monitoring.json", "w");                                     //
            cJSON *json = cJSON_CreateObject();                                                            //
                                                                                                           //
            cJSON_AddStringToObject(json, "status", isOnline);                                             //
            cJSON_AddStringToObject(json, "videoTitle", video);                                            //
            cJSON_AddNumberToObject(json, "videoDuration", (double)get_video_duration(video, LOCAL_PATH)); //
            cJSON_AddNumberToObject(json, "videoStartTime", (double)time(NULL) + current_time_offset);     //
            cJSON_AddNumberToObject(json, "streamStartTime", (double)timestart);                           //
            cJSON_AddNumberToObject(json, "viewers", (double)atoi(vwCount));                               //
                                                                                                           //
            char *json_str = cJSON_Print(json);                                                            //
            fputs(json_str, streamInfos);                                                                  //
            fclose(streamInfos);                                                                           //
            cJSON_free(json_str);                                                                          //
            cJSON_Delete(json);                                                                            //
            /*------------------------------------------HANDLE WEB MONITORING------------------------------------------*/
        }

        while (video == nextVideo)
            nextVideo = playlist[chooseVideo(size_of_double_array(playlist), SEED)];

        
        int KarmineToWait = KarmineAPI_timeto(streamer, time(NULL) + current_time_offset, 1);
        if (KarmineToWait <= (long)get_video_duration(nextVideo, LOCAL_PATH) + endtime + current_time_offset) // if next match starts before next video ends (with one more minute)
        {
            pthread_join(ffThr, NULL);
            ffdata data = {"./placeholder.mp4", &fifo_fd};
            pthread_create(&ffThr, NULL, ffwrite, (void *)&data); // wait until current video ends
            log2file("next match will be starting soon, raiding next stream...");

            snprintf(tmp, sizeof(tmp), "attempting to raid %s...", streamer);
            log2file(tmp);
            char *raidValue = TTV_API_raid(tokensInfos[0], TTV_API_get_streamer_id(tokensInfos[0], CHANNEL_NAME, BOT_ID), TTV_API_get_streamer_id(tokensInfos[0], streamer, BOT_ID), BOT_ID);

            if (raidValue)
                snprintf(tmp, sizeof(tmp), "Failed to raid %s: %s", streamer, raidValue);
            else
                snprintf(tmp, sizeof(tmp), "Successfully raided %s", streamer);

            free(raidValue);
            log2file(tmp);
            log2file("Attempting to close stream...");
            pthread_join(ffThr, NULL);
            system("echo q > /tmp/Karmine/video_fifo");
            close(fifo_fd);
        wait_again:
            KarmineToWait = KarmineAPI_timeto(streamer, time(NULL) + current_time_offset, 0);
            if (KarmineToWait > 20000)          // si il y a plus de 6h d'attente c'est bizarre, on restart
            {
                log2file("Waiting time is above 5h30, assuming an error was made, restarting...");
                goto restart;
            }

            snprintf(tmp, sizeof(tmp), "Waiting %02d:%02d:%02d until next match, estimated next loop starts at %02d:%02d:%02d...",
                     (KarmineToWait / 3600) % 24, (KarmineToWait / 60) % 60, KarmineToWait % 60,
                     ((KarmineToWait + (int)time(NULL) + current_time_offset) / 3600) % 24, ((KarmineToWait + (int)time(NULL) + current_time_offset) / 60) % 60, (KarmineToWait + (int)time(NULL) + current_time_offset) % 60);
            log2file(tmp);
            sleep(KarmineToWait + 60); // sleep until match endingimages/
            

            KarmineToWait = KarmineAPI_timeto(streamer, time(NULL) + current_time_offset, 1);
            int nextVidDuration = get_video_duration(nextVideo, LOCAL_PATH);

            if (nextVidDuration > KarmineToWait)
            {
                snprintf(tmp, sizeof(tmp), "Choosing not the restart the stream right now, next match starts at %02d:%02d:%02d while next video would end at %02d:%02d:%02d",
                         ((KarmineToWait + (int)time(NULL) + current_time_offset) / 3600) % 24, ((KarmineToWait + (int)time(NULL) + current_time_offset) / 60) % 60, (KarmineToWait + (int)time(NULL) + current_time_offset) % 60,
                         ((nextVidDuration + (int)time(NULL) + current_time_offset) / 3600) % 24, ((nextVidDuration + (int)time(NULL) + current_time_offset) / 60) % 60, (nextVidDuration + (int)time(NULL) + current_time_offset) % 60);
                log2file(tmp);
                goto wait_again;
            }
            else
            {
                log2file("found no blocking condition, restarting stream...");
                goto restart; // else, restart the stream
            }
        }
        video = nextVideo;
        pthread_join(ffThr, NULL);
    }
    if (((long)time(NULL) + current_time_offset) - timestart < 157000)
        log2file("Twitch limit of 48h is almost reached, resetting stream...");
    close(fifo_fd);
    sleep(10);
    goto restart;

    printf("alors ça, c'est pas normal DU TOUT\n");
    return 0;
}

/*
    TEST APPRONFONDI À FAIRE SUR get_video_duration(), prendre la return value actuelle du server

    online.c (full http):
        - execution normale sous js avec redirection vers c pour garder des logs et gérer les requepetes plus tard
        - assurer la bonne connection entre js et online
        - intégrer cJSON
        - ajouter un descripteur aux requêtes pour savoir ce qui a été trigger
        - créer les fonctions qui vont avec
        - créer le segment partagé entre les 2 programmes (le plus tard possible aled)
    OU ALORS je casse tout pour en faire un server HTTP et me passer du JS directement


    // quand j'ai vla le temps pck bon blc : ligne 184 à fix pour pas avoir à utiliser de fichier temporaire intermediaire
    trouver une putain de vidéo de transition merde
    revoir le système de logs en vue du changement vav des arguments de lancement
    utiliser des sockets (ou autre ?) pour échanger avec le server.js (potentiellement se débarraser sur js plus tard ? )

    POUR LES PREDICTIONS :
        - on fait une prédiction à chaque début de match sur qui va gagner, et on fetch le resultat dans karmine api pour avoir le résultat de la prédiction


    changer get_app_token pour seulement renvoyer le token, il sera simplement refresh à tous les restart donc blc
    passer web-monitoring sur le server c
    mettre en place une console par fichier externe (ou programme supplémentaire avec liaison mémoire ?) pour piloter un peu ce qu'il se passe malgré nohup (controler via commandes stream ?)
    changer les response.json par des fichiers à nom unique (pour éviter les potentiels conflits)

    un jour :
        coder le terrifiant chatbot twitch
            - commencer par faire communiquer 2 programmes ensemble de façon efficace
            - IMPORTANT : if (message == Cheap viewers on *)    perma_ban(user);
            - faire en sorte de pouvoir taper des commandes dans le chat pour commander le bot (verifier les user_id pour voir si t'es autorisé à rentrer la commande)
                - shutdown              shutdown after current video ends
                - force shutdown        stays down until "start"
                - restart               restart on a new video, can be done while playing something
                - pause                 pauses current video until "play"
                - play                  plays the current paued video
                - select video          select next video to play, if current video is paused, restart on this one

            - idées chatbot :
                - !random -> donne une phrase random (#KCWIN, fuck Karnage, prochain match dans hh:mm:ss, ...)
                - !next -> donne hh:mm:ss avant le prochain match
                - !(game) -> prochain match sur (game)
                - !resultats -> affiche tous les resultats des dernières 24h (ou le plus récent)
                - !resultats (jeux) -> affiche les 3 derniers resultats d'un jeu
                - !game (jeu) -> associe jeu à la BD pour cette vidéo si jamais aucun jeu ne lui est associé


            - point à faire gaffe :
                - fermer toutes les sub à la fermeture du programme (utiliser signal.h)
                - faire gaffe à app_token (refresh toutes les 48h histoire de dire)

        utiliser une db pour associer des infos aux vidéos
            - le jeux qui va avec
            - potentiellement qui a gagné (?)

    implémenter les arguments correctement
*/
