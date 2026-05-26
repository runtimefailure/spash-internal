#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <Windows.h>
#include <lstate.h>
#include <lgc.h>

#include <shared.hpp>
#include <lapi.h>
#include <ltable.h>
#include <lgc.h>
#include <lmem.h>
#include <iosfwd>
#include <filesystem>
#include <thread>
#include <string>
#include <fstream>
#include <cryptopp/aes.h>
#include <cryptopp/rsa.h>
#include <cryptopp/gcm.h>
#include <cryptopp/eax.h>
#include <cryptopp/md2.h>
#include <cryptopp/md5.h>
#include <cryptopp/sha.h>
#include <cryptopp/sha3.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/pssr.h>
#include <cryptopp/base64.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/serpent.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/rdrand.h>
#include <optional>
#include <lualib.h>

namespace crypt {

    namespace HelpFunctions {

        template<typename T>
        static std::string hash_with_algo(const std::string& Input)
        {
            T Hash;
            std::string Digest;

            CryptoPP::StringSource SS(Input, true,
                new CryptoPP::HashFilter(Hash,
                    new CryptoPP::HexEncoder(
                        new CryptoPP::StringSink(Digest), false
                    )));

            return Digest;
        }
        std::string b64encode(const std::string& stringToEncode) {
            std::string base64EncodedString;
            CryptoPP::Base64Encoder encoder{ new CryptoPP::StringSink(base64EncodedString), false };
            encoder.Put((byte*)stringToEncode.c_str(), stringToEncode.length());
            encoder.MessageEnd();

            return base64EncodedString;
        }

        std::string b64decode(const std::string& stringToDecode) {
            std::string base64DecodedString;
            CryptoPP::Base64Decoder decoder{ new CryptoPP::StringSink(base64DecodedString) };
            decoder.Put((byte*)stringToDecode.c_str(), stringToDecode.length());
            decoder.MessageEnd();

            return base64DecodedString;
        }

        std::string RamdonString(int len) {
            static const char* chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
            std::string str;
            str.reserve(len);

            for (int i = 0; i < len; ++i) {
                str += chars[rand() % (strlen(chars) - 1)];
            }

            return str;
        }
    }

    enum HashModes
    {
        //MD5
        MD5,

        //SHA1
        SHA1,

        //SHA2
        SHA224,
        SHA256,
        SHA384,
        SHA512,

        //SHA3
        SHA3_224,
        SHA3_256,
        SHA3_384,
        SHA3_512,
    };

    std::map<std::string, HashModes> HashTranslationMap = {
        //MD5
        { "md5", MD5 },

        //SHA1
        { "sha1", SHA1 },

        //SHA2
        { "sha224", SHA224 },
        { "sha256", SHA256 },
        { "sha384", SHA384 },
        { "sha512", SHA512 },

        //SHA3
        { "sha3-224", SHA3_224 },
        { "sha3_224", SHA3_224 },
        { "sha3-256", SHA3_256 },
        { "sha3_256", SHA3_256 },
        { "sha3-384", SHA3_384 },
        { "sha3_384", SHA3_384 },
        { "sha3-512", SHA3_512 },
        { "sha3_512", SHA3_512 },
    };

    int base64encode(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        size_t stringLength;
        const char* rawStringToEncode = lua_tolstring(L, 1, &stringLength);
        const std::string stringToEncode(rawStringToEncode, stringLength);
        const std::string encodedString = HelpFunctions::b64encode(stringToEncode);

        lua_pushlstring(L, encodedString.c_str(), encodedString.size());
        return 1;
    }

    int base64decode(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        size_t stringLength;
        const char* rawStringToDecode = lua_tolstring(L, 1, &stringLength);
        const auto stringToDecode = std::string(rawStringToDecode, stringLength);
        const std::string decodedString = HelpFunctions::b64decode(stringToDecode);

        lua_pushlstring(L, decodedString.c_str(), decodedString.size());
        return 1;
    }

    int generatebytes(lua_State* L) {
        luaL_checktype(L, 1, LUA_TNUMBER);
        const auto bytesSize = lua_tointeger(L, 1);

        CryptoPP::RDRAND rng;
        const auto bytesBuffer = new byte[bytesSize];
        rng.GenerateBlock(bytesBuffer, bytesSize);

        std::string base64EncodedBytes;
        CryptoPP::Base64Encoder encoder{ new CryptoPP::StringSink(base64EncodedBytes), false };
        encoder.Put(bytesBuffer, bytesSize);
        encoder.MessageEnd();

        delete bytesBuffer;
        lua_pushlstring(L, base64EncodedBytes.c_str(), base64EncodedBytes.size());
        return 1;
    }

