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

typedef size_t( * curl_write)(char * , size_t, size_t, std::string * );

std::string request(std::string word) {
  CURLcode res_code = CURLE_FAILED_INIT;
  CURL * curl = curl_easy_init();
  std::string result;
//   std::string url = "https://www.merriam-webster.com/dictionary/" + word;
  std::string url = "https://www.ptt.cc/bbs/CrossStrait/index" + word+".html";
  cout<<url;
  curl_global_init(CURL_GLOBAL_ALL);

  if (curl) {
    curl_easy_setopt(curl,
      CURLOPT_WRITEFUNCTION,
      static_cast < curl_write > ([](char * contents, size_t size,
        size_t nmemb, std::string * data) -> size_t {
        size_t new_size = size * nmemb;
        if (data == NULL) {
          return 0;
        }
        data -> append(contents, new_size);
        return new_size;
      }));
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, & result);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "simple scraper");

    res_code = curl_easy_perform(curl);

    if (res_code != CURLE_OK) {
      return curl_easy_strerror(res_code);
    }

    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  cout<<result;
  return result;
}

std::string str_replace(std::string search, std::string replace, std::string &subject)
{
  size_t count;
  for (std::string::size_type pos{};
       subject.npos != (pos = subject.find(search.data(), pos, search.length()));
       pos += replace.length(), ++count)
  {
    subject.replace(pos, search.length(), replace.data(), replace.length());
  }

  return subject;
}

std::string extract_text(GumboNode *node)
{
  if (node->type == GUMBO_NODE_TEXT)
  {
    
    return std::string(node->v.text.text);
  }
  else if (node->type == GUMBO_NODE_ELEMENT &&
           node->v.element.tag != GUMBO_TAG_SCRIPT &&
           node->v.element.tag != GUMBO_TAG_STYLE)
  {  // 解析HTML WIth Gumbo parser
    std::string contents = "";
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
      std::string text = extract_text((GumboNode *)children->data[i]);
      if (i != 0 && !text.empty())
      {
        contents.append("");
      }
      contents.append(str_replace(":", ">", text));
    }
    return contents;
  }
  else
  {
    return "";
  }
}

std::string find_definitions(GumboNode *node)
{
  std::string res = "";
  GumboAttribute *attr;
  if (node->type != GUMBO_NODE_ELEMENT)
  {
    
    return res;
  }

  if ((attr = gumbo_get_attribute(&node->v.element.attributes, "class")) && strstr(attr->value, "dtText") != NULL)
  {
    res += extract_text(node); // output Extract content
    res += "\n";
  }

  GumboVector *children = &node->v.element.children; // get the children Data
  for (int i = 0; i < children->length; ++i)
  {
    res += find_definitions(static_cast<GumboNode *>(children->data[i]));

  }
  return res;
}

std::string scrape(std::string markup)
{
  std::string res = "";
  GumboOutput *output = gumbo_parse_with_options(&kGumboDefaultOptions, markup.data(), markup.length());
  res += find_definitions(output->root);
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  return res;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cout << "Please provide a valid English word" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string arg = argv[1];

  std::string res = request(arg);
  std::cout << scrape(res) << std::endl;

  return EXIT_SUCCESS;
}
