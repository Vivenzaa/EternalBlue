from subprocess import run as sub_run
from subprocess import PIPE, Popen, CalledProcessError
from threading import Thread
from scrapetube import get_channel
from pytubefix import YouTube
from random import choices, shuffle
from json import loads
from time import sleep
from os import system, remove, listdir, rename
from os.path import exists
from twitchAPI.twitch import Twitch
from twitchAPI.oauth import refresh_access_token, revoke_token
from twitchAPI.type import AuthScope
from twitchAPI.helper import first
from asyncio import run



# Configurations Twitch
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
CATEGORY_IDS = [721077, 45067, 2816, 51914, 3499, 2989, 256, 509663]
# you can find category id's on https://github.com/Nerothos/TwithGameList
STREAM_TAGS = ["tags", "on", "stream"]
SERVER_CONFIG = ["server.ip", 22, "usernameOnServer", "passwordOnServer", "PathOnServer", "localPathToStoreFiles", "PathToSSHKeys/id_rsa"]



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


def download_from_server(remote_path, local_path):
    try:

        # Construire la commande rsync
        command = [
            "rsync",
            "-avz",  # Options rsync
            "-e", f"ssh -i {SERVER_CONFIG[6]}",  # Utiliser la clé privée
            f"{SERVER_CONFIG[2]}@{SERVER_CONFIG[0]}:{remote_path}",  # Source distante
            local_path  # Destination locale
        ]

        # Exécuter la commande
        sub_run(command, check=True)
        print(f"Fichiers téléchargés avec succès depuis {SERVER_CONFIG[0]}:{remote_path} vers {local_path}")
    except CalledProcessError as e:
        print(f"Erreur lors de l'exécution de rsync : {e}")
    except Exception as e:
        print(f"Erreur : {e}")


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
        rename(f"\"{youtubeObject.title}_full.flv\"", f"\"videos/{youtubeObject.title}.flv\"")
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
    a = listdir(SERVER_CONFIG[5])
    for i in range(len(a)):
        tmp = a[i]
        if tmp[-35:] == " (1080p_60fps_H264-128kbits_AAC).mp4":
            a[i] = f"{tmp[:-35]}.mp4"
            rename(f"\"{tmp}\"", f"\"{a[i]}\"")
            print(f"changed name of {tmp} to {a[i]}")


def get_all_files():
    fichier = open("files.txt", 'r')
    returnedValue = fichier.readlines()
    fichier.close()
    return returnedValue


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
        if toReturn >= 6000:
            return 6000
        return toReturn
    except Exception as e:
        print(f"An error occurred: {e}")
        return None


def start_ffmpeg_process(playlist):
    global CURRENT_VIDEO_PATH
    global TRIGGERED_NAME
    global SERVER_CONFIG
    try:
        
        video = chooseVideo(playlist)
        while video == CURRENT_VIDEO_PATH:
            video = chooseVideo(playlist)
        CURRENT_VIDEO_PATH = video
        download_from_server(SERVER_CONFIG[4] + CURRENT_VIDEO_PATH, CURRENT_VIDEO_PATH)
        
        while True:
            try:
                CURRENT_VIDEO_PATH = video
                print(f"Streaming video: {CURRENT_VIDEO_PATH}")
                # Commande FFmpeg pour une seule vidéo
                command = [
                    "ffmpeg",
                    "-re",  # Lecture en temps réel
                    "-i", CURRENT_VIDEO_PATH,
                    "-c:v", "libx264",  # Codec vidéo
                    "-preset", "superfast",         #veryfast crée des problème de Buffer sur Twitch
                    "-b:v", f"{get_video_bitrate(CURRENT_VIDEO_PATH)}k",  # Débit vidéo
                    "-maxrate", "6050k",        #50k au dessus de la limite pck on est des déglingots
                    "-bufsize", "9000k",
                    "-c:a", "aac",
                    "-b:a", "128k",
                    "-rtbufsize", "64M",  # Taille du buffer d'entrée
                    "-rtmp_buffer", "2500",  # Taille du tampon RTMP
                    "-flvflags", "no_duration_filesize",  # Supprime les métadonnées superflues
                    "-f", "flv",
                    f"rtmp://live.twitch.tv/app/{STREAM_KEY}"
                ]

                process = Popen(command, stdout=PIPE, stderr=PIPE)
                TRIGGERED_NAME = True
                old = video
                video = chooseVideo(playlist)
                while video == old:
                    video = chooseVideo(playlist)
                download_from_server(SERVER_CONFIG[4] + video, video)
                

                _, stderr = process.communicate()  # Attendre la fin du processus
                remove(old)



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
    global STREAM_TAGS
    global TWITCH_APP_ID
    global TWITCH_APP_SECRET


    ACCESS_TOKEN, REFRESH_TOKEN = await refresh_access_token(REFRESH_TOKEN, TWITCH_APP_ID, TWITCH_APP_SECRET)
    twitch = await Twitch(TWITCH_APP_ID, TWITCH_APP_SECRET)
    target_scope = [AuthScope.CHANNEL_MANAGE_BROADCAST, AuthScope.USER_EDIT_BROADCAST]

    await twitch.set_user_authentication(ACCESS_TOKEN, target_scope, REFRESH_TOKEN)

    channel_id = await first(twitch.get_users(logins=[CURRENT_CHANNEL_NAME]))
    channel_id = channel_id.id


    pathlist = get_all_files()
    for i in range(len(pathlist)):
        pathlist[i] = pathlist[i][:-1]          #removes the \n character
    #urls = getAllUrls()
    
    t1 = Thread(target=start_ffmpeg_process, args=[pathlist])
    t1.start()
    
    try:
        while True:
            if TRIGGERED_NAME:
                await twitch.modify_channel_information(
                    channel_id,
                    CATEGORY_IDS[getGame(getTitle(CURRENT_VIDEO_PATH)[len(SERVER_CONFIG[5]):])],
                    "fr",
                    getTitle(CURRENT_VIDEO_PATH)[len(SERVER_CONFIG[5]):] + " - [REDIFFUSION]",
                    tags=STREAM_TAGS
                    )
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



