#include Ecryption.h

#define SPLIT_SYMBOL "52168@E4B9!13Fe-33!B0D9CF6!$@!~"
FString UnrealUtils::Common::Encrypt(const FString& InputString, const FAES::FAESKey& Key)
{
	if (!ensure(!InputString.IsEmpty())) { return{}; }
	if (!ensure(Key.IsValid())) { return{}; }
	FString TempString = InputString;

	/** 插入垃圾符号. */
	const FString SplitSymbol = SPLIT_SYMBOL;
	TempString.Append(SplitSymbol);

	const auto BufferSize = TempString.Len();
	TArray<uint8> Buffer{};
	Buffer.AddUninitialized(BufferSize);
	StringToBytes(TempString, Buffer.GetData(), BufferSize);

  /** 调整，因为除非大小是 16 的倍数，否则无法使用. */
	const int32 OriginalSize = Buffer.Num();
	const int32 AlignedSize = Align(OriginalSize, FAES::AESBlockSize);
	Buffer.SetNumZeroed(AlignedSize);

	/** 加密. */
	FAES::EncryptData(Buffer.GetData(), Buffer.Num(), Key);

	const FString Result = BytesToString(Buffer.GetData(), AlignedSize);
	return Result;
}

FString UnrealUtils::Common::Decrypt(const FString& InputString, const FAES::FAESKey& Key)
{
	if (!ensure(!InputString.IsEmpty())) { return{}; }
	if (!ensure(Key.IsValid())) { return{}; }
	const auto BufferSize = InputString.Len();

	/** 大小不是 16 的倍数. */
	if (BufferSize % FAES::AESBlockSize != 0)
	{
		/** 由于大小无效，消息无法解密. */
		ensureMsgf(false, TEXT("Unable to decode message because message size is invalid."));
		return {};
	}
	TArray<uint8> Buffer{};
	Buffer.AddUninitialized(BufferSize);
	StringToBytes(InputString, Buffer.GetData(), BufferSize);

	/** 解密 */
	FAES::DecryptData(Buffer.GetData(), BufferSize, Key);

	const FString DecryptedString = BytesToString(Buffer.GetData(), BufferSize);

	/** 从垃圾符号中分离出所需的数据. */
	FString LeftData;
	FString RightData;
	const FString SplitSymbol = SPLIT_SYMBOL;
	DecryptedString.Split(SplitSymbol, &LeftData, &RightData, ESearchCase::CaseSensitive, ESearchDir::FromStart);

	return LeftData;
}

FString UnrealUtils::Common::EncryptBase64(const FString& InputString, const FAES::FAESKey& Key)
{
	if (!ensure(!InputString.IsEmpty())) { return{}; }
	if (!ensure(Key.IsValid())) { return{}; }
	FString TempString = InputString;

	/** 插入垃圾符号. */
	const FString SplitSymbol = SPLIT_SYMBOL;
	TempString.Append(SplitSymbol);

	const auto BufferSize = TempString.Len();
	TArray<uint8> Buffer{};
	Buffer.AddUninitialized(BufferSize);
	StringToBytes(TempString, Buffer.GetData(), BufferSize);

	/** 调整，因为除非大小是 16 的倍数，否则无法使用. */
	const int32 OriginalSize = Buffer.Num();
	const int32 AlignedSize = Align(OriginalSize, FAES::AESBlockSize);
	Buffer.SetNumZeroed(AlignedSize);

	/** 加密. */
	FAES::EncryptData(Buffer.GetData(), Buffer.Num(), Key);

	const FString Result = FBase64::Encode(Buffer.GetData(), AlignedSize);
	return Result;
}

FString UnrealUtils::Common::DecryptBase64(const FString& InputString, const FAES::FAESKey& Key)
{
	if (!ensure(!InputString.IsEmpty())) { return{}; }
	if (!ensure(Key.IsValid())) { return{}; }
	TArray<uint8> Buffer{};

	if (!ensure(FBase64::Decode(InputString, Buffer))) { return{}; }
	const auto BufferSize = Buffer.Num();
	/** 大小不是 16 的倍数. */
	if (BufferSize % FAES::AESBlockSize != 0)
	{
		/** 由于大小无效，消息无法解密. */
		ensureMsgf(false, TEXT("Unable to decode message because message size is invalid."));
		return {};
	}

	/** 解密 */
	FAES::DecryptData(Buffer.GetData(), BufferSize, Key);

	const FString DecryptedString = BytesToString(Buffer.GetData(), BufferSize);

	/** 从垃圾符号中分离出所需的数据. */
	FString LeftData;
	FString RightData;
	const FString SplitSymbol = SPLIT_SYMBOL;
	DecryptedString.Split(SplitSymbol, &LeftData, &RightData, ESearchCase::CaseSensitive, ESearchDir::FromStart);

	return LeftData;
}
#undef SPLIT_SYMBOL
