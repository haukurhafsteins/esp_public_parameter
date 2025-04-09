#include <string>
#include <string.h>
#include <map>
#include <list>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "pp.h"
#include "esp_debug_helpers.h"

#define MAX_PUBLIC_PARAMETERS 55
#define ID_SUBSCRIBE 1000
#define ID_UNSUBSCRIBE 1001
#define ID_COUNTER_START 1002
#define POST_WAIT_MS 50

typedef struct
{
    // Configuration part
    struct
    {
        const char *name;
        const pp_evloop_t *owner;
        pp_json_cb_t json_cb;
        parameter_type_t type;
    } conf;

    // State part
    struct
    {
        std::map<esp_event_loop_handle_t, pp_evloop_t> subscription_list;
        pp_subscribe_cb_t subscribe_cb;
        int32_t newstate_id;
        int32_t write_id;
        /// @brief True if the parameter is active, false if it is inactive.
        bool is_active;
        const void *valueptr;
        void *context;
    } state;
} public_parameter_t;

static public_parameter_t par_list[MAX_PUBLIC_PARAMETERS];
static std::map<std::string, public_parameter_t *> nameToPP;
static int32_t event_id_counter = ID_COUNTER_START;
static pp_hooks hooks = {malloc, calloc, free};

static const char *TAG = "PP";

static esp_err_t evloop_post(esp_event_loop_handle_t loop_handle, esp_event_base_t loop_base, int32_t id, void *data, size_t data_size)
{
    if (loop_handle == NULL)
        return esp_event_post(loop_base, id, data, data_size, pdMS_TO_TICKS(POST_WAIT_MS));
    return esp_event_post_to(loop_handle, loop_base, id, data, data_size, pdMS_TO_TICKS(POST_WAIT_MS));
}

static bool pp_newstate(public_parameter_t *p, void *data, size_t data_size)
{
    if (p == NULL)
    {
        ESP_LOGE(TAG, "%s: Parameter pointer is NULL", __func__);
        return false;
    }
    if (data == NULL)
    {
        ESP_LOGE(TAG, "%s: Data pointer is NULL", __func__);
        return false;
    }
    if (data_size == 0)
    {
        ESP_LOGE(TAG, "%s: Data size is NULL", __func__);
        return false;
    }
    int size = p->state.subscription_list.size();
    for (auto itc = p->state.subscription_list.begin(); itc != p->state.subscription_list.end(); itc++)
    {
        esp_err_t err = evloop_post(itc->second.loop_handle, itc->second.base, p->state.newstate_id, data, data_size);
        if (err == ESP_OK)
            size--;
        else
        {
            // ESP_LOGE(TAG, "%s: Failed sending %s to %s - %s", __func__, p->conf.name ,itc->second.base, esp_err_to_name(err));
            // esp_backtrace_print(5);
        }
    }
    return (size == 0); // all sends successful
}

static bool pp_newstate_irq(public_parameter_t *p, void *data, size_t data_size)
{
    int size = p->state.subscription_list.size();
    if (size > 0)
    {
        for (auto itc = p->state.subscription_list.begin(); itc != p->state.subscription_list.end(); itc++)
        {
            if (ESP_OK == esp_event_isr_post_to(itc->second.loop_handle, itc->second.base, p->state.newstate_id, data, data_size, NULL))
                size--;
        }
        return (size == 0); // all sends successful
    }
    return false;
}

static bool pp_event_handler_register(const pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb, void *p)
{
    esp_err_t err;
    if (evloop->loop_handle == NULL)
    {
        err = esp_event_handler_register(evloop->base, id, cb, p);
        ESP_ERROR_CHECK(err);
    }
    else
    {
        err = esp_event_handler_instance_register_with(evloop->loop_handle, evloop->base, id, cb, p, NULL);
        ESP_ERROR_CHECK(err);
    }
    return err == ESP_OK;
}

static bool pp_event_handler_unregister(const pp_evloop_t *evloop, int32_t id, esp_event_handler_t cb)
{
    esp_err_t err;
    if (evloop->loop_handle == NULL)
    {
        err = esp_event_handler_unregister(evloop->base, id, cb);
        ESP_ERROR_CHECK(err);
    }
    else
    {
        err = esp_event_handler_unregister_with(evloop->loop_handle, evloop->base, id, cb);
        ESP_ERROR_CHECK(err);
    }
    return err == ESP_OK;
}

