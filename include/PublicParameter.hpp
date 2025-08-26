#pragma once

#include <map>
#include <list>
#include <string>
#include <stdint.h>
#include <stddef.h>
#include "RtosMsgBuffer.hpp"

//-----------------------------------------------------------------------------
class ParameterBase
{
public:
    virtual ~ParameterBase() = default;
    const char *getName() const { return name; };
    virtual bool notify() = 0;
    void addSubscriber(RtosMsgBuffer &subscriber)
    {
        subscribers.push_back(subscriber);
    }
    void removeSubscriber(RtosMsgBuffer &subscriber)
    {
        subscribers.remove_if([&subscriber](const RtosMsgBuffer &s) { return &s == &subscriber; });
    }
protected:
    std::list<RtosMsgBuffer> subscribers;
private:
    const char *name;
};

//-----------------------------------------------------------------------------
template <typename T>
class Parameter : public ParameterBase
{
public:
    Parameter(const char *name, const T *data) : data(data) {}
    ~Parameter() = default;

    const T *getData() const { return data; }
    bool notify() override
    {
        for (auto &subscriber : subscribers)
        {
            subscriber.send(data, sizeof(T));
        }
        return true;
    }

private:
    const T *data;
};

//-----------------------------------------------------------------------------
class PublicParameter
{
public:
    template<typename T, typename C>
    static ParameterBase *add(const char *name, const C id, const T *data)
    {
        if (params.find(name) != params.end())
        {
            return nullptr; // Parameter already registered
        }
        params[name] = static_cast<ParameterBase *>(new Parameter<T>(name, data));
        return (params[name]);
    }
    static bool remove(const char *name)
    {
        auto it = params.find(name);
        if (it != params.end())
        {
            delete it->second;
            params.erase(it);
            return true;
        }
        return false;
    }
    template<typename C>
    static bool subscribe(const char *name, RtosMsgBuffer &receiver, const C msgId)
    {
        auto it = params.find(name);
        if (it == params.end())
        {
            return false;
        }

        return true;
    }

private:
    static std::map<std::string, ParameterBase *> params;
};