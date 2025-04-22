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
#include <errno.h>
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
    char **videos = file_lines("/tmp/Karmine/recentVids");
    remove("/tmp/Karmine/recentVids");
    char tmp[256];
    for (int i = size_of_double_array(videos) - 1; i >= 0; i--)
    { 
        log4c("Downloading %s...", videos[i]);

        system("mkdir /tmp/Karmine/ytdlp");

        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies-from-browser firefox -r 3000000 -f 299+140 %s%s -P /tmp/Karmine/ytdlp/", "https://www.youtube.com/watch?v=" + (32 * (videos[i][0] != '-')), videos[i]);
        int ret = system(tmp);
        if(ret)
        {
            log4c("yt-dlp couldn't download specified video, trying to resolve...");
            system("pip install yt-dlp -U --break-system-packages");
            snprintf(tmp, sizeof(tmp), "yt-dlp --cookies-from-browser firefox -r 3000000 -f 137+140 %s%s -P /tmp/Karmine/ytdlp/", "https://www.youtube.com/watch?v=" + (32 * (videos[i][0] != '-')), videos[i]);
            system(tmp);
        }

        char *videoTitle = YTAPI_Get_Video_Name(videos[i], GOOGLE_API_KEY);
        log4c("Successfully downloaded %s : %s", videos[i], videoTitle);

        char videotmp[strlen(videoTitle) + 5];
        strcpy(videotmp, videoTitle);
        strcat(videotmp, ".mp4");
        free(videoTitle);

        snprintf(tmp, sizeof(tmp), "mv /tmp/Karmine/ytdlp/* \"/tmp/Karmine/%s\"", videotmp);
        system(tmp);
        system("rm -r /tmp/Karmine/ytdlp");

        char *fullLink = malloc(32 + strlen(videos[i]) + 1);
        strcpy(fullLink, "https://www.youtube.com/watch?v=");
        strcat(fullLink, videos[i]);

        log4c("writing metadata to %s", videotmp);

        snprintf(tmp, sizeof(tmp), "/tmp/Karmine/%s", videotmp);
        write_metadata(tmp, fullLink);
        free(fullLink);
        char moov[512];
        snprintf(moov, sizeof(moov), "mv /tmp/Karmine/30784230304235.mp4 \"./%s%s\"", LOCAL_PATH, videotmp);
        system(moov);
    }

    recur_free(videos);
    log4c("Successfully downloaded all recent videos.");
    int *a = (int *)bool;
    *a = 1;
    bool = (void *)a;

    return bool;
}


void *ffwrite(void *data)
{
    ffdata *pathfifo = (ffdata *)data;

    char cmdFIFO[strlen("ffmpeg -hide_banner -re -i \"\" -c copy -f mpegts -") +
                 strlen(LOCAL_PATH) +
                 strlen(pathfifo->videopath) + 1];

    snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -hide_banner -re -i \"%s%s\" -c copy -f mpegts -", LOCAL_PATH, pathfifo->videopath);

    log4c("ffwrite: launching command: %s", cmdFIFO);

    FILE *ffmpeg = popen(cmdFIFO, "r");
    if (!ffmpeg) {
        log4c("ffwrite: popen failed: %s", strerror(errno));
        free(data);
        return NULL;
    }

    log4c("ffwrite: ffmpeg process opened");

    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), ffmpeg)) > 0) {
        ssize_t written = write(*pathfifo->fifo, buffer, bytesRead);
        if (written == -1) {
            log4c("ffwrite: write failed: %s", strerror(errno));
            break;
        } else if ((size_t)written != bytesRead) {
            log4c("ffwrite: partial write (%zd/%zu bytes)", written, bytesRead);
        }
    }

    if (ferror(ffmpeg)) {
        log4c("ffwrite: fread encountered an error: %s", strerror(errno));
    }

    int pclose_status = pclose(ffmpeg);
    if (pclose_status == -1) {
        log4c("ffwrite: pclose failed: %s", strerror(errno));
    } else {
        log4c("ffwrite: ffmpeg process closed with status: %d", pclose_status);
    }

    free(data);
    log4c("ffwrite: finished execution and freed memory");

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

    char *knownArgs[] = {"--help", "--enable-chatbot", "--enable-monitoring-server", "--full-fflog", "--log-current-token", "--keep-after-read", "-loglevel", "-logretention", "-dlspeed", "-delay"};
    unsigned long *settings = calloc(sizeof(char *), sizeof(knownArgs) / sizeof(char *));
    handleArgs(argv, argc, settings, knownArgs);

    if (settings[0])
    {
        printf("list of args : \n"
               "\t--help :                         affiche cette page\n"
               //"\t--enable-chatbot :               enables twitch chatbot, prediction handling etc\n"
               "\t--enable-monitoring-server :     enables monitoring via https url\n"
               "\t--full-fflog :                   allow creation of ff.log\n"
               //"\t--log-current-token              UNSECURE : logs current access and app token in console.log\n"
               //"\t--log-retention x hours          how many time before logs completely delete\n"
               //"\t--keep-after-read                keeps videos on the machine after downloading/reading them\n"
               //"\t-loglevel [0,1,2]                how precise you want logs to be :\n"
               //"\t    0: no logs at all\n"
               //"\t    1: only infos\n"
               //"\t    2: debug, log everything\n"
               //"\t--dlspeed                        specifies the download speed in bytes/s\n"
               );
        exit(0);
    }

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
    WORDLIST[4] = (char *[]){"lol", "league of legends", "div2lol", "emeamasters", "emea masters", "first stand", "redbull league of its own", "lfl", "lec", "lck", "eu masters", "academy", "karmine corp blue", "movistar riders", "aegis", "bk rog", "eumasters", "ldlc", "vitality bee", "kcb", "hantera", "t1", "faker", "vitality.bee", "cdf", "coupe de france", "botlane", "gold", "eum", "caliste", "saken", NULL};
    WORDLIST[5] = (char *[]){"trackmania", "otaaaq", "bren", NULL};
    WORDLIST[6] = (char *[]){"kurama", NULL};
    WORDLIST[7] = (char *[]){"fncs", NULL};
    WORDLIST[8] = NULL;
    const int CATEGORY_IDS[] = {509663, 30921, 513143, 516575, 21779, 687129551, 504461, 33214};
    
    

    char *streamer = malloc(8);
    char *channel_name = CHANNEL_NAME;
    char access_token[31];
    char refresh_token[51] = DEFAULT_REFRESH_TOKEN;
    TTV_API_refresh_access_token(BOT_ID, BOT_SECRET, refresh_token, access_token);
    char *streamer_channel_id = TTV_API_get_user_id(access_token, channel_name, BOT_ID);
    char tmp[512];

    // signal(SIGINT, intHandler);
    // signal(SIGTERM, intHandler);

    
    
