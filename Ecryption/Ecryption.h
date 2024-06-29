// Ecryption.h

#pragma once

#include "CoreMinimal.h"
#include "Misc/AES.h"
#include "Misc/Base64.h"

namespace UnrealUtils
{
    namespace Common
    {
        FString Encrypt(const FString& InputString, const FAES::FAESKey& Key);
        FString Decrypt(const FString& InputString, const FAES::FAESKey& Key);
        FString EncryptBase64(const FString& InputString, const FAES::FAESKey& Key);
        FString DecryptBase64(const FString& InputString, const FAES::FAESKey& Key);
    }
}
