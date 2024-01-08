## Group 40 PP Final Prokect : Parallel Web Crawl

### Enviroment Build 

#### libxml
在linux 22.04 環境下請執行
```
sudo apt-get update
sudo apt-get install libxml2 libxml2-dev
```
#### csv2 Reader/writer
在linux 22.04 環境下請執行
```
git clone https://github.com/p-ranav/csv2/tree/master

mkdir build && cd build
cmake -DCSV2_BUILD_TESTS=ON ..
make
cd test
./csv2_test

cd ../
python3 utils/amalgamate/amalgamate.py -c single_include.json -s .
``` 

接著請將在 csv2/include/ 中的 csv 資料夾複製到 /usr/include 資料夾下，即可直接使用g++ Ｃompile 執行。
形成以下結構：
/usr/include/csv2/
└── csv2
    ├── mio.hpp
    ├── parameters.hpp
    ├── reader.hpp
    └── writer.hpp

### OpenMP
.
├── openmp
│   ├── foo.csv
│   ├── Makefile
│   ├── parallel.cpp
│   └── web_craw_openmp_cookie18
└── pthread
    ├── ff_final.csv
    ├── Makefile
    ├── Parallel_pthread.cpp
    └── web_craw_pthread_cookie18

#### Compile Operation 
請在 openmp 資料夾下執行
```
make
```
會產出 web_craw_openmp_cookie18 執行檔，請依照 
```
./web_craw_openmp_cookie18 <Thread number> 
```
的方式輸入即可開始爬蟲。

### Pthread
.
├── openmp
│   ├── foo.csv
│   ├── Makefile
│   ├── parallel.cpp
│   └── web_craw_openmp_cookie18
└── pthread
    ├── ff_final.csv
    ├── Makefile
    ├── Parallel_pthread.cpp
    └── web_craw_pthread_cookie18

#### Compiler Operation
請在 pthread/ 下執行
```
make
```
會產出 web_craw_pthread_cookie18 此執行檔，請依照 
```
web_craw_pthread_cookie18 <Thread number> 
```
的方式輸入即可開始爬蟲。
