#include "../cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *CHAT_DATA(char *userName, char *userMessage, char *requestedModel, char *systemPrompt, double currTemp);

char* chat_call();