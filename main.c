#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <sys/time.h>

#define JSON_URL "https://rymuios.net/iphone_links.json"
#define PROGRESS_BAR_WIDTH 50

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct DownloadProgress {
    struct timeval start_time;
    curl_off_t dltotal;
    bool is_html;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int jsonProgressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    struct DownloadProgress *progress = (struct DownloadProgress *)clientp;
    double progress_ratio = 0.0;
    double mbps = 0.0;

    progress->dltotal = dltotal;

    if (dltotal > 0) {
        progress_ratio = (double)dlnow / (double)dltotal;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    double elapsed_time = (now.tv_sec - progress->start_time.tv_sec) + (now.tv_usec - progress->start_time.tv_usec) / 1000000.0;

    if (elapsed_time > 0) {
        mbps = (double)dlnow / 1000000.0 / elapsed_time;
    }

    int barWidth = (int)(progress_ratio * PROGRESS_BAR_WIDTH);

    printf("Downloading JSON (");
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < barWidth - 1) {
            printf("-");
        } else if (i == barWidth - 1) {
            printf(">");
        } else {
            printf(" ");
        }
    }
    printf(") (%.0f MB/%.0f MB) (%.2f MB/s)\r", (double)dlnow / 1000000.0, (double)dltotal / 1000000.0, mbps);
    fflush(stdout);
    return 0;
}

struct json_object *fetchJSON() {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    struct DownloadProgress progress;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        gettimeofday(&progress.start_time, NULL);
        progress.dltotal = 0;
        progress.is_html = false;

        curl_easy_setopt(curl, CURLOPT_URL, JSON_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, jsonProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return NULL;
        }

        curl_easy_cleanup(curl);

        struct json_object *parsed_json;
        parsed_json = json_tokener_parse(chunk.memory);
        free(chunk.memory);

        return parsed_json;
    }

    return NULL;
}

const char *getDownloadLink(struct json_object *json, const char *model, const char *iosVersion) {
    size_t n_links = json_object_array_length(json);
    for (size_t i = 0; i < n_links; i++) {
        struct json_object *obj = json_object_array_get_idx(json, i);
        const char *json_model = json_object_get_string(json_object_object_get(obj, "model"));
        const char *json_iosVersion = json_object_get_string(json_object_object_get(obj, "iosVersion"));
        const char *json_downloadLink = json_object_get_string(json_object_object_get(obj, "downloadLink"));

        if (strcmp(json_model, model) == 0 && strcmp(json_iosVersion, iosVersion) == 0) {
            return json_downloadLink;
        }
    }
    return NULL;
}

bool fileExists(const char *filename) {
    return access(filename, F_OK) != -1;
}

int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    struct DownloadProgress *progress = (struct DownloadProgress *)clientp;
    double progress_ratio = 0.0;
    double mbps = 0.0;

    progress->dltotal = dltotal;

    if (dltotal > 0) {
        progress_ratio = (double)dlnow / (double)dltotal;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    double elapsed_time = (now.tv_sec - progress->start_time.tv_sec) + (now.tv_usec - progress->start_time.tv_usec) / 1000000.0;

    if (elapsed_time > 0) {
        mbps = (double)dlnow / 1000000.0 / elapsed_time;
    }

    int barWidth = (int)(progress_ratio * PROGRESS_BAR_WIDTH);

    printf("(");
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < barWidth - 1) {
            printf("-");
        } else if (i == barWidth - 1) {
            printf(">");
        } else {
            printf(" ");
        }
    }
    printf(") (%.0f MB/%.0f MB) (%.2f MB/s)\r", (double)dlnow / 1000000.0, (double)dltotal / 1000000.0, mbps);
    fflush(stdout);
    return 0;
}

bool downloadFileWithProgress(const char *url, const char *filename, struct DownloadProgress *progress) {
    CURL *curl;
    FILE *file;
    bool success = false;

    gettimeofday(&progress->start_time, NULL);
    progress->dltotal = 0;
    progress->is_html = false;

    curl = curl_easy_init();
    if (curl) {
        file = fopen(filename, "wb");
        if (file) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progress);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

            CURLcode res = curl_easy_perform(curl);

            fclose(file);
            if (res == CURLE_OK && progress->dltotal > 0) {
                file = fopen(filename, "r");
                if (file) {
                    char buffer[512];
                    fread(buffer, 1, sizeof(buffer), file);
                    if (strstr(buffer, "<HTML>") != NULL || strstr(buffer, "<html>") != NULL) {
                        progress->is_html = true;
                    }
                    fclose(file);
                }

                if (!progress->is_html) {
                    printf("\nFile downloaded successfully.\n");
                    success = true;
                } else {
                    printf("\nError: Received HTML response.\n");
                    remove(filename);
                }
            } else {
                printf("\nError: File download failed.\n");
                remove(filename);
            }
        } else {
            printf("\nError: Could not open file %s for writing.\n", filename);
        }
        curl_easy_cleanup(curl);
    } else {
        printf("\nError: Could not initialize cURL.\n");
    }

    return success;
}

const char *extractFilename(const char *url) {
    const char *slash = strrchr(url, '/');
    if (slash) {
        return slash + 1;
    }
    return url;
}

char *replaceSecureWithNonSecure(const char *url) {
    const char *securePrefix = "https://secure-appldnld";
    const char *nonSecurePrefix = "http://appldnld";

    if (strncmp(url, securePrefix, strlen(securePrefix)) == 0) {
        size_t new_url_length = strlen(url) - strlen(securePrefix) + strlen(nonSecurePrefix);
        char *new_url = malloc(new_url_length + 1);
        if (new_url) {
            snprintf(new_url, new_url_length + 1, "%s%s", nonSecurePrefix, url + strlen(securePrefix));
            return new_url;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <model> <iosVersion>\n", argv[0]);
        return 1;
    }

    const char *model = argv[1];
    const char *iosVersion = argv[2];

    struct json_object *json = fetchJSON();
    if (!json) {
        printf("Failed to fetch or parse JSON.\n");
        return 1;
    }

    const char *downloadLink = getDownloadLink(json, model, iosVersion);
    if (!downloadLink) {
        printf("Download link not found for model %s and iOS version %s.\n", model, iosVersion);
        json_object_put(json);
        return 1;
    }

    printf("Found download link: %s\n", downloadLink);

    const char *filename = extractFilename(downloadLink);
    printf("Extracted filename: %s\n", filename);

    if (fileExists(filename)) {
        printf("File %s already exists. Skipping download.\n", filename);
        json_object_put(json);
        return 0;
    }

    struct DownloadProgress progress;
    bool success = downloadFileWithProgress(downloadLink, filename, &progress);

    if (!success && progress.is_html) {
        printf("\nAttempting non-secure download...\n");
        char *nonSecureLink = replaceSecureWithNonSecure(downloadLink);
        if (nonSecureLink) {
            success = downloadFileWithProgress(nonSecureLink, filename, &progress);
            free(nonSecureLink);
        }
    }

    json_object_put(json);
    return success ? 0 : 1;
}
