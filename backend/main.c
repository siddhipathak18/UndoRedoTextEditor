#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "editor.h"

/*
 * main.c
 * ------------------------------------------------------------------------
 * C HTTP Server using Windows Sockets (Winsock2)
 * Demonstrates: Undo-Redo Text Editor using Two Stacks (DSA Mini Project)
 *
 * Runs locally on http://localhost:8080
 * Connects web frontend to manual Two-Stack C engine.
 * ------------------------------------------------------------------------
 */

#define SERVER_PORT 8080
#define BUFFER_SIZE 65536
#define DATA_FILEPATH "data/document.txt"

// Global Text Editor state powered by two manual stacks
static Editor g_editor;

/*
 * Escape strings for safe JSON formatting
 */
static void json_escape(const char *src, char *dst, size_t dst_len) {
    if (!src || !dst || dst_len == 0) return;
    size_t d = 0;
    for (size_t s = 0; src[s] != '\0' && d + 2 < dst_len; s++) {
        unsigned char c = (unsigned char)src[s];
        if (c == '\"') {
            dst[d++] = '\\'; dst[d++] = '\"';
        } else if (c == '\\') {
            dst[d++] = '\\'; dst[d++] = '\\';
        } else if (c == '\n') {
            dst[d++] = '\\'; dst[d++] = 'n';
        } else if (c == '\r') {
            dst[d++] = '\\'; dst[d++] = 'r';
        } else if (c == '\t') {
            dst[d++] = '\\'; dst[d++] = 't';
        } else if (c < 32) {
            continue; // Skip non-printable control characters
        } else {
            dst[d++] = (char)c;
        }
    }
    dst[d] = '\0';
}

/*
 * Send standard HTTP response with CORS headers
 */
static void send_response(SOCKET client, int status_code, const char *status_text,
                          const char *content_type, const char *body, size_t body_len) {
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len);

    send(client, header, header_len, 0);
    if (body && body_len > 0) {
        send(client, body, (int)body_len, 0);
    }
}

/*
 * Send Editor JSON State
 */
static void send_editor_json(SOCKET client, int success, const char *message) {
    char escaped_text[MAX_TEXT_SIZE * 2];
    json_escape(editor_get_text(&g_editor), escaped_text, sizeof(escaped_text));

    char body[MAX_TEXT_SIZE * 2 + 1024];
    int len = snprintf(body, sizeof(body),
        "{\n"
        "  \"success\": %s,\n"
        "  \"message\": \"%s\",\n"
        "  \"text\": \"%s\",\n"
        "  \"length\": %zu,\n"
        "  \"undoCount\": %d,\n"
        "  \"redoCount\": %d,\n"
        "  \"canUndo\": %s,\n"
        "  \"canRedo\": %s\n"
        "}",
        success ? "true" : "false",
        message ? message : "",
        escaped_text,
        strlen(editor_get_text(&g_editor)),
        editor_get_undo_count(&g_editor),
        editor_get_redo_count(&g_editor),
        editor_can_undo(&g_editor) ? "true" : "false",
        editor_can_redo(&g_editor) ? "true" : "false");

    send_response(client, 200, "OK", "application/json", body, (size_t)len);
}

/*
 * Serve static files from frontend/
 */
static int serve_file(SOCKET client, const char *filepath, const char *content_type) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 0) {
        fclose(fp);
        return 0;
    }

    char *buf = (char *)malloc((size_t)size);
    if (!buf) {
        fclose(fp);
        return 0;
    }

    size_t read_bytes = fread(buf, 1, (size_t)size, fp);
    fclose(fp);

    send_response(client, 200, "OK", content_type, buf, read_bytes);
    free(buf);
    return 1;
}

/*
 * Simple JSON field extraction helpers
 */
