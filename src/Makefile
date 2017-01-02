CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99
all: tmdl

tmdl: tmdl.o networking.o generalMethods.o kissMangaRead.o kissMangaDownload.o chaptersToDownload.o
	${CC} ${CFLAGS} $^ -o $@

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

clean:
	rm *.o
