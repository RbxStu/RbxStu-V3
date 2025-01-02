//
// Created by Dottik on 2/11/2024.
//

#include "Crypt.hpp"

#include <cryptopp/base64.h>
#include <osrng.h>
#include "StuLuau/Extensions/luauext.hpp"

namespace RbxStu::StuLuau {
    struct YieldRequest;
}

namespace RbxStu::StuLuau::Environment::UNC {
    int Crypt::base64encode(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);

        std::size_t len{};
        const auto str = lua_tolstring(L, 1, &len);
        std::string encoded{};
        CryptoPP::Base64Encoder encoder{new CryptoPP::StringSink(encoded)};

        encoder.Put(reinterpret_cast<const CryptoPP::byte *>(str), len);
        encoder.MessageEnd();

        lua_preparepushcollectable(L, 1);
        lua_pushlstring(L, encoded.c_str(), encoded.length() - 1); // Encoded strings have no \0.
        return 1;
    }

    int Crypt::base64decode(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);

        std::size_t len{};
        const auto str = lua_tolstring(L, 1, &len);
        std::string decoded{};
        CryptoPP::Base64Decoder decoder{new CryptoPP::StringSink(decoded)};

        decoder.Put(reinterpret_cast<const CryptoPP::byte *>(str), len);

        decoder.MessageEnd();

        lua_preparepushcollectable(L, 1);
        lua_pushlstring(L, decoded.c_str(), decoded.length());
        return 1;
    }

    int Crypt::generatebytes(lua_State *L) {
        const auto bufSize = luaL_checkinteger(L, 1);
        lua_normalisestack(L, 1);

        CryptoPP::SecByteBlock block(bufSize);
        lua_pushlstring(L, reinterpret_cast<char *>(block.BytePtr()), block.size());

        lua_preparepushcollectable(L, 3);
        lua_pushcclosure(L, Crypt::base64encode, nullptr, 0);
        lua_pushvalue(L, 2);
        lua_call(L, 1, 1);
        return 1;
    }

    int Crypt::generatekey(lua_State *L) {
        lua_normalisestack(L, 0);
        lua_preparepushcollectable(L, 3);
        lua_pushcclosure(L, Crypt::generatebytes, nullptr, 0);
        lua_pushnumber(L, 32);
        lua_call(L, 1, 1);
        return 1;
    }


    const luaL_Reg *Crypt::GetFunctionRegistry() {
        const static luaL_Reg funcs[] = {{"base64encode", RbxStu::StuLuau::Environment::UNC::Crypt::base64encode},
                                         {"base64_encode", RbxStu::StuLuau::Environment::UNC::Crypt::base64encode},
                                         {"base64decode", RbxStu::StuLuau::Environment::UNC::Crypt::base64decode},
                                         {"base64_decode", RbxStu::StuLuau::Environment::UNC::Crypt::base64decode},
                                         {"generatebytes", RbxStu::StuLuau::Environment::UNC::Crypt::generatebytes},
                                         {"generatekey", RbxStu::StuLuau::Environment::UNC::Crypt::generatekey},
                                         {nullptr, nullptr}};

        return funcs;
    }
} // namespace RbxStu::StuLuau::Environment::UNC
