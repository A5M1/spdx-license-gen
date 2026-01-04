#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "wininet.lib")
char* str_replace(char* source, const char* target, const char* replacement) {
    if (!source || !target || !replacement) return NULL;
    char *result, *ins, *tmp;
    int len_target = strlen(target), len_repl = strlen(replacement), len_front, count;

    ins = source;
    for (count = 0; (tmp = strstr(ins, target)); ++count) ins = tmp + len_target;

    tmp = result = malloc(strlen(source) + (len_repl - len_target) * count + 1);
    if (!result) return NULL;

    while (count--) {
        ins = strstr(source, target);
        len_front = ins - source;
        tmp = strncpy(tmp, source, len_front) + len_front;
        tmp = strcpy(tmp, replacement) + len_repl;
        source += len_front + len_target;
    }
    strcpy(tmp, source);
    return result;
}

void download_and_stamp(const char* id, const char* year, const char* owner) {
    HINTERNET hInternet = InternetOpenA("SPDX-Generator", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return;

    char url[256];
    sprintf(url, "https://raw.githubusercontent.com/spdx/license-list-data/master/text/%s.txt", id);

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        fprintf(stderr, "Error: Could not find SPDX ID '%s'.\n", id);
        InternetCloseHandle(hInternet);
        return;
    }

    char buffer[65536] = {0}; 
    DWORD bytesRead;
    InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead);
    char* s1 = str_replace(buffer, "<year>", year);
    char* s2 = str_replace(s1 ? s1 : buffer, "[year]", year);
    char* s3 = str_replace(s2 ? s2 : buffer, "<copyright holders>", owner);
    char* final_text = str_replace(s3 ? s3 : buffer, "[fullname]", owner);
    FILE* fp = fopen("LICENSE.md", "w");
    if (fp) {
        fprintf(fp, "# %s License\n\n%s", id, final_text ? final_text : buffer);
        fclose(fp);
        printf("Successfully generated LICENSE.md (%s) for %s\n", id, owner);
    }
    free(s1); free(s2); free(s3); free(final_text);
    InternetCloseHandle(hUrl); InternetCloseHandle(hInternet);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <SPDX-ID> <Year> \"<Owner Name>\"\n", argv[0]);
        printf("Hint: Use '.' for the current year.\n");
        printf("Example: %s MIT 2026 \"John Doe\"\n", argv[0]);
        return 1;
    }
    char *id = argv[1];
    char year_buf[5];
    char *owner = argv[3];

    // automagical year detection
    if (strcmp(argv[2], ".") == 0) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(year_buf, "%d", tm.tm_year + 1900);
    } else {
        strncpy(year_buf, argv[2], 4);
        year_buf[4] = '\0';
    }
    download_and_stamp(id, year_buf, owner);
    return 0;
}