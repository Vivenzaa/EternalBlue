#ifndef APIs_H
#define APIs_H

typedef struct {
    char *title;
    char *score_kc;
    char *score_nullos;
    char date[11];
} result_info_t;

int  KarmineAPI_timeto(char **streamer, int current_time, char wantsStart);
char YTAPI_Get_Recent_Videos(unsigned int max, char *google_api_key);          // Creates file of form yt_id;yt_title sorted by most recent video
char YTAPI_Get_Video_infos(char **title, char * restrict date, char * restrict videoId, char * restrict google_api_key);
void TTV_API_revoke_access_token(char * restrict access, char * restrict bot_id);
void TTV_API_refresh_access_token(char * restrict bot_id, char * restrict bot_secret, char ** restrict refresh_token, char ** restrict token);
void TTV_API_get_stream_info(char * restrict access, char * restrict channel_name, char * restrict bot_id);
void TTV_API_update_stream_info(char * restrict streamerId, char * restrict access, int gameId, char * restrict title, char * restrict bot_id);
char *TTV_API_get_user_id(char * restrict access, char * restrict streamer_login, char * restrict bot_id);
char *TTV_API_raid(char * restrict access, char * restrict fromId, char * restrict toId, char * restrict bot_id);
int TTV_API_get_app_token(char * restrict bot_id, char * restrict bot_secret, char **token);
void TTV_API_ban_user(char * restrict bot_id, char * restrict token, char * restrict toban_login, char *mod_id, char *streamer_id);
void TTV_API_fetch_results(char *game, result_info_t array[]);

#endif