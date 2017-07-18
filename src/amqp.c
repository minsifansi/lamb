
/* 
 * Lamb Gateway Platform
 * By typefo <typefo@qq.com>
 * Update: 2017-07-14
 */

#include <stdio.h>
#include "amqp.h"

int lamb_amqp_connect(lamb_amqp_t *amqp, const char *host, int port) {
    int status;
    amqp->conn = amqp_new_connection();
    amqp->socket = amqp_tcp_socket_new(amqp->conn);

    if (!amqp->socket) {
        return -1;
    }

    status = amqp_socket_open(amqp->socket, host, port);
    if (status) {
        return -1;
    }

    return 0;
}

int lamb_amqp_login(lamb_amqp_t *amqp, const char *user, const char *password) {
    amqp_rpc_reply_t rep;
    rep = amqp_login(amqp->conn, "/", 0, AMQP_DEFAULT_FRAME_SIZE, 0, AMQP_SASL_METHOD_PLAIN, user, password);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return -1;
    }

    amqp_channel_open(conn, 1);

    rep = amqp_get_rpc_reply(conn);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return -1;
    }
    
    return 0;
}

int lamb_amqp_bind(lamb_amqp_t *amqp, char const *queue, char const *exchange, char const *key) {
    amqp_rpc_reply_t rep;
    amqp_queue_declare_ok_t *r; 

    amqp_channel_open(amqp->conn, 1);
    rep = amqp_get_rpc_reply(amqp->conn);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return -1;
    }
    
    r = amqp_queue_declare(amqp->conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
    rep = amqp_get_rpc_reply(conn);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return 0;
    }
    
    amqp->queue = amqp_bytes_malloc_dup(r->queue);
    
    if (amqp->queue.bytes == NULL) {
        return -1;
    }

    amqp_queue_bind(amqp->conn, 1, amqp->queue, amqp_cstring_bytes(exchange), amqp_cstring_bytes(key), amqp_empty_table);
    rep = amqp_get_rpc_reply(conn);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return -1;
    }

    return 0;
}

int lamb_amqp_basic_consume(lamb_amqp_t *amqp) {
    amqp_rpc_reply_t rep;
    amqp_basic_consume(amqp->conn, 1, amqp->queue, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return -1;
    }

    return 0;
}

int lamb_amqp_basic_publish(lamb_amqp_t *amqp, const char *exchange, const char *key, void *data, size_t len) {
    amqp_rpc_reply_t rep;
    amqp_bytes_t message;

    message.len = len;
    message.bytes = data;

    rep = amqp_basic_publish(amqp->conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(key), 0, 0, NULL, message);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return -1;
    }

    return 0;
}

int lamb_amqp_consume_message(lamb_amqp_t *amqp, void *buff, size_t len, long long milliseconds) {
    amqp_rpc_reply_t rep;
    amqp_envelope_t envelope;
    struct timeval timeout;

    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000000;

    amqp_maybe_release_buffers(amqp->conn);
    rep = amqp_consume_message(amqp->conn, &envelope, ((milliseconds > 0) ? &timeout : NULL), 0);
    if (rep.reply_type != AMQP_RESPONSE_NORMAL) {
        return -1;
    }

    len = (envelope.message.body.len > len) ? len : envelope.message.body.len;
    memcpy(buff, envelope.message.body.bytes, len);

    amqp_destroy_envelope(&envelope);

    return 0;
}

int lamb_amqp_destroy_message(lamb_amqp_message_t *message) {
    amqp_destroy_message(&message->message);
    return 0;
}

int lamb_amqp_destroy_connection(lamb_amqp_t *amqp) {
    amqp_channel_close(amqp->conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(amqp->conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(amqp->conn);
    return 0;
}
