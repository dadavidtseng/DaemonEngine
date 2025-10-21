//----------------------------------------------------------------------------------------------------
// KADIAuthenticationUtility.hpp
// Ed25519 cryptographic authentication utilities for KADI broker
//----------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------
// Ed25519 Key Pair Structure
//----------------------------------------------------------------------------------------------------
struct sEd25519KeyPair
{
	std::vector<unsigned char> publicKey;   // DER/SPKI format (~44 bytes for Ed25519)
	std::vector<unsigned char> privateKey;  // Raw format (32 bytes for Ed25519)

	// Encode keys to base64 for JSON transmission
	std::string GetPublicKeyBase64() const;
	std::string GetPrivateKeyBase64() const;

	// Decode keys from base64
	static sEd25519KeyPair FromBase64(std::string const& publicKeyBase64,
	                                  std::string const& privateKeyBase64);
};

//----------------------------------------------------------------------------------------------------
// KADI Authentication Utility
// Provides Ed25519 key generation and nonce signing for KADI authentication
//----------------------------------------------------------------------------------------------------
class KADIAuthenticationUtility
{
public:
	//----------------------------------------------------------------------------------------------------
	// Key Generation
	//----------------------------------------------------------------------------------------------------

	/// @brief Generate new Ed25519 key pair
	/// @param outKeyPair Output key pair structure
	/// @return true if generation succeeded, false otherwise
	static bool GenerateKeyPair(sEd25519KeyPair& outKeyPair);

	//----------------------------------------------------------------------------------------------------
	// Signing
	//----------------------------------------------------------------------------------------------------

	/// @brief Sign nonce challenge with private key
	/// @param nonce Challenge string from broker
	/// @param privateKey Ed25519 private key (64 bytes)
	/// @param outSignature Output signature buffer
	/// @return true if signing succeeded, false otherwise
	static bool SignNonce(std::string const& nonce,
	                      std::vector<unsigned char> const& privateKey,
	                      std::vector<unsigned char>& outSignature);

	/// @brief Verify signature (for testing purposes)
	/// @param nonce Original nonce challenge
	/// @param signature Signature to verify
	/// @param publicKey Ed25519 public key (32 bytes)
	/// @return true if signature is valid, false otherwise
	static bool VerifySignature(std::string const& nonce,
	                            std::vector<unsigned char> const& signature,
	                            std::vector<unsigned char> const& publicKey);

	//----------------------------------------------------------------------------------------------------
	// Encoding Utilities
	//----------------------------------------------------------------------------------------------------

	/// @brief Encode binary data to base64 string
	/// @param data Binary data to encode
	/// @return Base64-encoded string
	static std::string Base64Encode(std::vector<unsigned char> const& data);

	/// @brief Decode base64 string to binary data
	/// @param base64 Base64-encoded string
	/// @return Decoded binary data
	static std::vector<unsigned char> Base64Decode(std::string const& base64);

	/// @brief Encode binary data to hexadecimal string
	/// @param data Binary data to encode
	/// @return Hex-encoded string
	static std::string HexEncode(std::vector<unsigned char> const& data);

	/// @brief Decode hexadecimal string to binary data
	/// @param hex Hex-encoded string
	/// @return Decoded binary data
	static std::vector<unsigned char> HexDecode(std::string const& hex);
};
