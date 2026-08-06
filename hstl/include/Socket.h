#pragma once

#include <WinSock2.h>

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
	};
};
