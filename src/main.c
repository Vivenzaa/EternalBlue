    #include "../include/APIs.h"
    #include "../include/utils.h"
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


#define SEED 0xb00b5
#define DB_PATH "/var/tmp/K24/videos.db"

char *STREAM_KEY = NULL;
char *BOT_ID = NULL;
char *BOT_SECRET = NULL;
char *DEFAULT_REFRESH_TOKEN = NULL;
char *GOOGLE_API_KEY = NULL;
char *LOCAL_PATH = NULL;
char *CHANNEL_NAME = NULL;

unsigned long *settings = NULL;

int isDownloaded = 0;


void *download_videos(void* _useless)
{
    char **videos = file_lines("/tmp/Karmine/recentVids");
    remove("/tmp/Karmine/recentVids");
    char tmp[256];
    log4c(1, "Downloading %d video(s)...", size_of_double_array(videos));
    for (int i = size_of_double_array(videos) - 1; i >= 0; i--)
    { 
        log4c(1, "Downloading %s...", videos[i]);
        system("mkdir /tmp/Karmine/ytdlp");

        snprintf(tmp, sizeof(tmp), "yt-dlp --cookies-from-browser firefox -r %ld -f 299+140 %s%s -P /tmp/Karmine/ytdlp/", 3000000 * !settings[8] + settings[8],  "https://www.youtube.com/watch?v=" + (32 * (videos[i][0] != '-')), videos[i]);
        int ret = system(tmp);
        if(ret)
        {
            log4c(2, "yt-dlp couldn't download specified video, trying to resolve...");
            system("pip install yt-dlp -U --break-system-packages");
            snprintf(tmp, sizeof(tmp), "yt-dlp --cookies-from-browser firefox --merge-output-format mp4 --extractor-args \"youtube:player-client=default,-tv,web_safari,web_embedded\" -r %ld %s%s -P /tmp/Karmine/ytdlp/", 3000000 * !settings[8] + settings[8], "https://www.youtube.com/watch?v=" + (32 * (videos[i][0] != '-')), videos[i]);
            system(tmp);
        }

        char *videoTitle = YTAPI_Get_Video_Name(videos[i], GOOGLE_API_KEY);
        log4c(1, "Successfully downloaded %s : %s", videos[i], videoTitle);

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

        log4c(0, "writing metadata to %s", videotmp);

        snprintf(tmp, sizeof(tmp), "/tmp/Karmine/%s", videotmp);
        write_metadata(tmp, fullLink);
        free(fullLink);
        char moov[512];
        snprintf(moov, sizeof(moov), "mv /tmp/Karmine/30784230304235.mp4 \"./%s%s\"", LOCAL_PATH, videotmp);
        system(moov);
    }

    recur_free(videos);
    log4c(1, "Successfully downloaded all recent videos.");
    isDownloaded = 1;

    return _useless;
}


