#!/bin/bash -eu
cd $SRC
mkdir -p "$OUT"
sources=(
    src/core/bytes.cpp
    src/capture/capture.cpp
    src/domain/domain.cpp
    src/routing/route_index.cpp
    src/policy/policy_compiler.cpp
    src/policy/extension_planner.cpp
    src/dispatch/dispatch_plan.cpp
    src/diagnostics/extension_renderer.cpp
    src/charging/charging_projection.cpp
    src/recovery/checkpoint_journal.cpp
    src/trace/fragment_trace.cpp
    src/replay/replay_book.cpp
    src/engine/engine.cpp
)
objects=()
for src in "${sources[@]}"; do
    obj="$OUT/$(echo "$src" | tr '/' '_').o"
    $CXX $CXXFLAGS -std=c++20 -Iinclude -Isrc -c -o "$obj" "$src"
    objects+=("$obj")
done
ar rcs "$OUT/libaegis_fuzz.a" "${objects[@]}"
for fuzzer_src in fuzz/*_fuzzer.cc; do
    fuzzer_name=$(basename "${fuzzer_src%.cc}")
    $CXX $CXXFLAGS -std=c++20 -Iinclude -Isrc -o "$OUT/$fuzzer_name" \
        "$fuzzer_src" "$OUT/libaegis_fuzz.a" $LIB_FUZZING_ENGINE
done
cp -r fuzz/corpus "$OUT/corpus"
cp fuzz/dictionary.txt "$OUT/dictionary.txt"
