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
#include <sstream>
#include <chrono>
using namespace std;
using namespace csv2;

class ArticleData {
public:
    std::string author;
    std::string title;
    std::string time;
    std::string band;
    std::string content;

    ArticleData(std::string author, std::string title, std::string time, std::string band, std::string content)
        : author(author), title(title), time(time), band(band), content(content) {}
};


// 使用libxml2解析HTML并执行XPath查询，将结果存储在vector中
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

//
// cURL 寫回調函數
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    }
    catch (std::bad_alloc& e) {
        // 處理記憶體不足
        return 0;
    }
    return newLength;
}

//// 爬取網頁
string fetchHTML(const string& url) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");
        headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");

        // Add the "18 years old" token header
        headers = curl_slist_append(headers, "Cookie: _gid=GA1.2.1254144763.1704695744; _gat=1; over18=1; _ga=GA1.1.925644845.1704695744; _ga_DZ6Y3BY9GW=GS1.1.1704695743.1.1.1704695745.0.0.0");

        // Add more headers if needed
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
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
    string url = pageUrl;
    string html = fetchHTML(url);

    //找title
    std::vector<std::string> urls = findTitles(html);

    return urls;
//    std::ofstream outFile("output.txt", std::ios::out | std::ios::binary); // 使用二进制模式以保留UTF-8编码
//
//    if (!outFile) {
//        std::cerr << "无法打开文件用于写入" << std::endl;
//        return 1;
//    }
//for (const auto& title : titles) {
//		outFile << title << std::endl;
//	}
//
//	outFile.close();
}

ArticleData getArticle(string articleUrl) {
    string html = fetchHTML(articleUrl);
    ArticleData article = findArticleData(html);
    return article;
}

int main(int argc, char* argv[]) {
    //執行時間
    auto start = std::chrono::high_resolution_clock::now();
    int numThreads = 1;
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <num_threads>" << endl;
        return 1;
    }
    numThreads=atoi(argv[1]);
    cout << "Consumer Thread "<< numThreads<< endl;
    vector<string> urls;
    omp_set_num_threads(numThreads);
#pragma omp parallel for
    for (int i = 4000; i < 4001; i++) {
        string pageUrl = "https://www.ptt.cc/bbs/HatePolitics/index" + to_string(i) + ".html";
        vector<string> pageUrls = getUrls(pageUrl);
        urls.insert(urls.end(), pageUrls.begin(), pageUrls.end());
    }
    for (int i = 950; i < 951; i++) {
        string pageUrl = "https://www.ptt.cc/bbs/politics/index" + to_string(i) + ".html";
        vector<string> pageUrls = getUrls(pageUrl);
        urls.insert(urls.end(), pageUrls.begin(), pageUrls.end());
    }
    for (int i = 39058; i < 39059; i++) {
        string pageUrl = "https://www.ptt.cc/bbs/Gossiping/index" + to_string(i) + ".html";
        vector<string> pageUrls = getUrls(pageUrl);
        urls.insert(urls.end(), pageUrls.begin(), pageUrls.end());
    }
    //for (const std::string& title : urls) {
    //    std::cout << title << std::endl;
    //}
#pragma omp parallel for
    for (int i = 0; i < urls.size(); ++i) {
        // 注意: 由於這裡使用了多執行緒，請確保您的處理邏輯是執行緒安全的
        // 在此處理每個 URL
        std::cout << "Thread " << omp_get_thread_num() << " processing " << urls[i] << std::endl;
        string url = urls[i];


        // 创建文件输出流
        std::ofstream stream("foo.csv", std::ios_base::app);
        // 创建 csv2 的 Writer 对象
        Writer<delimiter<','>> writer(stream);

        ArticleData article = getArticle(url);
        cout << article.title << endl;

        std::vector<std::vector<std::string>> rows =
        {
           { article.title, article.time, article.content }

        };
        writer.write_rows(rows);
        //// 写入头部
        stream.close();
    }

    // 获取结束时间点
    auto finish = std::chrono::high_resolution_clock::now();
    // 计算经过的时间
    std::chrono::duration<double> elapsed = (finish - start)/60;
    // 输出结果
    std::cout << "Elapsed time: " << elapsed.count() << " mins" << std::endl;


    return 0;
}
