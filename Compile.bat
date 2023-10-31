xmake f -p windows -a x64 -m debug --cc=clang-cl --cxx=clang-cl
xmake project -k compile_commands build
xmake -w