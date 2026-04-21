#include "claves.h"
#include "clavesRPC.h"

#include <rpc/rpc.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR_LEN 255
#define MAX_V2_LEN 32

static char *dup_rpc_string(const char *text) {
    size_t length = strlen(text) + 1;
    char *copy = (char *)malloc(length);

    if (copy != NULL) {
        memcpy(copy, text, length);
    }

    return copy;
}

static int init_get_value_response(get_value_response_rpc *result) {
    memset(result, 0, sizeof(*result));
    result->value1 = dup_rpc_string("");
    return result->value1 != NULL;
}

static int valid_set_request(const set_value_request_rpc *request) {
    if (request == NULL || request->key == NULL || request->value1 == NULL) {
        return 0;
    }

    if (request->n_value2 < 1 || request->n_value2 > MAX_V2_LEN) {
        return 0;
    }

    if (request->v_value2.rpc_value2_len != (u_int)request->n_value2 || request->v_value2.rpc_value2_val == NULL) {
        return 0;
    }

    return 1;
}

bool_t destroy_rpc_1_svc(int *result, struct svc_req *rqstp) {
    (void)rqstp;
    *result = destroy();
    return TRUE;
}

bool_t set_value_rpc_1_svc(set_value_request_rpc arg1, int *result, struct svc_req *rqstp) {
    struct Paquete value3;

    (void)rqstp;

    if (!valid_set_request(&arg1)) {
        *result = -1;
        return TRUE;
    }

    value3.x = arg1.value3.x;
    value3.y = arg1.value3.y;
    value3.z = arg1.value3.z;
    *result = set_value(arg1.key, arg1.value1, arg1.n_value2, arg1.v_value2.rpc_value2_val, value3);
    return TRUE;
}

bool_t get_value_rpc_1_svc(key_request_rpc arg1, get_value_response_rpc *result, struct svc_req *rqstp) {
    char value1[MAX_STR_LEN + 1];
    int n_value2 = 0;
    float v_value2[MAX_V2_LEN];
    struct Paquete value3;
    char *value1_copy;
    float *v_value2_copy = NULL;

    (void)rqstp;

    if (!init_get_value_response(result)) {
        return FALSE;
    }

    result->status = -1;
    if (arg1.key == NULL) {
        return TRUE;
    }

    result->status = get_value(arg1.key, value1, &n_value2, v_value2, &value3);
    if (result->status != 0) {
        return TRUE;
    }

    value1_copy = dup_rpc_string(value1);
    if (value1_copy == NULL) {
        return FALSE;
    }

    v_value2_copy = (float *)malloc((size_t)n_value2 * sizeof(float));
    if (v_value2_copy == NULL) {
        free(value1_copy);
        return FALSE;
    }

    memcpy(v_value2_copy, v_value2, (size_t)n_value2 * sizeof(float));
    free(result->value1);
    result->value1 = value1_copy;
    result->n_value2 = n_value2;
    result->v_value2.rpc_value2_len = (u_int)n_value2;
    result->v_value2.rpc_value2_val = v_value2_copy;
    result->value3.x = value3.x;
    result->value3.y = value3.y;
    result->value3.z = value3.z;
    return TRUE;
}

bool_t modify_value_rpc_1_svc(set_value_request_rpc arg1, int *result, struct svc_req *rqstp) {
    struct Paquete value3;

    (void)rqstp;

    if (!valid_set_request(&arg1)) {
        *result = -1;
        return TRUE;
    }

    value3.x = arg1.value3.x;
    value3.y = arg1.value3.y;
    value3.z = arg1.value3.z;
    *result = modify_value(arg1.key, arg1.value1, arg1.n_value2, arg1.v_value2.rpc_value2_val, value3);
    return TRUE;
}

bool_t delete_key_rpc_1_svc(key_request_rpc arg1, int *result, struct svc_req *rqstp) {
    (void)rqstp;

    if (arg1.key == NULL) {
        *result = -1;
        return TRUE;
    }

    *result = delete_key(arg1.key);
    return TRUE;
}

bool_t exist_rpc_1_svc(key_request_rpc arg1, int *result, struct svc_req *rqstp) {
    (void)rqstp;

    if (arg1.key == NULL) {
        *result = -1;
        return TRUE;
    }

    *result = exist(arg1.key);
    return TRUE;
}

int claves_rpc_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result) {
    (void)transp;
    xdr_free(xdr_result, result);
    return 1;
}