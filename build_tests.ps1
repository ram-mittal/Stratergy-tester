$ZIG = "C:/Users/admin/AppData/Local/Programs/Python/Python312/Lib/site-packages/ziglang/zig.exe"
$env:CC = "$ZIG cc"
$env:CXX = "$ZIG c++"
cmake -G Ninja -B build -S . -DCMAKE_AR="C:/Users/admin/Desktop/system/zig-ar.cmd" -DULTRABACKTEST_BUILD_BENCHMARKS=ON -DULTRABACKTEST_BUILD_EXAMPLES=OFF -DULTRABACKTEST_BUILD_PYTHON=ON
cmake --build build