restart:
    current_time_offset = get_utc_offset() * 3600;
    
    char **playlist = getAllFiles(LOCAL_PATH);
    char *WantsFFLogs = malloc(strlen("/var/tmp/Karmine/ff.log") * settings[3]      +       strlen("/dev/null") * (!settings[3]) + 1);
    char *video = playlist[chooseVideo(size_of_double_array(playlist), SEED)];

    mkfifo("/tmp/Karmine/video_fifo", 0666);
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
    //OPTIMIZATION CAN BE DONE HERE
    free(WantsFFLogs);


    long timestart = time(NULL) + current_time_offset;
    char *nextVideo = video;

    pthread_t ffmpegThreadId;
    pthread_create(&ffmpegThreadId, NULL, cmdRunInThread, (void *)ffmpegCmd);

    int fifo_fd = open("/tmp/Karmine/video_fifo", 01);      // O_WRONLY

    log4c("checking for yt-dlp update...");
    system("pip install yt-dlp -U --break-system-packages");
    int isDownloaded = 0;
    if (get_undownloaded_videos(LOCAL_PATH, GOOGLE_API_KEY))
    {
        pthread_t downloaderThr;
        pthread_create(&downloaderThr, NULL, download_videos, (void *)&isDownloaded);
    }
    while (time(NULL) + current_time_offset - timestart < 157000)
    {
        ffdata *data = malloc(sizeof(ffdata));
        data->videopath = video;
        data->fifo = &fifo_fd;
        //{video, &fifo_fd};
        pthread_t ffThr;
        pthread_create(&ffThr, NULL, ffwrite, (void *)data);
        TTV_API_refresh_access_token(BOT_ID, BOT_SECRET, refresh_token, access_token);
            
        
        char *streamer_id = TTV_API_get_user_id(access_token, CHANNEL_NAME, BOT_ID);
        char *curl_name = curl_filename(video);
        if (curl_name == NULL)
            TTV_API_update_stream_info(streamer_id, access_token, CATEGORY_IDS[getGame(video, WORDLIST)], video, BOT_ID);
        else
        {
            TTV_API_update_stream_info(streamer_id, access_token, CATEGORY_IDS[getGame(video, WORDLIST)], curl_name, BOT_ID);
            free(curl_name);
        }
        free(streamer_id);

        if (isDownloaded)
        {
            isDownloaded = 0;
            recur_free(playlist);
            playlist = getAllFiles(LOCAL_PATH);
        }
        int endtime = get_video_duration(video, LOCAL_PATH) + (int)time(NULL) + current_time_offset;
        snprintf(tmp, sizeof(tmp), "Now playing %s, estimated end time : %02d:%02d:%02d", video, (endtime / 3600) % 24, (endtime / 60) % 60, endtime % 60);
        log4c(tmp);

        if (settings[2])
        {
            /*------------------------------------HANDLE WEB MONITORING------------------------------------*/
            log4c("web_monitoring: getting self stream infos...");                                      //
            TTV_API_get_stream_info(access_token, CHANNEL_NAME, BOT_ID);                                 //
            FILE *streamInfos = fopen("/tmp/Karmine/mntr.data", "r");                                      //
            char isOnline[8];                                                                              //
            char vwCount[6];                                                                               //
            fgets(isOnline, sizeof(isOnline), streamInfos);                                                //
            isOnline[strlen(isOnline) - 1] = '\0';                                                         //
            fgets(tmp, sizeof(tmp), streamInfos);                                                          //
            fgets(vwCount, sizeof(vwCount), streamInfos);                                                  //
            fclose(streamInfos);                                                                           //
            remove("/tmp/Karmine/mntr.data");                                                              //
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
            /*------------------------------------HANDLE WEB MONITORING------------------------------------*/
        }

        while (video == nextVideo)
            nextVideo = playlist[chooseVideo(size_of_double_array(playlist), SEED)];
    
        int KarmineToWait = KarmineAPI_timeto(&streamer, (int)time(NULL) + current_time_offset, 1);
        if (KarmineToWait + (int)time(NULL) + current_time_offset <= (int)get_video_duration(nextVideo, LOCAL_PATH) + endtime) // if next match starts before next video ends (with one more minute)
        {
            pthread_join(ffThr, NULL); // wait until current video ends
            ffdata *data = malloc(sizeof(ffdata));
            data->videopath = "./placeholder.mp4";
            data->fifo = &fifo_fd;
            pthread_create(&ffThr, NULL, ffwrite, (void *)data); 
            log4c("next match will be starting soon, attempting to raid %s...", streamer);

            char *to_id = TTV_API_get_user_id(access_token, streamer, BOT_ID);
            char *raidValue = TTV_API_raid(access_token, streamer_channel_id, to_id, BOT_ID);
            
            free(to_id);

            if (raidValue)
                log4c("Failed to raid %s: %s", streamer, raidValue);
            else
                log4c("Successfully raided %s", streamer);

            free(raidValue);
            log4c("Attempting to close stream...");
            pthread_join(ffThr, NULL);
            system("echo q > /tmp/Karmine/video_fifo");
            close(fifo_fd);
        wait_again:
            KarmineToWait = KarmineAPI_timeto(&streamer, time(NULL) + current_time_offset, 0);
            if (KarmineToWait > 20000)          // si il y a plus de 6h d'attente c'est bizarre, on restart
            {
                log4c("Waiting time is above 5h30, assuming an error was made, restarting...");
                recur_free(playlist);
                goto restart;
            }

            snprintf(tmp, sizeof(tmp), "Waiting %02d:%02d:%02d until next match, estimated next loop starts at %02d:%02d:%02d...",
                     (KarmineToWait / 3600) % 24, (KarmineToWait / 60) % 60, KarmineToWait % 60,
                     ((KarmineToWait + (int)time(NULL) + current_time_offset) / 3600) % 24, ((KarmineToWait + (int)time(NULL)) / 60) % 60, (KarmineToWait + (int)time(NULL)) % 60);
            log4c(tmp);
            sleep(KarmineToWait + 10);
            

            KarmineToWait = KarmineAPI_timeto(&streamer, time(NULL) + current_time_offset, 1);
            int nextVidDuration = get_video_duration(nextVideo, LOCAL_PATH);

            if (nextVidDuration > KarmineToWait)
            {
                snprintf(tmp, sizeof(tmp), "Choosing not the restart the stream right now, next match starts at %02d:%02d:%02d while next video would end at %02d:%02d:%02d",
                        ((KarmineToWait + (int)time(NULL) + current_time_offset) / 3600) % 24, ((KarmineToWait + (int)time(NULL)) / 60) % 60, (KarmineToWait + (int)time(NULL)) % 60,
                        ((nextVidDuration + (int)time(NULL) + current_time_offset) / 3600) % 24, ((nextVidDuration + (int)time(NULL)) / 60) % 60, (nextVidDuration + (int)time(NULL)) % 60);
                log4c(tmp);
                goto wait_again;
            }
            else
            {
                log4c("found no blocking condition, restarting stream...");
                recur_free(playlist);
                goto restart; // else, restart the stream
            }
        }
        video = nextVideo;
        pthread_join(ffThr, NULL);
    }
    
    log4c("Twitch limit of 48h is almost reached, resetting stream...");
    close(fifo_fd);
    sleep(10);
    recur_free(playlist);
    goto restart;

    return 0;
}






