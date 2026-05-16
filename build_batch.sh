#!/bin/bash
set -e

cd ~/sotsuken/mwcas-benchmark

VARIANTS_DIR="/home/unno/sotsuken/mwcas-benchmark/out/for_ieice2026"

# 1. パスとバリエーションの設定
TARGET_BENCH="/home/unno/sotsuken/mwcas-benchmark/src/mwcas_bench.cpp"
TARGET_CPP="/home/unno/sotsuken/mwcas-benchmark/external/mwcas/src/lock_free/mwcas_descriptor.cpp"
TARGET_HPP="/home/unno/sotsuken/mwcas-benchmark/external/mwcas/include/dbgroup/atomic/mwcas/lock_free/mwcas_descriptor.hpp"
VARIANTS=("NR" "SW" "WC")

mkdir -p bench_binaries

# 2. 総当たりコンパイル
for VARIANT in "${VARIANTS[@]}"; do
    echo "================================================="
    echo " 🔄 $VARIANT をセットしています..."
    echo "================================================="

    # 思考停止で .cpp と .hpp を両方上書き（運用側で必ず両方用意するルール）
    cp "${VARIANTS_DIR}/variants/mwcas_bench_${VARIANT}.cpp" "$TARGET_BENCH"
    cp "${VARIANTS_DIR}/variants/mwcas_descriptor_${VARIANT}.cpp" "$TARGET_CPP"
    cp "${VARIANTS_DIR}/variants/mwcas_descriptor_${VARIANT}.hpp" "$TARGET_HPP"

    for BIT_NUM in {48..62}; do
        echo "▶ コンパイル: $VARIANT / Bit: $BIT_NUM"

        # ビルド実行
        ./build.sh Release "$BIT_NUM" > /dev/null

        # バイナリの回収
        cp build/vscode/mwcas_bench "bench_binaries/mwcas_${VARIANT}_bit${BIT_NUM}"
    done
done

echo "🎉 完了しました！"
echo "💡 git restore src/ include/ で元のコードに戻してください。"
