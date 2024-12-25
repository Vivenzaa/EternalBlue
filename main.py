######################################### THIS SCRIPT IS ONLY SUITABLE FOR LINUX, some changes may be needed to run it on others systems such as Windows ######################################### 
from subprocess import run as sub_run
from subprocess import PIPE, Popen
from threading import Thread
from scrapetube import get_channel
from pytubefix import YouTube
from random import choices, shuffle
from json import loads
from time import sleep
from os import system, remove, listdir
from os.path import exists
from twitchAPI.twitch import Twitch
from twitchAPI.oauth import refresh_access_token, revoke_token
from twitchAPI.type import AuthScope
from twitchAPI.helper import first
from asyncio import run



CHANNEL_ID = "id of the channel you're streaming to"
TWITCH_APP_ID = "pretty obvious"
TWITCH_APP_SECRET = "twitch app's secret string"
ACCESS_TOKEN = "twitch app's token"
REFRESH_TOKEN = "twitch app's refresh token"
STREAM_KEY =  "stream_key"
CURRENT_CHANNEL_NAME = "name of the channel you're streaming to"
CURRENT_VIDEO_PATH = -1
TIMER_BOOL = False
TRIGGER_REFRESH_TOKEN = False
TRIGGERED_NAME = False
WORDLIST = [ 
    ["words"],
    ["that"],
    ["identifies"],
    ["games"],
    ["contained"],
    ["in"],
    ["the"],
    ["title"] #if no game is found, this option is chosen (by default : special event)
    ]
CATEGORY_IDS = [721077, 45067, 2816, 51914, 3499, 2989, 256, 509663] #category id's to pass to modify_channel_informations (last one is choen by default)
# you can find id's on https://github.com/Nerothos/TwithGameList
STREAM_TAGS = ["247Stream", "Français", "KarmineCorp"] #tags to put on your stream



def fillPlaylist(filepath, currentPlaylist):
    global CURRENT_PLAYLIST
    if currentPlaylist == 1:
        fichier = open("play2.txt", 'w')
        fichier.write(f"file \"{filepath}\"\nfile \"play1.txt\"")
        CURRENT_PLAYLIST = 2
    else:
        fichier = open("play1.txt", 'w')
        fichier.write(f"file \"{filepath}\"\nfile \"play2.txt\"") 
        CURRENT_PLAYLIST = 1


def getAllUrls():
    print("Fetching all Urls...")
    videos = get_channel(channel_username="KarmineCorpVOD")
    videoIds = []
    for v in videos:
        videoIds.append(v['videoId'])

    return videoIds


def Download(u):        #needs to be fixed, doesn't work (need to download with an account's cookies/credentials)
    url = "https://www.youtube.com/watch?v={}".format(u)
    youtubeObject = YouTube(url)
    print("downloading {}...".format(youtubeObject.title))
    video_stream = youtubeObject.streams.filter(res="1080p", file_extension="mp4", progressive=False).first()
    audio_stream = youtubeObject.streams.filter(only_audio=True, file_extension="mp4").first()
    if video_stream == None or audio_stream == None:
        print("error while fetching video/audio data, maybe video doen't exists in 1080p")
        return -1
    try:
        if exists(f"videos/{youtubeObject.title}.flv"):
            print("file already exists")
            return f"videos/{youtubeObject.title}.flv"
        video_stream.download()
        audio_stream.download()
        system("ffmpeg -i \"./{}.mp4\" -i \"./{}.m4a\" -c:v copy -c:a aac \"./{}_full.flv\"".format(youtubeObject.title, youtubeObject.title, youtubeObject.title))
        remove(f"{youtubeObject.title}.mp4")
        remove(f"{youtubeObject.title}.m4a")
        system(f"mv \"{youtubeObject.title}_full.flv\" \"videos/{youtubeObject.title}.flv\"")
    except:
        print("couldn't download Youtube video : {}".format(url))
        return -1
    print("Download is completed successfully")
    return f"videos/{youtubeObject.title}.flv"


def chooseVideo(urlList):
    n = len(urlList)
    poids = [n - i for i in range(n)]           # algo de distribution de proba

    somme_poids = sum(poids)
    poids_normalises = [p / somme_poids for p in poids]
    return choices(urlList, weights=poids_normalises, k=1)[0]
    

def get_video_duration(video_path):
    """
    Retourne la durée totale d'une vidéo en secondes (float)
    """
    try:
        cmd = [
            "ffprobe", 
            "-v", "error",
            "-select_streams", "v:0",
            "-show_entries", "format=duration",
            "-of", "json",
            video_path
        ]
        result = sub_run(cmd, stdout=PIPE, stderr=PIPE, text=True)
        metadata = loads(result.stdout)
        duration = float(metadata["format"]["duration"])
        return duration
    except Exception as e:
        print(f"Erreur lors de la récupération de la durée : {e}")
        return None


def countdown(timeto):
    global TIMER_BOOL
    for _ in range(int(round(timeto))):
        sleep(1)
    TIMER_BOOL = True
    return True


def countdown_to_token():
    global TRIGGER_REFRESH_TOKEN
    sleep(10100)
    TRIGGER_REFRESH_TOKEN = True


def getTitle(input, from_url = False):
    if from_url:
        input = "https://www.youtube.com/watch?v={}".format(input)
        return YouTube(input).title
    return input[:-4]


def getGame(u, from_url = False):
    if from_url:
        title = getTitle(u).lower()
    else:
        title = u.lower()
    for i in range(len(WORDLIST)):
        for j in WORDLIST[i]:
            if j in title:
                return i
    return -1


def merge(lstOfLsts):
    final = []
    for i in range(len(lstOfLsts[0])):
        final.append([])
        for j in range(len(lstOfLsts)):
            final[-1].append(lstOfLsts[j][i])
    return final


