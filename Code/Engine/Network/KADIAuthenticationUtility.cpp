//----------------------------------------------------------------------------------------------------
// KADIAuthenticationUtility.cpp
// Ed25519 cryptographic authentication implementation using OpenSSL
//----------------------------------------------------------------------------------------------------

#include "Engine/Network/KADIAuthenticationUtility.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

// OpenSSL includes - using ThirdParty prebuilt OpenSSL 3.6.0
#include <ThirdParty/openssl/include/openssl/evp.h>
#include <ThirdParty/openssl/include/openssl/rand.h>
#include <ThirdParty/openssl/include/openssl/bio.h>
#include <ThirdParty/openssl/include/openssl/buffer.h>
#include <ThirdParty/openssl/include/openssl/x509.h>  // For i2d_PUBKEY() and d2i_PUBKEY()

#include <memory>
#include <cstring>

//----------------------------------------------------------------------------------------------------
// RAII Wrapper for OpenSSL Resources
//----------------------------------------------------------------------------------------------------

struct OpenSSLDeleter
{
	void operator()(EVP_PKEY* ptr) const { if (ptr) EVP_PKEY_free(ptr); }
	void operator()(EVP_PKEY_CTX* ptr) const { if (ptr) EVP_PKEY_CTX_free(ptr); }
	void operator()(EVP_MD_CTX* ptr) const { if (ptr) EVP_MD_CTX_free(ptr); }
	void operator()(BIO* ptr) const { if (ptr) BIO_free_all(ptr); }
};

template<typename T>
using OpenSSLUniquePtr = std::unique_ptr<T, OpenSSLDeleter>;

//----------------------------------------------------------------------------------------------------
// Key Pair Structure Implementation
//----------------------------------------------------------------------------------------------------

std::string sEd25519KeyPair::GetPublicKeyBase64() const
{
	return KADIAuthenticationUtility::Base64Encode(publicKey);
}

std::string sEd25519KeyPair::GetPrivateKeyBase64() const
{
	return KADIAuthenticationUtility::Base64Encode(privateKey);
}

sEd25519KeyPair sEd25519KeyPair::FromBase64(std::string const& publicKeyBase64,
                                             std::string const& privateKeyBase64)
{
	sEd25519KeyPair keyPair;
	keyPair.publicKey = KADIAuthenticationUtility::Base64Decode(publicKeyBase64);
	keyPair.privateKey = KADIAuthenticationUtility::Base64Decode(privateKeyBase64);
	return keyPair;
}

//----------------------------------------------------------------------------------------------------
// Key Generation
//----------------------------------------------------------------------------------------------------

bool KADIAuthenticationUtility::GenerateKeyPair(sEd25519KeyPair& outKeyPair)
{
	// Create key generation context
	OpenSSLUniquePtr<EVP_PKEY_CTX> ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr));
	if (!ctx)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to create EVP_PKEY_CTX\n");
		return false;
	}

	// Initialize key generation
	if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to initialize key generation\n");
		return false;
	}

	// Generate key pair
	EVP_PKEY* rawPkey = nullptr;
	if (EVP_PKEY_keygen(ctx.get(), &rawPkey) <= 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to generate key pair\n");
		return false;
	}

	OpenSSLUniquePtr<EVP_PKEY> pkey(rawPkey);

	// Export public key in DER/SPKI format (SubjectPublicKeyInfo)
	// This format is required by KADI broker for signature verification
	unsigned char* derBuffer = nullptr;
	int derLen = i2d_PUBKEY(pkey.get(), &derBuffer);
	if (derLen <= 0 || !derBuffer)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to export public key to DER\n");
		return false;
	}

	// Copy DER-encoded public key to output (will be ~44 bytes for Ed25519)
	outKeyPair.publicKey.assign(derBuffer, derBuffer + derLen);
	OPENSSL_free(derBuffer);  // Free OpenSSL-allocated memory

	size_t publicKeyLen = outKeyPair.publicKey.size();

	// Extract private key (64 bytes for Ed25519 - includes public key)
	outKeyPair.privateKey.resize(32);  // Ed25519 private key is 32 bytes
	size_t privateKeyLen = outKeyPair.privateKey.size();
	if (EVP_PKEY_get_raw_private_key(pkey.get(), outKeyPair.privateKey.data(), &privateKeyLen) <= 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to extract private key\n");
		return false;
	}

	DebuggerPrintf("KADIAuthenticationUtility: Generated Ed25519 key pair (pub=%zu bytes, priv=%zu bytes)\n",
	               publicKeyLen, privateKeyLen);

	return true;
}

//----------------------------------------------------------------------------------------------------
// Signing
//----------------------------------------------------------------------------------------------------

bool KADIAuthenticationUtility::SignNonce(std::string const& nonce,
                                           std::vector<unsigned char> const& privateKey,
                                           std::vector<unsigned char>& outSignature)
{
	if (privateKey.size() != 32)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Invalid private key size: %zu (expected 32)\n", privateKey.size());
		return false;
	}

	// Create EVP_PKEY from raw private key
	EVP_PKEY* rawPkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr,
	                                                  privateKey.data(), privateKey.size());
	if (!rawPkey)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to create EVP_PKEY from private key\n");
		return false;
	}

	OpenSSLUniquePtr<EVP_PKEY> pkey(rawPkey);

	// Create signing context
	OpenSSLUniquePtr<EVP_MD_CTX> mdctx(EVP_MD_CTX_new());
	if (!mdctx)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to create EVP_MD_CTX\n");
		return false;
	}

	// Initialize signing (Ed25519 doesn't use hash algorithm, pass nullptr)
	if (EVP_DigestSignInit(mdctx.get(), nullptr, nullptr, nullptr, pkey.get()) <= 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to initialize signing\n");
		return false;
	}

	// Determine signature length (Ed25519 signatures are always 64 bytes)
	size_t signatureLen = 0;
	if (EVP_DigestSign(mdctx.get(), nullptr, &signatureLen,
	                   reinterpret_cast<unsigned char const*>(nonce.data()), nonce.size()) <= 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to determine signature length\n");
		return false;
	}

	// Sign the nonce
	outSignature.resize(signatureLen);
	if (EVP_DigestSign(mdctx.get(), outSignature.data(), &signatureLen,
	                   reinterpret_cast<unsigned char const*>(nonce.data()), nonce.size()) <= 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to sign nonce\n");
		return false;
	}

	outSignature.resize(signatureLen);

	DebuggerPrintf("KADIAuthenticationUtility: Signed nonce (%zu bytes) -> signature (%zu bytes)\n",
	               nonce.size(), signatureLen);

	return true;
}

