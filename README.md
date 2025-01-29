# Public Parameter Library (PP)

This library provides a flexible and efficient way to manage and interact with parameters in an embedded system, particularly on ESP32 devices. It supports various parameter types, including integers, floats, booleans, arrays, strings, and binary data. The library also integrates with the ESP-IDF event loop system to enable event-driven updates and subscriptions.

## Features

- **Multiple Parameter Types**: Supports int32, int64, float, bool, float arrays, int16 arrays, strings, and binary data.
- **Event-Driven Updates**: Parameters can be updated and monitored using the ESP-IDF event loop.
- **Subscription Model**: Allows multiple subscribers to receive updates when a parameter changes.
- **JSON Support**: Parameters can be converted to JSON strings for easy serialization.
- **Memory Management**: Provides functions to allocate and reset arrays.

## Usage

### Initialization

1. Ensure you have the ESP-IDF environment set up.
2. Copy `pp.h` and `pp.cpp` into your project and add to your makefile.
3. To use the library, include the `pp.h` header file in your project:
```c
#include "pp.h"
```
4. Compile using ESP-IDF.

### Creating Parameters
You can create parameters of different types using the provided functions:
```c
pp_t my_int32_param = pp_create_int32("my_int32", &my_evloop, my_write_cb, &my_int32_value);
pp_t my_float_param = pp_create_float("my_float", &my_evloop, my_write_cb, &my_float_value);
pp_t my_object = pp_create_binary("my_object", &my_evloop, my_write_cb);
...
```
### Subscribing to Parameters
To receive updates when a parameter changes, subscribe to it:
```c
pp_subscribe(my_int32_param, &my_evloop, my_event_cb);
```
### Posting Updates
You can update the value of a parameter and notify all subscribers:
```c
pp_post_newstate_int32(my_int32_param, 42);
pp_post_newstate_float(my_float, 98.6f);
pp_post_newstate_binary(my_binary, &my_structure, sizeof(my_structure));
...
```
### Serializing to JSON
To get a parameter as a JSON string:
```c
char buf[128];
size_t bufsize = sizeof(buf);
pp_get_as_string(my_int32_param, buf, &bufsize, true);
```
### Deleting Parameters
When a parameter is no longer needed, you can delete it:
```c
pp_delete(my_int32_param);
```
### Examples
#### Example 1: Creating and Updating a Parameter
```c
#include "pp.h"

void my_write_cb(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    // Handle write event
}

void my_event_cb(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    // Handle parameter update event
}

void app_main() {
    pp_evloop_t my_evloop = { .loop_handle = NULL, .base = "my_evloop" };
    int32_t my_int32_value = 0;
    pp_t my_int32_param = pp_create_int32("my_int32", &my_evloop, my_write_cb, &my_int32_value);
    pp_subscribe(my_int32_param, &my_evloop, my_event_cb);

    // Update the parameter value
    pp_post_newstate_int32(my_int32_param, 42);
}
```
#### Example 2: Converting a Parameter to JSON
```c
#include "pp.h"

void app_main() {
    pp_evloop_t my_evloop = { .loop_handle = NULL, .base = "my_evloop" };
    float my_float_value = 3.14;
    pp_t my_float_param = pp_create_float("my_float", &my_evloop, NULL, &my_float_value);

    char buf[128];
    size_t bufsize = sizeof(buf);
    pp_get_as_string(my_float_param, buf, &bufsize, true);

    printf("JSON: %s\n", buf);  // Output: {"my_float":3.140000}
}
```
#### Example 3: Register a callback to convert a parameter/object to json
This function will be called whenever the parameter needs to be converted into JSON.
```c
#include <stdio.h>
#include <string.h>
#include "pp.h"

struct my_struct {
    int the_int;
    float the_float;
};

bool my_json_callback(pp_t pp, char *buf, size_t *bufsize, bool json) {
    // Get the parameter name and value
    const char *param_name = pp_get_name(pp);

    // Format the JSON string
    if (json)
        *bufsize = snprintf(buf, *bufsize, "{ \"%s\": {\"the_int\": %d, \"the_float\": %f}}", param_name, my_struct.the_int, my_struct.the_float);
    else
        *bufsize = snprintf(buf, *bufsize, "{ %s: the_int: %d, the_float: %f", param_name, my_struct.the_int, my_struct.the_float);

    return true;
}

void setup_parameter_with_json_callback() {
    // Create a parameter
    pp_evloop_t my_evloop = { .loop_handle = NULL, .base = "my_event_base" };
    pp_t my_param = pp_create_binary("my_struct_pp", &my_evloop, NULL);

    // Set the JSON callback
    if (pp_set_json_cb(my_param, my_json_callback)) {
        printf("Custom JSON callback registered successfully.\n");
    } else {
        printf("Failed to register JSON callback.\n");
    }
}

```

#### Example 4: Usage of pp_event_handler_register_subscribe_cb
We want to register a callback function that gets called every time a new subscriber is added to a parameter in a specific event loop.
First define the Event Handler Callback. The callback function will be triggered when a new subscriber is added:
```c
void my_subscribe_callback(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    pp_t pp = (pp_t)event_data;
    const char* param_name = pp_get_name(pp);
    printf("New subscription detected for parameter: %s\n", param_name);
}
```
Register the Subscription Callback:
```c
void setup_subscribe_callback() {
    if (pp_event_handler_register_subscribe_cb(&my_evloop, my_subscribe_callback, NULL)) {
        printf("Subscription callback registered successfully.\n");
    } else {
        printf("Failed to register subscription callback.\n");
    }
}
```

### API Reference
For a detailed description of all functions and types, refer to the header file documentation.

### License
This library is licensed under the MIT License. See the LICENSE file for more details.

This `README.md` file provides an overview of the library, its features, and how to use it. It also includes examples to help users get started quickly. The API reference section directs users to the header file documentation for more detailed information.
