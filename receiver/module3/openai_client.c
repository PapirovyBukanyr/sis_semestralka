/*
 * openai_client.c
 *
 * Minimal OpenAI Chat Completion client using libcurl.
 * This implementation intentionally keeps dependencies small: it uses libcurl
 * and performs a simple string-based extraction of the assistant reply.
 *
 * Build: add `-lcurl` to the linker command. Set `OPENAI_API_KEY` env var.
 */

#include "openai_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

/**
 * Buffer to hold response data
 * 
 * @struct memchunk
 * @member buf pointer to data buffer
 * @member size size of data buffer
 */
struct memchunk {
    char *buf;
    size_t size;
};

/**
 * Callback for libcurl to write received data into a memory buffer.
 * 
 * @param contents pointer to received data
 * @param size size of each data element
 * @param nmemb number of data elements
 * @param userp pointer to user data (memchunk struct)
 * @return number of bytes actually handled
 */
static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp){
    size_t realsize = size * nmemb;
    struct memchunk *m = (struct memchunk*)userp;
    char *ptr = realloc(m->buf, m->size + realsize + 1);
    if(!ptr) return 0;
    m->buf = ptr;
    memcpy(&(m->buf[m->size]), contents, realsize);
    m->size += realsize;
    m->buf[m->size] = '\0';
    return realsize;
}

/**
 * In-place unescape common escape sequences in a string.
 * 
 * @param s string to unescape in place
 */
