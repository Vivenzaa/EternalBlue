#ifndef APIs_H
#define APIs_H

int  KarmineAPI_timeto(char **streamer, int current_time, char wantsStart);
char YTAPI_Get_Recent_Videos(unsigned int max, char *google_api_key);          // Creates file of form yt_id;yt_title sorted by most recent video
char *YTAPI_Get_Video_Name(char * restrict videoId, char * restrict google_api_key);
void TTV_API_revoke_access_token(char * restrict access, char * restrict bot_id);
char **TTV_API_refresh_access_token(char * restrict refresh_token, char * restrict bot_id, char * restrict bot_secret);
char TTV_API_get_stream_info(char * restrict access, char * restrict channel_name, char * restrict bot_id);
void TTV_API_update_stream_info(char * restrict streamerId, char * restrict access, int gameId, char * restrict title, char * restrict bot_id);
char *TTV_API_get_streamer_id(char * restrict access, char * restrict streamer_login, char * restrict bot_id);
char *TTV_API_raid(char * restrict access, char * restrict fromId, char * restrict toId, char * restrict bot_id);
char *TTV_API_get_app_token(char * restrict bot_id, char * restrict bot_secret);

#endif