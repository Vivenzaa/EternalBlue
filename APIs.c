#include "APIs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>


int KarmineAPI_timeto(char **streamer, int current_time, char wantsStart)
{
    log4c("Fetching API datas of La Prestigieuse...");
    system("curl -X GET https://api2.kametotv.fr/karmine/group_a -o /var/tmp/Karmine/api.json");
    FILE *fichier = fopen("/var/tmp/Karmine/api.json", "r");
    char tmp[SizeOfFile("/var/tmp/Karmine/api.json")];
    fread(tmp, 1, sizeof(tmp), fichier); 
    fclose(fichier);
    cJSON *useless = cJSON_Parse(tmp); 
    if (useless == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log4c("Couldn't fetch Karmine's API data");
        }
        log4c("json file is NULL, aborting...");
        *streamer = NULL;
        cJSON_Delete(useless);
        return 0;
    }
    
    int returnValue = 0;
    cJSON *json = cJSON_GetObjectItemCaseSensitive(useless, "events");
    for(int i = 0; i <= cJSON_GetArraySize(json); i++)
    {
        cJSON *upcomming = cJSON_GetArrayItem(json, i);
        if(upcomming == NULL)
        {
            log4c("upcomming is NULL, aborting...");
            *streamer = NULL;
            cJSON_Delete(useless);
            return 0;
        }
        cJSON *start    = cJSON_GetObjectItemCaseSensitive(upcomming, "start");
        
        if(convert_to_timestamp(start->valuestring) > current_time)
        {
            int tmp = 0;
            if (wantsStart)
                tmp = convert_to_timestamp(start->valuestring);
            else
                tmp = convert_to_timestamp(cJSON_GetObjectItemCaseSensitive(upcomming, "end")->valuestring);
            tmp -= current_time;
            if (tmp <= 0)
                continue;
            cJSON *channel  = cJSON_GetObjectItemCaseSensitive(upcomming, "streamLink");
            *streamer = realloc(*streamer, strlen(channel->valuestring) + 1);
            strcpy(*streamer, channel->valuestring);
            returnValue = tmp;
            break;
        }
        else
        {
            int endtime = convert_to_timestamp(cJSON_GetObjectItemCaseSensitive(upcomming, "end")->valuestring) - current_time;
            if (endtime > 0)
            {
                if (wantsStart)
                    continue;
                cJSON *channel  = cJSON_GetObjectItemCaseSensitive(upcomming, "streamLink");
                *streamer = realloc(*streamer, strlen(channel->valuestring) + 1);
                strcpy(*streamer, channel->valuestring);
                returnValue = endtime;
                break;
            }
        }
    }
    cJSON_Delete(useless);
    return returnValue;
}


char YTAPI_Get_Recent_Videos(unsigned int max, char *google_api_key)          // Creates file of form yt_id;yt_title sorted by most recent video
{
    if (max <= 20)
    {   
        {
            char tmpcmd[218];       // request len(175) + maxResult string len(max 2) + google api key len(40) + \0
            snprintf(tmpcmd, sizeof(tmpcmd), "curl -X GET \"https://www.googleapis.com/youtube/v3/search?"
                "part=snippet&channelId=UCApmTE4So9oX7sPkDGgSpFQ&maxResults=%d&order=date&type=video&key=%s\" > /tmp/Karmine/response.json", 
                max, google_api_key);
            system(tmpcmd);
        }
        char tmp[SizeOfFile("/tmp/Karmine/response.json")];
        FILE *fichier = fopen("/tmp/Karmine/response.json", "r");
        fread(tmp, 1, sizeof(tmp), fichier);
        fclose(fichier);


        cJSON *json = cJSON_Parse(tmp);
        if (json == NULL) { 
            const char *error_ptr = cJSON_GetErrorPtr(); 
            if (error_ptr != NULL) { 
                log4c((char *)error_ptr); 
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
            log4c("YTAPI_Get_Recent_Videos: Request failed: %s", errorstring->valuestring);
            cJSON_Delete(json);
            return -1;
        }
        remove("/tmp/Karmine/response.json");

        cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "items");
        for(unsigned int i = 0; i < max; i++)
        {
            cJSON *item = cJSON_GetArrayItem(data, i);  
            cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
            cJSON *videoId = cJSON_GetObjectItemCaseSensitive(id, "videoId");

            fichier = fopen("/tmp/Karmine/recentVids", "a");
            fprintf(fichier, "%s\n", videoId->valuestring);
            fclose(fichier);
        }
        cJSON_Delete(json);
    }
    else
    {
        log4c("requested video number exceeded 20, assuming there was a problem with a video...");
        return -1;
    }
        

    return 0;
}


