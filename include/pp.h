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
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_FLOAT_ARRAY,
    TYPE_INT16_ARRAY,
    TYPE_EXECUTE,
    TYPE_STRING,
    TYPE_BINARY
} parameter_type_t;

typedef enum
{
    UNIT_NONE,  // ""
    UNIT_V,     // "V"
    UNIT_A,     // "A"
    UNIT_Wh,    // "Wh"
    UNIT_lpm,   // "lpm"
    UNIT_Kg,     // "Kg"
    UNIT_PERCENTAGE, // %
    UNIT_G,      // g
    UNIT_END
} pp_unit_t;

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
    void* hd;
    int socket;
} pp_webclient_t;

typedef struct
{
    pp_webclient_t client;
    char payload[0];
} pp_websocket_data_t;

typedef struct
{
    esp_event_loop_handle_t loop_handle;
    esp_event_base_t base;
} pp_evloop_t;

typedef size_t (*tojson_cb_t)(void *binary, char *json, size_t json_size);

typedef void* pp_t;
typedef void* pp_event_t;

/**
 * @brief Initialize the Public Parameter handler.
 * 
 */
<<<<<<< HEAD
void pp_init(unsigned task_priority);
=======
void pp_start();
>>>>>>> 4c6b326a86d2e46b9d4f181a4ed06d994a651a9e

/**
 * @brief Create a public float parameter.
 * 
 * @param name Paremeter name
 * @param evloop The eventloop that owns the parameter.
 * @return pp_t 
 */
pp_t pp_create_float(const char* name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, float* valueptr);
bool pp_delete(pp_t pp);
bool pp_post_newstate_float(pp_t pp, float f);
bool pp_post_write_float(pp_t pp, float value);
float pp_get_float_value(pp_t pp);

pp_t pp_create_float_array(const char* name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb);
bool pp_post_newstate_float_array(pp_t pp, pp_float_array_t *array);
pp_float_array_t *pp_allocate_float_array(size_t len);
size_t pp_get_float_array_size(size_t len);
void pp_reset_float_array(pp_float_array_t *array);

pp_t pp_create_bool(const char* name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, bool* valueptr);
pp_t pp_create_binary(const char* name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, tojson_cb_t tojson);


/**
 * @brief Triggers an update of a parameter.
 * 
 * @param name Parameter name
 * @param evloop The eventloop that owns the parameter
 * @param event_cb The callback the eventloop uses to handle request/execution
 * @return pp_t 
 */
pp_t pp_create_string(const char* name, pp_evloop_t *evloop, esp_event_handler_t event_write_cb, tojson_cb_t tojson);

/**
 * @brief Subscribe to a parameter and get an event if it changes. 
 * 
 * @param pp The parameter handle
 * @param receiver The eventloop that will receive the newstate event
 * @return A handle to the parameter 
 */
pp_t pp_subscribe(pp_t pp, pp_evloop_t *receiver, esp_event_handler_t event_cb);
pp_t pp_unsubscribe(pp_t pp, pp_evloop_t *receiver, esp_event_handler_t event_cb);

/**
 * @brief Write to a public parameter
 * 
 * @param pp The paramter to write.
 * @param newvalue The new value for the paramter.
 * @param newvalue_size Size of the new value in bytes
 * @return true 
 * @return false 
 */
bool pp_post_write_bool(pp_t pp, bool value);
bool pp_post_write_string(pp_t pp, const char* str);

bool pp_post_newstate_binary(pp_t pp, void* bin, size_t size);

pp_event_t pp_event_add(pp_evloop_t *evloop, int id, int ms, bool periodic, void *data, size_t data_size);
void pp_event_remove(pp_event_t ev);

pp_t pp_get(const char *name);
bool pp_print_parameter(int index, char *buf, size_t bufsize);
void pp_reset_int16_array(pp_int16_array_t *array);

int pp_vprintf ( const char * format, va_list arg );

const char* pp_unit_to_str(pp_unit_t unit);
pp_unit_t pp_string_to_unit(const char *str);

bool pp_set_unit(pp_t pp, pp_unit_t unit);
pp_unit_t pp_get_unit(pp_t pp);

parameter_type_t pp_get_type(pp_t pp);
const char* pp_get_name(pp_t pp);
tojson_cb_t pp_get_tojson(pp_t pp);
void* pp_get_valueptr(pp_t pp);
pp_t pp_get_par(int index);
pp_evloop_t *pp_get_owner(pp_t pp);
int pp_get_subscriptions(pp_t pp);
void pp_event_handler_register(pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb, void* p);
void pp_event_handler_unregister(pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb);
bool pp_evloop_post(pp_evloop_t *evloop, int32_t id, void *data, size_t data_size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
