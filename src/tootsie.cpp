#include "tootsie.h"

#include "Arduino.h"

const String duration_to_timestamp(int duration)
{
    unsigned short hours = duration / 3600;
    duration -= hours * 3600;
    unsigned short minutes = duration / 60;
    duration -= minutes * 60;
    unsigned short seconds = duration % 60;

    char result[12];
    sprintf(result, "%02hu:%02hu:%02hu", hours, minutes, seconds);
    return String(result);
}

HAMqttDeviceRegistry::HAMqttDeviceRegistry() {}
HAMqttDeviceRegistry::~HAMqttDeviceRegistry() {}

HAMqttDeviceRegistry &HAMqttDeviceRegistry::addAttribute(const String &name, const String &value)
{
    _attributes.push_back({name, value});
    return *this;
}

HAMqttDeviceRegistry &HAMqttDeviceRegistry::addIdentifier(const String &identifier)
{
    _identifiers.push_back(identifier);
    return *this;
}

HAMqttDeviceRegistry &HAMqttDeviceRegistry::clearAttributes()
{
    _attributes.clear();
    _identifiers.clear();
    return *this;
}

const String HAMqttDeviceRegistry::getPayload() const
{
    String attrPayload = "{";

    for (uint8_t i = 0; i < _attributes.size(); i++)
    {
        attrPayload.concat('"');
        attrPayload.concat(_attributes[i].key);
        attrPayload.concat("\":\"");
        attrPayload.concat(_attributes[i].value);
        attrPayload.concat("\",");
    }

    attrPayload.concat("\"ids\":[");
    for (uint8_t i = 0; i < _identifiers.size(); i++)
    {
        attrPayload.concat('"');
        attrPayload.concat(_identifiers[i]);
        attrPayload.concat("\",");
    }
    attrPayload.setCharAt(attrPayload.length() - 1, ']');

    attrPayload.concat("}");

    return attrPayload;
}