#include <iostream>
#include <cstdlib>
#include <curl/curl.h>
#include <gumbo.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <vector>
#include <string>
#include <fstream>
#include <omp.h>
#include <csv2/reader.hpp>
#include <csv2/writer.hpp>

using namespace std;
using namespace csv2;

class ArticleData {
public:
    std::string author;sa_family_t
    std::string title;
    std::string time;
    std::string band;
    std::string content;

    ArticleData(std::string author, std::string title, std::string time, std::string band, std::string content)
        : author(author), title(title), time(time), band(band), content(content) {}
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 用於保護共享資源的 mutex

// 使用libxml2解析HTML並執行XPath查詢，將結果存儲在vector中
std::vector<std::string> findTitles(const std::string& html) {
    std::vector<std::string> titles;
    htmlDocPtr doc = htmlReadMemory(html.c_str(), html.size(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == NULL) {
        std::cerr << "Error parsing HTML document." << std::endl;
        return titles;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        std::cerr << "Error creating XPath context." << std::endl;
        xmlFreeDoc(doc);
        return titles;
    }

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)"//div[@class='title']/a", xpathCtx);
    if (xpathObj == NULL) {
        std::cerr << "Error evaluating XPath expression." << std::endl;
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return titles;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    if (nodes != NULL) {
        for (int i = 0; i < nodes->nodeNr; ++i) {
            xmlNodePtr node = nodes->nodeTab[i];
            if (node->children != NULL) {
                // 在这里，我们假设node是一个<a>标签
                xmlChar* href = xmlGetProp(node, (const xmlChar*)"href");
                if (href != NULL) {
                    string url = "https://www.ptt.cc" + string(reinterpret_cast<const char*>(href));
                    titles.push_back(url);
                    xmlFree(href);  // 记得释放分配给href的内存
                }
            }
        }
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return titles;
}

ArticleData findArticleData(const std::string& html) {
    std::string author, title, time, band, content;
    htmlDocPtr doc = htmlReadMemory(html.c_str(), html.size(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == NULL) {
        std::cerr << "Error parsing HTML document." << std::endl;
        return ArticleData("", "", "", "", "");
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        std::cerr << "Error creating XPath context." << std::endl;
        xmlFreeDoc(doc);
        return ArticleData("", "", "", "", "");
    }

    // Fetching metadata
    xmlXPathObjectPtr metaObj = xmlXPathEvalExpression((const xmlChar*)"//span[@class='article-meta-value']", xpathCtx);
    if (metaObj != NULL) {
        xmlNodeSetPtr nodes = metaObj->nodesetval;
        if (nodes != NULL) {
            int nodeCount = (nodes->nodeNr < 4) ? nodes->nodeNr : 4; // Assuming there are at least 4 metadata nodes
            for (int i = 0; i < nodeCount; ++i) {
                xmlNodePtr node = nodes->nodeTab[i];
                xmlChar* metaContent = xmlNodeGetContent(node);
                if (metaContent != NULL) {
                    switch (i) {
                    case 0: author = reinterpret_cast<const char*>(metaContent); break;
                    case 1: band = reinterpret_cast<const char*>(metaContent); break;
                    case 2: title  = reinterpret_cast<const char*>(metaContent); break;
                    case 3: time  = reinterpret_cast<const char*>(metaContent); break;
                    }
                    xmlFree(metaContent);
                }
            }
        }
        xmlXPathFreeObject(metaObj);
    }

    // Fetching main content
    xmlXPathObjectPtr contentObj = xmlXPathEvalExpression((const xmlChar*)"//*[@id='main-content']", xpathCtx);
    if (contentObj != NULL) {
        xmlNodeSetPtr nodes = contentObj->nodesetval;
        if (nodes != NULL && nodes->nodeNr > 0) {
            xmlNodePtr node = nodes->nodeTab[0];
            if (node != NULL) {
                xmlChar* contentChar = xmlNodeGetContent(node);
                if (contentChar != NULL) {
                    content = reinterpret_cast<const char*>(contentChar);
                    xmlFree(contentChar);
                }
            }
        }
        xmlXPathFreeObject(contentObj);
    }

    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return ArticleData(author, title, time, band, content);
}

// cURL寫回調函數
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch (std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

// 爬取網頁
string fetchHTML(const string& url) {
    // ...
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            cerr << "cURL error: " << curl_easy_strerror(res) << endl;
        }
    }
    return readBuffer;
}

vector<string> getUrls(string pageUrl) {
    // ...
    string url = pageUrl;
    string html = fetchHTML(url);

    //找title
    std::vector<std::string> urls = findTitles(html);

    return urls;
}

ArticleData getArticle(string articleUrl) {
    // ...
    string html = fetchHTML(articleUrl);
    ArticleData article = findArticleData(html);
    return article;
}

// 線程函數，處理每個 URL
void* processUrl(void* arg) {
    string url = *(static_cast<string*>(arg));

    // ...

    // 進行爬蟲並寫入 CSV
    pthread_mutex_lock(&mutex);
    string fileName = "ff_final.csv";
    ofstream stream(fileName, std::ios_base::app); // 開啟檔案以追加模式寫入
    Writer<delimiter<','>> writer(stream);
    ArticleData article = getArticle(url);
    cout << article.title << endl;
    std::vector<std::vector<std::string>> rows = { { article.title, article.time, article.content } };
    writer.write_rows(rows);
    stream.close();
    pthread_mutex_unlock(&mutex);

    return NULL;
}





int main() {
    vector<string> urls;

    // ...
    for (int i = 4000; i < 4001; i++) {
        string pageUrl = "https://www.ptt.cc/bbs/HatePolitics/index" + to_string(i) + ".html";
        vector<string> pageUrls = getUrls(pageUrl);
        urls.insert(urls.end(), pageUrls.begin(), pageUrls.end());
    }

    // 創建一個 pthread 數組
    pthread_t* threads = new pthread_t[urls.size()];

    // 初始化 mutex
    pthread_mutex_init(&mutex, NULL);

    // 創建每個 URL 的線程
    for (size_t i = 0; i < urls.size(); ++i) {
        pthread_create(&threads[i], NULL, processUrl, &urls[i]);
    }

    // 等待每個線程完成
    for (size_t i = 0; i < urls.size(); ++i) {
        pthread_join(threads[i], NULL);
    }

    // 釋放資源
    delete[] threads;
    pthread_mutex_destroy(&mutex);

    return 0;
}
