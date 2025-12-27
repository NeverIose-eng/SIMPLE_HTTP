# SIMPLE_HTTP Module

# English README

## Introduction

A lightweight C++ HTTP client module based on WinINet, supporting basic HTTP GET and POST requests.

## Features

- Supports HTTP GET/POST requests

- Automatic response header parsing (including cookies)

- Multi-type response body conversion (UTF-8 string, raw byte buffer, ANSI string)

- Proxy configuration support

- Automatic redirection control

- System locale-based Accept-Language header generation

## Limitations

- Windows-only (x86/x64), depends on WinINet API

- Only supports HTTP/1.1

- POST data is limited to application/x-www-form-urlencoded format

- Requires C++23 or higher (module support)

## Quick Start

### 1. Basic GET Request

```cpp

import SIMPLE_HTTP;
using namespace simple;

int main() {
    try {
        http client("http://example.com");
        auto response = client.get("/");
        std::cout << "Status: " << response->status_code << " " << response->http_status_description() << std::endl;
        std::cout << "Body: " << response->get_body<std::string>() << std::endl;
        delete response;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
```

### 2. Basic POST Request

```cpp

import SIMPLE_HTTP;
using namespace simple;

int main() {
    try {
        http client("http://example.com/api");
        request_data post_data = {{"key1", "value1"}, {"key2", "value2"}};
        auto response = client.post("/submit", post_data);
        std::cout << "Status: " << response->status_code << std::endl;
        delete response;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
```

### 3. With Proxy

```cpp

http client("http://example.com");
client.set_proxy("127.0.0.1:8080"); // Format: host:port
auto response = client.get("/");
```

---

# 中文README

## 介绍

一个基于WinINet的轻量级C++ HTTP客户端模块，支持基础的HTTP GET和POST请求。

## 特性

- 支持HTTP GET/POST请求

- 自动解析响应头（含Cookie提取）

- 多类型响应体转换（UTF-8字符串、原始字节缓冲区、ANSI字符串）

- 支持代理配置

- 可控制自动重定向

- 基于系统区域生成Accept-Language请求头

## 限制条件

- 仅支持Windows系统（x86/x64），依赖WinINet API

- 仅支持HTTP/1.1协议

- POST数据仅支持application/x-www-form-urlencoded格式

- 要求C++23及以上版本（支持模块特性）

## 快速开始

### 1. 基础GET请求

```cpp

import SIMPLE_HTTP;
using namespace simple;

int main() {
    try {
        http client("http://example.com");
        auto response = client.get("/");
        std::cout << "状态码: " << response->status_code << " " << response->http_status_description() << std::endl;
        std::cout << "响应体: " << response->get_body<std::string>() << std::endl;
        delete response;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
    }
    return 0;
}
```

### 2. 基础POST请求

```cpp

import SIMPLE_HTTP;
using namespace simple;

int main() {
    try {
        http client("http://example.com/api");
        request_data post_data = {{"key1", "value1"}, {"key2", "value2"}};
        auto response = client.post("/submit", post_data);
        std::cout << "状态码: " << response->status_code << std::endl;
        delete response;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
    }
    return 0;
}
```

### 3. 使用代理

```cpp

http client("http://example.com");
client.set_proxy("127.0.0.1:8080"); // 格式：主机:端口
auto response = client.get("/");
```
> （注：文档内容由 @AI(chatgpt.com) 生成）
