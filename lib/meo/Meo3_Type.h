#pragma once

#include <Arduino.h>
#include <functional>

using MeoLogFunction = std::function<void(const char* level, const char* message)>;
