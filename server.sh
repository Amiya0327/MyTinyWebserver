#!/bin/bash

# 端口配置
PORT1=8081
PORT2=8082
PORT3=8083
PORT4=8084

# PID文件目录（用于安全识别自己的进程）
PID_DIR="./pids"
mkdir -p "$PID_DIR"  # 这里会在当前目录创建pids文件夹，没问题

# 关闭我们自己的后台进程（只关闭我们知道的端口）
stop_servers() {
    echo "关闭server进程..."
    
    # 只关闭我们知道的端口对应的进程
    for port in $PORT1 $PORT2 $PORT3 $PORT4; do
        # 使用lsof安全地查找进程
        pid=$(lsof -ti:$port 2>/dev/null)  # 问题：需要root权限才能查看其他用户的进程
        if [ ! -z "$pid" ]; then
            echo "关闭端口 $port 的进程 (PID: $pid)"
            kill $pid 2>/dev/null
        fi
        
        # 也检查PID文件
        pid_file="$PID_DIR/server_$port.pid"
        if [ -f "$pid_file" ]; then
            pid=$(cat "$pid_file")
            echo "从PID文件关闭端口 $port 的进程 (PID: $pid)"
            kill $pid 2>/dev/null
            rm -f "$pid_file"
        fi
    done
    
    sleep 1
    
    # 只强制关闭我们端口的进程
    for port in $PORT1 $PORT2 $PORT3 $PORT4; do
        pid=$(lsof -ti:$port 2>/dev/null)  # 同样需要root权限
        if [ ! -z "$pid" ]; then
            echo "强制关闭端口 $port 的残留进程"
            kill -9 $pid 2>/dev/null
        fi
    done
    
    echo "完成"
}

# 启动后台进程
start_servers() {
    stop_servers  # 先关闭之前的进程
    
    echo "启动4个后台进程..."
    
    # 启动并保存PID
    ./server $PORT1 &  # 这里假设当前目录有server可执行文件
    echo $! > "$PID_DIR/server_$PORT1.pid"
    echo "启动进程1: 端口 $PORT1, PID: $!"
    
    ./server $PORT2 &
    echo $! > "$PID_DIR/server_$PORT2.pid"
    echo "启动进程2: 端口 $PORT2, PID: $!"
    
    ./server $PORT3 &
    echo $! > "$PID_DIR/server_$PORT3.pid"
    echo "启动进程3: 端口 $PORT3, PID: $!"
    
    ./server $PORT4 &
    echo $! > "$PID_DIR/server_$PORT4.pid"
    echo "启动进程4: 端口 $PORT4, PID: $!"
    
    echo "所有进程已启动"
}

# 主逻辑
if [ "$1" = "1" ]; then
    start_servers
elif [ "$1" = "0" ]; then
    stop_servers
else
    echo "用法: $0 <0|1>"
    echo "  0 - 关闭所有进程"
    echo "  1 - 启动所有进程"
    exit 1
fi