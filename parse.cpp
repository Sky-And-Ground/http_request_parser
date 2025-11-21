/*
    http request parser, only for header part.
    written in C++17.
*/
#include <iostream>
#include <vector>
#include <string>
#include <map>

enum class ParseResult {
    success,
    method_too_long,
    url_too_long,
    version_too_long,
    invalid_method,
    invalid_url,
    invalid_version,
    invalid_format,
    invalid_headers,
    invalid_crlf
};

struct Request {
    std::string method;
    std::string url;
    std::string version;
    std::map<std::string, std::string> headers;
};

class Parser {
    static constexpr char SPACE = ' ';
    static constexpr char CR = '\r';
    static constexpr char LF = '\n';
    static constexpr const char* CRLF = "\r\n";

    enum class State {
        method,
        url,
        version,
        headers
    };

    std::vector<std::string> string_split(const std::string& str, const std::string& delimiter) const {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = str.find(delimiter);
        
        while (end != std::string::npos) {
            tokens.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        
        tokens.push_back(str.substr(start));
        return tokens;
    }

    bool split_headers(const std::string& str, Request& req) const {
        auto tokens = string_split(str, CRLF);

        for (const auto& token : tokens) {
            auto iter = token.find(": ");

            if (iter == std::string::npos) {   // invalid header.
                return false;
            }

            std::string key = token.substr(0, iter);
            std::string value = token.substr(iter + 2);

            req.headers.emplace(key, value);
        }

        return true;
    }
public:
    static constexpr int max_method_len = 32;
    static constexpr int max_url_len = 1024;
    static constexpr int max_version_len = 32;

    Parser() {}

    ParseResult parse(const std::string& data, size_t len, Request& req) const {
        size_t i = 0;
        State state = State::method;
        std::string headers;
        bool stop = false;

        while (i < len && !stop) {
            char c = data[i];

            switch(state) {
                case State::method:
                    if (c == SPACE) {
                        if (req.method.empty()) {
                            return ParseResult::invalid_method;
                        }

                        state = State::url;
                    }
                    else if (c == CR || c == LF) {
                        return ParseResult::invalid_format;
                    }
                    else {
                        if (req.method.length() == max_method_len) {
                            return ParseResult::method_too_long;
                        }

                        req.method += c;
                    }

                    ++i;
                    break;
                case State::url:
                    if (c == SPACE) {
                        if (req.url.empty()) {
                            return ParseResult::invalid_url;
                        }

                        state = State::version;
                    }
                    else if (c == CR || c == LF) {
                        return ParseResult::invalid_format;
                    }
                    else {
                        if (req.url.length() == max_url_len) {
                            return ParseResult::url_too_long;
                        }

                        req.url += c;
                    }

                    ++i;
                    break;
                case State::version:
                    if (c == CR) {
                        if (req.version.empty()) {
                            return ParseResult::invalid_format;
                        }

                        if (i + 1 >= len) {
                            return ParseResult::invalid_format;
                        }
                        else {
                            if (data[i + 1] != LF) {
                                return ParseResult::invalid_crlf;
                            }
                            else {
                                state = State::headers;
                                i += 2;
                            }
                        }
                    }
                    else {
                        if (req.version.length() == max_version_len) {
                            return ParseResult::version_too_long;
                        }

                        req.version += c;
                        ++i;
                    }

                    break;
                case State::headers:
                    if (c == CR) {
                        if (i + 1 >= len || data[i + 1] != LF) {
                            return ParseResult::invalid_format;
                        }
                        else {
                            if (i + 2 < len && data[i + 2] == CR && i + 3 < len && data[i + 3] == LF) {
                                stop = true;
                            }
                            else {
                                headers += CRLF;
                                i += 2;
                            }
                        }
                    }
                    else if (c == LF) {   // this case means only 1 LF, no CR.
                        return ParseResult::invalid_headers;
                    }
                    else {
                        headers += c;
                        ++i;
                    }

                    break;
                default:
                    break;
            }
        }

        if (!split_headers(headers, req)) {
            return ParseResult::invalid_headers;
        }

        return ParseResult::success;
    }
};

const char* parse_result_str(ParseResult res) {
    switch(res) {
        case ParseResult::success:
            return "success";
        case ParseResult::method_too_long:
            return "method too long";
        case ParseResult::url_too_long:
            return "url too long";
        case ParseResult::version_too_long:
            return "version too long";
        case ParseResult::invalid_method:
            return "invalid method";
        case ParseResult::invalid_url:
            return "invalid url";
        case ParseResult::invalid_version:
            return "invalid version";
        case ParseResult::invalid_format:
            return "invalid format";
        case ParseResult::invalid_crlf:
            return "invalid crlf";
        case ParseResult::invalid_headers:
            return "invalid headers";
        default:
            return "unknown parse error";
    }
}

int main() {
    const std::string url = "GET http://www.hatsunemiku.com/ HTTP/1.1\r\n"
                            "Host: www.example.com\r\n"
                            "Content-Length: 10\r\n"
                            "Accept-Encoding: utf-8\r\n"
                            "\r\n"
                            "Hello World";
    
    Request req;
    Parser parser;
    ParseResult result = parser.parse(url, url.length(), req);

    if (result != ParseResult::success) {
        std::cerr << parse_result_str(result) << "\n";
        return 1;
    }

    std::cout << req.method << "\n";
    std::cout << req.url << "\n";
    std::cout << req.version << "\n";

    for (const auto&[k, v] : req.headers) {
        std::cout << k << " " << v << "\n";
    }

    return 0;
}
