#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>

#include "Result.h"

namespace hstl
{
	class Socket
	{
	private:
		SOCKET _handle{INVALID_SOCKET};

	private:
		Socket(SOCKET handle):
			_handle{handle}
		{

		}

	public:
		Socket(const Socket&) = delete;
		Socket& operator=(const Socket&) = delete;

		Socket(Socket&& source):
			_handle{source._handle}
		{
			source._handle = INVALID_SOCKET;
		}

		Socket& operator=(Socket&& source)
		{
			if (this == &source)
			{
				return *this;
			}

			if (_handle != INVALID_SOCKET)
			{
				closesocket(_handle);
			}

			_handle = source._handle;

			source._handle = INVALID_SOCKET;

			return *this;
		}

		~Socket()
		{
			if (_handle != INVALID_SOCKET)
			{
				closesocket(_handle);
				_handle = INVALID_SOCKET;
			}
		}

	public:
		// Creates an unbound, unconnected TCP socket over IPv4.
		static Result<Socket> create()
		{
			SOCKET handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if (handle == INVALID_SOCKET)
			{
				return Err("Failed to create a socket: {}", WSAGetLastError());
			}

			return Socket{handle};
		}

		// Adopts an already-created handle, such as the one accept() returns.
		static Result<Socket> adopt(SOCKET handle)
		{
			if (handle == INVALID_SOCKET)
			{
				return Err("Refused to adopt an invalid socket handle");
			}

			return Socket{handle};
		}

		bool is_valid() const
		{
			return _handle != INVALID_SOCKET;
		}

		SOCKET handle() const
		{
			return _handle;
		}

		Result<void> bind(uint16_t port, Str_View ip_address = "0.0.0.0" /*must be null-terminated*/)
		{
			IN_ADDR parsed_address{};
			auto res = inet_pton(AF_INET, ip_address.data(), &parsed_address);

			// NOTE: inet_pton returns 0 for a malformed address string without touching the
			// error state, so WSAGetLastError() would report something unrelated here.
			if (res != 1)
			{
				return Err("'{}' is not a valid IPv4 address", ip_address);
			}

			sockaddr_in socket_desc {};
			socket_desc.sin_family = AF_INET;
			socket_desc.sin_port = htons(port);
			// NOTE: inet_pton writes the address in network byte order already, so unlike the
			// port there is no conversion to do here.
			socket_desc.sin_addr = parsed_address;
			res = ::bind(_handle, (sockaddr*)&socket_desc, sizeof(socket_desc));
			if (res != 0)
			{
				return Err("Failed to bind the socket to port: '{}' and address '{}': {}", port, ip_address, WSAGetLastError());
			}

			return {};
		}

		// Turns a bound socket into a listening one
		// backlog caps how many completed-but-unaccepted connections that queue may hold. Past
		// it the OS refuses new arrivals
		Result<void> listen(int backlog = SOMAXCONN)
		{
			if (::listen(_handle, backlog) != 0)
			{
				return Err("Failed to listen on the socket: {}", WSAGetLastError());
			}

			return {};
		}
	};
};