    int generatekey(lua_State* L) {
        const auto bytesBuffer = new byte[CryptoPP::AES::MAX_KEYLENGTH];

        CryptoPP::RDRAND rng;
        rng.GenerateBlock(bytesBuffer, CryptoPP::AES::MAX_KEYLENGTH);

        std::string base64EncodedBytes;
        CryptoPP::Base64Encoder encoder{ new CryptoPP::StringSink(base64EncodedBytes), false };
        encoder.Put(bytesBuffer, CryptoPP::AES::MAX_KEYLENGTH);
        encoder.MessageEnd();

        delete bytesBuffer;
        lua_pushlstring(L, base64EncodedBytes.c_str(), base64EncodedBytes.size());
        return 1;
    }

    int hash(lua_State* L) {
        std::string data = luaL_checklstring(L, 1, NULL);
        std::string algo = luaL_checklstring(L, 2, NULL);

        std::transform(algo.begin(), algo.end(), algo.begin(), tolower);

        if (!HashTranslationMap.count(algo))
        {
            luaL_argerror(L, 1, "non-existant hash algorithm");
            return 0;
        }

        const auto ralgo = HashTranslationMap[algo];

        std::string hash;

        if (ralgo == MD5) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::Weak::MD5>(data);
        }
        else if (ralgo == SHA1) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA1>(data);
        }
        else if (ralgo == SHA224) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA224>(data);
        }
        else if (ralgo == SHA256) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA256>(data);
        }
        else if (ralgo == SHA384) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA384>(data);
        }
        else if (ralgo == SHA512) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA512>(data);
        }
        else if (ralgo == SHA3_224) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA3_224>(data);
        }
        else if (ralgo == SHA3_256) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA3_256>(data);
        }
        else if (ralgo == SHA3_384) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA3_384>(data);
        }
        else if (ralgo == SHA3_512) {
            hash = HelpFunctions::hash_with_algo<CryptoPP::SHA3_512>(data);
        }
        else {
            luaL_argerror(L, 1, "non-existant hash algorithm");
            return 0;
        }

        lua_pushlstring(L, hash.c_str(), hash.size());

        return 1;
    }
    using ModePair = std::pair<std::unique_ptr<CryptoPP::CipherModeBase>, std::unique_ptr<CryptoPP::CipherModeBase> >;

    std::optional<ModePair> getEncryptionDecryptionMode(const std::string& modeName) {
        if (modeName == "cbc") {
            //("Mode:cbc");
            return ModePair{
                std::make_unique<CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption>(),
                std::make_unique<CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption>()
            };
        }
        else if (modeName == "cfb") {
            return ModePair{
                std::make_unique<CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption>(),
                std::make_unique<CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption>()
            };
        }
        else if (modeName == "ofb") {
            return ModePair{
                std::make_unique<CryptoPP::OFB_Mode<CryptoPP::AES>::Encryption>(),
                std::make_unique<CryptoPP::OFB_Mode<CryptoPP::AES>::Decryption>()
            };
        }
        else if (modeName == "ctr") {
            return ModePair{
                std::make_unique<CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption>(),
                std::make_unique<CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption>()
            };
        }
        else if (modeName == "ecb") {
            return ModePair{
                std::make_unique<CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption>(),
                std::make_unique<CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption>()
            };
        }
        else {
            return std::nullopt;
        }
    }

    int encrypt(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);

        const auto rawDataString = lua_tostring(L, 1);
        lua_pushstring(L, HelpFunctions::b64encode(rawDataString).c_str());
        lua_pushstring(L, "");

        return 2;
    }

    int decrypt(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TSTRING);
        luaL_checktype(L, 4, LUA_TSTRING);

        const auto rawDataString = lua_tostring(L, 1);
        lua_pushstring(L, HelpFunctions::b64decode(rawDataString).c_str());
        return 1;
    }
    void initialize(lua_State* L)
    {
        Function(L, "base64_encode", crypt::base64encode);
        Function(L, "base64_decode", crypt::base64decode);

        lua_newtable(L);
        TableFunction(L, "encode", crypt::base64encode);
        TableFunction(L, "decode", crypt::base64decode);
        lua_setfield(L, LUA_GLOBALSINDEX, ("base64"));

        lua_newtable(L);
        TableFunction(L, "base64encode", crypt::base64encode);
        TableFunction(L, "base64decode", crypt::base64decode);
        TableFunction(L, "base64_encode", crypt::base64encode);
        TableFunction(L, "base64_decode", crypt::base64decode);

        lua_newtable(L);
        TableFunction(L, "encode", crypt::base64encode);
        TableFunction(L, "decode", crypt::base64decode);
        lua_setfield(L, -2, "base64");

        TableFunction(L, "encrypt", crypt::encrypt);
        TableFunction(L, "decrypt", crypt::decrypt);
        TableFunction(L, "generatebytes", crypt::generatebytes);
        TableFunction(L, "generatekey", crypt::generatekey);
        TableFunction(L, "hash", crypt::hash);
        lua_setfield(L, LUA_GLOBALSINDEX, "crypt");
    }
};