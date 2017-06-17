## 《基于Linux的分布式云盘的设计与开发》系统部署
本云盘的特色是高并发，服务端负载均衡，**提供文件的秒传，支持断点续传**，对重复文件进行了空间优化，**所有相同的文件只占用一份存储空间**。
### 部署服务端
0. 确保在Linux上g++版本在4.8.5及以上，已安装cmake
1. 在Linux上安装修改版的muduo库
```sh
    tar zxvf muduo.tar.gz
    cd muduo
    ./build.sh -j4 install
```
2. 在Linux上安装服务端
```sh
    cd server && make -j4
```
3. 酌情配置config.json文件
```js
    {
        // for both slave and master
        "is_master": true,
        "listen_port": 8086,
        "heart_beat": 300,
        "max_connection_num": 200,
        
        // only for master
        "users_path": "./users.json",

        // only for slave
        "files_hash_path": "./files_hash.json",
        "master_addr": {
            "ip": "125.45.26.187",
            "port": 8086
        }
    }
```
4. 部署Master或者Slave
```sh
    ./qcloud
```


### 部署客户端
0. 在Windows上安装qt-opensource-windows-x86-mingw530-5.8.0.exe版本5.8及以上
1. 配置client目录下的config.json
2. 使用QT构建并运行项目