#include "APIs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>


#define KARMINE_API_NB_FETCHED 3

char **KarmineAPI_handle()         // returns all most recent and relevant datas from KarmineCorp API in an array of strings
{
    system("curl -X GET https://api2.kametotv.fr/karmine/group_a -o /var/tmp/Karmine/api.json");
    log2file("Fetching api data of La Prestigieuse...");
    FILE *fichier = fopen("/var/tmp/Karmine/api.json", "r");
    char tmp[SizeOfFile("/var/tmp/Karmine/api.json")];
    fread(tmp, 1, sizeof(tmp), fichier); 
    cJSON *useless = cJSON_Parse(tmp); 
    if (useless == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log2file("Couldn't fetch Karmine's API data");
        } 
        cJSON_Delete(useless); 
        return 0; 
    }
    cJSON *json = cJSON_GetObjectItemCaseSensitive(useless, "events");
    cJSON *upcomming = cJSON_GetArrayItem(json, 0);
    cJSON *start    = cJSON_GetObjectItemCaseSensitive(upcomming, "start");
    cJSON *end      = cJSON_GetObjectItemCaseSensitive(upcomming, "end");
    cJSON *channel  = cJSON_GetObjectItemCaseSensitive(upcomming, "streamLink");

    int totalsize = sizeof(char *) * KARMINE_API_NB_FETCHED;

    char **strArr = malloc(totalsize);
    strArr[0] = malloc(strlen(start->valuestring) + 1);
        strcpy(strArr[0], start->valuestring);
        totalsize += strlen(start->valuestring) + 1;
    strArr[1] = malloc(strlen(end->valuestring) + 1);
        strcpy(strArr[1], end->valuestring);
        totalsize += strlen(end->valuestring) + 1;
    strArr[2] = malloc(strlen(channel->valuestring) + 1);
        strcpy(strArr[2], channel->valuestring);
        totalsize += strlen(end->valuestring) + 1;

    char *memHdlr[totalsize];
    memcpy(memHdlr, strArr, sizeof(char *) * KARMINE_API_NB_FETCHED);
    free(strArr);

    char **toReturn = memHdlr;

    cJSON_Delete(useless);
    return toReturn;
}


char YTAPI_Get_Recent_Videos(unsigned int max, char *google_api_key)          // Creates file of form yt_id;yt_title sorted by most recent video
{
    if (max <= 20)
    {   
        {
            char tmpcmd[222];       // request len(179) + maxResult string len(max 2) + google api key len(40) + \0
            snprintf(tmpcmd, sizeof(tmpcmd), "curl -X GET \"https://www.googleapis.com/youtube/v3/search?"
                "part=snippet&channelId=UCApmTE4So9oX7sPkDGgSpFQ&maxResults=%d&order=date&type=video&key=%s\" > /var/tmp/Karmine/response.json", 
                max, google_api_key);
            system(tmpcmd);
        }
        char tmp[SizeOfFile("/var/tmp/Karmine/response.json")];
        FILE *fichier = fopen("/var/tmp/Karmine/response.json", "r");
        fread(tmp, 1, sizeof(tmp), fichier);
        fclose(fichier);


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
        cJSON *errorFullObject = cJSON_GetObjectItemCaseSensitive(error, "errors");
        cJSON *errorArray = cJSON_GetArrayItem(errorFullObject, 0);
        if (errorArray)
        {
            cJSON *errorstring = cJSON_GetObjectItemCaseSensitive(errorArray, "reason");
            snprintf(tmp, sizeof(tmp), "YTAPI_Get_Recent_Videos: Request failed: %s", errorstring->valuestring);
            log2file(tmp);
            cJSON_Delete(json);
            return -1;
        }
        remove("/var/tmp/Karmine/response.json");

        cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "items");
        for(unsigned int i = 0; i < max; i++)
        {
            cJSON *item = cJSON_GetArrayItem(data, i);  
            cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
            cJSON *videoId = cJSON_GetObjectItemCaseSensitive(id, "videoId");

            fichier = fopen("/var/tmp/Karmine/recentVids", "a");
            fprintf(fichier, "%s\n", videoId->valuestring);
            fclose(fichier);
        }
        cJSON_Delete(json);
    }
    else
    {
        log2file("requested video number exceeded 20, assuming there was a problem with a video...");
        return -1;
    }
        

    return 0;
}


