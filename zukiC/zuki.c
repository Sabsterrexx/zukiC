#include "../cJSON/cJSON.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define API_KEY "zu-01b094de5f7f9993b6cb6529a64f071a"

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size * nmemb;
  s->ptr = realloc(s->ptr, new_len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size * nmemb;
}

cJSON *get_chat_data(const char *userName, const char *userMessage,
                     const char *requestedModel, const char *systemPrompt,
                     double currTemp) {
  cJSON *data = cJSON_CreateObject();

  cJSON_AddStringToObject(data, "model", requestedModel);

  cJSON *messages = cJSON_AddArrayToObject(data, "messages");

  cJSON *system_msg = cJSON_CreateObject();
  cJSON_AddStringToObject(system_msg, "role", "system");
  cJSON_AddStringToObject(system_msg, "content", systemPrompt);
  cJSON_AddItemToArray(messages, system_msg);

  cJSON *user_msg = cJSON_CreateObject();
  cJSON_AddStringToObject(user_msg, "role", "user");
  char user_content[512];
  snprintf(user_content, sizeof(user_content),
           "%s\n Here is a message a user called %s sent you: %s", systemPrompt,
           userName, userMessage);
  cJSON_AddStringToObject(user_msg, "content", user_content);
  cJSON_AddItemToArray(messages, user_msg);

  cJSON_AddNumberToObject(data, "temperature", currTemp);

  return data;
}

char *chat_call(const char *userName, const char *userMessage,
                const char *requestedModel, const char *systemPrompt,
                double currTemp, const char *endpoint) {
  CURL *curl;
  CURLcode res;
  struct string response;

  init_string(&response);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (curl) {
    cJSON *chat_data = get_chat_data(userName, userMessage, requestedModel,
                                     systemPrompt, currTemp);
    char *json_data = cJSON_Print(chat_data);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s",
             API_KEY);
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, endpoint);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    cJSON_Delete(chat_data);
    free(json_data);
  }
  curl_global_cleanup();

  return response.ptr;
}