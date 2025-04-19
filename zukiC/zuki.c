#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON/cJSON.h"

// just stores len and string
struct string {
  char *ptr;
  size_t len;
};

// initializes empty string given struct
void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}


/** writefunc is a function for cURL, and is used to store streamed or downloaded data
  *  into a dynamically allocated string buffer.
  *  @param ptr a void pointer to the chunk of data recieved by cURL
  *  @param size a size_t int of the size of each element.
  *  @param nmemb number (nm) of elements (em) in bytes (b) (together is nmemb).
  *  @param s struct to string
  */
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  // total size is size times number of elements
  size_t new_len = s->len + size * nmemb;

  //try to reallocate string to new size, accounting for null byte
  s->ptr = realloc(s->ptr, new_len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  //copy string data from ptr into s, retaining original data
  //could possibly be used to implement streaming
  memcpy(s->ptr + s->len, ptr, size * nmemb);

  //null terminate
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size * nmemb;
}

/** creates a json out of chat data
 *
 */
cJSON *get_chat_data(const char *userName, const char *userMessage,
                     const char *requestedModel, const char *systemPrompt,
                     double currTemp, int stream) {
  cJSON *data = cJSON_CreateObject();

  cJSON_AddStringToObject(data, "model", requestedModel);
  cJSON_AddBoolToObject(data, "stream", stream);

  cJSON *messages = cJSON_AddArrayToObject(data, "messages");

  cJSON *system_msg = cJSON_CreateObject();
  cJSON_AddStringToObject(system_msg, "role", "system");
  cJSON_AddStringToObject(system_msg, "content", systemPrompt);
  cJSON_AddItemToArray(messages, system_msg);
  //create the user message
  cJSON *user_msg = cJSON_CreateObject();
  cJSON_AddStringToObject(user_msg, "role", "user");
  char user_content[512];
  //hmmm, idk if the userName is important, but I'm gonna leave it here cuz
  //yall know way more than me
  snprintf(user_content, sizeof(user_content),
           "%s\n Here is a message a user called %s sent you: %s", systemPrompt,
           userName, userMessage);
  cJSON_AddStringToObject(user_msg, "content", user_content);
  cJSON_AddItemToArray(messages, user_msg);

  cJSON_AddNumberToObject(data, "temperature", currTemp);

  return data;
}

char *extract_content_from_response(const char *response) {
  cJSON *json = cJSON_Parse(response);
  if (!json) {
    fprintf(stderr, "Error parsing JSON response\n");
    return NULL;
  }

  cJSON *choices = cJSON_GetObjectItem(json, "choices");
  if (!choices || !cJSON_IsArray(choices)) {

    //print out error response instead of JSON format
    if (!response) {
      fprintf(stderr, "Invalid JSON format\n");
    }
    else {
      printf("%s", response);
    }
    cJSON_Delete(json);
    return NULL;
  }

  cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
  //there are no choices
  if (!first_choice) {
    //probably a wrapped free
    cJSON_Delete(json);
    fprintf(stderr, "No choices found\n");
    return NULL;
  }

  //always pick first choice?
  cJSON *message = cJSON_GetObjectItem(first_choice, "message");
  //empty message content
  if (!message) {
    cJSON_Delete(json);
    fprintf(stderr, "No message found\n");
    return NULL;
  }

  cJSON *content = cJSON_GetObjectItem(message, "content");
  if (!content || !cJSON_IsString(content)) {
    cJSON_Delete(json);
    fprintf(stderr, "No content found\n");
    return NULL;
  }

  char *content_str = strdup(content->valuestring);
  cJSON_Delete(json);

  return content_str;
}


size_t stream_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    char *buffer = (char *)malloc(realsize + 1);
    memcpy(buffer, ptr, realsize);
    buffer[realsize] = '\0';


    // Split the buffer by newlines to handle multiple SSE events
    char *line = strtok(buffer, "\n");
    while (line != NULL) {
        // Check if line starts with "data: "
        if (strncmp(line, "data: ", 6) == 0) {
            // Skip "data: [DONE]" messages
            if (strcmp(line + 6, "[DONE]") != 0) {
                // Parse the JSON data
                cJSON *json = cJSON_Parse(line + 6);
                if (json != NULL) {
                    cJSON *choices = cJSON_GetObjectItem(json, "choices");
                    if (choices != NULL && cJSON_IsArray(choices)) {
                        cJSON *choice = cJSON_GetArrayItem(choices, 0);
                        cJSON *delta = cJSON_GetObjectItem(choice, "delta");
                        cJSON *content = cJSON_GetObjectItem(delta, "content");

                        if (content != NULL && cJSON_IsString(content)) {
                            printf("%s", content->valuestring);
                            fflush(stdout);  // Ensure immediate output
                        }
                    }
                    cJSON_Delete(json);
                }
            }
        }
        else {
          //line does not start with data, probably error, print out everything anyway.
          printf("%s", ptr);
          fflush(stdout);
          exit(EXIT_FAILURE);
        }
        line = strtok(NULL, "\n");
    }

    free(buffer);
    return realsize;
}

int stream_chat_call(const char *userName, const char *userMessage,
                       const char *requestedModel, const char *systemPrompt,
                       double currTemp, const char *endpoint, const char* API_KEY) {

  //initialize
  CURL *curl;
  CURLcode res;
  struct string response;
  char auth_header[256];

  init_string(&response);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "%s", "CURL Not initialized");
    exit(EXIT_FAILURE);
  }

  cJSON *chat_data = get_chat_data(userName, userMessage, requestedModel,
                                   systemPrompt, currTemp, 1);
  char *json_data = cJSON_Print(chat_data);
  struct curl_slist *headers = NULL;

  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", API_KEY);
  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, endpoint);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_callback);

  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed %s\n", curl_easy_strerror(res));
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  cJSON_Delete(chat_data);
  free(json_data);
  free(response.ptr);

  curl_global_cleanup();

  return(res == CURLE_OK) ? 0 : 1;
}
/* chat_call sends a request
 */
char *chat_call(const char *userName, const char *userMessage,
                const char *requestedModel, const char *systemPrompt,
                double currTemp, const char *endpoint, const char* API_KEY) {
  CURL *curl;
  CURLcode res;
  struct string response;

  init_string(&response);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (curl) {
    //no streaming, direct response return
    cJSON *chat_data = get_chat_data(userName, userMessage, requestedModel,
                                     systemPrompt, currTemp, 0);
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

  char *content = extract_content_from_response(response.ptr);
  free(response.ptr);

  return content;
}
