#!/bin/bash

name="mini"
package_name="net.snipsel.$name"
android_version=33
build_tools_version=33.0.2
ndk_version=25.2.9519653
vulkan_version='1.3.296.0'

cc_flags="-std=c++2b -fno-rtti -fno-exceptions -fvisibility=hidden \
		 -Wall -Wextra -Wno-conversion -Wshadow -Wunused -Wno-missing-field-initializers \
		 -MJ main_compile_command.json \
		 -nostdlib++ -fPIC -shared -uANativeActivity_onCreate -landroid"
cc_debug_flags="-O0 -llog"
cc_release_flags="-O3 -DDISABLE_LOGGING"

####################################################################################################

mkdir -p build/spv
cd build
toolchain="sdk/ndk/$ndk_version/toolchains/llvm/prebuilt/linux-x86_64"
sdkmanager="sdk/cmdline-tools/latest/bin/sdkmanager"
aapt2="sdk/build-tools/$build_tools_version/aapt2"
zipalign="sdk/build-tools/$build_tools_version/zipalign"
apksigner="sdk/build-tools/$build_tools_version/apksigner"
cc="$toolchain/bin/aarch64-linux-android${android_version}-clang++"

# download and run sdkmanager to download the android sdk
if [ ! -f "$sdkmanager" ]; then
    rm -rf cmdline-tools sdk
    mkdir -p  cmdline-tools sdk/cmdline-tools/latest
    if [ ! -f "cmdline-tools.zip" ]; then
        echo ">>> downloading cmdline-tools.zip"
        curl -s https://developer.android.com/studio \
            | grep -Eo "https://dl.google.com/android/repository/.*linux.*\.zip" \
            | xargs curl > cmdline-tools.zip
    fi

    echo ">>> extracting cmdline-tools.zip"
    unzip -qq cmdline-tools.zip
    mv cmdline-tools/* sdk/cmdline-tools/latest/
    rm -r cmdline-tools

    echo ">>> using sdkmanager to download sdk"
	yes | $sdkmanager --licenses > /dev/null
	$sdkmanager \
        "platforms;android-$android_version" \
        "build-tools;$build_tools_version" \
        "ndk;$ndk_version" \
        "patcher;v4" \
        "platform-tools" \
        "tools"
fi

if [ ! -f "debug.keystore" ]; then
    echo ">>> generating debug key"
    keytool -genkey -v -keystore debug.keystore -alias debug -keyalg RSA -keysize 2048 -validity 10000 \
        -storepass debug_password -keypass debug_password -dname "CN=example.com, OU=ID, O=Example, L=Doe, S=John, C=GB"
fi

# compile shaders
glslc -mfmt=c --target-env=vulkan1.3 -fshader-stage=vert ../shaders/triangle.vs.glsl -o spv/triangle.vs.h || exit 1
glslc -mfmt=c --target-env=vulkan1.3 -fshader-stage=frag ../shaders/white.fs.glsl    -o spv/white.fs.h    || exit 1

# compile program
echo ">>> compiling program"
mkdir -p lib/arm64-v8a
#    --sysroot $toolchain/sysroot \
#    --target=aarch64-unknown-linux-android$android_version \
#    todo: compile aarch64 compiler-rt (libclang_rt.builtints-aarch64.a
$cc ../src/main.cc -o lib/arm64-v8a/lib$name.so \
    $cc_flags $cc_debug_flags \
    -I$toolchain/sysroot/usr/include/ \
    -L$toolchain/sysroot/usr/lib/aarch64-linux-android/$android_version \
    || exit 1
echo "[" > ../compile_commands.json
cat main_compile_command.json >> ../compile_commands.json
echo "]" >> ../compile_commands.json

# validation layers
if [ ! -f android-binaries-${vulkan_version}.zip ]; then
    curl -L -o android-binaries-${vulkan_version}.zip \
        https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/download/vulkan-sdk-${vulkan_version}/android-binaries-${vulkan_version}.zip
fi
unzip -p android-binaries-${vulkan_version}.zip android-binaries-${vulkan_version}/arm64-v8a/libVkLayer_khronos_validation.so > lib/arm64-v8a/libVkLayer_khronos_validation.so

# package program
echo ">>> packaging apk"
android_version=$android_version name=$name package_name=$package_name \
envsubst <../AndroidManifest.xml > AndroidManifest.xml 
$aapt2 link -o unaligned.apk -v --no-compress \
    --manifest AndroidManifest.xml \
    -I sdk/platforms/android-$android_version/android.jar
zip -r unaligned.apk lib
$zipalign -v 4 unaligned.apk -f $name.apk
$apksigner sign --key-pass pass:debug_password --ks-pass pass:debug_password --ks debug.keystore $name.apk
rm unaligned.apk AndroidManifest.xml main_compile_command.json

adb install mini.apk

7z l mini.apk | tail -n +15 | cut --bytes 27-
