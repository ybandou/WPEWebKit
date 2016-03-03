/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CacheModel.h"

#include <algorithm>

#include <stdio.h>
#include <string.h>

const char * overrideEnvVar = "WPE_CACHE_OVERRIDE";

// Enables turning on/off of writing before and after values to file.
const bool writeValuesToFile = true;
const char * dumpFile = "/tmp/dump_wpe_cache.txt";

// Example line to set env var:
// export WPE_CACHE_OVERRIDE='cacheTotalCapacity=123:cacheMinDeadCapacity=234:cacheMaxDeadCapacity=345:pageCacheCapacity=456:urlCacheMemoryCapacity=567:urlCacheDiskCapacity=678'

static void WriteLineToFile(const char * text)
{
   char buffer[256];
   sprintf(buffer, "%s\n", text);

   FILE * outFile = fopen(dumpFile, "a");
   fwrite(buffer, 1, strlen(buffer), outFile);
   fclose(outFile);
}

static void WriteValueToFile(const char * name, unsigned long value)
{
   char buffer[256];
   sprintf(buffer, "%s %lu\n", name, value);

   FILE * outFile = fopen(dumpFile, "a");
   fwrite(buffer, 1, strlen(buffer), outFile);
   fclose(outFile);
}

static int GetValueFromEnvString(const char * envString, const char * name, int currentValue)
{
   const int envStringLength = strlen(envString);

   // See if value is listed in env string, if not, return current value.
   const char * nameStart = strstr(envString, name);
   if (nameStart == nullptr) {
      return currentValue;
   }

   // Set index after name.
   int index = nameStart - envString + strlen(name);

   // Check if we didn't reach end of string.
   if (index >= envStringLength) {
      return currentValue;
   }

   // Check if there is a '='.
   if (envString[index] != '=') {
      return currentValue;
   }

   // Move to first char after '='.
   ++index;

   // Store beginning of number.
   int numberBegin = index;

   // Find end of number.
   while (true) {
      // Check if we reached end of string.
      if (index >= envStringLength) {
         break;
      }

      // Check if we still see a digit.
      if ((envString[index] < '0') || (envString[index] > '9')) {
         break;
      }

      // Move to next char.
      ++index;
   }

   // if first char was already not a digit, bail out.
   if (numberBegin == index) {
      return currentValue;
   }

   const int bufferSize = 64;
   char buffer[bufferSize];        //unsigned& cacheTotalCapacity, unsigned& cacheMinDeadCapacity, unsigned& cacheMaxDeadCapacity, std::chrono::seconds& deadDecodedDataDeletionInterval,
   //unsigned& pageCacheCapacity, unsigned long& urlCacheMemoryCapacity, unsigned long& urlCacheDiskCapacity)
   //WriteValueToFile("deadDecodedDataDeletionInterval", deadDecodedDataDeletionInterval);



   int numberLength = index - numberBegin;

   // Check buffer size.
   if (numberLength >= bufferSize) {
      return currentValue;
   }

   // Copy part of env string containing new value.
   // TODO: could we use atoi straight on env string instead?
   memcpy(buffer, envString + numberBegin, numberLength);
   buffer[numberLength] = '\0';

   int newValue = atoi(buffer);

   // Check if something went wrong in atoi (or zero was defined).
   if (newValue == 0) {
      return currentValue;
   }

   return newValue;
}

namespace WebKit {

void calculateCacheSizes(CacheModel cacheModel, uint64_t memorySize, uint64_t diskFreeSize,
    unsigned& cacheTotalCapacity, unsigned& cacheMinDeadCapacity, unsigned& cacheMaxDeadCapacity, std::chrono::seconds& deadDecodedDataDeletionInterval,
    unsigned& pageCacheCapacity, unsigned long& urlCacheMemoryCapacity, unsigned long& urlCacheDiskCapacity)
{
    switch (cacheModel) {
    case CacheModelDocumentViewer: {
        // Page cache capacity (in pages)
        pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memorySize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memorySize >= 1536)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memorySize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memorySize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;

        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;

        // Foundation memory cache capacity (in bytes)
        urlCacheMemoryCapacity = 0;

        // Foundation disk cache capacity (in bytes)
        urlCacheDiskCapacity = 0;

        break;
    }
    case CacheModelDocumentBrowser: {
        // Page cache capacity (in pages)
        if (memorySize >= 1024)
            pageCacheCapacity = 3;
        else if (memorySize >= 512)
            pageCacheCapacity = 2;
        else if (memorySize >= 256)
            pageCacheCapacity = 1;
        else
            pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memorySize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memorySize >= 1536)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memorySize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memorySize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;

        cacheMinDeadCapacity = cacheTotalCapacity / 8;
        cacheMaxDeadCapacity = cacheTotalCapacity / 4;

        // Foundation memory cache capacity (in bytes)
        if (memorySize >= 2048)
            urlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memorySize >= 1024)
            urlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memorySize >= 512)
            urlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            urlCacheMemoryCapacity =      512 * 1024; 

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            urlCacheDiskCapacity = 50 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            urlCacheDiskCapacity = 40 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            urlCacheDiskCapacity = 30 * 1024 * 1024;
        else
            urlCacheDiskCapacity = 20 * 1024 * 1024;

        break;
    }
    case CacheModelPrimaryWebBrowser:
    case CacheModelEnvVarOverride: {
        // Even if we want to override some values from environment string,
        // still set them according to primary webbrowser, this way we can decide
        // to modify only a subset.

        // Page cache capacity (in pages)
        if (memorySize >= 1024)
            pageCacheCapacity = 3;
        else if (memorySize >= 512)
            pageCacheCapacity = 2;
        else if (memorySize >= 256)
            pageCacheCapacity = 1;
        else
            pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        // (Testing indicates that value / MB depends heavily on content and
        // browsing pattern. Even growth above 128MB can have substantial 
        // value / MB for some content / browsing patterns.)
        if (memorySize >= 2048)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memorySize >= 1536)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memorySize >= 1024)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memorySize >= 512)
            cacheTotalCapacity = 32 * 1024 * 1024;

        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;

        // This code is here to avoid a PLT regression. We can remove it if we
        // can prove that the overall system gain would justify the regression.
        cacheMaxDeadCapacity = std::max(24u, cacheMaxDeadCapacity);

        deadDecodedDataDeletionInterval = std::chrono::seconds { 60 };

