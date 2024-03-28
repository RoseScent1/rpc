# rpc
### 安装protobuf和libtinyxml

### 日志模块开发

日志模块:
``` 
1. 日志级别
2. 打印到文件,支持日期命名,以及日志滚动
4. 线程安全
```

LogLevel 日志级别
```
Debug
Info
Error
```
LogEvent:封装打印
```
信息包括
文件名
行号
MsgNumber(标记每个RPC请求)
ThreadId
进程号
日期.
自定义消息
```
日志格式
```
[LogLevel]\t[%y-%m-%d %H:%M:%s.%ms]\t[pid:thread_id]\t[file_name:line][%Msg]
```

日志打印类 Logger

提供打印日志方法
设置日志输出路径


### eventloop
事件模块