/*
 * openai_client.h
 *
 * Minimal OpenAI Chat Completion client using libcurl.
 * Requires linking with libcurl. Use environment variable `OPENAI_API_KEY`.
 */

#ifndef OPENAI_CLIENT_H
#define OPENAI_CLIENT_H

#include <stddef.h>

/*
 * Send `text` as a user message to the OpenAI Chat Completions API using
 * `model`. Returns a newly-allocated C string containing the assistant
 * reply (caller must free). On error returns NULL.
 */
char* openai_interpret_text(const char* text, const char* model);

/*
 * Send `system_text` (system role) and `user_text` (user role) as messages
 * to the OpenAI Chat Completions API. Returns a newly-allocated C string
 * containing the assistant reply (caller must free). On error returns NULL.
 */
char* openai_interpret_with_system(const char* system_text, const char* user_text, const char* model);

#endif /* OPENAI_CLIENT_H */
