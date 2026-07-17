@echo off
echo ========================================
echo Generating C++ files from message.proto...
echo ========================================

:: 定义工具路径 (请确保路径与你的实际环境一致)
set PROTOC_EXE=C:\Users\yuyu\vcpkg\packages\protobuf_x64-windows\tools\protobuf\protoc.exe
set PLUGIN_EXE=C:\Users\yuyu\vcpkg\packages\grpc_x64-windows\tools\grpc\grpc_cpp_plugin.exe

:: 执行编译 (同时生成 pb.cc 和 grpc.pb.cc)
"%PROTOC_EXE%" -I="." --cpp_out="." --grpc_out="." --plugin=protoc-gen-grpc="%PLUGIN_EXE%" "message.proto"

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Generation failed!
) else (
    echo [SUCCESS] .pb.cc, .pb.h, .grpc.pb.cc, .grpc.pb.h generated successfully!
)

pause