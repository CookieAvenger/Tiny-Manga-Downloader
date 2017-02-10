CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99#-g
LIBLINK = -lcurl -lpthread -lm
all: tmdl

tmdl: tmdl.o networking.o generalMethods.o kissMangaRead.o kissMangaDownload.o chaptersToDownload.o currentChapter.o blacklist.o hashMap.o customParser.o experimental.o
	${CC} ${CFLAGS} $^ -o $@ ${LIBLINK}

tmdl.o: tmdl.c tmdl.h
	${CC} ${CFLAGS} -c $<

networking.o: networking.c networking.h
	${CC} ${CFLAGS} -c $<

generalMethods.o: generalMethods.c generalMethods.h
	${CC} ${CFLAGS} -c $<

kissMangaRead.o: kissMangaRead.c kissMangaRead.h
	${CC} ${CFLAGS} -c $<

kissMangaDownload.o: kissMangaDownload.c kissMangaDownload.h
	${CC} ${CFLAGS} -c $<

chaptersToDownload.o: chaptersToDownload.c chaptersToDownload.h
	${CC} ${CFLAGS} -c $<

currentChapter.o : currentChapter.c currentChapter.h
	${CC} ${CFLAGS} -c $<

hashMap.o : hashMap.c hashMap.h
	${CC} ${CFLAGS} -c $<

blacklist.o : blacklist.c blacklist.h
	${CC} ${CFLAGS} -c $<

customParser.o : customParser.c customParser.h
	${CC} ${CFLAGS} -c $<

experimental.o: experimental.c experimental.h
	${CC} ${CFLAGS} -c $<

clean:
	find . -type f -name '*.o' -delete && find . -type f -name '*.gch' -delete
