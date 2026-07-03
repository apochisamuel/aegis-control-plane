# Development

Use the CMake build for normal development. The ClusterFuzzLite build script is
self-contained and compiles fuzz-target dependency closures with the compiler
and sanitizer environment supplied by the runner. Keep source changes offline
and deterministic. Seed corpora are normal valid captures, not crash files.
