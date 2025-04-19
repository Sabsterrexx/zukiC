#include "../cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *CHAT_DATA();

char *chat_call(const char *userName, const char *userMessage,
    const char *requestedModel, const char *systemPrompt,
    double currTemp, const char *endpoint, const char* API_KEY); // suppress stupid compiler warning

int stream_chat_call(const char *userName, const char *userMessage,
    const char *requestedModel, const char *systemPrompt,
    double currTemp, const char *endpoint, const char* API_KEY); // preemptively suppress compiler warning