//----------------------------------------------------------------------------------------------------
// Verification
//----------------------------------------------------------------------------------------------------

bool KADIAuthenticationUtility::VerifySignature(std::string const& nonce,
                                                 std::vector<unsigned char> const& signature,
                                                 std::vector<unsigned char> const& publicKey)
{
	// Public key is now in DER/SPKI format (not raw 32 bytes)
	// Expected size is ~44 bytes for Ed25519 in DER format
	if (publicKey.empty())
	{
		DebuggerPrintf("KADIAuthenticationUtility: Empty public key\n");
		return false;
	}

	// Create EVP_PKEY from DER-encoded public key
	unsigned char const* derPtr = publicKey.data();  // OpenSSL modifies this pointer
	EVP_PKEY* rawPkey = d2i_PUBKEY(nullptr, &derPtr, static_cast<long>(publicKey.size()));
	if (!rawPkey)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to decode DER public key\n");
		return false;
	}

	OpenSSLUniquePtr<EVP_PKEY> pkey(rawPkey);

	// Create verification context
	OpenSSLUniquePtr<EVP_MD_CTX> mdctx(EVP_MD_CTX_new());
	if (!mdctx)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to create EVP_MD_CTX\n");
		return false;
	}

	// Initialize verification
	if (EVP_DigestVerifyInit(mdctx.get(), nullptr, nullptr, nullptr, pkey.get()) <= 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Failed to initialize verification\n");
		return false;
	}

	// Verify signature
	int result = EVP_DigestVerify(mdctx.get(), signature.data(), signature.size(),
	                              reinterpret_cast<unsigned char const*>(nonce.data()), nonce.size());

	if (result == 1)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Signature verification succeeded\n");
		return true;
	}
	else
	{
		DebuggerPrintf("KADIAuthenticationUtility: Signature verification failed\n");
		return false;
	}
}

//----------------------------------------------------------------------------------------------------
// Encoding Utilities
//----------------------------------------------------------------------------------------------------

std::string KADIAuthenticationUtility::Base64Encode(std::vector<unsigned char> const& data)
{
	if (data.empty())
	{
		return "";
	}

	// Create BIO chain: memory BIO -> base64 BIO
	OpenSSLUniquePtr<BIO> b64(BIO_new(BIO_f_base64()));
	BIO* bmem = BIO_new(BIO_s_mem());
	BIO_push(b64.get(), bmem);

	// Disable newlines in base64 encoding
	BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);

	// Write data
	BIO_write(b64.get(), data.data(), static_cast<int>(data.size()));
	BIO_flush(b64.get());

	// Extract encoded data
	BUF_MEM* bptr = nullptr;
	BIO_get_mem_ptr(b64.get(), &bptr);

	std::string result(bptr->data, bptr->length);
	return result;
}

std::vector<unsigned char> KADIAuthenticationUtility::Base64Decode(std::string const& base64)
{
	if (base64.empty())
	{
		return {};
	}

	// Create BIO chain: memory BIO -> base64 BIO
	OpenSSLUniquePtr<BIO> b64(BIO_new(BIO_f_base64()));
	BIO* bmem = BIO_new_mem_buf(base64.data(), static_cast<int>(base64.size()));
	BIO_push(b64.get(), bmem);

	// Disable newlines in base64 decoding
	BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);

	// Decode
	std::vector<unsigned char> result(base64.size());
	int decodedLen = BIO_read(b64.get(), result.data(), static_cast<int>(result.size()));

	if (decodedLen < 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Base64 decode failed\n");
		return {};
	}

	result.resize(static_cast<size_t>(decodedLen));
	return result;
}

std::string KADIAuthenticationUtility::HexEncode(std::vector<unsigned char> const& data)
{
	static char const hexChars[] = "0123456789abcdef";
	std::string result;
	result.reserve(data.size() * 2);

	for (unsigned char byte : data)
	{
		result.push_back(hexChars[byte >> 4]);
		result.push_back(hexChars[byte & 0x0F]);
	}

	return result;
}

std::vector<unsigned char> KADIAuthenticationUtility::HexDecode(std::string const& hex)
{
	if (hex.size() % 2 != 0)
	{
		DebuggerPrintf("KADIAuthenticationUtility: Invalid hex string length (must be even)\n");
		return {};
	}

	std::vector<unsigned char> result;
	result.reserve(hex.size() / 2);

	for (size_t i = 0; i < hex.size(); i += 2)
	{
		char highNibble = hex[i];
		char lowNibble = hex[i + 1];

		auto hexCharToValue = [](char c) -> unsigned char {
			if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
			if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
			if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
			return 0;
		};

		unsigned char byte = (hexCharToValue(highNibble) << 4) | hexCharToValue(lowNibble);
		result.push_back(byte);
	}

	return result;
}
