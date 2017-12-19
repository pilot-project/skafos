#include <curl/curl.h>

#include "request.h"
#include "env/env.h"

using namespace std;
using namespace json11;

#define ENDPOINT(str) Url{(API_URL + str)}

#define LOGIN_URL "/users/login"
#define PING_URL  "/ping"
#define TOKEN_URL "/api_tokens/"


// Private
Request *Request::instance_ = 0;

Request *Request::instance() {
  if (instance_ == 0) {
    instance_ = new Request();
  }

  return instance_;
}

Request::Request() {
  RestClient::init();

  this->connection = new RestClient::Connection(API_URL);
  this->connection->SetTimeout(30);
  this->connection->SetUserAgent("Skafos");
  this->connection->FollowRedirects(true);
}

Request::~Request() {
  RestClient::disable();
}

RestClient::HeaderFields Request::_default_headers() {
  RestClient::HeaderFields headers;
  headers["Content-Type"] = "application/json";

  return headers;
}

RestClient::HeaderFields Request::_api_headers() {
  RestClient::HeaderFields headers  = this->_default_headers();
  headers["x-api-token"]            = Env::instance()->get(METIS_API_TOKEN);
  
  return headers;
}

RestClient::HeaderFields Request::_oauth_headers() {
RestClient::HeaderFields headers  = this->_default_headers();
  headers["Authorization"]        = "Bearer " + Env::instance()->get(METIS_AUTH_TOKEN);
  
  return headers;
}

RestClient::Response Request::_authenticate(std::string email, std::string password) {
  RestClient::HeaderFields headers = this->_default_headers();
  
  this->connection->SetHeaders(headers);
  
  Json body = Json::object{
    {"email",       email}, 
    {"password",    password},
    {"client_id",   CLIENT_ID},
    {"grant_type",  "password"}
  };

  return this->connection->post(LOGIN_URL, body.dump());
}

RestClient::Response Request::_ping() {
  RestClient::HeaderFields headers = this->_api_headers();

  this->connection->SetHeaders(headers);

  return this->connection->get(PING_URL);
}

RestClient::Response Request::_tokens() {
  RestClient::HeaderFields headers = this->_api_headers();

  this->connection->SetHeaders(headers);

  return this->connection->get(TOKEN_URL);
}

RestClient::Response Request::_generate_token() {
  RestClient::HeaderFields headers = this->_oauth_headers();

  this->connection->SetHeaders(headers);

  return this->connection->post(TOKEN_URL, "");
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

void Request::_download(std::string repo_url, std::string save_path) {
  CURL *curl = curl_easy_init();
  FILE *fp;
  CURLcode res;
  char *url = (char *)repo_url.c_str();
  char output[FILENAME_MAX];

  strcpy(output, save_path.c_str());

  if(curl) {
    fp = fopen(output, "wb");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
      cerr << "Error downloading " << repo_url << " (" << curl_easy_strerror(res) << endl;
    }

    fclose(fp);
    curl_easy_cleanup(curl);
  }
}

// Public
RestClient::Response Request::authenticate(string email, string password) {
  return instance()->_authenticate(email, password);
}

RestClient::Response Request::ping() {
  return instance()->_ping();
}

RestClient::Response Request::tokens() {
  return instance()->_tokens();
}


RestClient::Response Request::generate_token() {
  return instance()->_generate_token();
}

void Request::download(string url, string save_path) {
  instance()->_download(url, save_path);
}

