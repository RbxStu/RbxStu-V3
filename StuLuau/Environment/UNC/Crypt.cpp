//
// Created by Dottik on 2/11/2024.
//

#include "Crypt.hpp"
#include <Windows.h>

#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include <cryptopp/base64.h>

#include <Scheduling/TaskScheduler.hpp>
#include <StuLuau/ExecutionEngine.hpp>
#include <lobject.h>
#include <lstate.h>
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
        CryptoPP::StringSource src(std::string(str, len), true,
                                   new CryptoPP::Base64Encoder(new CryptoPP::StringSink(encoded)));

        lua_pushlstring(L, encoded.data(), encoded.length());
        return 1;
    }

    int Crypt::base64decode(lua_State *L) {
        luaL_checkstring(L, 1);
        lua_normalisestack(L, 1);

        std::size_t len{};
        const auto str = lua_tolstring(L, 1, &len);

        std::string decoded{};
        CryptoPP::StringSource src(std::string(str, len), true,
                                   new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded)));

        lua_pushlstring(L, decoded.data(), decoded.length());
        return 1;
    }

    int Crypt::generatebytes(lua_State *L) {
        const auto bufSize = luaL_checkinteger(L, 1);
        lua_normalisestack(L, 1);

        CryptoPP::SecByteBlock block(bufSize);
        CryptoPP::OS_GenerateRandomBlock(true, block.data(), bufSize);
        lua_pushlstring(L, reinterpret_cast<char *>(block.BytePtr()), block.size());

        lua_pushcclosure(L, Crypt::base64encode, nullptr, 0);
        lua_pushvalue(L, 2);
        lua_call(L, 1, 1);
        return 1;
    }

    const luaL_Reg *Crypt::GetFunctionRegistry() {
        const static luaL_Reg funcs[] = {{"base64encode", RbxStu::StuLuau::Environment::UNC::Crypt::base64encode},
                                         {"base64_encode", RbxStu::StuLuau::Environment::UNC::Crypt::base64encode},
                                         {"base64decode", RbxStu::StuLuau::Environment::UNC::Crypt::base64decode},
                                         {"base64_decode", RbxStu::StuLuau::Environment::UNC::Crypt::base64decode},
                                         {"generatebytes", RbxStu::StuLuau::Environment::UNC::Crypt::generatebytes},
                                         {nullptr, nullptr}};

        return funcs;
    }
} // namespace RbxStu::StuLuau::Environment::UNC
