#ifndef __pp_h__
#define __pp_h__

#include "esp_event.h"

#define MAX_PAR_NAME 16
#define MAX_ARRAY_SIZE 2048
#define ABS_MAX_ARRAY_SIZE 4096

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        TYPE_UNKNOWN = -1,
        TYPE_INT32,
        TYPE_INT64,
        TYPE_FLOAT,
        TYPE_BOOL,
        TYPE_FLOAT_ARRAY,
        TYPE_INT16_ARRAY,
        TYPE_EXECUTE,
        TYPE_STRING,
        TYPE_BINARY
    } parameter_type_t;

    typedef struct
    {
        size_t len;
        float data[];
    } pp_float_array_t;

    typedef struct
    {
        size_t len;
        int16_t data[MAX_ARRAY_SIZE];
    } pp_int16_array_t;

    typedef struct
    {
        esp_event_loop_handle_t loop_handle;
        esp_event_base_t base;
    } pp_evloop_t;

    typedef struct pp_info_t {
        const char* name;
        parameter_type_t type;
        pp_evloop_t *owner;
        const void* valueptr;
        size_t subscriptions;
    } pp_info_t;

    typedef void *pp_t;
    typedef void *pp_event_t;

    /// @brief Callback function to convert a parameter to a JSON string.
    /// @param pp The parameter to convert.
    /// @param buf The buffer to write the JSON string to.
    /// @param bufsize The size of the buffer.
    /// @param json If true, a json document is returned, otherwise a single json variable is returned.
    typedef bool (*pp_json_cb_t)(pp_t pp, char* buf, size_t *bufsize, bool json);

    pp_t pp_get(const char *name);
    float pp_get_float_value(pp_t pp);
    size_t pp_get_float_array_byte_size(size_t len);
    parameter_type_t pp_get_type(pp_t pp);
    const char *pp_get_name(pp_t pp);
    const void *pp_get_valueptr(pp_t pp);
    pp_t pp_get_par(int index);
    pp_evloop_t *pp_get_owner(pp_t pp);
    int pp_get_subscriptions(pp_t pp);

    void pp_enable(pp_t pp, bool enable);
    bool pp_is_enabled(pp_t pp);

    pp_float_array_t *pp_allocate_float_array(size_t len);
    void pp_reset_float_array(pp_float_array_t *array);
    void pp_reset_int16_array(pp_int16_array_t *array);

    pp_t pp_create_int32(const char *name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, const int32_t *valueptr);
    pp_t pp_create_int64(const char *name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, const int64_t *valueptr);
    pp_t pp_create_float(const char *name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, float *valueptr);
    pp_t pp_create_float_array(const char *name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb);
    pp_t pp_create_bool(const char *name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, bool *valueptr);
    pp_t pp_create_binary(const char *name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb/*, tojson_cb_t tojson*/);
    pp_t pp_create_string(const char *name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb/*, tojson_cb_t tojson*/);

    bool pp_delete(pp_t pp);

    bool pp_post_newstate_int32(pp_t pp, int32_t i);
    bool pp_post_newstate_int32_irq(pp_t pp, int32_t i);
    bool pp_post_newstate_int64(pp_t pp, int64_t i);
    bool pp_post_newstate_bool(pp_t pp, bool b);
    bool pp_post_newstate_float(pp_t pp, float f);
    bool pp_post_newstate_float_array(pp_t pp, pp_float_array_t *array);
    bool pp_post_newstate_binary(pp_t pp, void *bin, size_t size);
    bool pp_post_newstate_string(pp_t pp, const char *str);

    bool pp_post_write_int32(pp_t pp, int32_t value);
    bool pp_post_write_float(pp_t pp, float value);
    bool pp_post_write_bool(pp_t pp, bool value);
    bool pp_post_write_string(pp_t pp, const char *str);

    bool pp_get_as_string(pp_t pp, char *buf, size_t *bufsize, bool json);

    //pp_event_t pp_event_add(pp_evloop_t *evloop, int id, int ms, bool periodic, void *data, size_t data_size);
    bool pp_event_handler_register(pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb, void *p);
    bool pp_event_handler_unregister(pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb);
    void pp_event_remove(pp_event_t ev);

    bool pp_subscribe(pp_t pp, pp_evloop_t *receiver, esp_event_handler_t event_cb);
    bool pp_unsubscribe(pp_t pp, pp_evloop_t *receiver, esp_event_handler_t event_cb);

    int pp_get_info(int index, pp_info_t *info);

    bool pp_set_json_cb(pp_t pp, pp_json_cb_t cb);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
