#include "claves.h"
#include "clavesRPC.h"

#include <rpc/clnt.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define RPC_TIMEOUT_SEC 3
#define KEY_MAX_LEN 255
#define VALUE2_MAX_ELEMS 32

static size_t bounded_len(const char *text, size_t max_len) {
    size_t length = 0;

    while (length < max_len && text[length] != '\0') {
        ++length;
    }

    return length;
}

static int valid_cstr_255(const char *text) {
    if (text == NULL) {
        return 0;
    }

    return bounded_len(text, KEY_MAX_LEN + 1) <= KEY_MAX_LEN;
}

static CLIENT *create_rpc_client(void) {
    const char *host = getenv("IP_TUPLAS");
    struct timeval timeout;
    CLIENT *client;

    if (host == NULL || host[0] == '\0') {
        return NULL;
    }

    timeout.tv_sec = RPC_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    client = clnt_create_timed(host, CLAVES_RPC_PROG, CLAVES_RPC_V1, "tcp", &timeout);
    if (client == NULL) {
        return NULL;
    }

    if (!clnt_control(client, CLSET_TIMEOUT, (char *)&timeout)) {
        clnt_destroy(client);
        return NULL;
    }

    return client;
}

int destroy(void) {
    CLIENT *client = create_rpc_client();
    int result = -1;

    if (client == NULL) {
        return -1;
    }

    if (destroy_rpc_1(&result, client) != RPC_SUCCESS) {
        clnt_destroy(client);
        return -1;
    }

    clnt_destroy(client);
    return result;
}

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    CLIENT *client;
    set_value_request_rpc request;
    int result = -1;

    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > VALUE2_MAX_ELEMS || V_value2 == NULL) {
        return -1;
    }

    client = create_rpc_client();
    if (client == NULL) {
        return -1;
    }

    memset(&request, 0, sizeof(request));
    request.key = key;
    request.value1 = value1;
    request.n_value2 = N_value2;
    request.v_value2.rpc_value2_len = (u_int)N_value2;
    request.v_value2.rpc_value2_val = V_value2;
    request.value3.x = value3.x;
    request.value3.y = value3.y;
    request.value3.z = value3.z;

    if (set_value_rpc_1(request, &result, client) != RPC_SUCCESS) {
        clnt_destroy(client);
        return -1;
    }

    clnt_destroy(client);
    return result;
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    CLIENT *client;
    key_request_rpc request;
    get_value_response_rpc response;
    int result;

    if (!valid_cstr_255(key) || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        return -1;
    }

    client = create_rpc_client();
    if (client == NULL) {
        return -1;
    }

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    request.key = key;

    if (get_value_rpc_1(request, &response, client) != RPC_SUCCESS) {
        clnt_destroy(client);
        return -1;
    }

    result = response.status;
    if (result == 0) {
        if (response.value1 == NULL || response.n_value2 < 1 || response.n_value2 > VALUE2_MAX_ELEMS ||
            response.v_value2.rpc_value2_len != (u_int)response.n_value2 || response.v_value2.rpc_value2_val == NULL) {
            result = -1;
        } else {
            strcpy(value1, response.value1);
            *N_value2 = response.n_value2;
            memcpy(V_value2, response.v_value2.rpc_value2_val, (size_t)response.n_value2 * sizeof(float));
            value3->x = response.value3.x;
            value3->y = response.value3.y;
            value3->z = response.value3.z;
        }
    }

    xdr_free((xdrproc_t)xdr_get_value_response_rpc, (char *)&response);
    clnt_destroy(client);
    return result;
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    CLIENT *client;
    set_value_request_rpc request;
    int result = -1;

    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > VALUE2_MAX_ELEMS || V_value2 == NULL) {
        return -1;
    }

    client = create_rpc_client();
    if (client == NULL) {
        return -1;
    }

    memset(&request, 0, sizeof(request));
    request.key = key;
    request.value1 = value1;
    request.n_value2 = N_value2;
    request.v_value2.rpc_value2_len = (u_int)N_value2;
    request.v_value2.rpc_value2_val = V_value2;
    request.value3.x = value3.x;
    request.value3.y = value3.y;
    request.value3.z = value3.z;

    if (modify_value_rpc_1(request, &result, client) != RPC_SUCCESS) {
        clnt_destroy(client);
        return -1;
    }

    clnt_destroy(client);
    return result;
}

int delete_key(char *key) {
    CLIENT *client;
    key_request_rpc request;
    int result = -1;

    if (!valid_cstr_255(key)) {
        return -1;
    }

    client = create_rpc_client();
    if (client == NULL) {
        return -1;
    }

    memset(&request, 0, sizeof(request));
    request.key = key;

    if (delete_key_rpc_1(request, &result, client) != RPC_SUCCESS) {
        clnt_destroy(client);
        return -1;
    }

    clnt_destroy(client);
    return result;
}

int exist(char *key) {
    CLIENT *client;
    key_request_rpc request;
    int result = -1;

    if (!valid_cstr_255(key)) {
        return -1;
    }

    client = create_rpc_client();
    if (client == NULL) {
        return -1;
    }

    memset(&request, 0, sizeof(request));
    request.key = key;

    if (exist_rpc_1(request, &result, client) != RPC_SUCCESS) {
        clnt_destroy(client);
        return -1;
    }

    clnt_destroy(client);
    return result;
}