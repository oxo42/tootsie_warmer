#ifndef TOOTSIE_H
#define TOOTSIE_H

#define DEBUG 0

#include "Arduino.h"

const String duration_to_timestamp(int duration);

class HAMqttDeviceRegistry
{
private:
    struct Attribute
    {
        String key;
        String value;
    };
    std::vector<Attribute> _attributes;

    std::vector<String> _identifiers;

public:
    HAMqttDeviceRegistry();
    ~HAMqttDeviceRegistry();

    HAMqttDeviceRegistry &addAttribute(const String &key, const String &value);
    HAMqttDeviceRegistry &addIdentifier(const String &identifier);
    HAMqttDeviceRegistry &clearAttributes();

    const String getPayload() const;
};

#endif