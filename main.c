#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

#include "iphone_models.h"

const char *getDownloadLink(const char *model, const char *iosVersion) {
    for (int i = 0; i < getNumiPhoneModels(); i++) {
        if (strcmp(iphoneList[i].model, model) == 0 && strcmp(iphoneList[i].iosVersion, iosVersion) == 0) {
            return iphoneList[i].downloadLink;
        }
    }
    return NULL;
}


#define PROGRESS_BAR_WIDTH 50


bool fileExists(const char *filename) {
    return access(filename, F_OK) != -1;
}


int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    double progress = 0.0;

    if (dltotal > 0) {
        progress = (double)dlnow / (double)dltotal;
    }

    int barWidth = (int)(progress * PROGRESS_BAR_WIDTH);

    printf("[");
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < barWidth) {
            printf("-");
        } else if (i == barWidth) {
            printf(">");
        } else {
            printf(" ");
        }
    }

    printf("] %.1f%% (%.0f MB/%.0f MB)\r", progress * 100.0, (double)dlnow / 1000000.0, (double)dltotal / 1000000.0);
    fflush(stdout);
    return 0;
}

bool downloadFileWithProgress(const char *url, const char *filename) {
    CURL *curl;
    FILE *file;

    curl = curl_easy_init();
    if (curl) {
        file = fopen(filename, "wb");
        if (file) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

            CURLcode res = curl_easy_perform(curl);

            fclose(file);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK) {
                printf("\nFile downloaded successfully.\n");
                return true;
            }
        }
    }

    printf("\nError: File download failed.\n");
    return false;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <model_number> <ios_version>\n", argv[0]);
        return 1;
    }

    char *modelNumber = argv[1];
    char *iosVersion = argv[2];

    const char *downloadLink = getDownloadLink(modelNumber, iosVersion);


    if (downloadLink) {
        char *fname = malloc(strlen(modelNumber) + strlen(iosVersion) + 6);
        strcpy(fname, modelNumber);
        strcat(fname, "-");
        strcat(fname, iosVersion);
        strcat(fname, ".ipsw");
        const char *filename = fname;
        const char *url = downloadLink;

        if (!fileExists(filename)) {
            printf("File does not exist. Downloading...\n");

            if (downloadFileWithProgress(url, filename)) {
            } else {
                return 1;
            }
        } else {
            printf("File already exists.\n");
        }
    } else {
        printf("Model or iOS version not supported.\n");
    }

    return 0;
}