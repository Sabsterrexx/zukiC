#include <curl/curl.h>
#include <stdio.h>

#include "cJSON/cJSON.h"
#include "zukiC/zuki.h"

int main() {
  const char *userName = "John Doe";
  const char *userMessage = "Hello, how are you?";
  const char *requestedModel = "gpt-3.5-turbo";
  const char *systemPrompt = "You are a helpful assistant.";
  double currTemp = 0.7;
  const char *endpoint = "https://zukijourney.xyzbot.net/v1/chat/completions";

  char *response = chat_call(userName, userMessage, requestedModel, systemPrompt, currTemp, endpoint);
  if (response) {
    printf("Response: %s\n", response);
    free(response);
  }

  return 0;
}