static pp_t pp_create(const char *name, const pp_evloop_t *evloop, parameter_type_t type, esp_event_handler_t event_write_cb, const void *valueptr)
{
    if (name == NULL)
    {
        ESP_LOGE(TAG, "%s: Name is NULL", __func__);
        return NULL;
    }

    if (name[0] == 0)
    {
        ESP_LOGE(TAG, "%s: Name is empty, owner %s", __func__, (evloop == NULL) ? "unknown (no evloop)" : evloop->base);
        return NULL;
    }

    std::string key = name;
    std::map<std::string, public_parameter_t *>::iterator it = nameToPP.find(key);
    if (it != nameToPP.end())
    {
        ESP_LOGW(TAG, "%s: %s exist", __func__, name);
        return it->second;
    }

    int par_list_index;
    for (par_list_index = 0; par_list_index < MAX_PUBLIC_PARAMETERS; par_list_index++)
    {
        if (par_list[par_list_index].conf.name == NULL)
            break;
    }

    if (par_list_index >= MAX_PUBLIC_PARAMETERS)
    {
        ESP_LOGW(TAG, "%s: %s not created, reached maximum %d", __func__, name, MAX_PUBLIC_PARAMETERS);
        return NULL;
    }

    public_parameter_t *p = &par_list[par_list_index];
    nameToPP[key] = p;

    p->conf.name = name;
    p->conf.owner = evloop;
    p->conf.type = type;
    p->conf.json_cb = NULL;
    p->state.newstate_id = event_id_counter++;
    p->state.write_id = event_id_counter++;
    p->state.valueptr = valueptr;
    p->state.is_active = true;

    if (event_write_cb && evloop)
        pp_event_handler_register(evloop, p->state.write_id, event_write_cb, p);
    return p;
}

static int pp_json_no_valueptr(const char *name, char *buf, size_t *bufsize, bool json)
{
    if (json)
        return snprintf(buf, *bufsize, "{\"%s\":null}", name);
    return snprintf(buf, *bufsize, "null");
}

static bool pp_json_int32(pp_t pp, char *buf, size_t *bufsize, bool json)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p->state.valueptr != NULL)
    {
        int32_t value = *((int32_t *)p->state.valueptr);
        if (json)
            *bufsize = snprintf(buf, *bufsize, "{\"%s\":%li}", p->conf.name, value);
        else
            *bufsize = snprintf(buf, *bufsize, "%li", value);
    }
    else
    {
        *bufsize = pp_json_no_valueptr(p->conf.name, buf, bufsize, json);
    }
    return true;
}

static bool pp_json_int64(pp_t pp, char *buf, size_t *bufsize, bool json)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p->state.valueptr != NULL)
    {
        int64_t value = *((int64_t *)p->state.valueptr);
        if (json)
            *bufsize = snprintf(buf, *bufsize, "{\"%s\":%lli}", p->conf.name, value);
        else
            *bufsize = snprintf(buf, *bufsize, "%lli", value);
    }
    else
    {
        *bufsize = pp_json_no_valueptr(p->conf.name, buf, bufsize, json);
    }
    return true;
}

static bool pp_json_float(pp_t pp, char *buf, size_t *bufsize, bool json)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p->state.valueptr != NULL)
    {
        float value = *((float *)p->state.valueptr);
        if (json)
            *bufsize = snprintf(buf, *bufsize, "{\"%s\":%f}", p->conf.name, value);
        else
            *bufsize = snprintf(buf, *bufsize, "%f", value);
    }
    else
    {
        *bufsize = pp_json_no_valueptr(p->conf.name, buf, bufsize, json);
    }
    return true;
}

static bool pp_json_bool(pp_t pp, char *buf, size_t *bufsize, bool json)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p->state.valueptr != NULL)
    {
        bool value = *((bool *)p->state.valueptr);
        if (json)
            *bufsize = snprintf(buf, *bufsize, "{\"%s\":%s}", p->conf.name, value ? "true" : "false");
        else
            *bufsize = snprintf(buf, *bufsize, "%s", value ? "true" : "false");
    }
    else
    {
        *bufsize = pp_json_no_valueptr(p->conf.name, buf, bufsize, json);
    }
    return true;
}

