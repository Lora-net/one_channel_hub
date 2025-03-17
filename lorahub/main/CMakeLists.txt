set(libtools "base64.c" "parson.c")
set(pkt-fwd "jitqueue.c" "config_nvs.c" "display.c" "wifi.c" "http_server.c" "pkt_fwd.c" "main.c" )

idf_component_register(SRCS "${libtools}" "${pkt-fwd}"
                       INCLUDE_DIRS ".")
