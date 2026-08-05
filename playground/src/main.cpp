#include <WinSock2.h>

#include "Log.h"
#include "Defer.h"
#include "Str.h"

int main() {

	WSAData socket_data {};
	int error_code = WSAStartup(MAKEWORD(2, 2), &socket_data);
	if (error_code != 0){
		hstl::log_error("Failed to initialize Windows Sockets: {}", error_code);
		return -1;
	}
	DEFER{WSACleanup();};
	hstl::log_info("Windows Scokets Library was initialized successfully");

	auto listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_socket == INVALID_SOCKET)
	{
		hstl::log_error("Failed to create a new socket");
		return -1;
	}
	DEFER{closesocket(listen_socket);};
	hstl::log_info("A socket was created successfully");


	sockaddr_in socket_desc {};
	socket_desc.sin_family = AF_INET;
	socket_desc.sin_port = htons(8080);
	socket_desc.sin_addr.s_addr = INADDR_ANY;
	auto result = bind(listen_socket, (sockaddr*)&socket_desc, sizeof(socket_desc));
	if (result != 0)
	{
		hstl::log_error("Failed to bind the socket to a specific port and address: {}", WSAGetLastError());
		return -1;
	}
	hstl::log_info("The socket was bound successfully");

	result = listen(listen_socket, SOMAXCONN);
	if (result != 0)
	{
		hstl::log_error("Failed to setup the listing socket: {}", WSAGetLastError());
		return -1;
	}
	hstl::log_info("Setting up a listening socket was successful");

	while (true)
	{
		auto connection_socket = accept(listen_socket, nullptr, nullptr);
		if (connection_socket == INVALID_SOCKET)
		{
			hstl::log_error("Failed to accept an arriving request: {}", WSAGetLastError());
			break;
		}
		DEFER{closesocket(connection_socket);};

		char buffer[4096];
		auto recieved_bytes = recv(connection_socket, buffer, 4096, 0);

		if (recieved_bytes == 0)
		{
			continue;
		}

		if (recieved_bytes == SOCKET_ERROR)
		{
			hstl::log_error("Failed to read the data");
			continue;
		}
		buffer[recieved_bytes] = '\0';
		hstl::log_info("{}", buffer);

		hstl::Str_View response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 11\r\n"
			"\r\n"
			"Hello World";

		auto send_res = send(connection_socket, response.data(), response.count(), 0);
		if (send_res == SOCKET_ERROR)
		{
			hstl::log_error("Failed to send the response");
			continue;
		}
	}

	return 0;
}