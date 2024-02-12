#include "HttpClient.h"

using namespace std;

int main()
{
	HttpClient client;
	try {
		HttpHeaders headers;
		headers.SetHeader(L"Accept", L"*/*");
		headers.SetHeader(L"User-Agent", L"HttpClient");
		Response resp = client.Get(L"https://www.google.com/", headers);
		cout << "Status Code: " << resp.statusCode << endl;
	}
	catch (const HttpException& e) {
		std::wcerr << L"The request threw an error: " << e.what() << std::endl;
	}
	return 0;
}
