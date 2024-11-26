import os
import sys
import subprocess
import argparse
import shutil
from pathlib import Path
from os import walk

# EmscriptenSDKPath = "C:/APPS/emsdk"

EmscriptenSDKPath = os.environ.get('EMSCRIPTEN')
BUILD_TYPE = "Debug"

def build_windows():
    compilePath = "build/windows"
    cmake_cmd = [
        "cmake",
        "-B", compilePath,
    ]
    result = subprocess.run(cmake_cmd)
    if result.returncode != 0:
        return

    build_cmd = ["cmake", "--build", compilePath, "--config", BUILD_TYPE,]
    result = subprocess.run(build_cmd)
    if result.returncode != 0:
        return

    dstPath = "build/OUT/runtimes/win-x64/native/"
    os.makedirs(os.path.dirname(dstPath), exist_ok=True)
    shutil.copy2(f"{compilePath}/{BUILD_TYPE}/cmb.dll", dstPath)
    if BUILD_TYPE == "Debug":
        shutil.copy2(f"{compilePath}/{BUILD_TYPE}/cmb.pdb", dstPath)

def build_wasm():
    compilePath = "build/wasm"
    cmake_cmd = [
        "cmake",
        "-B", compilePath,
        "-GNinja",
        f"-DCMAKE_BUILD_TYPE={BUILD_TYPE}",
        f"-DCMAKE_TOOLCHAIN_FILE={EmscriptenSDKPath}/cmake/Modules/Platform/Emscripten.cmake",
        f"-DCMAKE_CROSSCOMPILING_EMULATOR={EmscriptenSDKPath}/node/16.20.0_64bit/bin/node.exe",
    ]
    result = subprocess.run(cmake_cmd)
    if result.returncode != 0:
        return

    build_cmd = ["cmake", "--build", compilePath, "--target", "cmb"]
    result = subprocess.run(build_cmd)
    if result.returncode != 0:
        return
    
    arScriptStr = "CREATE build/OUT/build/wasm-binaries/cmb.a\n"
    arScriptStr += f"ADDLIB {compilePath}/libcmb.a\n"
    arScriptStr += f"ADDLIB {compilePath}/shewchuk_predicates/libshewchuk_predicates.a\n"
    #arScriptStr += f"ADDLIB {compilePath}/clang_20.0_cxx20_32_release/libtbb.a\n"
    arScriptStr += "SAVE\nEND\n"
    
    arScriptFileName = "build/wasm/cmb.ar"
    with open(arScriptFileName, 'w') as f:
        f.write(arScriptStr)

    ar_cmd = [
        f"{EmscriptenSDKPath}/emar.bat",
        "-M",
        f"<{arScriptFileName}"
    ]
    result = subprocess.run(ar_cmd)
    if result.returncode != 0:
        return

    #########
    #dstPath = "build/OUT/build/wasm-binaries/cmb.a"
    #os.makedirs(os.path.dirname(dstPath), exist_ok=True)
    #shutil.copy2(f"{compilePath}/libcmb.a", dstPath)

parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", action="store_true")
parser.add_argument("--emscripten_sdk", help = "Path to the Emscripten SDK install dir")
args = parser.parse_args()

build_windows()

if True:
    if EmscriptenSDKPath is None:
        print("Warning: EMSCRIPTEN environment variable not set.\nPlease install Emscripten and set the EMSCRIPTEN environment variable to the path of installation.")
    else:
        print(EmscriptenSDKPath)
        build_wasm()