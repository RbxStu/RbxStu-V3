//
// Created by Dottik on 11/10/2024.
//
#include <cstdlib>
#include <cstdio>

#include <Windows.h>

#include "ExceptionHandler.hpp"

void Entry() {
    RbxStu::ExceptionHandler::InstallHandler();

    printf(R"(
 *******   **               ********   **           **      **  ****
/**////** /**              **//////   /**          /**     /** */// *
/**   /** /**      **   **/**        ****** **   **/**     /**/    /*
/*******  /****** //** ** /*********///**/ /**  /**//**    **    ***
/**///**  /**///** //***  ////////**  /**  /**  /** //**  **    /// *
/**  //** /**  /**  **/**        /**  /**  /**  /**  //****    *   /*
/**   //**/******  ** //** ********   //** //******   //**    / ****
//     // /////   //   // ////////     //   //////     //      ////
)");
}

BOOL WINAPI DllMain(const HINSTANCE hModule, const DWORD fdwReason, const LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            CreateThread(nullptr, 0x1000, reinterpret_cast<LPTHREAD_START_ROUTINE>(Entry), nullptr, 0, nullptr);
            break;

        case DLL_PROCESS_DETACH:
            if (lpvReserved != nullptr) {
                break; // do not do cleanup if process termination scenario
            }

            FreeLibrary(GetModuleHandleA("Luau.VM.dll"));
            break;
        default:
            break;
    }
    return true;
}
