const RPC_MAX_STR_LEN = 255;
const RPC_MAX_V2_LEN = 32;

struct paquete_rpc {
	int x;
	int y;
	int z;
};

typedef string rpc_string<RPC_MAX_STR_LEN>;
typedef float rpc_value2<RPC_MAX_V2_LEN>;

struct key_request_rpc {
	rpc_string key;
};

struct set_value_request_rpc {
	rpc_string key;
	rpc_string value1;
	int n_value2;
	rpc_value2 v_value2;
	paquete_rpc value3;
};

struct get_value_response_rpc {
	int status;
	rpc_string value1;
	int n_value2;
	rpc_value2 v_value2;
	paquete_rpc value3;
};

program CLAVES_RPC_PROG {
	version CLAVES_RPC_V1 {
		int DESTROY_RPC(void) = 1;
		int SET_VALUE_RPC(set_value_request_rpc) = 2;
		get_value_response_rpc GET_VALUE_RPC(key_request_rpc) = 3;
		int MODIFY_VALUE_RPC(set_value_request_rpc) = 4;
		int DELETE_KEY_RPC(key_request_rpc) = 5;
		int EXIST_RPC(key_request_rpc) = 6;
	} = 1;
} = 0x31234567;