void *ffwrite(void *data)
{
    ffdata *pathfifo = (ffdata *)data;

    char cmdFIFO[51 +
                 strlen(LOCAL_PATH) +
                 strlen(pathfifo->videopath)];

    snprintf(cmdFIFO, sizeof(cmdFIFO), "ffmpeg -hide_banner -re -i \"%s%s\" -c copy -f mpegts -", LOCAL_PATH, pathfifo->videopath);

    log4c(0, "ffwrite: launching command: %s", cmdFIFO);

    FILE *ffmpeg = popen(cmdFIFO, "r");
    if (!ffmpeg) {
        log4c(3, "ffwrite: popen failed: %s", strerror(errno));
        free(data);
        return NULL;
    }

    log4c(0, "ffwrite: ffmpeg process opened");

    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), ffmpeg)) > 0) {
        if (fcntl(*pathfifo->fifo, F_GETFD) == -1) 
            log4c(3, "ffwrite: invalid FIFO fd %d: %s", *pathfifo->fifo, strerror(errno));
        
        ssize_t written = write(*pathfifo->fifo, buffer, bytesRead);
        if (written == -1) {
            log4c(3, "ffwrite: write failed: %s", strerror(errno));
            break;
        } else if ((size_t)written != bytesRead) {
            log4c(2, "ffwrite: partial write (%zd/%zu bytes)", written, bytesRead);
        }
    }

    if (ferror(ffmpeg)) {
        log4c(3, "ffwrite: fread encountered an error: %s", strerror(errno));
    }

    int pclose_status = pclose(ffmpeg);
    if (pclose_status == -1) {
        log4c(2, "ffwrite: pclose failed: %s", strerror(errno));
    } else {
        log4c(0, "ffwrite: ffmpeg process closed with status: %d", pclose_status);
    }

    free(data);
    log4c(0, "ffwrite: finished execution and freed memory");

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
    
    char *knownArgs[] = {"--help", "--enable-chatbot", "--enable-monitoring-server", "--full-fflog", "--log-current-token", "--keep-after-read", "-loglevel", "-logretention", "-dlspeed", "--configure"};
    settings = calloc(sizeof(knownArgs) / sizeof(char *), sizeof(unsigned long));
    handleArgs(argv, argc, settings, knownArgs);
    logDepth = settings[6];

    
    if (settings[0])
    {
        printf("list of args : \n"
               "\t--help :                          affiche cette page\n"
               //"\t--enable-chatbot :               enables twitch chatbot, prediction handling etc\n"
               "\t--enable-monitoring-server :      enables monitoring via https url\n"
               "\t--full-fflog :                    allow creation of ff.log\n"
               "\t--log-current-token               UNSECURE : logs current access and app token in console.log\n"
               //"\t--log-retention x hours          how many time before logs completely delete\n"
               //"\t--keep-after-read                keeps videos on the machine after downloading/reading them\n"
               "\t-loglevel [0,1,2,3]               how precise you want logs to be :\n"
               "\t    0: no logs at all\n"
               "\t    1: only infos\n"
               "\t    2: log everything\n"
               "\t    3: debug\n"
               "\t-dlspeed                          specifies the download speed in bytes/s\n"
               "\t--configure                       manually prompts for API keys update\n"
               );
        exit(0);
    }

    STREAM_KEY = malloc(47);                //init_array_cheat(STREAM_KEY, 47);
    BOT_ID = malloc(31);                    //init_array_cheat(BOT_ID, 31);
    BOT_SECRET = malloc(31);                //init_array_cheat(BOT_SECRET, 31);
    DEFAULT_REFRESH_TOKEN = malloc(51);     //init_array_cheat(DEFAULT_REFRESH_TOKEN, 51);
    GOOGLE_API_KEY = malloc(40);            //init_array_cheat(GOOGLE_API_KEY, 40);
    LOCAL_PATH = malloc(256);               //init_array_cheat(LOCAL_PATH, 256);
    CHANNEL_NAME = malloc(128);             //init_array_cheat(CHANNEL_NAME, 128);

    

    {
        char envpath[512];
        get_env_filepath(envpath, sizeof(envpath));

        get_env_infos(envpath, STREAM_KEY, BOT_ID, BOT_SECRET, DEFAULT_REFRESH_TOKEN, GOOGLE_API_KEY, LOCAL_PATH, CHANNEL_NAME);
        if (settings[9] || !get_env_infos(envpath, STREAM_KEY, BOT_ID, BOT_SECRET, DEFAULT_REFRESH_TOKEN, GOOGLE_API_KEY, LOCAL_PATH, CHANNEL_NAME))
        {
            askSave_env_infos(envpath);
            get_env_infos(envpath, STREAM_KEY, BOT_ID, BOT_SECRET, DEFAULT_REFRESH_TOKEN, GOOGLE_API_KEY, LOCAL_PATH, CHANNEL_NAME);
        }
    }
        
        

    system("rm -r /tmp/Karmine/");
    system("rm -r /var/tmp/Karmine");
    system("mkdir /tmp/Karmine");
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
    char *access_token = malloc(31);
    char *refresh_token = malloc(51);
        strcpy(refresh_token, DEFAULT_REFRESH_TOKEN);
        TTV_API_refresh_access_token(BOT_ID, BOT_SECRET, &refresh_token, &access_token);
    char *streamer_channel_id = TTV_API_get_user_id(access_token, channel_name, BOT_ID);
    char tmp[512];

    // signal(SIGINT, intHandler);
    // signal(SIGTERM, intHandler);
    if(settings[4])
        log4c(1, "launching program with Twitch token %s...", access_token);
    
    
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

    log4c(1, "checking for yt-dlp update...");
    system("pip install yt-dlp -U --break-system-packages");
    if (get_undownloaded_videos(LOCAL_PATH, GOOGLE_API_KEY))
    {
        pthread_t downloaderThr;
        pthread_create(&downloaderThr, NULL, download_videos, NULL);
    }
    while (time(NULL) + current_time_offset - timestart < 157000)
    {
        ffdata *data = malloc(sizeof(ffdata));
        data->videopath = video;
        data->fifo = &fifo_fd;
        
        pthread_t ffThr;
        pthread_create(&ffThr, NULL, ffwrite, (void *)data);
        TTV_API_refresh_access_token(BOT_ID, BOT_SECRET, &refresh_token, &access_token);
        if(settings[4])
            log4c(0, "refreshed Twitch token to %s", access_token);
        
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
        log4c(1, tmp);

        if (settings[2])
        {
            /*------------------------------------HANDLE WEB MONITORING-------------------------------------*/
            log4c(0, "web_monitoring: getting self stream infos...");                                          //
            TTV_API_get_stream_info(access_token, CHANNEL_NAME, BOT_ID);                                    //
            FILE *streamInfos = fopen("/tmp/Karmine/mntr.data", "r");                                       //
            char isOnline[8];                                                                               //
            char vwCount[6];                                                                                //
            fgets(isOnline, sizeof(isOnline), streamInfos);                                                 //
            isOnline[strlen(isOnline) - 1] = '\0';                                                          //
            fgets(tmp, sizeof(tmp), streamInfos);                                                           //
            fgets(vwCount, sizeof(vwCount), streamInfos);                                                   //
            fclose(streamInfos);                                                                            //
            remove("/tmp/Karmine/mntr.data");                                                               //
                                                                                                            //
            streamInfos = fopen("/var/www/html/monitoring.json", "w");                                      //
            cJSON *json = cJSON_CreateObject();                                                             //
                                                                                                            //
            cJSON_AddStringToObject(json, "status", isOnline);                                              //
            cJSON_AddStringToObject(json, "videoTitle", video);                                             //
            cJSON_AddNumberToObject(json, "videoDuration", (double)get_video_duration(video, LOCAL_PATH));  //
            cJSON_AddNumberToObject(json, "videoStartTime", (double)time(NULL) + current_time_offset);      //
            cJSON_AddNumberToObject(json, "streamStartTime", (double)timestart);                            //
            cJSON_AddNumberToObject(json, "viewers", (double)atoi(vwCount));                                //
                                                                                                            //
            char *json_str = cJSON_Print(json);                                                             //
            fputs(json_str, streamInfos);                                                                   //
            fclose(streamInfos);                                                                            //
            cJSON_free(json_str);                                                                           //
            cJSON_Delete(json);                                                                             //
            /*------------------------------------HANDLE WEB MONITORING-------------------------------------*/
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
            log4c(1, "next match will be starting soon, attempting to raid %s...", streamer);

            char *to_id = TTV_API_get_user_id(access_token, streamer, BOT_ID);
            char *raidValue = TTV_API_raid(access_token, streamer_channel_id, to_id, BOT_ID);
            
            free(to_id);
            if (raidValue)
                log4c(2, "Failed to raid %s: %s", streamer, raidValue);
            else
                log4c(1, "Successfully raided %s", streamer);

            free(raidValue);
            log4c(1, "Attempting to close stream...");
            pthread_join(ffThr, NULL);
            log4c(3, "closing fifo_fd...");
            system("echo q > /tmp/Karmine/video_fifo");
            close(fifo_fd);
        wait_again:
            KarmineToWait = KarmineAPI_timeto(&streamer, time(NULL) + current_time_offset, 0);
            if (KarmineToWait > 20000)          // si il y a plus de 6h d'attente c'est bizarre, on restart
            {
                log4c(2, "Waiting time is above 5h30, assuming an error was made, restarting...");
                recur_free(playlist);
                goto restart;
            }

            snprintf(tmp, sizeof(tmp), "Waiting %02d:%02d:%02d until next match, estimated next loop starts at %02d:%02d:%02d...",
                     (KarmineToWait / 3600) % 24, (KarmineToWait / 60) % 60, KarmineToWait % 60,
                     ((KarmineToWait + (int)time(NULL) + current_time_offset) / 3600) % 24, ((KarmineToWait + (int)time(NULL)) / 60) % 60, (KarmineToWait + (int)time(NULL)) % 60);
            log4c(1, tmp);
            sleep(KarmineToWait + 10);
            

            KarmineToWait = KarmineAPI_timeto(&streamer, time(NULL) + current_time_offset, 1);
            int nextVidDuration = get_video_duration(nextVideo, LOCAL_PATH);

            if (nextVidDuration > KarmineToWait)
            {
                snprintf(tmp, sizeof(tmp), "Choosing not the restart the stream right now, next match starts at %02d:%02d:%02d while next video would end at %02d:%02d:%02d",
                        ((KarmineToWait + (int)time(NULL) + current_time_offset) / 3600) % 24, ((KarmineToWait + (int)time(NULL)) / 60) % 60, (KarmineToWait + (int)time(NULL)) % 60,
                        ((nextVidDuration + (int)time(NULL) + current_time_offset) / 3600) % 24, ((nextVidDuration + (int)time(NULL)) / 60) % 60, (nextVidDuration + (int)time(NULL)) % 60);
                log4c(1, tmp);
                goto wait_again;
            }
            else
            {
                log4c(1, "found no blocking condition, restarting stream...");
                recur_free(playlist);
                goto restart;
            }
        }
        video = nextVideo;
        pthread_join(ffThr, NULL);
    }
    
    log4c(2, "Twitch limit of 48h is almost reached, resetting stream...");
    sleep(10);
    recur_free(playlist);
    goto restart;

    return 0;
}






/*  
    mettre un système d'interface semi-graphiques 
    créer un header web.c/h avec les fonctions web (qui seront utiles pour le serveur et pour tous les sources utilisant curl)
    trouver une solution pour le stack des videos (+1.2To actuellement, c cho)
        - implémentation du --keep-after-read ? 
            - possiblement un nombre de keep possible à passer en arguments
            - peut etre pouvoir spécifier de garder une seule catégorie 
            - si not keep, que faire en cas de video strike/mise en privée ou jsp, ce serait con de les perdre
                - peut etre foutre les vidéos not keep sur une chaine youtube 
                    - dans ce cas faut vite implémenter la base de donnée, on verifier si elle est sur la chaine custom, si oui on la prends la sinon sur la chaine offi
        - compression ?  (on retarde juste le problème)


    passer par popen pour éviter la création de fichiers temporaires pas super utiles

    FIRST LAUNCH :
        - allow for argument --configure to review/change keys


    REPUBLICATION DU CODE:
        - cleanup le code 
        - choisir entre anglais et français (pck bon le mélange des 2 ça va 5m)
        
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
