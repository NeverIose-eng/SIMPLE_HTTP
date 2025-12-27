// Platform check
#if !defined(_WIN32)
#error "SIMPLE_HTTP module requires Windows (x86 or x64)"
#endif

module;
#include     <windows.h>
#include     <wininet.h>

export module SIMPLE_HTTP;

// Include headers
import     <string   >;
import     <map      >;
import     <stdexcept>;
import     <sstream  >;
import     <vector   >;
import     <algorithm>;
// Linker settings
#pragma comment(lib, "wininet.lib")
// Module implementation

export namespace simple {
	export using request_headers = std::map<std::string, std::string>;
	export using response_headers = std::map<std::string, std::string>;
	export using request_data    = std::map<std::string, std::string>;
	class http;
	class request_body {
	public:
		response_headers headers;
		std::string cookie;
		int status_code;
		// Generic accessor for the response body. Throws for unsupported types.
		template<typename T>
		T get_body() { throw std::runtime_error("Unsupported type for request_body::get"); }
		// Return the response body as a UTF-8 (u8) string.
		template<>
		std::u8string get_body<std::u8string>() { return std::u8string(reinterpret_cast<const char8_t*>(body_buffer.data()), body_buffer.size());}
		// Return a newly allocated raw byte buffer containing the response body.
		template<>
		unsigned char* get_body<unsigned char*>() {
			size_t sz = this->body_buffer.size();
			auto buffer = new unsigned char[sz + 1];
			memcpy_s(buffer, sz, this->body_buffer.data(), sz);
			return buffer;
		}
		// Return the response body converted to the system ANSI std::string.
		template<>
		std::string get_body<std::string>() { return this->uta(std::string(body_buffer.begin(), body_buffer.end())).c_str(); }
		// Provide a human-readable HTTP status description for the numeric status code.
		std::string http_status_description() const {
			static const std::map<int, std::string> status_map = {
				{100, "Continue"},
				{101, "Switching Protocols"},
				{102, "Processing"},
				{200, "OK"},
				{201, "Created"},
				{202, "Accepted"},
				{203, "Non-Authoritative Information"},
				{204, "No Content"},
				{205, "Reset Content"},
				{206, "Partial Content"},
				{207, "Multi-Status"},
				{208, "Already Reported"},
				{226, "IM Used"},
				{300, "Multiple Choices"},
				{301, "Moved Permanently"},
				{302, "Found"},
				{303, "See Other"},
				{304, "Not Modified"},
				{305, "Use Proxy"},
				{307, "Temporary Redirect"},
				{308, "Permanent Redirect"},
				{400, "Bad Request"},
				{401, "Unauthorized"},
				{402, "Payment Required"},
				{403, "Forbidden"},
				{404, "Not Found"},
				{405, "Method Not Allowed"},
				{406, "Not Acceptable"},
				{407, "Proxy Authentication Required"},
				{408, "Request Timeout"},
				{409, "Conflict"},
				{410, "Gone"},
				{411, "Length Required"},
				{412, "Precondition Failed"},
				{413, "Payload Too Large"},
				{414, "URI Too Long"},
				{415, "Unsupported Media Type"},
				{416, "Range Not Satisfiable"},
				{417, "Expectation Failed"},
				{418, "I'm a teapot"},
				{421, "Misdirected Request"},
				{422, "Unprocessable Entity"},
				{423, "Locked"},
				{424, "Failed Dependency"},
				{426, "Upgrade Required"},
				{428, "Precondition Required"},
				{429, "Too Many Requests"},
				{431, "Request Header Fields Too Large"},
				{451, "Unavailable For Legal Reasons"},
				{500, "Internal Server Error"},
				{501, "Not Implemented"},
				{502, "Bad Gateway"},
				{503, "Service Unavailable"},
				{504, "Gateway Timeout"},
				{505, "HTTP Version Not Supported"},
				{506, "Variant Also Negotiates"},
				{507, "Insufficient Storage"},
				{508, "Loop Detected"},
				{510, "Not Extended"},
				{511, "Network Authentication Required"}
			};
			auto it = status_map.find(this->status_code);
			return it != status_map.end() ? it->second : "Unknown Status";
		}
	private:
		// Convert a UTF-8 encoded std::string to the system ANSI encoding.
		std::string uta(const std::string& u8Str) {
			if (u8Str.empty()) return "";
			int wideSize = MultiByteToWideChar(CP_UTF8, 0, u8Str.c_str(), -1, nullptr, 0);
			if (wideSize == 0) throw std::runtime_error("UTF-8 to Wide conversion failed");
			std::vector<wchar_t> wideBuffer(wideSize);
			if (MultiByteToWideChar(CP_UTF8, 0, u8Str.c_str(), -1, wideBuffer.data(), wideSize) == 0) throw std::runtime_error("UTF-8 to Wide conversion failed");
			int ansiSize = WideCharToMultiByte(CP_ACP, 0, wideBuffer.data(), -1, nullptr, 0, nullptr, nullptr);
			if (ansiSize == 0) throw std::runtime_error("Wide to ANSI conversion failed");
			std::vector<char> ansiBuffer(ansiSize);
			if (WideCharToMultiByte(CP_ACP, 0, wideBuffer.data(), -1, ansiBuffer.data(), ansiSize, nullptr, nullptr) == 0) throw std::runtime_error("Wide to ANSI conversion failed");
			return std::string(ansiBuffer.data(), ansiSize - 1);
		}
		// Parse raw HTTP headers into header map and cookie string.
		void parse_headers(const std::string& raw_headers) {
			std::istringstream stream(raw_headers);
			std::string line;
			while (std::getline(stream, line)) {
				if (!line.empty() && line.back() == '\r') line.pop_back();
				if (line.find("HTTP/") == 0) {
					auto pos1 = line.find(' ');
					auto pos2 = line.find(' ', pos1 + 1);
					if (pos1 != std::string::npos && pos2 != std::string::npos) {
						this->status_code = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
					}
					continue;
				}
				auto pos = line.find(':');
				if (pos == std::string::npos) continue;
				std::string key = line.substr(0, pos);
				std::string value = line.substr(pos + 1);
				key.erase(0, key.find_first_not_of(" \t"));
				key.erase(key.find_last_not_of(" \t") + 1);
				value.erase(0, value.find_first_not_of(" \t"));
				value.erase(value.find_last_not_of(" \t") + 1);
				if (_stricmp(key.c_str(), "Set-Cookie") == 0) {
					auto semicolon = value.find(';');
					std::string kv = (semicolon == std::string::npos) ? value : value.substr(0, semicolon);
					if (!this->cookie.empty()) this->cookie += "; ";
					this->cookie += kv;
				}
				else {
					this->headers[key] = value;
				}
			}
		}
		std::vector<unsigned char>body_buffer;
		friend class http;
	};
	class http
	{
	public:
		// Construct http client with a host URL (must include http:// or https://).
		http(const std::string& host)
		{
			if(host.find("http:")==std::string::npos && host.find("https:") == std::string::npos)
				throw std::invalid_argument("Invalid URL: Missing http:// or https://");
			this->__host = host;
		}
		// Perform an HTTP GET request for the specified page and optional headers.
		auto get(const std::string& page,const request_headers& headers = {},const std::string& user_agent = "SIMPLE_HTTP C++/1.0")
		{
			this->__page = page;
			this->__headers = headers;
			this->__request_type_post = false;
			this->__user_agent = user_agent;
			return this->__get();
		}
		// Perform an HTTP POST request with form-like data and optional headers.
		auto post(const std::string& page, const request_data& data, const request_headers& headers = {}, const std::string& user_agent = "SIMPLE_HTTP C++/1.0") {
			this->__page = page;
			this->__headers = headers;
			this->__request_type_post = true;
			this->__user_agent = user_agent;
			this->__data = data;
			return this->__get();
		}
		// Configure an HTTP proxy (format: host:port). Returns this for chaining.
		auto set_proxy(const std::string& ip) {
			if (ip.find(":") == std::string::npos && ip.find(".") == std::string::npos)
				throw std::invalid_argument("Invalid proxy format The correct format should be 127.0.0.1:8080");
			this->__proxy = ip;
			return this;
		};
		// Enable or disable automatic redirection handling.
		auto set_no_redirection_allowed(bool v = true) { this->__no_redirection_allowed = v; return this; };
	private:
		// Serialize headers map into a CRLF-separated header string.
		std::string __get_headers() const {
			std::string result;
			result.reserve(this->__headers.size() * 32);
			for (const auto& [k, v] : this->__headers) result.append(k).append(": ").append(v).append("\r\n");
			return result;
		}
		// Encode request data map into application/x-www-form-urlencoded body.
		std::string __get_post_data() const {
			std::string result;
			for (const auto& [k, v] : this->__data) {
				if (!result.empty()) result.append("&");
				result.append(k).append("=").append(v);
			}
			return result;
		}
		// Derive an Accept-Language value from the system/user locale.
		std::string get_system_accept_language() {
			WCHAR localeName[LOCALE_NAME_MAX_LENGTH]{};
			if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0) {
				std::wstring wstr(localeName);
				std::string str(wstr.begin(), wstr.end());
				for (auto& c : str) if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
				return str;
			}
			return "en-us";
		}
		// Perform the low-level request using WinINet and return a populated request_body.
		request_body* __get() {
			const bool is_https = this->__host.find("https://") != std::string::npos;
			std::string proxy_str;
			if (!__proxy.empty() && !is_https) proxy_str = "http=" + __proxy;
			HINTERNET hInternet = InternetOpenA(
				this->__user_agent.c_str(),
				__proxy.empty() ? INTERNET_OPEN_TYPE_PRECONFIG : INTERNET_OPEN_TYPE_PROXY,
				__proxy.empty() ? NULL : (is_https ? __proxy.c_str() : proxy_str.c_str()),
				NULL,
				0
			);
			if (!hInternet) throw std::runtime_error("InternetOpenA failed");
			std::string host = this->__host;
			if (host.find("://") != std::string::npos)
				host = host.substr(host.find("://") + 3);
			size_t colon_pos = host.find(':');
			DWORD port = is_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
			if (colon_pos != std::string::npos) {
				port = static_cast<DWORD>(std::stoi(host.substr(colon_pos + 1)));
				host = host.substr(0, colon_pos);
			}
			HINTERNET hConnect = InternetConnectA(
				hInternet,
				host.c_str(),
				static_cast<INTERNET_PORT>(port),
				NULL,
				NULL,
				INTERNET_SERVICE_HTTP,
				0,
				0
			);
			if (!hConnect) {
				InternetCloseHandle(hInternet);
				throw std::runtime_error("InternetConnectA failed");
			}
			auto request_flags = INTERNET_FLAG_RELOAD | INTERNET_COOKIE_THIRD_PARTY;
			request_flags |= (this->__headers.count("Cookies") || this->__headers.count("cookies")) ? 0 : INTERNET_FLAG_NO_COOKIES;
			request_flags |= this->__no_redirection_allowed ? INTERNET_FLAG_NO_AUTO_REDIRECT : 0;
			request_flags |= is_https ? INTERNET_FLAG_SECURE : INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
			HINTERNET hRequest = HttpOpenRequestA(
				hConnect,
				this->__request_type_post ? "POST" : "GET",
				this->__page.c_str(),
				"HTTP/1.1",
				NULL,
				NULL,
				request_flags,
				0
			);
			if (!hRequest) {
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				throw std::runtime_error("HttpOpenRequestA failed");
			}
			DWORD flags = 127872;
			InternetSetOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
			std::string data_str = this->__get_post_data();
			if (this->__headers.empty()) {
				this->__headers["Accept"] = "*/*";
				this->__headers["Referer"] = this->__host + this->__page;
				this->__headers["Accept-Language"] = get_system_accept_language();
				this->__headers["Content-Length"] = std::to_string(data_str.size());
			}
			auto header_str = this->__get_headers();
			if(!__request_type_post)
				HttpSendRequestA(
					hRequest,
					header_str.c_str(),
					static_cast<DWORD>(header_str.size()),
					NULL,
					0
				);
			else {
				INTERNET_BUFFERSA buffersIn{};
				buffersIn.dwStructSize = sizeof(INTERNET_BUFFERSA);
				buffersIn.lpcszHeader = header_str.c_str();
				buffersIn.dwHeadersLength = static_cast<DWORD>(header_str.size());
				buffersIn.dwBufferTotal = static_cast<DWORD>(data_str.size());
				if (!HttpSendRequestExA(hRequest, &buffersIn, NULL, HSR_INITIATE, 0)) {
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);
					throw std::runtime_error("HttpSendRequestExA failed");
				}
				DWORD dwWritten = 0;
				size_t offset = 0;
				while (offset < data_str.size()) {
					DWORD chunk_size = static_cast<DWORD>(std::min<size_t>(4096, data_str.size() - offset));
					if (!InternetWriteFile(hRequest, data_str.data() + offset, chunk_size, &dwWritten) || dwWritten == 0) {
						HttpEndRequestA(hRequest, NULL, 0, 0);
						InternetCloseHandle(hRequest);
						InternetCloseHandle(hConnect);
						InternetCloseHandle(hInternet);
						throw std::runtime_error("InternetWriteFile failed");
					}
					offset += dwWritten;
				}
				HttpEndRequestA(hRequest, NULL, 0, 0);
			}
			long i = 0;
			unsigned char* cache_buffer = new unsigned char[1024];
			std::vector<unsigned char>buffer;
			while (true) {
				DWORD dwRead = 0;
				if (!InternetReadFile(hRequest, cache_buffer, 1024, &dwRead) || dwRead == 0) break;
				buffer.insert(buffer.end(), cache_buffer, cache_buffer + dwRead);
			}
			delete[] cache_buffer;
			DWORD header_size = 0;
			HttpQueryInfoA(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &header_size, NULL);
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
				throw std::runtime_error("HttpQueryInfoA failed to get required size");
			char* header_buffer = new char[header_size];
			if (!HttpQueryInfoA(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, header_buffer, &header_size, NULL)) {
				delete[] header_buffer;
				throw std::runtime_error("HttpQueryInfoA failed to get headers");
			}
			std::string headers_str(header_buffer, header_size);
			delete[] header_buffer;
			InternetCloseHandle(hRequest);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInternet);
			request_body* body = new request_body();
			body->body_buffer = buffer;
			body->parse_headers(headers_str);
			return body;
		}
	private:
		std::string __proxy,__host,__page, __user_agent;
		bool __no_redirection_allowed,__request_type_post;
		request_headers __headers;
		request_data __data;
	};
}