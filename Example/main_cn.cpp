import SIMPLE_HTTP;
#include <iostream>
#include <string>

int main() {
    try {
        // 创建客户端对象
        simple::http client("http://127.0.0.1:9933");

        // 可选设置
        client.set_no_redirection_allowed(true);
        client.set_proxy("127.0.0.1:8080");

        // -----------------------
        // GET 请求示例
        // -----------------------
        auto get_body_str = client.get("/get")->get_body<std::string>();
        std::cout << "GET /get as std::string:\n" << get_body_str << "\n\n";

        auto get_body_utf8 = client.get("/get")->get_body<std::u8string>();
        std::cout << "GET /get as std::u8string:\n"
                  << reinterpret_cast<const char*>(get_body_utf8.c_str()) << "\n\n";

        auto get_body_raw = client.get("/get")->get_body<unsigned char*>();
        std::cout << "GET /get as unsigned char*:\n"
                  << reinterpret_cast<const char*>(get_body_raw) << "\n\n";
        delete[] get_body_raw; // 记得释放

        // GET 请求带自定义头
        simple::http::request_headers get_headers;
        get_headers["Custom-Header"] = "Hello";
        auto get_with_headers = client.get("/", get_headers)->get_body<std::string>();
        std::cout << "GET / with custom header:\n" << get_with_headers << "\n\n";

        // -----------------------
        // POST 请求示例
        // -----------------------
        std::string post_data = "token=123456&name=example";

        auto post_body_str = client.post("/post", post_data)->get_body<std::string>();
        std::cout << "POST /post as std::string:\n" << post_body_str << "\n\n";

        // POST 带自定义头
        simple::http::request_headers post_headers;
        post_headers["Content-Type"] = "application/x-www-form-urlencoded";
        post_headers["Custom-Header"] = "TestValue";

        auto post_with_headers = client.post("/post", post_data, post_headers)->get_body<std::string>();
        std::cout << "POST /post with custom headers:\n" << post_with_headers << "\n\n";

    } catch (const std::exception& e) {
        std::cerr << "HTTP request failed: " << e.what() << "\n";
    }
}
