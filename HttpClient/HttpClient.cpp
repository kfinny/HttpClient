// HttpClient.cpp : Defines the entry point for the application.
//

#include "HttpClient.h"
#include <vector>

HttpClient::HttpClient() : HttpClient(L"HttpClient") {}

HttpClient::HttpClient(const std::wstring& userAgent) {
    hInternet = InternetOpen(userAgent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
}

HttpClient::~HttpClient() {
    if (hInternet) {
        InternetCloseHandle(hInternet);
    }
}

std::wstring HttpClient::ConstructQueryString(const std::map<std::wstring, std::wstring>& queryParams) {
    std::wstring queryString;
    for (auto it = queryParams.begin(); it != queryParams.end(); ++it) {
        if (it != queryParams.begin()) {
            queryString += L"&";
        }
        queryString += it->first + L"=" + it->second; // Simple concatenation; for real use, consider URL encoding values
    }
    return queryString;
}

bool HttpClient::OpenConnection(const std::wstring& url) {
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);

    // Allocate buffers for the components.
    wchar_t schemeBuffer[16] = L"";
    wchar_t hostNameBuffer[256] = L"";
    wchar_t urlPathBuffer[1024] = L"";
    wchar_t extraInfoBuffer[1024] = L"";

    urlComp.lpszScheme = schemeBuffer;
    urlComp.dwSchemeLength = _countof(schemeBuffer);
    urlComp.lpszHostName = hostNameBuffer;
    urlComp.dwHostNameLength = _countof(hostNameBuffer);
    urlComp.nPort = 0; // Allows InternetCrackUrl to fill in the port
    urlComp.lpszUrlPath = urlPathBuffer;
    urlComp.dwUrlPathLength = _countof(urlPathBuffer);
    urlComp.lpszExtraInfo = extraInfoBuffer;
    urlComp.dwExtraInfoLength = _countof(extraInfoBuffer);
    
    if (InternetCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        scheme = schemeBuffer;
        hostName = hostNameBuffer;
        urlPath = urlPathBuffer;
        extraInfo = extraInfoBuffer;
        port = urlComp.nPort;

        // Close any existing connection.
        if (hConnect) {
            InternetCloseHandle(hConnect);
            hConnect = NULL;
        }

        bool secure = scheme == L"https";
        if (port == 0) { // Use default ports if none specified
            port = secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
        }
        flags = secure ? INTERNET_FLAG_SECURE : 0;

        hConnect = InternetConnect(hInternet, hostName.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, flags, 0);
        return hConnect != NULL;
    }
    return false;
}

std::map<std::wstring, std::wstring> HttpClient::ParseResponseHeaders(HINTERNET hRequest) {
    DWORD bufferSize = 0;
    HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &bufferSize, NULL);
    std::vector<wchar_t> buffer(bufferSize / sizeof(wchar_t));
    if (!HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, buffer.data(), &bufferSize, NULL)) {
        // Handle error...
    }

    std::map<std::wstring, std::wstring> headers;
    std::wstring headersStr(buffer.begin(), buffer.end());
    std::wistringstream stream(headersStr);
    std::wstring line;
    while (std::getline(stream, line)) {
        size_t colonPos = line.find(L":");
        if (colonPos != std::wstring::npos) {
            std::wstring name = line.substr(0, colonPos);
            std::wstring value = line.substr(colonPos + 2, line.length() - colonPos - 3); // Also remove \r
            headers[name] = value;
        }
    }
    return headers;
}

DWORD HttpClient::GetStatusCode(HINTERNET hRequest) {
    DWORD statusCode = 0;
    DWORD length = sizeof(statusCode);
    HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &length, NULL);
    return statusCode;
}

std::string HttpClient::ReadResponse(HINTERNET hRequest) {
    DWORD bytesRead;
    std::string content;
    char buffer[4096];
    BOOL readResult;
    while (true) {
        readResult = InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead);
        if (readResult == FALSE) {
            throw HttpException("Failed to read the HTTP response.", GetLastError());
        }
        if (bytesRead == 0) {
            break;
        }
        content.append(buffer, bytesRead);
    }

    return content;
}

Response HttpClient::Get(const std::wstring& url) {
    HttpHeaders headers;
    QueryParameters parameters;
    return Get(url, parameters, headers);
}

Response HttpClient::Get(const std::wstring& url, const HttpHeaders& headers) {
    QueryParameters parameters;
    return Get(url, parameters, headers);
}

Response HttpClient::Get(const std::wstring& url, const QueryParameters& parameters) {
    HttpHeaders headers;
    return Get(url, parameters, headers);

}

Response HttpClient::Get(const std::wstring& url, const QueryParameters& parameters, const HttpHeaders& headers) {
    Response response;
    if (!hInternet) {
        throw InternetOpenException("Failed to initialize an Internet session.");
    }

    if (!OpenConnection(url)) {
        throw InternetConnectException("Failed to connect to the server.");
    }

    std::wstring formattedParameters = parameters.Format();
    std::wstring fullPath = urlPath + (formattedParameters.empty() ? L"" : formattedParameters);

    flags |= INTERNET_FLAG_RELOAD;
    HINTERNET hRequest = HttpOpenRequest(hConnect, L"GET", fullPath.c_str(), NULL, NULL, NULL, flags, 0);

    if (!hRequest) {
        InternetCloseHandle(hConnect);
        throw HttpException("Failed to open request.");
    }

    std::wstring formattedHeaders = headers.Format();
    if (!HttpSendRequest(hRequest, formattedHeaders.empty() ? NULL : formattedHeaders.c_str(), (DWORD)formattedHeaders.length(), NULL, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        throw HttpException("Failed to send request.");
    }

    response.statusCode = GetStatusCode(hRequest);
    response.headers = ParseResponseHeaders(hRequest);
    response.content = ReadResponse(hRequest);

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);

    return response;
}