// static bool pp_json_string(pp_t pp, char *buf, size_t *bufsize, bool json)
// {
//     public_parameter_t *p = (public_parameter_t *)pp;
//     const char *value = (const char *)p->state.valueptr;
//     *bufsize = snprintf(buf, *bufsize, "%s", value);
//     return true;
// }

//-----------------------------------------------------------------------
// Public stuff
//-----------------------------------------------------------------------

bool pp_register_subscribe_cb(const pp_t pp, pp_subscribe_cb_t cb)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (pp == NULL)
    {
        ESP_LOGW(TAG, "%s: parameter is NULL", __func__);
        return false;
    }
    if (p->state.subscribe_cb == cb)
    {
        ESP_LOGW(TAG, "%s: %s already registered", __func__, p->conf.name);
        return false;
    }
    p->state.subscribe_cb = cb;
    return pp_event_handler_register(p->conf.owner, ID_SUBSCRIBE, [](void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
                                     {
        public_parameter_t *p = (public_parameter_t *)arg;
        p->state.subscribe_cb(p, true); }, pp);
}

pp_t pp_create_int32(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, const int32_t *valueptr)
{
    pp_t ret = pp_create(name, evloop, TYPE_INT32, event_write_cb, valueptr);
    if (ret != NULL)
        pp_set_json_cb(ret, pp_json_int32);
    return ret;
}
pp_t pp_create_int64(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, const int64_t *valueptr)
{
    pp_t ret = pp_create(name, evloop, TYPE_INT64, event_write_cb, valueptr);
    if (ret != NULL)
        pp_set_json_cb(ret, pp_json_int64);
    return ret;
}
pp_t pp_create_float(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, float *valueptr)
{
    pp_t ret = pp_create(name, evloop, TYPE_FLOAT, event_write_cb, valueptr);
    if (ret != NULL)
        pp_set_json_cb(ret, pp_json_float);
    return ret;
}
bool pp_delete(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p == NULL)
        return false;

    nameToPP.erase(p->conf.name);
    p->conf.name = NULL;
    return true;
}
float pp_get_float_value(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p == NULL)
        return 0.0f;

    return *((float *)p->state.valueptr);
}
pp_t pp_create_float_array(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb)
{
    return pp_create(name, evloop, TYPE_FLOAT_ARRAY, event_write_cb, NULL);
}
pp_t pp_create_bool(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb, bool *valueptr)
{
    pp_t ret = pp_create(name, evloop, TYPE_BOOL, event_write_cb, valueptr);
    if (ret != NULL)
        pp_set_json_cb(ret, pp_json_bool);
    return ret;
}
pp_t pp_create_string(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb /*, tojson_cb_t tojson*/)
{
    pp_t pp = pp_create(name, evloop, TYPE_STRING, event_write_cb, 0);
    return pp;
}
pp_t pp_create_binary(const char *name, const pp_evloop_t *evloop, esp_event_handler_t event_write_cb /*, tojson_cb_t tojson*/)
{
    pp_t pp = pp_create(name, evloop, TYPE_BINARY, event_write_cb, NULL);
    return pp;
}

pp_t pp_get(const char *name)
{
    std::string key = name;
    std::map<std::string, public_parameter_t *>::iterator it = nameToPP.find(key);
    if (it != nameToPP.end())
        return (pp_t)it->second;
    ESP_LOGW(TAG, "%s: parameter %s not found", __func__, name);
    return NULL;
}

