#ifndef APIs_H
#define APIs_H

char KarmineAPI_handle(int event, char ***array);
char YTAPI_Get_Recent_Videos(unsigned int max, char *google_api_key);          // Creates file of form yt_id;yt_title sorted by most recent video
char *YTAPI_Get_Video_Name(char *videoId, char *google_api_key);
void TTV_API_revoke_access_token(char *access, char *bot_id);
char **TTV_API_refresh_access_token(char *refresh_token, char *bot_id, char *bot_secret);
char TTV_API_get_stream_info(char *access, char *channel_name, char *bot_id);
void TTV_API_update_stream_info(char *streamerId, char *access, int gameId, char *title, char *bot_id);
char *TTV_API_get_streamer_id(char *access, char *streamer_login, char *bot_id);
char *TTV_API_raid(char *access, char *fromId, char *toId, char *bot_id);
char *TTV_API_get_app_token(char *bot_id, char *bot_secret);

#endif