char *YTAPI_Get_Video_Name(char * restrict videoId, char * restrict google_api_key)
{
    {
    char tmp[165];      // request len(95) + google api key len(40) + video id len(11) + \0
    snprintf(tmp, sizeof(tmp), "curl -X GET \"https://www.googleapis.com/youtube/v3/videos?part=snippet&id=%s&key=%s\" > /tmp/Karmine/response.json", 
        videoId, google_api_key);
    system(tmp);
    }

    char tmp[SizeOfFile("/tmp/Karmine/response.json")];
    FILE *fichier = fopen("/tmp/Karmine/response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("/tmp/Karmine/response.json");


    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log4c((char *)error_ptr); 
            return NULL;
        } 
        cJSON_Delete(json); 
        return NULL; 
    }
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        log4c("YTAPI_Get_Video_Name: Request failed: %s", error->valuestring);
        cJSON_Delete(json);
        return NULL;
    }

    cJSON *data =       cJSON_GetObjectItemCaseSensitive(json, "items");
    cJSON *item =       cJSON_GetArrayItem(data, 0);
    cJSON *snippet =    cJSON_GetObjectItemCaseSensitive(item, "snippet");
    cJSON *title =      cJSON_GetObjectItemCaseSensitive(snippet, "title");

    char *toReturn = malloc(strlen(title->valuestring) + 1);
    strcpy(toReturn, title->valuestring);

    cJSON_Delete(json);
    return toReturn;
}


void TTV_API_revoke_access_token(char * restrict access, char * restrict bot_id)
{
    char tmp[strlen(access) + 159]; // request len + BOT_ID len + \0

    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://id.twitch.tv/oauth2/revoke' "
    "-H 'Content-Type: application/x-www-form-urlencoded' -d 'client_id=%s&token=%s'", bot_id, access);
    system(tmp);

    log4c("Revoked token %s", access);
}


void TTV_API_refresh_access_token(char * restrict bot_id, char * restrict bot_secret, char * restrict refresh_token, char * restrict token)
{
    log4c("refreshing TwitchAPI tokens...");
    {
    char tmp[316];      // request len(184) + refresh token len(51) + BOT_ID len(31) + BOT_SECRET len(31) + \0
    snprintf(tmp, sizeof(tmp), 
    "curl -X POST https://id.twitch.tv/oauth2/token -H 'Content-Type: application/x-www-form-urlencoded' " 
    "-d 'grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s' -o /tmp/Karmine/response.json", refresh_token, bot_id, bot_secret);
    system(tmp);
    }

    char tmp[SizeOfFile("/tmp/Karmine/response.json")];
    FILE *fichier = fopen("/tmp/Karmine/response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier); 


    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL)
            log4c((char *)error_ptr); 
        
        fclose(fichier);
        cJSON_Delete(json); 
        return; 
    }
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        log4c("request failed with code %d", error->valueint);
        return;
    }
    cJSON *access = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    cJSON *refresh = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
    

    strcpy(token, access->valuestring);
    strcpy(refresh_token, refresh->valuestring);

    fclose(fichier);
    remove("/tmp/Karmine/response.json");
    cJSON_Delete(json);
}


void TTV_API_get_stream_info(char * restrict access, char * restrict channel_name, char * restrict bot_id)
{
    {
    char tmp[strlen(channel_name) + 201];
    snprintf(tmp, sizeof(tmp), "curl -X GET 'https://api.twitch.tv/helix/streams?user_login=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-id: %s' -o /tmp/Karmine/response.json", channel_name, access, bot_id);
    system(tmp);
    }

    char tmp[SizeOfFile("/tmp/Karmine/response.json")];
    FILE *fichier = fopen("/tmp/Karmine/response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);

    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log4c((char *)error_ptr); 
            return;
        } 
        cJSON_Delete(json); 
        return; 
    }
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        log4c("Request failed: %s", error->valuestring);
        cJSON_Delete(json);
        return;
    }
    cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
    cJSON *item = cJSON_GetArrayItem(data, 0);

    fclose(fichier);
    remove("/tmp/Karmine/response.json");
    fichier = fopen("/tmp/Karmine/mntr.data", "w");
    char *isOnline = "offline";
    cJSON *vwCount = cJSON_GetObjectItemCaseSensitive(item, "viewer_count");
    if (vwCount)
        isOnline = "online";
    else
    {
        fputs("offline\n0", fichier);
        fclose(fichier);
        cJSON_Delete(json);
        return;
    }

    int toReturn = vwCount->valueint;
    snprintf(tmp, sizeof(tmp), "%s\n%d", isOnline, toReturn);
    fputs(tmp, fichier);
    fclose(fichier);
    cJSON_Delete(json);
}


