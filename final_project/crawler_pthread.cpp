#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "web.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <queue>
#include <getopt.h>
#include <hash_set>
#include "curl/curl.h"
#include "string.h"
#include <gumbo.h>
#include "algorithm"
using namespace std;

#define MAX_URL_NUM 1
#define MAX_URL_LENGTH 256


const char *url_list[MAX_URL_NUM];

void freeUrlListMemory() {
    // 釋放 url_list 中的內存
    for (int i = 0; i < MAX_URL_NUM; ++i) {
        free((void*)url_list[i]);
    }
}

void writeDataToGlobalArray() {
    // 寫入資料到全域陣列
    int baseline=1986;
    int index=0;
    char buffer[50];
    string currenturl;
    for (int i = 0; i < MAX_URL_NUM; ++i) {
        index=baseline-i;
        // currenturl="https://www.ptt.cc/bbs/CrossStrait/index"+to_string(index)+".html";
        sprintf(buffer,"https://www.ptt.cc/bbs/CrossStrait/index%d.html",index);
        url_list[i] = strdup(buffer);
        cout<<buffer<<endl;
    };
    
}
typedef struct{
    char url[MAX_URL_LENGTH];
    int thread_id;
}ThreadArgs;

typedef struct {
    char *data;
    char *title = nullptr;
    char *href = nullptr;
    size_t size;
}MemoryStruct;

static size_t WriteMemoryCallback(void *contents, size_t size ,size_t nmemb,void *userp){
    size_t realsize=size*nmemb;
    MemoryStruct *mem = (MemoryStruct *) userp;
    
    mem->data =  (char *)realloc(mem->data,mem->size + realsize + 1);
    
    if (mem->data == NULL){
        printf("Not Enough Memory (readlloc return NULL)\n");
        return 0;
    }
    memcpy(&(mem->data[mem->size]),contents,realsize);
    mem->size+=realsize;
    mem->data[mem->size]=0;
    return realsize;
}


// Function to get elements by tag name
static void getElements(GumboNode *node, const char *tagName, GumboNode **elements, int *count) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboTag tag = node->v.element.tag;
        if (tag == gumbo_tag_enum(tagName)) {
            elements[(*count)++] = node;
        }
    }

    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        getElements(static_cast<GumboNode *>(children->data[i]), tagName, elements, count);
    }
}

// Function to find elements by class
static void findElementsByClass(GumboNode *node, const char *className, GumboNode **elements, int *count) {
    if (*count >= MAX_URL_NUM) {
        return;  // 避免超出陣列範圍
    }

   if (node->type == GUMBO_NODE_ELEMENT) {
        GumboAttribute *classAttr = gumbo_get_attribute(&node->v.element.attributes, "class");
        if (classAttr && classAttr->value && strstr(classAttr->value, className)) {
            elements[(*count)++] = node;
        }
    }
    GumboVector *children = &node->v.element.children;

    for (unsigned int i = 0; i < children->length; ++i) {
        GumboNode *childNode = static_cast<GumboNode *>(children->data[i]);
        findElementsByClass(childNode, className, elements, count);
    }
}


// Function to get attribute value by name
static std::string getAttributeValue(GumboNode *node, const char *attrName) {
    GumboAttribute *attr = gumbo_get_attribute(&node->v.element.attributes, attrName);
    return attr ? std::string(attr->value) : "";
}

// Function to get text content of a node
static std::string getTextContent(GumboNode *node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    }

    if (node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_DOCUMENT) {
        std::string textContent;
        GumboVector *children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            textContent += getTextContent(static_cast<GumboNode *>(children->data[i]));
        }
        return textContent;
    }

    return "";
}

static void parseHTML(const char *html, MemoryStruct *chunk) {
    GumboOutput *output = gumbo_parse(html);
    if (output) {
        // Find elements with class "title"
        GumboNode *titleElements[MAX_URL_NUM];
        int titleCount = 0;
        findElementsByClass(output->root, "title", titleElements, &titleCount);
        cout<<"OK"<<endl;
        // Process each title element
        for (int i = 0; i < titleCount; ++i) {
            GumboNode *titleElement = titleElements[i];
            if (titleElement){
            std::string href = getAttributeValue(titleElement, "href");
            std::string textContent = getTextContent(titleElement);
            // Allocate memory for chunk->title and chunk->href
            // size_t len = textContent.length() + 1;
            // chunk->title = (char *)realloc(chunk->title, len);
            // snprintf(chunk->title, len, "%s", textContent.c_str());

            // len = href.length() + 1;
            // chunk->href = (char *)realloc(chunk->href, len);
            // snprintf(chunk->href, len, "%s", href.c_str());
            // Print href and text content
            std::cout << "Href: " << href << ", Text: " << textContent << std::endl;
            }
        }

        // Clean up GumboOutput
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    } else {
        std::cout << "Gumbo parsing failed." << std::endl;
    }
}
static void freeMemoryStruct(MemoryStruct *chunk) {
    free(chunk->data);
    if (chunk->title) {
        free(chunk->title);
    }
    if (chunk->href) {
        free(chunk->href);
    }
}
static void *CrawlThreadFunc(void *args){
    ThreadArgs *targs =(ThreadArgs *) args;
    char *url = targs->url;
    int thread_id = targs-> thread_id;
    CURL *curl= curl_easy_init();
    CURLcode res_code = CURLE_FAILED_INIT;
    MemoryStruct chunk;
    printf("Thread %d: Downloading %s \n",thread_id,url);
    chunk.title = nullptr;  // 修改此處
    chunk.href = nullptr;   // 修改此處
    chunk.size = 0;
    if (curl){
        curl_easy_setopt(curl,CURLOPT_URL,url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void *)&chunk);
        res_code = curl_easy_perform(curl);
    if (res_code != CURLE_OK) {
        printf("Thread %d: Download Failed : %s\n",thread_id,curl_easy_strerror(res_code)); 
    }
    else{
        parseHTML(chunk.data, &chunk);
    }
    curl_easy_cleanup(curl);
    }
    //Write in chunk data
    //start to parse the data
    freeMemoryStruct(&chunk);
    pthread_exit(NULL);
}

int main(int argc,char **argv){
    pthread_t threads[MAX_URL_NUM];
    int rc,i;
    ThreadArgs targs[MAX_URL_NUM];
    writeDataToGlobalArray();
    curl_global_init(CURL_GLOBAL_ALL);
    for (i =0 ;i<MAX_URL_NUM;i++){
        strncpy(targs[i].url,url_list[i],MAX_URL_LENGTH+1);
        targs[i].thread_id=i;
        rc= pthread_create(&threads[i],NULL,CrawlThreadFunc,(void *) &targs[i]);
        if (rc){
            printf("Error : retunr code from pthread_creat() is %d\n",rc);
            exit(EXIT_FAILURE);
        }
    }
    for (i=0;i<MAX_URL_NUM;i++){
        rc=pthread_join(threads[i],NULL);
        if (rc){
            printf("Error : retunr code from pthread_join() is %d\n",rc);
            exit(EXIT_FAILURE);
        }
    }
    curl_global_cleanup();
    printf("All Downloads complete!\n");
    freeUrlListMemory();
    return 0;

}