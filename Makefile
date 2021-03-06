CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99 -g
LIBLINK = -lcurl -lpthread -lm
all: manga-dl

manga-dl: src/tmdl.o src/networking.o src/generalMethods.o src/kissMangaRead.o src/kissMangaDownload.o src/chaptersToDownload.o src/currentChapter.o src/blacklist.o src/hashMap.o src/customParser.o src/experimental.o src/mangaSeeSupport.o src/updater.o
	${CC} ${CFLAGS} $^ -o $@ ${LIBLINK}

tmdl.o: src/tmdl.c src/tmdl.h
	${CC} ${CFLAGS} -c $< -o src/$@

networking.o: src/networking.c src/networking.h
	${CC} ${CFLAGS} -c src/$<

generalMethods.o: src/generalMethods.c src/generalMethods.h
	${CC} ${CFLAGS} -c src/$<

kissMangaRead.o: src/kissMangaRead.c src/kissMangaRead.h
	${CC} ${CFLAGS} -c src/$<

kissMangaDownload.o: src/kissMangaDownload.c src/kissMangaDownload.h
	${CC} ${CFLAGS} -c src/$<

chaptersToDownload.o: src/chaptersToDownload.c src/chaptersToDownload.h
	${CC} ${CFLAGS} -c src/$<

currentChapter.o : src/currentChapter.c src/currentChapter.h
	${CC} ${CFLAGS} -c src/$<

hashMap.o : src/hashMap.c src/hashMap.h
	${CC} ${CFLAGS} -c src/$<

blacklist.o : src/blacklist.c src/blacklist.h
	${CC} ${CFLAGS} -c src/$<

customParser.o : src/customParser.c src/customParser.h
	${CC} ${CFLAGS} -c src/$<

experimental.o: src/experimental.c src/experimental.h
	${CC} ${CFLAGS} -c src/$<

mangaSeeSupport.o: src/mangaSeeSupport.c src/mangaSeeSupport.h
	${CC} ${CFLAGS} -c src/$<

updater.o: src/updater.c src/updater.h
	${CC} ${CFLAGS} -c src/$<

clean:
	find . -type f -name '*.o' -delete && find . -type f -name '*.gch' -delete && rm -f manga-dl

install:
	cp manga-dl /usr/local/bin && cp ./man/manga-dl.1 /usr/local/share/man/man1/
uninstall:
	rm -f /usr/local/bin/manga-dl && rm -f /usr/local/share/man/man1/manga-dl.1