#if PLATFORM(IOS)
        if (memorySize >= 1024)
            urlCacheMemoryCapacity = 16 * 1024 * 1024;
        else
            urlCacheMemoryCapacity = 8 * 1024 * 1024;
#else
        // Foundation memory cache capacity (in bytes)
        // (These values are small because WebCore does most caching itself.)
        if (memorySize >= 1024)
            urlCacheMemoryCapacity = 4 * 1024 * 1024;
        else if (memorySize >= 512)
            urlCacheMemoryCapacity = 2 * 1024 * 1024;
        else if (memorySize >= 256)
            urlCacheMemoryCapacity = 1 * 1024 * 1024;
        else
            urlCacheMemoryCapacity =      512 * 1024;
#endif

        // Foundation disk cache capacity (in bytes)
        if (diskFreeSize >= 16384)
            urlCacheDiskCapacity = 175 * 1024 * 1024;
        else if (diskFreeSize >= 8192)
            urlCacheDiskCapacity = 150 * 1024 * 1024;
        else if (diskFreeSize >= 4096)
            urlCacheDiskCapacity = 125 * 1024 * 1024;
        else if (diskFreeSize >= 2048)
            urlCacheDiskCapacity = 100 * 1024 * 1024;
        else if (diskFreeSize >= 1024)
            urlCacheDiskCapacity = 75 * 1024 * 1024;
        else
            urlCacheDiskCapacity = 50 * 1024 * 1024;

        break;
    }
    default:
        ASSERT_NOT_REACHED();
    };

    // Override values if they are set via environment string.
    if (cacheModel == CacheModelEnvVarOverride) {
        const char * envString = getenv(overrideEnvVar);

        if (envString != NULL) {
            if (writeValuesToFile) {
                WriteLineToFile("before modification");
                WriteValueToFile("cacheTotalCapacity", cacheTotalCapacity);
                WriteValueToFile("cacheMinDeadCapacity", cacheMinDeadCapacity);
                WriteValueToFile("cacheMaxDeadCapacity", cacheMaxDeadCapacity);
                WriteValueToFile("pageCacheCapacity", pageCacheCapacity);
                WriteValueToFile("urlCacheMemoryCapacity", urlCacheMemoryCapacity);
                WriteValueToFile("urlCacheDiskCapacity", urlCacheDiskCapacity);
            }

            cacheTotalCapacity = GetValueFromEnvString(envString, "cacheTotalCapacity", cacheTotalCapacity);
            cacheMinDeadCapacity = GetValueFromEnvString(envString, "cacheMinDeadCapacity", cacheMinDeadCapacity);
            cacheMaxDeadCapacity = GetValueFromEnvString(envString, "cacheMaxDeadCapacity", cacheMaxDeadCapacity);
            pageCacheCapacity = GetValueFromEnvString(envString, "pageCacheCapacity", pageCacheCapacity);
            urlCacheMemoryCapacity = GetValueFromEnvString(envString, "urlCacheMemoryCapacity", urlCacheMemoryCapacity);
            urlCacheDiskCapacity = GetValueFromEnvString(envString, "urlCacheDiskCapacity", urlCacheDiskCapacity);

            if (writeValuesToFile) {
                WriteLineToFile("after modification");
                WriteValueToFile("cacheTotalCapacity", cacheTotalCapacity);
                WriteValueToFile("cacheMinDeadCapacity", cacheMinDeadCapacity);
                WriteValueToFile("cacheMaxDeadCapacity", cacheMaxDeadCapacity);
                WriteValueToFile("pageCacheCapacity", pageCacheCapacity);
                WriteValueToFile("urlCacheMemoryCapacity", urlCacheMemoryCapacity);
                WriteValueToFile("urlCacheDiskCapacity", urlCacheDiskCapacity);
            }
        }
    }
}

} // namespace WebKit
