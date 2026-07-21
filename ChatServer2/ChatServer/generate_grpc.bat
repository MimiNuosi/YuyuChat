@echo off
echo ===========================================
echo Generating C++ files from message.proto...
echo ===========================================

:: 修改后的工具路径，指向 installed 目录
set PROTOC_EXE=D:\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe
set PLUGIN_EXE=D:\vcpkg\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe

:: 执行编译
"%PROTOC_EXE%" -I="." --cpp_out="." --grpc_out="." --plugin=protoc-gen-grpc="%PLUGIN_EXE%" "message.proto"

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Generation failed!
) else (
    echo [SUCCESS] .pb.cc, .pb.h, .grpc.pb.cc, .grpc.pb.h generated successfully!
)

pause