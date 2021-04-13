// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file azure_iot_version.h
 *
 * @brief Provides version information.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef AZURE_IOT_VERSION_H
#define AZURE_IOT_VERSION_H

/// The version in string format used for telemetry following the `semver.org` standard
/// (https://semver.org).
#define azureiotVERSION_STRING "0.0.1-alpha"

/// Major numeric identifier.
#define azureiotVERSION_MAJOR 0

/// Minor numeric identifier.
#define azureiotVERSION_MINOR 0

/// Patch numeric identifier.
#define azureiotVERSION_PATCH 1

/// Optional pre-release identifier. SDK is in a pre-release state when present.
#define azureiotVERSION_PRERELEASE "alpha"

#endif // AZURE_IOT_VERSION_H
