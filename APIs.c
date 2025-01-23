#include "APIs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>


void *KarmineAPI_handle()
{
    system("curl https://api2.kametotv.fr/karmine/events -o api.json");
    log2file("Fetching api data of La Prestigieuse...");
    FILE *fichier = fopen("api.json", "r");
    char tmp[SizeOfFile("api.json")];
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


char YTAPI_Get_Recent_Videos(unsigned int max, char *google_api_key)          // Creates file of form yt_id;yt_title sorted by most recent video
{
    if (max <= 50)
    {   
        {
            char tmpcmd[205];       // request len(162) + maxResult string len(max 2) + google api key len(40) + \0
            snprintf(tmpcmd, sizeof(tmpcmd), "curl -X GET \"https://www.googleapis.com/youtube/v3/search?"
                "part=snippet&channelId=UCApmTE4So9oX7sPkDGgSpFQ&maxResults=%d&order=date&type=video&key=%s\" > response.json", 
                max, google_api_key);
            system(tmpcmd);
        }
        char tmp[SizeOfFile("response.json")];
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
    }
    else
    {
        char tmp[144];      // request len + potential "max" string len(assumed 4) + \0
        snprintf(tmp, sizeof(tmp), "yt-dlp --flat-playlist --sleep-requests 27 \"https://www.youtube.com/@KarmineCorpVOD/videos\" --print \"%%(id)s\" --playlist-end %d > recentVids", max);
        system(tmp);
    }
        

    return 0;
}


char *YTAPI_Get_Video_Name(char *videoId, char *google_api_key)
{
    {
    char tmp[151];      // request len(99) + google api key len(40) + video id len(11) + \0
    snprintf(tmp, sizeof(tmp), "curl -X GET \"https://www.googleapis.com/youtube/v3/videos?part=snippet&id=%s&key=%s\" > response.json", 
        videoId, google_api_key);
    system(tmp);
    }

    char tmp[SizeOfFile("response.json")];
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


void TTV_API_revoke_access_token(char *access, char *bot_id)
{
    char tmp[strlen(access) + 158]; // request len + BOT_ID len + \0

    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://id.twitch.tv/oauth2/revoke' "
    "-H 'Content-Type: application/x-www-form-urlencoded' -d 'client_id=%s&token=%s'", bot_id, access);
    system(tmp);

    snprintf(tmp, sizeof(tmp), "Revoked token %s", access);
    log2file(tmp);
}


char **TTV_API_refresh_access_token(char *refresh_token, char *bot_id, char *bot_secret)
{
    {
    char tmp[302];      // request len(188) + refresh token len(51) + BOT_ID len(31) + BOT_SECRET len(31) + \0
    snprintf(tmp, sizeof(tmp), 
    "curl -X POST https://id.twitch.tv/oauth2/token -H 'Content-Type: application/x-www-form-urlencoded' " 
    "-d 'grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s' -o response.json", refresh_token, bot_id, bot_secret);
    system(tmp);
    }

    char tmp[SizeOfFile("response.json")];
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


char TTV_API_get_stream_info(char *access, char *channel_name, char *bot_id)
{
    {
    char tmp[strlen(channel_name) + 187];      // + request len(124) + access len(31) + BOT_ID len(31) + \0
    snprintf(tmp, sizeof(tmp), "curl -X GET 'https://api.twitch.tv/helix/streams?user_login=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-id: %s' -o response.json", channel_name, access, bot_id);
    system(tmp);
    }

    char tmp[SizeOfFile("response.json")];
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


void TTV_API_update_stream_info(char *streamerId, char *access, int gameId, char *title, char *bot_id)
{
    char tmp[1024];
    char titleRediff[strlen(title) + 14];
    strcpy(titleRediff, title);
    titleRediff[strlen(title) - 4] = '\0';
    strcat(titleRediff, " - [REDIFFUSION]");
    snprintf(tmp, sizeof(tmp), "curl -X PATCH 'https://api.twitch.tv/helix/channels?broadcaster_id=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -H 'Content-Type: application/json' "
    "--data-raw '{\"game_id\":\"%d\", \"title\":\"%s\", \"broadcaster_language\":\"fr\",  \"tags\":[\"247Stream\", \"botstream\", \"Français\", \"KarmineCorp\"]}' -o response.json", 
    streamerId, access, bot_id, gameId, titleRediff);
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


char *TTV_API_get_streamer_id(char *access, char *streamer_login, char *bot_id)
{
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "curl -X GET 'https://api.twitch.tv/helix/users?login=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o response.json", streamer_login, access, bot_id);
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


char *TTV_API_raid(char *access, char *fromId, char *toId, char *bot_id)
{
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://api.twitch.tv/helix/raids?from_broadcaster_id=%s&to_broadcaster_id=%s' "
        "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o response.json", fromId, toId, access, bot_id);
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