/*
 * SignalBoard v1.0 - Version Information
 * Build time metadata and version tracking
 */

#pragma once

#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 0
#define FW_VERSION_PATCH 0
#define FW_BUILD_DATE __DATE__
#define FW_BUILD_TIME __TIME__

#define FW_VERSION_STRING "1.0.0"
#define FW_PRODUCT_NAME "SignalBoard C6"
#define FW_MANUFACTURER "Michele Bigi"

// Git commit hash (to be filled by CI/CD)
#define FW_GIT_COMMIT "development"

// Hardware target
#define HW_TARGET "ESP32-C6"
#define HW_VARIANT "DevKit-M1"
