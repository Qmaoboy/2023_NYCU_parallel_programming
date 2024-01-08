cxx=g++
cxx_flags=-lcurl -lgumbo -lxml2 -fopenmp -pthread -std=c++11

all:pthread

pthread:
	${cxx} Parallel_pthread.cpp ${cxx_flags} -o web_craw_pthread_cookie18 -I/usr/include/libxml2

clean:
	rm -rf *.o *.s web_scrape *~