bool pp_subscribe(pp_t pp, const pp_evloop_t *evloop, esp_event_handler_t event_cb)
{
    if (pp == NULL)
    {
        ESP_LOGW(TAG, "%s: parameter is NULL", __func__);
        return false;
    }
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p->conf.type == TYPE_EXECUTE)
    {
        ESP_LOGW(TAG, "%s: No subscription for execute", __func__);
        return false;
    }
    if (pp_event_handler_register(evloop, p->state.newstate_id, event_cb, p))
    {
        p->state.subscription_list[evloop->loop_handle] = *evloop;
        if (p->conf.owner != NULL && p->state.subscribe_cb != NULL)
            evloop_post(p->conf.owner->loop_handle, p->conf.owner->base, ID_SUBSCRIBE, pp, sizeof(pp_t));

        return true;
    }
    return false;
}
bool pp_unsubscribe(pp_t pp, const pp_evloop_t *evloop, esp_event_handler_t event_cb)
{
    if (pp == NULL)
    {
        ESP_LOGW(TAG, "%s: parameter is NULL", __func__);
        return false;
    }

    public_parameter_t *p = (public_parameter_t *)pp;
    if (p->conf.owner != NULL)
        evloop_post(p->conf.owner->loop_handle, p->conf.owner->base, ID_UNSUBSCRIBE, pp, sizeof(pp_t));
    return pp_event_handler_unregister(evloop, p->state.newstate_id, event_cb);
}
bool pp_post_write_bool(pp_t pp, bool value)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p == NULL || p->conf.owner == NULL)
        return false;
    return ESP_OK == evloop_post(p->conf.owner->loop_handle, p->conf.owner->base, p->state.write_id, (void *)&value, sizeof(bool));
}
bool pp_post_write_int32(pp_t pp, int32_t value)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p == NULL || p->conf.owner == NULL)
        return false;
    return ESP_OK == evloop_post(p->conf.owner->loop_handle, p->conf.owner->base, p->state.write_id, (void *)&value, sizeof(int32_t));
}
bool pp_post_write_int64(pp_t pp, int64_t value)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p == NULL || p->conf.owner == NULL)
        return false;
    return ESP_OK == evloop_post(p->conf.owner->loop_handle, p->conf.owner->base, p->state.write_id, (void *)&value, sizeof(int64_t));
}
bool pp_post_write_float(pp_t pp, float value)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p == NULL || p->conf.owner == NULL)
        return false;
    return ESP_OK == evloop_post(p->conf.owner->loop_handle, p->conf.owner->base, p->state.write_id, (void *)&value, sizeof(float));
}
bool pp_post_write_string(pp_t pp, const char *str)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p == NULL || p->conf.owner == NULL)
        return false;
    return ESP_OK == evloop_post(p->conf.owner->loop_handle, p->conf.owner->base, p->state.write_id, (void *)str, strlen(str) + 1);
}