/*
    passer par popen pour éviter la création de fichiers temporaires pas super utiles

    FIRST LAUNCH :
        - if no creds.json is found, prompt for manual API KEYs registering via stdin
        - allow for argument --configure to review/change keys
        - at every launch, search for api keys by searching both creds.json and config file 


    REPUBLICATION DU CODE:
        - mettre en place un fichier .cred à lire pour les token API -> pour éviter de tout re leak mdrr
        - changer les clés API au cas ou 
        - cleanup le code 
        - choisir entre anglais et français (pck bon le mélange des 2 ça va 5m)
        - lancer la v1 sans les server.c, chatbot.c 
        
        Q:
            - est ce que le server.c doit etre packagé avec le reste ? il y aura du code privé dedans (radio) 
                et meme il y a du code franchement pas très utile pour le projet 
            - est ce qu'il faut proposer une alternative d'hebergement du code chatbot.c avec un micro httpd ? 
            - est ce que chatbot.c doit etre modulable avec launch argument 
                (ex: --own-host pour court circuiter server.c ou -host STRING pour rediriger vers un micro httpd ou Apache ?)
            


    DONNER PLUS D'IMPORTANCE AU SERVEUR HTTP CUSTOM :
        - gérer l'arborescence (url /, /webhook, /monitoring, /radio(?) etc)
            = encapsuler le /webhook
                - créer un fichier server.c qui va gérer les fonctions de base du serveur
                - créer un fichier chatbot.c qui va se greffer au server.c
                - cahier des charges server.c :
                    - arborescence standardisée (répertoire public)
                    - doit etre super facilement modulable (pouvoir ajouter des urls facilement)
                    - doit comporter une API à clé pour les fonctions sensibles (controle du chatbot/main et de la radio)
                    - pour chaque URL supporté, collecter les informations, si applicable et necessaire, puis appeler une fonction/programme qui fait le taff à la place

        - migrer le web_monitoring vers du serveur HTTP sans graphismes
        - cleanup/restructurer le code -> faire passer le code par des fonctions pour la lisibilité
        - main.c et chatbot.c sont INDEPENDANTS (useless de s'emmerder a faire des ponts alors qu'on peut juste faire --enable-chatbot et lancer le code en side)



    // quand j'ai vla le temps pck bon blc : ligne 184 à fix pour pas avoir à utiliser de fichier temporaire intermediaire
    trouver une putain de vidéo de transition merde
    revoir le système de logs en vue du changement vav des arguments de lancement
    globalement finir l'intégrations des arguments de lancement

    POUR LES PREDICTIONS :
        - on fait une prédiction à chaque début de match sur qui va gagner, et on fetch le resultat dans karmine api pour avoir le résultat de la prédiction


    changer get_app_token pour seulement renvoyer le token, il sera simplement refresh à tous les restart donc blc
    mettre en place une console par fichier externe (ou programme supplémentaire avec liaison mémoire ?) pour piloter un peu ce qu'il se passe malgré nohup (controler via commandes stream ?)
    changer les response.json par des fichiers à nom unique (pour éviter les potentiels conflits)

    un jour :
        coder le terrifiant chatbot twitch
            - IMPORTANT : if (message == Cheap viewers on *)    perma_ban(user);
            - faire en sorte de pouvoir taper des commandes dans le chat pour commander le bot (verifier les user_id pour voir si t'es autorisé à rentrer la commande)
                - shutdown              shutdown after current video ends
                - force shutdown        stays down until "start"
                - restart               restart on a new video, can be done while playing something
                - pause                 pauses current video until "play"
                - play                  plays the current paued video
                - select video          select next video to play, if current video is paused, restart on this one
                - get viewers           returns the current viewers number

            - idées chatbot :
                - !random -> donne une phrase random (#KCWIN, fuck Karnage, prochain match dans hh:mm:ss, ...)
                - !next -> donne hh:mm:ss avant le prochain match
                
                - !resultats -> affiche tous les resultats des dernières 24h (ou le plus récent)
                - !resultats (jeux) -> affiche les 3 derniers resultats d'un jeu
                - !game (jeu) -> associe jeu à la BD pour cette vidéo si jamais aucun jeu ne lui est associé


            - point à faire gaffe :
                - fermer toutes les sub à la fermeture du programme (utiliser signal.h)
                - faire gaffe à app_token (refresh toutes les 48h histoire de dire)

        utiliser une db pour associer des infos aux vidéos
            - le jeux qui va avec
            - potentiellement qui a gagné (?)


        créer une API pour le monitoring par exemple

    implémenter les arguments correctement
*/