def clearTitle():
    a = get_all_files()
    for i in range(len(a)):
        tmp = a[i]
        if tmp[-36:] == " (1080p_60fps_H264-128kbits_AAC).mp4":
            a[i] = f"{tmp[:-37]}.mp4"
            system(f"mv \"{tmp}\" \"{a[i]}\"")
            print(f"changed name of {tmp} to {a[i]}")


def get_all_files():
    a = listdir("videos/")
    for i in range(len(a)):
        a[i] = f"videos/{a[i]}"
    return a


def get_playlist():
    pathlist = get_all_files()
    shuffle(pathlist)
    if len(pathlist) > 2:
        return [pathlist[0], pathlist[1], pathlist[2]]
    return pathlist


def save_tokens(access, refresh):
    with open("tokens.cred", "w") as fichier:
        fichier.write(f"{access}\n{refresh}")


def read_tokens():
    with open("tokens.cred", "r") as fichier:
        access = fichier.read(30)
        fichier.read(1)
        refresh = fichier.read(50)
        return [access, refresh]


def get_video_bitrate(video_path):
    try:
        # Run the ffprobe command
        result = sub_run(
            [
                'ffprobe', 
                '-v', 'error',
                '-select_streams', 'v:0',
                '-show_entries', 'stream=bit_rate',
                '-of', 'json',
                video_path
            ],
            stdout=PIPE,
            stderr=PIPE,
            text=True
        )

        toReturn = int(loads(result.stdout)['streams'][0]['bit_rate']) // 1000
        if toReturn + 400 >= 6000:
            return 5650
        return toReturn
    except Exception as e:
        print(f"An error occurred: {e}")
        return None


def start_ffmpeg_process(playlist):
    global CURRENT_VIDEO_PATH
    global TRIGGERED_NAME
    try:
        while True:
            try:
                video = chooseVideo(playlist)
                while video == CURRENT_VIDEO_PATH:
                    video = chooseVideo(playlist)
                CURRENT_VIDEO_PATH = video

                print(f"Streaming video: {CURRENT_VIDEO_PATH}")
                # Commande FFmpeg pour une seule vidéo
                command = [
                    "ffmpeg",
                    "-re",  # Lecture en temps réel
                    "-i", CURRENT_VIDEO_PATH,  # Vidéo actuelle
                    "-c:v", "libx264",  # Codec vidéo
                    "-preset", "veryfast",  # Profil d'encodage
                    "-b:v", f"{get_video_bitrate(CURRENT_VIDEO_PATH) + 400}k",  # Débit vidéo
                    "-c:a", "aac",  # Codec audio
                    "-b:a", "128k",  # Débit audio
                    "-rtbufsize", "6M",  # Taille du buffer
                    "-f", "flv",  # Format de sortie
                    "-rtmp_buffer", "1000",
                    f"rtmp://live.twitch.tv/app/{STREAM_KEY}"
                ]
                process = Popen(command, stdout=PIPE, stderr=PIPE)
                TRIGGERED_NAME = True
                _, stderr = process.communicate()  # Attendre la fin du processus

                if process.returncode != 0:
                    print(f"FFmpeg error: {stderr.decode('utf-8')}")
                    print("Restarting FFmpeg for the next video...")
                else:
                    print(f"{CURRENT_VIDEO_PATH} streamed successfully.")
            except KeyboardInterrupt:
                break

    except Exception as e:
        print(f"An error occurred: {e}")


async def main():
    global ACCESS_TOKEN
    global REFRESH_TOKEN
    global TRIGGERED_NAME
    global TRIGGER_REFRESH_TOKEN
    global CURRENT_VIDEO_PATH

    ACCESS_TOKEN, REFRESH_TOKEN = await refresh_access_token(REFRESH_TOKEN, TWITCH_APP_ID, TWITCH_APP_SECRET)
    twitch = await Twitch(TWITCH_APP_ID, TWITCH_APP_SECRET)
    target_scope = [AuthScope.CHANNEL_MANAGE_BROADCAST, AuthScope.USER_EDIT_BROADCAST]

    await twitch.set_user_authentication(ACCESS_TOKEN, target_scope, REFRESH_TOKEN)

    channel_id = await first(twitch.get_users(logins=[CURRENT_CHANNEL_NAME]))
    channel_id = channel_id.id


    clearTitle()
    #urls = getAllUrls()
    pathlist = get_all_files()
    
    t1 = Thread(target=start_ffmpeg_process, args=[pathlist])
    t1.start()
    
    try:
        while True:
            if TRIGGERED_NAME:
                print(f"changed streams infos to game_id : {CATEGORY_IDS[getGame(getTitle(CURRENT_VIDEO_PATH)[7:])]} and title : {getTitle(CURRENT_VIDEO_PATH)[7:]}")
                await twitch.modify_channel_information(channel_id, CATEGORY_IDS[getGame(getTitle(CURRENT_VIDEO_PATH)[7:])], "fr", getTitle(CURRENT_VIDEO_PATH)[7:], tags=STREAM_TAGS)
                TRIGGERED_NAME = False
            if TRIGGER_REFRESH_TOKEN:
                TRIGGER_REFRESH_TOKEN = False
                await revoke_token(TWITCH_APP_ID, ACCESS_TOKEN)
                ACCESS_TOKEN, REFRESH_TOKEN = await refresh_access_token(REFRESH_TOKEN, TWITCH_APP_ID, TWITCH_APP_SECRET)
                ttoken = Thread(target=countdown_to_token, args=[])
                ttoken.start()
            
    except KeyboardInterrupt:
        print("Arrêt du stream.")


if __name__ == "__main__":
    run(main())



