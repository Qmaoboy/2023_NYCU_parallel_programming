cxx=g++
cxx_flags=-lcurl -lgumbo -std=c++11

all:web_carwler


web_carwler:
	${cxx} Parallel_pthread.cpp ${cxx_flags} -o Parallel_pthread -I/usr/include/libxml2

clean:
	rm -rf *.o *.s web_scrape *~