static void unescape_inplace(char *s){
    char *r = s, *w = s;
    while(*r){
        if(r[0] == '\\' && r[1]){
            r++;
            if(*r == 'n') *w++ = '\n';
            else if(*r == 'r') *w++ = '\r';
            else if(*r == 't') *w++ = '\t';
            else *w++ = *r;
            r++;
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

/**
 * Extract assistant content from OpenAI API response.
 * 
 * @param resp full JSON response string
 * @return newly allocated string with assistant content, or NULL on error
 */
static char* extract_assistant_content(const char *resp){
    if(!resp) return NULL;
    const char *key = "\"content\"";
    const char *p = strstr(resp, key);
    if(!p) return NULL;
    /* Move to after the key and the following ':' */
    p = strchr(p, ':');
    if(!p) return NULL;
    p++;
    /* Skip whitespace */
    while(*p && (*p==' '||*p=='\n' || *p=='\r' || *p=='\t')) p++;
    /* Content could be a string starting with '"' */
    if(*p == '"'){
        p++;
        const char *q = p;
        /* find closing unescaped quote */
        while(*q){
            if(*q == '"'){
                /* check if escaped */
                int backslashes = 0;
                const char *b = q - 1;
                while(b >= p && *b == '\\'){ backslashes++; b--; }
                if((backslashes % 2) == 0) break; /* not escaped */
            }
            q++;
        }
        if(!*q) return NULL;
        size_t len = q - p;
        char *out = (char*)malloc(len + 1);
        if(!out) return NULL;
        strncpy(out, p, len);
        out[len] = '\0';
        unescape_inplace(out);
        return out;
    }
    return NULL;
}

/**
 * Send `text` as a user message to the OpenAI Chat Completions API using
 * `model`. Returns a newly-allocated C string containing the assistant
 * reply (caller must free). On error returns NULL.
 * 
 * @param text user message text
 * @param model model name (e.g., "gpt-4o-mini"), or NULL for default
 * @return newly allocated assistant reply string or NULL on error
 */
char* openai_interpret_text(const char* text, const char* model){
    if(!text) return NULL;
    const char *key = getenv("OPENAI_API_KEY");
    if(!key) return NULL;

    CURL *curl = NULL;
    CURLcode res;
    struct memchunk chunk;
    chunk.buf = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if(!curl){ free(chunk.buf); return NULL; }

    char url[] = "https://api.openai.com/v1/chat/completions";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

    /* Build minimal JSON payload. Escape double quotes in `text`. */
    size_t tlen = strlen(text);
    char *esc = malloc(tlen * 2 + 16);
    if(!esc){ curl_easy_cleanup(curl); free(chunk.buf); return NULL; }
    char *dst = esc;
    for(const char *s = text; *s; ++s){
        if(*s == '\\') { *dst++ = '\\'; *dst++ = '\\'; }
        else if(*s == '"'){ *dst++ = '\\'; *dst++ = '"'; }
        else if(*s == '\n'){ *dst++ = '\\'; *dst++ = 'n'; }
        else *dst++ = *s;
    }
    *dst = '\0';

    const char *model_used = model ? model : "gpt-4o-mini";
    /* safe payload length estimate */
    size_t payload_sz = strlen(esc) + strlen(model_used) + 256;
    char *payload = malloc(payload_sz);
    if(!payload){ free(esc); curl_easy_cleanup(curl); free(chunk.buf); return NULL; }
    snprintf(payload, payload_sz,
             "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"max_tokens\":256}",
             model_used, esc);

    struct curl_slist *headers = NULL;
    char authh[512];
    snprintf(authh, sizeof(authh), "Authorization: Bearer %s", key);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authh);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

    res = curl_easy_perform(curl);
    free(payload);
    free(esc);
    curl_slist_free_all(headers);

    if(res != CURLE_OK){
        curl_easy_cleanup(curl);
        free(chunk.buf);
        return NULL;
    }

    char *reply = extract_assistant_content(chunk.buf);

    curl_easy_cleanup(curl);
    free(chunk.buf);

    return reply;
}

/**
 * Send `system_text` (system role) and `user_text` (user role) as messages
 * to the OpenAI Chat Completions API using `model`. Returns a newly-allocated
 * C string containing the assistant reply (caller must free). On error
 * returns NULL.
 * 
 * @param system_text system role message text
 * @param user_text user role message text
 * @param model model name (e.g., "gpt-4o-mini"), or NULL for default
 * @return newly allocated assistant reply string or NULL on error
 */
char* openai_interpret_with_system(const char* system_text, const char* user_text, const char* model){
    if(!user_text) return NULL;
    const char *key = getenv("OPENAI_API_KEY");
    if(!key) return NULL;

    CURL *curl = NULL;
    CURLcode res;
    struct memchunk chunk;
    chunk.buf = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if(!curl){ free(chunk.buf); return NULL; }

    char url[] = "https://api.openai.com/v1/chat/completions";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

    /* Escape strings for JSON */
    size_t ulen = strlen(user_text);
    char *uesc = malloc(ulen * 2 + 16);
    if(!uesc){ curl_easy_cleanup(curl); free(chunk.buf); return NULL; }
    char *dst = uesc;
    for(const char *s = user_text; *s; ++s){
        if(*s == '\\') { *dst++ = '\\'; *dst++ = '\\'; }
        else if(*s == '"'){ *dst++ = '\\'; *dst++ = '"'; }
        else if(*s == '\n'){ *dst++ = '\\'; *dst++ = 'n'; }
        else *dst++ = *s;
    }
    *dst = '\0';

    char *sesc = NULL;
    if(system_text){
        size_t slen = strlen(system_text);
        sesc = malloc(slen * 2 + 16);
        if(!sesc){ free(uesc); curl_easy_cleanup(curl); free(chunk.buf); return NULL; }
        dst = sesc;
        for(const char *s = system_text; *s; ++s){
            if(*s == '\\') { *dst++ = '\\'; *dst++ = '\\'; }
            else if(*s == '"'){ *dst++ = '\\'; *dst++ = '"'; }
            else if(*s == '\n'){ *dst++ = '\\'; *dst++ = 'n'; }
            else *dst++ = *s;
        }
        *dst = '\0';
    }

    const char *model_used = model ? model : "gpt-4o-mini";
    /* safe payload length estimate */
    size_t payload_sz = (sesc ? strlen(sesc) : 0) + strlen(uesc) + strlen(model_used) + 512;
    char *payload = malloc(payload_sz);
    if(!payload){ free(uesc); if(sesc) free(sesc); curl_easy_cleanup(curl); free(chunk.buf); return NULL; }

    if(sesc){
        snprintf(payload, payload_sz,
                 "{\"model\":\"%s\",\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},{\"role\":\"user\",\"content\":\"%s\"}],\"max_tokens\":256}",
                 model_used, sesc, uesc);
    } else {
        snprintf(payload, payload_sz,
                 "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"max_tokens\":256}",
                 model_used, uesc);
    }

    struct curl_slist *headers = NULL;
    char authh[512];
    snprintf(authh, sizeof(authh), "Authorization: Bearer %s", key);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authh);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

    res = curl_easy_perform(curl);
    free(payload);
    free(uesc);
    if(sesc) free(sesc);
    curl_slist_free_all(headers);

    if(res != CURLE_OK){
        curl_easy_cleanup(curl);
        free(chunk.buf);
        return NULL;
    }

    char *reply = extract_assistant_content(chunk.buf);

    curl_easy_cleanup(curl);
    free(chunk.buf);

    return reply;
}
