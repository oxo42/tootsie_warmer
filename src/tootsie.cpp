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