char *YTAPI_Get_Video_Name(char *videoId, char *google_api_key)
{
    {
    char tmp[169];      // request len(99) + google api key len(40) + video id len(11) + \0
    snprintf(tmp, sizeof(tmp), "curl -X GET \"https://www.googleapis.com/youtube/v3/videos?part=snippet&id=%s&key=%s\" > /var/tmp/Karmine/response.json", 
        videoId, google_api_key);
    system(tmp);
    }

    char tmp[SizeOfFile("/var/tmp/Karmine/response.json")];
    FILE *fichier = fopen("/var/tmp/Karmine/response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("/var/tmp/Karmine/response.json");


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
    char tmp[strlen(access) + 159]; // request len + BOT_ID len + \0

    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://id.twitch.tv/oauth2/revoke' "
    "-H 'Content-Type: application/x-www-form-urlencoded' -d 'client_id=%s&token=%s'", bot_id, access);
    system(tmp);

    snprintf(tmp, sizeof(tmp), "Revoked token %s", access);
    log2file(tmp);
}


char **TTV_API_refresh_access_token(char *refresh_token, char *bot_id, char *bot_secret)
{
    {
    char tmp[320];      // request len(188) + refresh token len(51) + BOT_ID len(31) + BOT_SECRET len(31) + \0
    snprintf(tmp, sizeof(tmp), 
    "curl -X POST https://id.twitch.tv/oauth2/token -H 'Content-Type: application/x-www-form-urlencoded' " 
    "-d 'grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s' -o /var/tmp/Karmine/response.json", refresh_token, bot_id, bot_secret);
    system(tmp);
    }

    char tmp[SizeOfFile("/var/tmp/Karmine/response.json")];
    FILE *fichier = fopen("/var/tmp/Karmine/response.json", "r");
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
    cJSON *refresh = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");

    
    char **buffer = malloc(3 * sizeof(char*));
    for (int i =0 ; i < 3; ++i)
        buffer[i] = malloc(64 * sizeof(char));
    

    strcpy(buffer[0], access->valuestring);
    strcpy(buffer[1], refresh->valuestring);
    buffer[2] = NULL;
    char **toReturn = buffer;                       // memcpy() --> free()

    fclose(fichier);
    remove("response.json");
    cJSON_Delete(json);
    return toReturn;
}


char TTV_API_get_stream_info(char *access, char *channel_name, char *bot_id)
{
    {
    char tmp[strlen(channel_name) + 205];      // + request len(124) + access len(31) + BOT_ID len(31) + \0
    snprintf(tmp, sizeof(tmp), "curl -X GET 'https://api.twitch.tv/helix/streams?user_login=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-id: %s' -o /var/tmp/Karmine/response.json", channel_name, access, bot_id);
    system(tmp);
    }

    char tmp[SizeOfFile("/var/tmp/Karmine/response.json")];
    FILE *fichier = fopen("/var/tmp/Karmine/response.json", "r");
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
    remove("/var/tmp/Karmine/response.json");
    fichier = fopen("/var/tmp/Karmine/mntr.data", "w");
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
    "--data-raw '{\"game_id\":\"%d\", \"title\":\"%s\", \"broadcaster_language\":\"fr\",  \"tags\":[\"Rediffusion\", \"247Stream\", \"botstream\", \"Français\", \"KarmineCorp\"]}' -o /var/tmp/Karmine/response.json", 
    streamerId, access, bot_id, gameId, titleRediff);
    system(tmp);

    FILE *fichier = fopen("/var/tmp/Karmine/response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("/var/tmp/Karmine/response.json");
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
    "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o /var/tmp/Karmine/response.json", streamer_login, access, bot_id);
    system(tmp);

    FILE *fichier = fopen("/var/tmp/Karmine/response.json", "r");
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
    remove("/var/tmp/Karmine/response.json");
    cJSON_Delete(json);
    return toReturn;
}


char *TTV_API_raid(char *access, char *fromId, char *toId, char *bot_id)
{
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://api.twitch.tv/helix/raids?from_broadcaster_id=%s&to_broadcaster_id=%s' "
        "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o /var/tmp/Karmine/response.json", fromId, toId, access, bot_id);
    system(tmp);

    FILE *fichier = fopen("/var/tmp/Karmine/response.json", "r");
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
        remove("/var/tmp/Karmine/response.json");
        return toReturn;
    }
    return NULL;
}