static int extract_json_string(const char *json, const char *key, char *out, size_t out_size) {
    if (!json || !key || !out || out_size == 0) return 0;
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return 0;
    p += strlen(needle);
    while (*p && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    if (*p != '\"') return 0;
    p++; // skip quote
    size_t i = 0;
    while (*p && *p != '\"' && i + 1 < out_size) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            if (*p == 'n') out[i++] = '\n';
            else if (*p == 'r') out[i++] = '\r';
            else if (*p == 't') out[i++] = '\t';
            else if (*p == '\"') out[i++] = '\"';
            else if (*p == '\\') out[i++] = '\\';
            else out[i++] = *p;
        } else {
            out[i++] = *p;
        }
        p++;
    }
    out[i] = '\0';
    return 1;
}

static int extract_json_int(const char *json, const char *key, int *out) {
    if (!json || !key || !out) return 0;
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return 0;
    p += strlen(needle);
    while (*p && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    *out = atoi(p);
    return 1;
}

/*
 * Handle an incoming HTTP request
 */
static void handle_client(SOCKET client) {
    char request_buf[BUFFER_SIZE];
    int total_received = recv(client, request_buf, sizeof(request_buf) - 1, 0);
    if (total_received <= 0) {
        return;
    }
    request_buf[total_received] = '\0';

    // Handle HTTP 100 Continue (required by .NET / PowerShell HttpWebRequest)
    if (strstr(request_buf, "100-continue") || strstr(request_buf, "100-Continue")) {
        const char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
        send(client, cont, (int)strlen(cont), 0);
    }

    // Locate header end
    char *header_end = strstr(request_buf, "\r\n\r\n");
    int header_len = 0;
    if (header_end) {
        header_len = (int)(header_end - request_buf) + 4;
    } else {
        header_end = strstr(request_buf, "\n\n");
        if (header_end) {
            header_len = (int)(header_end - request_buf) + 2;
        }
    }

    // Check Content-Length to ensure entire body is read
    int content_length = 0;
    char *cl = strstr(request_buf, "Content-Length:");
    if (!cl) cl = strstr(request_buf, "content-length:");
    if (cl) {
        content_length = atoi(cl + 15);
    }

    if (header_len > 0 && content_length > 0) {
        while (total_received < header_len + content_length && total_received < BUFFER_SIZE - 1) {
            int more = recv(client, request_buf + total_received, (BUFFER_SIZE - 1) - total_received, 0);
            if (more <= 0) break;
            total_received += more;
            request_buf[total_received] = '\0';
        }
    }

    // Parse method and path from HTTP request line
    char method[16] = {0};
    char path[256] = {0};
    if (sscanf(request_buf, "%15s %255s", method, path) < 2) {
        send_response(client, 400, "Bad Request", "text/plain", "Bad Request", 11);
        return;
    }

    // Locate body
    char *body = "";
    if (header_len > 0 && header_len < total_received) {
        body = request_buf + header_len;
    }

    // CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        send_response(client, 204, "No Content", "text/plain", NULL, 0);
        return;
    }

    // 1. Static Files
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            if (serve_file(client, "frontend/index.html", "text/html")) return;
        } else if (strcmp(path, "/style.css") == 0) {
            if (serve_file(client, "frontend/style.css", "text/css")) return;
        } else if (strcmp(path, "/script.js") == 0) {
            if (serve_file(client, "frontend/script.js", "application/javascript")) return;
        } else if (strcmp(path, "/api/text") == 0 || strcmp(path, "/api/status") == 0) {
            send_editor_json(client, 1, "Status retrieved successfully");
            return;
        }
    }

    // 2. API Endpoints
    if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/api/set-text") == 0 || strcmp(path, "/api/set_text") == 0) {
            char text_to_set[MAX_TEXT_SIZE] = {0};
            if (!extract_json_string(body, "text", text_to_set, sizeof(text_to_set))) {
                strncpy(text_to_set, body, sizeof(text_to_set) - 1);
            }

            editor_set_text(&g_editor, text_to_set);
            send_editor_json(client, 1, "Changes applied successfully");
            return;
        }

        if (strcmp(path, "/api/insert") == 0) {
            char text_to_insert[MAX_TEXT_SIZE] = {0};
            if (!extract_json_string(body, "text", text_to_insert, sizeof(text_to_insert))) {
                strncpy(text_to_insert, body, sizeof(text_to_insert) - 1);
            }

            if (strlen(text_to_insert) == 0) {
                send_editor_json(client, 0, "Insert text cannot be empty");
                return;
            }

            if (editor_insert(&g_editor, text_to_insert)) {
                send_editor_json(client, 1, "Text inserted successfully");
            } else {
                send_editor_json(client, 0, "Insert failed: maximum buffer size reached");
            }
            return;
        }

        if (strcmp(path, "/api/delete") == 0) {
            int count = 1;
            extract_json_int(body, "count", &count);
            if (count <= 0) {
                send_editor_json(client, 0, "Delete count must be greater than 0");
                return;
            }

            if (editor_delete(&g_editor, count)) {
                send_editor_json(client, 1, "Deleted characters successfully");
            } else {
                send_editor_json(client, 0, "Nothing to delete (text is already empty)");
            }
            return;
        }

        if (strcmp(path, "/api/undo") == 0) {
            if (editor_undo(&g_editor)) {
                send_editor_json(client, 1, "Undo performed: state popped from Undo Stack to Redo Stack");
            } else {
                send_editor_json(client, 0, "Cannot undo: Undo Stack is empty");
            }
            return;
        }

        if (strcmp(path, "/api/redo") == 0) {
            if (editor_redo(&g_editor)) {
                send_editor_json(client, 1, "Redo performed: state popped from Redo Stack to Undo Stack");
            } else {
                send_editor_json(client, 0, "Cannot redo: Redo Stack is empty");
            }
            return;
        }

        if (strcmp(path, "/api/clear") == 0) {
            editor_clear(&g_editor);
            send_editor_json(client, 1, "Document cleared (state saved to Undo Stack)");
            return;
        }

        if (strcmp(path, "/api/save") == 0) {
            if (editor_save_to_file(&g_editor, DATA_FILEPATH)) {
                send_editor_json(client, 1, "Saved document successfully to data/document.txt");
            } else {
                send_editor_json(client, 0, "Failed to save document to file");
            }
            return;
        }

        if (strcmp(path, "/api/load") == 0) {
            if (editor_load_from_file(&g_editor, DATA_FILEPATH)) {
                send_editor_json(client, 1, "Loaded document successfully from data/document.txt");
            } else {
                send_editor_json(client, 0, "Failed to load document: data/document.txt not found or empty");
            }
            return;
        }
    }

    // 3. Fallback 404
    const char *not_found = "{\"success\":false,\"message\":\"Endpoint not found\"}";
    send_response(client, 404, "Not Found", "application/json", not_found, strlen(not_found));
}

int main(void) {
    // 1. Initialize Winsock
    WSADATA wsaData;
    int wsa_res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_res != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", wsa_res);
        return 1;
    }

    // 2. Initialize Text Editor with Two Stacks
    editor_init(&g_editor);

    // 3. Create listening TCP socket
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    printf("========================================================================\n");
    printf("   UNDO-REDO TEXT EDITOR (TWO STACKS DSA MINI PROJECT)\n");
    printf("   C HTTP Backend Server is running!\n");
    printf("========================================================================\n");
    printf("   URL: http://localhost:%d\n", SERVER_PORT);
    printf("   Undo/Redo Engine: Manual Two-Stack implementation in C\n");
    printf("   Storage: File-based persistence (%s)\n", DATA_FILEPATH);
    printf("   Ready to accept requests...\n");
    printf("========================================================================\n\n");
    fflush(stdout);

    // 4. Main accept loop
    while (1) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "Accept failed with error: %d\n", WSAGetLastError());
            continue;
        }

        handle_client(client_socket);
        closesocket(client_socket);
    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
