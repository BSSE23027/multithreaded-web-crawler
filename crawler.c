#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

#define MAX_WORD_LEN 100
#define MAX_LINE_LEN 1024

// For storing downloaded content
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback for libcurl to write data into memory
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("Out of memory!\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = '\0';

    return realsize;
}

// Cleans and lowers word
void cleanWord(char *word) {
    int len = strlen(word);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (isalpha(word[i])) {
            word[j++] = tolower(word[i]);
        }
    }
    word[j] = '\0';
}

// Analyze text content
void analyzeText(char *text, int *lengthDist, int *startCharFreq, char longestWords[][MAX_WORD_LEN], int *maxLen, int *longestCount) {
    char *token = strtok(text, " \n\r\t");
    while (token) {
        cleanWord(token);
        if (strlen(token) == 0) {
            token = strtok(NULL, " \n\r\t");
            continue;
        }

        int len = strlen(token);

        // Update word length distribution
        if (len <= 20) lengthDist[len]++;
        else lengthDist[21]++;

        // Update start char frequency
        if (isalpha(token[0]))
            startCharFreq[toupper(token[0]) - 'A']++;

        // Update longest words
        if (len > *maxLen) {
            *maxLen = len;
            *longestCount = 0;
            strncpy(longestWords[(*longestCount)++], token, *maxLen);
        } else if (len == *maxLen) {
            strncpy(longestWords[(*longestCount)++], token, *maxLen);
        }

        token = strtok(NULL, " \n\r\t");
    }
}

// Download and process each URL
void processURL(const char *url, int *lengthDist, int *startCharFreq, char longestWords[][MAX_WORD_LEN], int *maxLen, int *longestCount) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk = { .memory = malloc(1), .size = 0 };

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);

    res = curl_easy_perform(curl_handle);

    if (res == CURLE_OK) {
        analyzeText(chunk.memory, lengthDist, startCharFreq, longestWords, maxLen, longestCount);
        printf("Processed: %s\n", url);
    } else {
        fprintf(stderr, "Failed to download %s: %s\n", url, curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <url_file.txt>\n", argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    FILE *urlFile = fopen(argv[1], "r");
    if (!urlFile) {
        perror("Error opening URL file");
        return 1;
    }

    int lengthDist[22] = {0};             // Index 1-20, 21 for 20+
    int startCharFreq[26] = {0};          // A-Z
    char longestWords[100][MAX_WORD_LEN]; // Store up to 100 longest words
    int maxLen = 0, longestCount = 0;

    char url[MAX_LINE_LEN];
    while (fgets(url, sizeof(url), urlFile)) {

        url[strcspn(url, "\r\n")] = '\0'; // Remove newline
        if (strlen(url) > 0)
            processURL(url, lengthDist, startCharFreq, longestWords, &maxLen, &longestCount);
    }

    fclose(urlFile);
    curl_global_cleanup();

    // Output results
    printf("\n=== Longest Word(s) [%d chars] ===\n", maxLen);
    for (int i = 0; i < longestCount; i++) {
        printf("%s\n", longestWords[i]);
    }

    printf("\n=== Word Length Distribution ===\n");
    for (int i = 1; i <= 20; i++) {
        printf("Length %2d: %d\n", i, lengthDist[i]);
    }
    printf("Length 20+: %d\n", lengthDist[21]);

    printf("\n=== Word Frequency by Starting Character ===\n");
    for (int i = 0; i < 26; i++) {
        printf("%c: %d\n", 'A' + i, startCharFreq[i]);
    }

    return 0;
}
