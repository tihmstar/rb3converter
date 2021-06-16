from bs4 import BeautifulSoup
import requests
import sys
import os
import subprocess

url = "https://db.c3universe.com/songs"


def listSongsForSearch(query,limit = 0):
    ret = []
    print("Searching for '%s'"%query)
    data = {'search_text':query}
    page = 1
    while True:
        print("\tscanning page %d ..."%page)
        r = requests.post(url+"?page="+str(page), data=data)
        soup = BeautifulSoup(r.text,features="html.parser")
        for song in soup.find_all('tr', attrs={'class':'odd'}):
            name = song.find('div', attrs={'class':'c3ttitlemargin'})
            album = song.find('div', attrs={'class':'c3tartist'})
            if not name:
                continue
            columns = song.find_all('td')
            songTitle = name.contents[0][1:-1]
            songAlbum = album.contents[0][1:-1]
            songArtist = columns[2].contents[0].contents[0][1:-1]
            songUrl = columns[3].select_one("a[href*=song]").attrs["href"]
            s = {'title': songTitle, 'album' : songAlbum, 'artist' : songArtist, 'url': songUrl}
            ret.append(s)
        pages_bar = soup.find('div', attrs={'class':'paginationBody'})
        if limit and len(ret) >= limit:
            break
        if not pages_bar:
            #there was only one page of results
            break
        pages = pages_bar.find_all('a')
        if page < len(pages):
            page += 1
            continue
        break
    if limit:
        return ret[0:limit]
    return ret

def downloaderMediafire(url):
    r2 = requests.get(url)
    fname = url.split("/")[-2]
    soup = BeautifulSoup(r2.text,features="html.parser")
    u = soup.select_one("a[id=downloadButton]")
    downloadUrl = u.attrs["href"]
    print("\turlMediafire got %s"%downloadUrl)
    print("Downloading...")
    r = requests.get(downloadUrl)
    return r.content,fname


def downloaderC3Universe(url):
    r2 = requests.post(url)
    soup2 = BeautifulSoup(r2.text,features="html.parser")
    print("Getting downloadUrl")
    downloadUrl = soup2.select_one("a[href*=\"dl.c3universe.com\"]").attrs["href"]
    print("\turlC3Universe got %s"%downloadUrl)
    fname = downloadUrl.split("/")[-1]
    print("Downloading...")
    r = requests.get(downloadUrl)
    return r.content,fname

def downloaerGDrive(url):
    print("Getting downloadUrl")
    if "id=" in url:
        id = url.split("id=")[1].split("&")[0]
    elif "drive/folders/" in url:
        print("Skipping folders. Unimplemented, sorry!")
        return None,""
    else:
        url = url.split("?")[0]
        id = url.split("file/d/")[1].split("/")[0]
    downloadUrl = "https://drive.google.com/uc?export=download&id="+id
    print("\turlGDrive got %s"%downloadUrl)
    print("Downloading...")
    r = requests.get(downloadUrl)
    return r.content,id

def downloaerMega(url):
    s = url.split("/")[-1].split("#")
    if len(s) >=1:
        filename = s[1]
    else:
        filename = s[0]
    proc = subprocess.Popen(['megadl','--path=-',url],stdout=subprocess.PIPE)
    return proc.stdout.read(),filename

def downloaerDropbox(url):
    url = url.split("?")[0]
    filename = url.split("/")[-1]
    downloadUrl = url +"?dl=1"
    print("\turlDropbox got %s"%downloadUrl)
    print("Downloading...")
    r = requests.get(downloadUrl)
    return r.content,filename


def downloadSongFromSongPage(url_songpage):
    print("Getting nexturl for '%s'"%url_songpage)
    r = requests.post(url_songpage)
    soup = BeautifulSoup(r.text,features="html.parser")
    soupC3Universe = soup.select_one("a[href*=\"dl.c3universe.com\"]")
    if soupC3Universe:
        urlC3Universe = soupC3Universe.attrs["href"]
        print("\turlC3Universe got %s"%urlC3Universe)
        return downloaderC3Universe(urlC3Universe)

    soupMediafire = soup.select_one("a[href*=\"mediafire.com\"]")
    if soupMediafire:
            urlMediafire = soupMediafire.attrs["href"]
            print("\turlMediafire got %s"%urlMediafire)
            return downloaderMediafire(urlMediafire)

    soupGDrive = soup.select_one("a[href*=\"drive.google.com\"]")
    if soupGDrive:
            urlGDrive = soupGDrive.attrs["href"]
            print("\turlGDrive got %s"%urlGDrive)
            return downloaerGDrive(urlGDrive)

    soupMega = soup.select_one("a[href*=\"mega.nz\"]")
    if soupMega:
            urlMega = soupMega.attrs["href"]
            print("\turlMega got %s"%urlMega)
            return downloaerMega(urlMega)

    soupDropbox = soup.select_one("a[href*=\"dropbox.com\"]")
    if soupDropbox:
            urlDropbox = soupDropbox.attrs["href"]
            print("\turlDropbox got %s"%urlDropbox)
            return downloaerDropbox(urlDropbox)

    soupXBoxMarketplace = soup.select_one("a[href*=\"marketplace.xbox.com\"]")
    soupXBoxStore = soup.select_one("a[href*=\"store.xbox.com\"]")
    soupMicrosoft = soup.select_one("a[href*=\"microsoft.com\"]")
    if soupXBoxMarketplace or soupMicrosoft or soupXBoxStore:
        print("\tThis is a payed title! Can't download :(")
        return None,""

    soupBitly = soup.select_one("a[href*=\"bit.ly\"]")
    if soupBitly:
        urlBitly = soupBitly.attrs["href"]
        print("\tbit.ly links not supported -_-")
        return None,""

    soupNone = soup.select_one("a[href='none']")
    if soupNone:
        print("\tDownloadlink is missing? o.O")
        return None,""


def main():
    if len(sys.argv) < 2:
        print("Usage: %s <query> [limit]" % sys.argv[0])
        exit(0)

    justList = False

    if len(sys.argv) >= 3 and sys.argv[1] == "-l":
        justList = True
        print("Just listing, not downloading!")
        sys.argv = sys.argv[1:]

    query = sys.argv[1]
    limit = 0
    print("Got query '%s'"%query)
    if len(sys.argv) >=3:
        limit = int(sys.argv[2])
        print("Limiting to %d results" %limit )


    if "http" in query:
        data,filename = downloadSongFromSongPage(query)
        if data:
            with open(filename, "wb") as f:
                f.write(data)
        exit(0)

    songs = listSongsForSearch(query,limit)
    for song in songs:
        if justList:
            print(song)
            continue

        filename = song["title"]
        if len(song["album"]):
            filename += "_" +song["album"]
        if len(song["artist"]):
            filename += "_" +song["artist"]
        while " " in filename:
            filename = filename.replace(" ","-")
        while "/" in filename:
            filename = filename.replace("/","-")
        print("Processing '%s'" %filename)
        data,_ = downloadSongFromSongPage(song["url"])
        if data == None:
            print("Got no data, skipping!");
            continue
        with open(filename, "wb") as f:
            f.write(data)



main()
