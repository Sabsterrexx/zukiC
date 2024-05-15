#include "../zukiC/zuki.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON/cJSON.h"

char *CHAT_DATA(char *userName, char *userMessage, char *requestedModel,
                char *systemPrompt, double currTemp) {
  cJSON *data = cJSON_CreateObject();  // Create cJSON object

  // Add model to cJSON object
  cJSON_AddStringToObject(data, "model", requestedModel);

  // Create messages array
  cJSON *messages = cJSON_CreateArray();

  // Create system message object
  cJSON *systemMessage = cJSON_CreateObject();
  cJSON_AddStringToObject(systemMessage, "role", "system");
  cJSON_AddStringToObject(systemMessage, "content", systemPrompt);
  cJSON_AddItemToArray(messages, systemMessage);

  // Create user message object
  cJSON *userMessageObj = cJSON_CreateObject();
  cJSON_AddStringToObject(userMessageObj, "role", "user");
  char userContent[512];  // Buffer for user content
  snprintf(userContent, sizeof(userContent),
           "%s\n Here is a message a user called %s sent you: %s", systemPrompt,
           userName, userMessage);
  cJSON_AddStringToObject(userMessageObj, "content", userContent);
  cJSON_AddItemToArray(messages, userMessageObj);

  // Add messages array to cJSON object
  cJSON_AddItemToObject(data, "messages", messages);

  // Add temperature to cJSON object
  cJSON_AddNumberToObject(data, "temperature", currTemp);

  char *jsonData =
      cJSON_PrintUnformatted(data);  // Convert cJSON object to string
  cJSON_Delete(data);                // Free cJSON object

  return jsonData;
}

void chat_call() {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* Perform the request, res gets the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
}