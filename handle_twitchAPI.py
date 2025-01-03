from os.path import exists
from twitchAPI.twitch import Twitch
from twitchAPI.oauth import refresh_access_token, revoke_token
from twitchAPI.type import AuthScope
from twitchAPI.helper import first
from asyncio import run
from argparse import ArgumentParser


CHANNEL_ID = "Channel id"
TWITCH_APP_ID = "twitch bot id"
TWITCH_APP_SECRET = "twitch bot password or something ?"
ACCESS_TOKEN = "bot access token"
REFRESH_TOKEN = "bot refresh token (when access token expires)"
CURRENT_CHANNEL_NAME = "channel name"
WORDLIST = [ 
    ["[special event]"],
    ["words"]
    ["that"]
    ["identifies"]
    ["a"]
    ["game"]
    ["uniquely"]
    ["default option is special event (the first one)"]
    ]
CATEGORY_IDS = [509663, 30921, 513143, 516575, 21779, 8224, 504461, 33214]
# category id's founds at https://github.com/Nerothos/TwithGameList
STREAM_TAGS = ["tags", "on", "stream"]

def getGame(u):
    title = u.lower()
    for i in range(len(WORDLIST)):
        for j in WORDLIST[i]:
            if j in title:
                return i
    return 0


def save_tokens(access, refresh):
    with open("tokens.cred", "w") as fichier:
        fichier.write(f"{access}\n{refresh}")


def read_tokens():
    with open("tokens.cred", "r") as fichier:
        access = fichier.read(30)
        fichier.read(1)
        refresh = fichier.read(50)
        return [access, refresh]


async def update(path):
    global ACCESS_TOKEN
    global REFRESH_TOKEN

    twitch = await Twitch(TWITCH_APP_ID, TWITCH_APP_SECRET)
    target_scope = [AuthScope.CHANNEL_MANAGE_BROADCAST, AuthScope.USER_EDIT_BROADCAST]

    if exists("tokens.cred"):
        ACCESS_TOKEN, REFRESH_TOKEN = read_tokens()
    await twitch.set_user_authentication(ACCESS_TOKEN, target_scope, REFRESH_TOKEN)
    channel_id = await first(twitch.get_users(logins=[CURRENT_CHANNEL_NAME]))

    await twitch.modify_channel_information(
        channel_id.id,
        CATEGORY_IDS[getGame(path[:-4])],
        "fr",
        path[:-4] + " - [REDIFFUSION]",
        tags=STREAM_TAGS
        )
    
    
async def raid(streamer):
    global ACCESS_TOKEN
    global REFRESH_TOKEN
    twitch = await Twitch(TWITCH_APP_ID, TWITCH_APP_SECRET)
    target_scope = [AuthScope.CHANNEL_MANAGE_BROADCAST, AuthScope.USER_EDIT_BROADCAST]
    if exists("tokens.cred"):
        ACCESS_TOKEN, REFRESH_TOKEN = read_tokens()
    await twitch.set_user_authentication(ACCESS_TOKEN, target_scope, REFRESH_TOKEN)
    from_channel_id = await first(twitch.get_users(logins=[CURRENT_CHANNEL_NAME]))
    to_channel_id = await first(twitch.get_users(logins=[streamer]))

    await twitch.start_raid(from_channel_id.id, to_channel_id.id)


async def refresh():
    global ACCESS_TOKEN
    global REFRESH_TOKEN
    if exists("tokens.cred"):
        ACCESS_TOKEN, REFRESH_TOKEN = read_tokens()
    await revoke_token(TWITCH_APP_ID, ACCESS_TOKEN)
    ACCESS_TOKEN, REFRESH_TOKEN = await refresh_access_token(REFRESH_TOKEN, TWITCH_APP_ID, TWITCH_APP_SECRET)
    save_tokens(ACCESS_TOKEN, REFRESH_TOKEN)


async def main():
    parser = ArgumentParser()
    parser.add_argument("-u", "--update-stream", type=str, help="updates stream informations such as title ans category")
    parser.add_argument("-re",  "--refresh-token", action="store_true", help="refreshes twitch tokens")
    parser.add_argument("-ra",  "--raid-channel", type=str, help="raid given twitch channel")

    args = parser.parse_args()
    if args.update_stream:
        await update(args.update_stream)
    if args.refresh_token:
        await refresh()
    if args.raid_channel:
        await raid(args.raid_channel)




if __name__ == "__main__":
    run(main())



