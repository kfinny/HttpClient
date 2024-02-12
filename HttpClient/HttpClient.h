// HttpClient.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>

#include <windows.h>
#include <wininet.h>
#include <string>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

#pragma comment(lib, "wininet.lib")

struct Response {
    DWORD statusCode;
    std::map<std::wstring, std::wstring> headers;
    std::string content;
    

    Response() : statusCode(0) {}
    Response(const std::string& content, DWORD statusCode, const std::map<std::wstring, std::wstring>& headers) : content(content), statusCode(statusCode), headers(headers) {}
};


class HttpException : public std::runtime_error {
protected:
    unsigned long errorNumber;
public:
    HttpException(const std::string& message, unsigned long errorNum = GetLastError()) : std::runtime_error(message), errorNumber(errorNum) {}

    const char* what() const noexcept override{
        static std::string errMsg;
        std::ostringstream oss;
        oss << std::runtime_error::what() << " (Error " << errorNumber << ")";
        errMsg = oss.str();
        return errMsg.c_str();
    }

    unsigned long getErrorNumber() const noexcept {
        return errorNumber;
    }
};

class InternetOpenException : public HttpException {
public:
    InternetOpenException(const std::string& message) : HttpException(message) {}
};

class InternetConnectException : public HttpException {
public:
    InternetConnectException(const std::string& message) : HttpException(message) {}
};


class HttpKeyValuePairs {
protected:
    std::map<std::wstring, std::wstring> keyValuePairs;

    void Set(const std::wstring& key, const std::wstring& value) {
        keyValuePairs[key] = value;
    }

    void Remove(const std::wstring& key) {
        keyValuePairs.erase(key);
    }

    std::wstring Get(const std::wstring& key) const {
        auto it = keyValuePairs.find(key);
        return it != keyValuePairs.end() ? it->second : L"";
    }

    const std::map<std::wstring, std::wstring>& GetKeyValuePairs() const {
        return keyValuePairs;
    }

public:
    // To be overridden by subclasses for specific formatting
    virtual std::wstring Format() const = 0;
};


class HttpHeaders : public HttpKeyValuePairs {
public:
    void SetHeader(const std::wstring& name, const std::wstring& value) {
        Set(name, value);
    }

    void RemoveHeader(const std::wstring& name) {
        Remove(name);
    }

    std::wstring GetHeader(const std::wstring& name) const {
        return Get(name);
    }

    std::wstring Format() const override {
        std::wostringstream stream;
        for (const auto& pair : keyValuePairs) {
            stream << pair.first << L": " << pair.second << L"\r\n";
        }
        return stream.str();
    }
    const std::map<std::wstring, std::wstring>& GetHeaders() const {
        return GetKeyValuePairs();
    }
};

class QueryParameters : public HttpKeyValuePairs {
public:
    void SetParameter(const std::wstring& name, const std::wstring& value) {
        Set(name, value);
    }

    void RemoveParameter(const std::wstring& name) {
        Remove(name);
    }

    std::wstring GetParameter(const std::wstring& name) const {
        return Get(name);
    }

    std::wstring Format() const override {
        std::wostringstream stream;
        for (const auto& pair : keyValuePairs) {
            stream << pair.first << L": " << pair.second << L"\r\n";
        }
        return stream.str();
    }
    const std::map<std::wstring, std::wstring>& GetParameters() const {
        return GetKeyValuePairs();
    }
};

class HttpClient {
private:
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    std::wstring hostName;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTP_PORT;
    std::wstring scheme;
    std::wstring urlPath;
    std::wstring extraInfo;
    DWORD flags = 0;
    bool OpenConnection(const std::wstring& url);
    std::wstring ConstructQueryString(const std::map<std::wstring, std::wstring>& queryParams);
    std::map<std::wstring, std::wstring> ParseResponseHeaders(HINTERNET hRequest);
    DWORD GetStatusCode(HINTERNET hRequest);
    std::string ReadResponse(HINTERNET hRequest);


public:
    HttpClient();
    HttpClient(const std::wstring& userAgent);
    ~HttpClient();
    Response Get(const std::wstring& url);
    Response Get(const std::wstring& url, const HttpHeaders& headers);
    Response Get(const std::wstring& url, const QueryParameters& parameters);
    Response Get(const std::wstring& url, const QueryParameters& parameters, const HttpHeaders& headers);
    // Response Post(const std::wstring& url);
};