bool pp_post_newstate_string(pp_t pp, const char *str)
{
    if (pp == NULL || str == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    if (p->state.subscription_list.size() > 0)
        return pp_newstate(p, (void *)str, strlen(str) + 1);
    return false;
}

bool pp_post_newstate_binary(pp_t pp, void *bin, size_t size)
{
    if (pp == NULL || bin == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    if (p->state.subscription_list.size() > 0)
        return pp_newstate(p, (void *)bin, size);
    return true;
}

bool pp_post_newstate_int32(pp_t pp, int32_t i)
{
    if (pp == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    if (p->state.subscription_list.size() > 0)
        return pp_newstate(p, &i, sizeof(int32_t));
    return true;
}

bool pp_post_newstate_int64(pp_t pp, int64_t i)
{
    if (pp == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    if (p->state.subscription_list.size() > 0)
        return pp_newstate(p, &i, sizeof(int64_t));
    return true;
}

bool pp_post_newstate_int32_irq(pp_t pp, int32_t i)
{
    if (pp == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    return pp_newstate_irq(p, &i, sizeof(int32_t));
}

bool pp_post_newstate_bool(pp_t pp, bool b)
{
    if (pp == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    if (p->state.subscription_list.size() > 0)
        return pp_newstate(p, &b, sizeof(bool));
    return true;
}

bool pp_post_newstate_bool_irq(pp_t pp, bool b)
{
    if (pp == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    return pp_newstate_irq(p, &b, sizeof(bool));
}

bool pp_post_newstate_float(pp_t pp, float f)
{
    if (pp == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    if (p->state.subscription_list.size() > 0)
        return pp_newstate(p, &f, sizeof(float));
    return true;
}

bool pp_post_newstate_float_irq(pp_t pp, float f)
{
    if (pp == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    return pp_newstate_irq(p, &f, sizeof(float));
}

bool pp_post_newstate_float_array(pp_t pp, pp_float_array_t *fsrc)
{
    if (pp == NULL || fsrc == NULL)
        return false;

    public_parameter_t *p = (public_parameter_t *)pp;

    if (p->state.subscription_list.size() > 0)
        return pp_newstate(p, (void *)fsrc, pp_get_float_array_byte_size(fsrc->len));
    return false;
}

void pp_reset_float_array(pp_float_array_t *array)
{
    memset(array->data, 0, sizeof(float) * array->len);
}

void pp_reset_int16_array(pp_int16_array_t *array)
{
    memset(array->data, 0, sizeof(int16_t) * array->len);
}

pp_float_array_t *pp_allocate_float_array(size_t len)
{
    pp_float_array_t *p = (pp_float_array_t *)hooks.calloc_fn(1, pp_get_float_array_byte_size(len));
    if (p == 0)
    {
        ESP_LOGE(TAG, "Failed to allocate %d bytes", len);
        esp_backtrace_print(5);
        return NULL;
    }
    p->len = len;
    return p;
}

size_t pp_get_float_array_byte_size(size_t len)
{
    return sizeof(pp_float_array_t) + sizeof(float) * len;
}

const void *pp_get_valueptr(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    return p->state.valueptr;
}

const char *pp_get_name(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    return p->conf.name;
}

parameter_type_t pp_get_type(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    return p->conf.type;
}

// tojson_cb_t pp_get_tojson(pp_t pp)
// {
//     public_parameter_t *p = (public_parameter_t *)pp;
//     return p->conf.tojson;
// }

const pp_evloop_t *pp_get_owner(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    return p->conf.owner;
}

int pp_get_subscriptions(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    return p->state.subscription_list.size();
}

pp_t pp_get_par(int index)
{
    if (index >= MAX_PUBLIC_PARAMETERS)
        return NULL;
    return (pp_t)&par_list[index];
}

bool pp_set_json_cb(pp_t pp, pp_json_cb_t json_cb)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    p->conf.json_cb = json_cb;
    return true;
}

bool pp_set_valueptr(pp_t pp, const void *valueptr)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    p->state.valueptr = valueptr;
    return true;
}

bool pp_get_as_string(pp_t pp, char *buf, size_t *bufsize, bool json)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    if (p->conf.json_cb == NULL)
        return false;
    return p->conf.json_cb(p, buf, bufsize, false);
}

int pp_get_info(int index, pp_info_t *info)
{
    while (index < MAX_PUBLIC_PARAMETERS)
    {
        public_parameter_t *p = &par_list[index];
        if (p->conf.name != NULL)
        {
            info->name = p->conf.name;
            info->type = p->conf.type;
            info->owner = p->conf.owner;
            info->subscriptions = p->state.subscription_list.size();
            info->valueptr = p->state.valueptr;
            return index++;
        }
        index++;
    }
    return -1;
}

void pp_enable(pp_t pp, bool enable)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    p->state.is_active = enable;
}

bool pp_is_enabled(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    return p->state.is_active;
}

bool pp_set_context(pp_t pp, void *context)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    p->state.context = context;
    return true;
}

void *pp_get_context(pp_t pp)
{
    public_parameter_t *p = (public_parameter_t *)pp;
    return p->state.context;
}

void pp_init_hooks(pp_hooks *h)
{
    if (h == NULL)
        return;
    hooks = *h;
}

void pp_free(void *ptr)
{
    if (ptr == NULL)
        return;
    hooks.free_fn(ptr);
}

size_t pp_get_parameter_count(void)
{
    return nameToPP.size();
}

bool pp_get_parameter_list_as_json(char **buf, parameter_type_t type)
{
    if (buf == NULL)
        return false;

    size_t parCount = pp_get_parameter_count();
    // Go through all parameters and find the length of all the names
    size_t totalNameLength = 0;
    for (auto it = nameToPP.begin(); it != nameToPP.end(); it++)
    {
        totalNameLength += strlen(it->first.c_str()) + 3; // 2 for quotes and 1 for comma
    }
    totalNameLength += 2; // 2 for brackets
    char *json = (char *)hooks.malloc_fn(totalNameLength);
    if (json == NULL)
    {
        ESP_LOGE(TAG, "%s: Failed to allocate memory for json", __func__);
        return false;
    }
    memset(json, 0, totalNameLength);
    size_t len = 0;
    json[len++] = '[';
    const char *comma = NULL;
    for (auto it = nameToPP.begin(); it != nameToPP.end(); it++)
    {
        public_parameter_t *p = it->second;
        if (!(p->conf.type & type))
            continue;

        if (comma != NULL)
            json[len++] = ',';
        comma = ",";
        size_t nameLen = strlen(it->first.c_str());
        json[len++] = '"';
        memcpy(&json[len], it->first.c_str(), nameLen);
        len += nameLen;
        json[len++] = '"';
    }
    json[len++] = ']';
    json[len] = 0;
    *buf = json;
    return true;
}
