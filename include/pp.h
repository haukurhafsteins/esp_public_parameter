#pragma once

#include "esp_event.h"

#define MAX_PAR_NAME 16
#define MAX_ARRAY_SIZE 2048
#define ABS_MAX_ARRAY_SIZE 4096

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct pp_hooks
    {
        void *(*malloc_fn)(size_t sz);
        void *(*calloc_fn)(size_t sz, size_t count);
        void(*free_fn)(void *ptr);
    } pp_hooks;

    /// @brief Enumeration of parameter types.
    typedef enum
    {
        TYPE_UNKNOWN = 0,          ///< Unknown type.
        TYPE_INT32 = 0x1,          ///< 32-bit integer type.
        TYPE_INT64 = 0x2,          ///< 64-bit integer type.
        TYPE_FLOAT = 0x4,          ///< Floating-point type.
        TYPE_BOOL = 0x8,           ///< Boolean type.
        TYPE_FLOAT_ARRAY = 0x10,   ///< Array of floating-point numbers.
        TYPE_INT16_ARRAY = 0x20,   ///< Array of 16-bit integers.
        TYPE_EXECUTE = 0x40,       ///< Execute type (used for triggering actions).
        TYPE_STRING = 0x80,        ///< String type.
        TYPE_BINARY = 0x100,       ///< Binary data type.
        TYPE_ALL = 0x7FFFFFFF,     ///< All types (used for masking).
        TYPE_HIDE = 0x80000000,    ///< Hidden type (not shown in the list).
    } parameter_type_t;

    /// @brief Structure representing a float array.
    typedef struct
    {
        size_t len;   ///< Length of the array.
        float data[]; ///< Array of float values.
    } pp_float_array_t;

    /// @brief Structure representing an int16 array.
    typedef struct
    {
        size_t len;                   ///< Length of the array.
        int16_t data[MAX_ARRAY_SIZE]; ///< Array of int16 values.
    } pp_int16_array_t;

    /// @brief Structure representing an event loop.
    typedef struct
    {
        esp_event_loop_handle_t loop_handle; ///< Event loop handle.
        esp_event_base_t base;               ///< Event base.
    } pp_evloop_t;

    /// @brief Structure representing information about a parameter.
    typedef struct pp_info_t
    {
        const char *name;         ///< Name of the parameter.
        parameter_type_t type;    ///< Type of the parameter.
        const pp_evloop_t *owner; ///< Owner event loop of the parameter.
        const void *valueptr;     ///< Pointer to the parameter's value.
        size_t subscriptions;     ///< Number of subscriptions to the parameter.
    } pp_info_t;

    typedef struct public_parameter_t public_parameter_t; ///< Opaque handle to a public parameter.
    typedef public_parameter_t *pp_t;       ///< Opaque handle to a parameter.
    typedef void *pp_event_t; ///< Opaque handle to an event.

    /// @brief Callback function to convert a parameter to a JSON string.
    /// @param pp The parameter to convert.
    /// @param buf The buffer to write the JSON string to.
    /// @param bufsize The size of the buffer.
    /// @param json If true, a JSON document is returned; otherwise, a single JSON variable is returned.
    typedef bool (*pp_json_cb_t)(pp_t pp, const char* format, char *buf, size_t *bufsize, bool json);

    typedef void (*pp_subscribe_cb_t)(pp_t pp, bool subscribe);

    /// @brief Get a parameter by its name.
    /// @param name The name of the parameter.
    /// @return A handle to the parameter, or NULL if not found.
    pp_t pp_get(const char *name);

    /// @brief Get the float value of a parameter.
    /// @param pp The parameter handle.
    /// @return The float value of the parameter.
    float pp_get_float_value(pp_t pp);

    /// @brief Get the byte size required for a float array of a given length.
    /// @param len The length of the float array.
    /// @return The byte size required for the float array.
    size_t pp_get_float_array_byte_size(size_t len);

    /// @brief Get the type of a parameter.
    /// @param pp The parameter handle.
    /// @return The type of the parameter.
    parameter_type_t pp_get_type(pp_t pp);

    /// @brief Get the name of a parameter.
    /// @param pp The parameter handle.
    /// @return The name of the parameter.
    const char *pp_get_name(pp_t pp);

    /// @brief Get the value pointer of a parameter.
    /// @param pp The parameter handle.
    /// @return The pointer to the parameter's value.
    const void *pp_get_valueptr(pp_t pp);

    /// @brief Get a parameter by its index.
    /// @param index The index of the parameter.
    /// @return A handle to the parameter, or NULL if the index is out of bounds.
    pp_t pp_get_par(int index);

    /// @brief Get the owner event loop of a parameter.
    /// @param pp The parameter handle.
    /// @return The owner event loop of the parameter.
    const pp_evloop_t *pp_get_owner(pp_t pp);

    /// @brief Get the number of subscriptions to a parameter.
    /// @param pp The parameter handle.
    /// @return The number of subscriptions to the parameter.
    int pp_get_subscriptions(pp_t pp);

    /// @brief Enable or disable a parameter.
    /// @param pp The parameter handle.
    /// @param enable True to enable the parameter, false to disable it.
    void pp_enable(pp_t pp, bool enable);

    /// @brief Check if a parameter is enabled.
    /// @param pp The parameter handle.
    /// @return True if the parameter is enabled, false otherwise.
    bool pp_is_enabled(pp_t pp);

    /// @brief Allocate memory for a float array.
    /// @param len The length of the float array.
    /// @return A pointer to the allocated float array, or NULL if allocation failed. 
    /// @attention The user is responsible for freeing the memory using pp_free().
    pp_float_array_t *pp_allocate_float_array(size_t len);

    /// @brief Free allocated memory for all pp functions that allocate and return a pointer.
    /// @param ptr The pointer to free.
    void pp_free(void* ptr);
    
    /// @brief Reset a float array to zero.
    /// @param array The float array to reset.
    void pp_reset_float_array(pp_float_array_t *array);

    /// @brief Reset an int16 array to zero.
    /// @param array The int16 array to reset.
    void pp_reset_int16_array(pp_int16_array_t *array);

    /// @brief Create a new int32 parameter.
    /// @param name The name of the parameter.
    /// @param evloop The event loop associated with the parameter.
    /// @param event_write_cb The callback function for write events.
    /// @param valueptr The pointer to the parameter's value.
    /// @return A handle to the created parameter.
    pp_t pp_create_int32(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, const int32_t *valueptr);

    /// @brief Create a new int64 parameter.
    /// @param name The name of the parameter.
    /// @param evloop The event loop associated with the parameter.
    /// @param event_write_cb The callback function for write events.
    /// @param valueptr The pointer to the parameter's value.
    /// @return A handle to the created parameter.
    pp_t pp_create_int64(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, const int64_t *valueptr);

    /// @brief Create a new float parameter.
    /// @param name The name of the parameter.
    /// @param evloop The event loop associated with the parameter.
    /// @param event_write_cb The callback function for write events.
    /// @param valueptr The pointer to the parameter's value.
    /// @return A handle to the created parameter.
    pp_t pp_create_float(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, float *valueptr);

    /// @brief Create a new float array parameter.
    /// @param name The name of the parameter.
    /// @param evloop The event loop associated with the parameter.
    /// @param event_write_cb The callback function for write events.
    /// @return A handle to the created parameter.
    pp_t pp_create_float_array(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb);

    /// @brief Create a new boolean parameter.
    /// @param name The name of the parameter.
    /// @param evloop The event loop associated with the parameter.
    /// @param event_write_cb The callback function for write events.
    /// @param valueptr The pointer to the parameter's value.
    /// @return A handle to the created parameter.
    pp_t pp_create_bool(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, bool *valueptr);

    /// @brief Create a new binary parameter.
    /// @param name The name of the parameter.
    /// @param evloop The event loop associated with the parameter.
    /// @param event_write_cb The callback function for write events.
    /// @return A handle to the created parameter.
    pp_t pp_create_binary(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb);

    /// @brief Create a new string parameter.
    /// @param name The name of the parameter.
    /// @param evloop The event loop associated with the parameter.
    /// @param event_write_cb The callback function for write events.
    /// @return A handle to the created parameter.
    pp_t pp_create_string(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb);

    /// @brief Delete a parameter.
    /// @param pp The parameter handle.
    /// @return True if the parameter was successfully deleted, false otherwise.
    bool pp_delete(pp_t pp);

    /// @brief Post a new state for an int32 parameter.
    /// @param pp The parameter handle.
    /// @param i The new int32 value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_int32(pp_t pp, int32_t i);

    /// @brief Post a new state for an int32 parameter from an ISR.
    /// @param pp The parameter handle.
    /// @param i The new int32 value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_int32_irq(pp_t pp, int32_t i);

    /// @brief Post a new state for an int64 parameter.
    /// @param pp The parameter handle.
    /// @param i The new int64 value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_int64(pp_t pp, int64_t i);

    /// @brief Post a new state for a boolean parameter.
    /// @param pp The parameter handle.
    /// @param b The new boolean value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_bool(pp_t pp, bool b);

    /// @brief Post a new state for a boolean parameter from an ISR.
    /// @param pp The parameter handle.
    /// @param b The new boolean value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_bool_irq(pp_t pp, bool b);

    /// @brief Post a new state for a float parameter.
    /// @param pp The parameter handle.
    /// @param f The new float value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_float(pp_t pp, float f);

    /// @brief Post a new state for a float parameter from an ISR.
    /// @param pp The parameter handle.
    /// @param f The new float value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_float_irq(pp_t pp, float f);

    /// @brief Post a new state for a float array parameter.
    /// @param pp The parameter handle.
    /// @param array The new float array.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_float_array(pp_t pp, pp_float_array_t *array);

    /// @brief Post a new state for a binary parameter.
    /// @param pp The parameter handle.
    /// @param bin The new binary data.
    /// @param size The size of the binary data.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_binary(pp_t pp, void *bin, size_t size);

    /// @brief Post a new state for a string parameter.
    /// @param pp The parameter handle.
    /// @param str The new string value.
    /// @return True if the new state was successfully posted, false otherwise.
    bool pp_post_newstate_string(pp_t pp, const char *str);

    /// @brief Post a write event for an int32 parameter.
    /// @param pp The parameter handle.
    /// @param value The new int32 value.
    /// @return True if the write event was successfully posted, false otherwise.
    bool pp_post_write_int32(pp_t pp, int32_t value);

    /// @brief Post a write event for an int64 parameter.
    /// @param pp The parameter handle.
    /// @param value The new int64 value.
    /// @return True if the write event was successfully posted, false otherwise.
    bool pp_post_write_int64(pp_t pp, int64_t value);

    /// @brief Post a write event for a float parameter.
    /// @param pp The parameter handle.
    /// @param value The new float value.
    /// @return True if the write event was successfully posted, false otherwise.
    bool pp_post_write_float(pp_t pp, float value);

    /// @brief Post a write event for a boolean parameter.
    /// @param pp The parameter handle.
    /// @param value The new boolean value.
    /// @return True if the write event was successfully posted, false otherwise.
    bool pp_post_write_bool(pp_t pp, bool value);

    /// @brief Post a write event for a string parameter.
    /// @param pp The parameter handle.
    /// @param str The new string value.
    /// @return True if the write event was successfully posted, false otherwise.
    bool pp_post_write_string(pp_t pp, const char *str);

    /// @brief Get the parameter's value as string.
    /// @param pp The parameter handle.
    /// @param format The format string. If NULL, the default format is used.
    /// @param buf The buffer to store the string.
    /// @param bufsize The size of the buffer.
    /// @return True if the value was successfully converted to a string, false otherwise.
    bool pp_to_string(pp_t pp, const char* format, char *buf, size_t *bufsize);

    /// @brief Get the parameter's value as json string.
    /// @param pp The parameter handle.
    /// @param format The format string. If NULL, the default format is used.
    /// @param buf The buffer to store the string.
    /// @param bufsize The size of the buffer.
    /// @return True if the value was successfully converted to a string, false otherwise.
    bool pp_to_json_string(pp_t pp, const char* format, char *buf, size_t *bufsize);

    /// @brief Register an event handler for a specific event loop.
    /// @param evloop The event loop.
    /// @param id The event ID.
    /// @param cb The callback function.
    /// @param p The context pointer.
    /// @return True if the event handler was successfully registered, false otherwise.
    // bool pp_event_handler_register(const pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb, void *p);

    /// @brief Unregister an event handler for a specific event loop.
    /// @param evloop The event loop.
    /// @param id The event ID.
    /// @param cb The callback function.
    /// @return True if the event handler was successfully unregistered, false otherwise.
    // bool pp_event_handler_unregister(const pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb);

    /// @brief Register a callback for subscription events.
    /// @param evloop The event loop.
    /// @param cb The callback function.
    /// @param p The context pointer, will contain the pp_t handle.
    /// @return True if the callback was successfully registered, false otherwise.
    bool pp_register_subscribe_cb(const pp_t pp, pp_subscribe_cb_t cb);
    // bool pp_event_handler_register_subscribe_cb(const pp_evloop_t *evloop, esp_event_handler_t cb, void *p);

    /// @brief Register a callback for unsubscription events.
    /// @param evloop The event loop.
    /// @param cb The callback function.
    /// @param p The context pointer.
    /// @return True if the callback was successfully registered, false otherwise.
    // bool pp_event_handler_register_unsubscribe_cb(const pp_evloop_t *evloop, esp_event_handler_t cb, void *p);

    /// @brief Subscribe to a parameter.
    /// @param pp The parameter handle.
    /// @param receiver The event loop to receive updates.
    /// @param event_cb The callback function for updates.
    /// @return True if the subscription was successful, false otherwise.
    bool pp_subscribe(pp_t pp, const pp_evloop_t *receiver, esp_event_handler_t event_cb);

    /// @brief Unsubscribe from a parameter.
    /// @param pp The parameter handle.
    /// @param receiver The event loop to unsubscribe from.
    /// @param event_cb The callback function to remove.
    /// @return True if the unsubscription was successful, false otherwise.
    bool pp_unsubscribe(pp_t pp, const pp_evloop_t *receiver, esp_event_handler_t event_cb);

    /// @brief Get information about a parameter by index.
    /// @param index The index of the parameter.
    /// @param info The structure to store the parameter information.
    /// @return The index of the next parameter, or -1 if no more parameters are available.
    int pp_get_info(int index, pp_info_t *info);

    /// @brief Set a JSON callback for a parameter.
    /// @param pp The parameter handle.
    /// @param cb The JSON callback function.
    /// @return True if the callback was successfully set, false otherwise.
    bool pp_set_json_cb(pp_t pp, pp_json_cb_t cb);

    /// @brief Set the value pointer for a parameter.
    /// @param pp The parameter handle.
    /// @param valueptr The pointer to the parameter's value.
    /// @return True if the value pointer was successfully set, false otherwise.
    bool pp_set_valueptr(pp_t pp, const void *valueptr);

    /// @brief Set the context for a parameter.
    /// @param pp The parameter handle.
    /// @param context The context pointer.
    /// @return True if the context was successfully set, false otherwise.
    bool pp_set_context(pp_t pp, void *context);

    /// @brief Get the context of a parameter.
    /// @param pp The parameter handle.
    /// @return The context pointer.
    void *pp_get_context(pp_t pp);

    /// @brief Supply malloc, realloc and free functions.
    /// @param hooks The hooks structure containing the functions.
    void pp_init_hooks(pp_hooks *hooks);

    /// @brief Get the number of parameters.
    /// @return The number of parameters.
    size_t pp_get_parameter_count(void);

    /// @brief Get a list of all parameters as JSON.
    /// @param buf The buffer to store the JSON string. 
    /// @return True if the JSON string was successfully generated, false otherwise.
    /// @attention The user is responsible for freeing the buffer using pp_free().
    bool pp_get_parameter_list_as_json(char **buf, parameter_type_t type);

#ifdef __cplusplus
} // extern "C"
#endif