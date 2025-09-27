cd gfx
rm -rf build
cmake -DCMAKE_BUILD_TYPE=Debug -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd ../sbx
rm -rf build
python Amake.py
# -DCMAKE_CXX_COMPILER=g++-13