void TTV_API_update_stream_info(char * restrict streamerId, char * restrict access, int gameId, char * restrict title, char * restrict bot_id)
{
    char tmp[1024];
    char titleRediff[strlen(title) + 14];
    strcpy(titleRediff, title);
    titleRediff[strlen(title) - 4] = '\0';
    strcat(titleRediff, " - [REDIFFUSION]");
    snprintf(tmp, sizeof(tmp), "    curl -X PATCH 'https://api.twitch.tv/helix/channels?broadcaster_id=%s' "
                                "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -H 'Content-Type: application/json' "
                                "--data-raw '{\"game_id\":\"%d\", \"title\":\"%s\", \"broadcaster_language\":\"fr\",  \"tags\":[\"KCORP\", \"Rediffusion\", \"247Stream\", \"botstream\", \"Français\", \"KarmineCorp\"]}' -o /tmp/Karmine/response.json", 
                                streamerId, access, bot_id, gameId, titleRediff);
    system(tmp);

    FILE *fichier = fopen("/tmp/Karmine/response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("/tmp/Karmine/response.json");
    cJSON *json = cJSON_Parse(tmp);
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
        log4c("Request failed to update stream informations: %d: %s", error->valueint, error->valuestring);
    
    else
        log4c("Updated stream infos to game_id: %d and title %s", gameId, titleRediff);
    
    cJSON_Delete(json);
}


char *TTV_API_get_user_id(char * restrict access, char * restrict user_login, char * restrict bot_id)
{
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "curl -X GET 'https://api.twitch.tv/helix/users?login=%s' "
    "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o /tmp/Karmine/response.json", user_login, access, bot_id);
    system(tmp);

    FILE *fichier = fopen("/tmp/Karmine/response.json", "r");
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
    char *toReturn = malloc(strlen(id->valuestring) + 1);
    strcpy(toReturn, id->valuestring);
    remove("/tmp/Karmine/response.json");
    cJSON_Delete(json);
    return toReturn;
}


char *TTV_API_raid(char * restrict access, char * restrict fromId, char * restrict toId, char * restrict bot_id)
{
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://api.twitch.tv/helix/raids?from_broadcaster_id=%s&to_broadcaster_id=%s' "
        "-H 'Authorization: Bearer %s' -H 'Client-Id: %s' -o /tmp/Karmine/response.json", fromId, toId, access, bot_id);
    system(tmp);

    FILE *fichier = fopen("/tmp/Karmine/response.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);

    cJSON *json = cJSON_Parse(tmp);
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        cJSON *msg = cJSON_GetObjectItemCaseSensitive(json, "message");
        char *toReturn = malloc(strlen(msg->valuestring) + 1);
        strcpy(toReturn, msg->valuestring);
        
        cJSON_Delete(json);
        remove("/tmp/Karmine/response.json");
        return toReturn;
    }
    return NULL;
}


char *TTV_API_get_app_token(char * restrict bot_id, char * restrict bot_secret)
{
    {
        char tmp[254];
        snprintf(tmp, sizeof(tmp), "curl -X POST https://id.twitch.tv/oauth2/token -H 'Content-Type: application/x-www-form-urlencoded' "
        "-d 'client_id=%s&client_secret=%s&grant_type=client_credentials' -o /tmp/Karmine/appresponse.json", bot_id, bot_secret);
        system(tmp);
    }

    char tmp[SizeOfFile("/tmp/Karmine/appresponse.json")];
    FILE *fichier = fopen("/tmp/Karmine/appresponse.json", "r");
    fread(tmp, 1, sizeof(tmp), fichier);
    fclose(fichier);
    remove("/tmp/Karmine/appresponse.json");
    cJSON *json = cJSON_Parse(tmp);
    if (json == NULL) { 
        const char *error_ptr = cJSON_GetErrorPtr(); 
        if (error_ptr != NULL) { 
            log4c((char *)error_ptr); 
            return NULL;
        } 
        cJSON_Delete(json); 
        return NULL; 
    }
    
    cJSON *error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error)
    {
        log4c("Request failed: %s", error->valuestring);
        cJSON_Delete(json);
        return NULL;
    }

    
    cJSON *token = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    
    
    char *toReturn = malloc(strlen(token->valuestring) + 1);
    strcpy(toReturn, token->valuestring);

    return toReturn;
}


void TTV_API_ban_user(char * restrict bot_id, char * restrict token, char * restrict toban_login, char *mod_id, char *streamer_id)
{
    char *toban_id = TTV_API_get_user_id(token, toban_login, bot_id);
    char tmp[289 + strlen(streamer_id) + strlen(mod_id) + strlen(toban_id)];
    snprintf(tmp, sizeof(tmp), "curl -X POST 'https://api.twitch.tv/helix/moderation/bans?broadcaster_id=%s&moderator_id=%s' -H 'Authorization: Bearer %s' -H 'Client-Id: %s' -H 'Content-Type: application/json' -d '{\"data\": {\"user_id\":\"%s\",\"reason\":\"fait pas ta pub fdp\"}}'", streamer_id, mod_id, token, bot_id, toban_id);
    system(tmp);
    
    free(toban_id);
}


