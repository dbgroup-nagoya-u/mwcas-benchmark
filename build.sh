#!/bin/bash

# エラーが起きたらスクリプトを停止する
set -e

# 引数1: ビルドモード (デフォルトは Release)
BUILD_TYPE=${1:-Release}
# 引数2: MwCASのビット数 (デフォルトは 60 に設定)
BIT_NUM=${2:-60}

BUILD_DIR="build/vscode"

# ==========================================
# 1. 共通設定 (yamlの共通部分)
# ==========================================
MAX_TARGET_NUM=8
USE_PMWCAS="OFF"
USE_TBBMALLOC="OFF"
BENCH_BUILD_TESTS="ON"
DBGROUP_MAX_THREAD_NUM=1024

# 内側の設定
MWCAS_BUILD_TESTS="ON"
THREAD_NUM=8
RANDOM_SEED=0

# ==========================================
# 2. ビルドモード固有の設定 (yamlの分岐ロジック)
# ==========================================
case "${BUILD_TYPE,,}" in # 小文字に変換して判定
    debug)
        CMAKE_BUILD_TYPE="Debug"
        USE_MIMALLOC="OFF"
        ;;
    relwithdebinfo)
        CMAKE_BUILD_TYPE="RelWithDebInfo"
        USE_MIMALLOC="OFF"
        ;;
    release)
        CMAKE_BUILD_TYPE="Release"
        USE_MIMALLOC="ON" # Releaseのみ mimalloc を有効化
        ;;
    *)
        echo "エラー: 不明なビルドタイプです。'Debug', 'Release', 'RelWithDebInfo' のいずれかを指定してください。"
        exit 1
        ;;
esac

echo "========================================"
echo " 🛠️  Build Configuration"
echo " Build Type : $CMAKE_BUILD_TYPE"
echo " Bit Num    : $BIT_NUM"
echo " mimalloc   : $USE_MIMALLOC"
echo "========================================"

# ==========================================
# 3. CMake 構成 (Configure)
# ==========================================
# yamlに書かれていたsettingsをすべて -D で渡します
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DMWCAS_VALUE_BIT_NUM="$BIT_NUM" \
    -DMWCAS_BENCH_MAX_TARGET_NUM="$MAX_TARGET_NUM" \
    -DMWCAS_BENCH_USE_PMWCAS="$USE_PMWCAS" \
    -DMWCAS_BENCH_USE_MIMALLOC="$USE_MIMALLOC" \
    -DMWCAS_BENCH_USE_TBBMALLOC="$USE_TBBMALLOC" \
    -DMWCAS_BENCH_BUILD_TESTS="$BENCH_BUILD_TESTS" \
    -DMWCAS_BUILD_TESTS="$MWCAS_BUILD_TESTS" \
    -DDBGROUP_TEST_THREAD_NUM="$THREAD_NUM" \
    -DDBGROUP_TEST_RANDOM_SEED="$RANDOM_SEED" \
    -DDBGROUP_MAX_THREAD_NUM="$DBGROUP_MAX_THREAD_NUM"

# ==========================================
# 4. ビルド (Build)
# ==========================================
cmake --build "$BUILD_DIR" --parallel
