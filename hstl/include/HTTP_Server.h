#pragma once

#include "Str.h"
#include "Array.h"
#include "Memory.h"
#include "Socket.h"

#include <cctype>
#include <cstdint>
#include <utility>

namespace hstl
{
	enum class HTTP_METHOD : uint8_t
	{
		HTTP_GET,
		HTTP_POST,
		HTTP_PUT,
		HTTP_DELETE,
		HTTP_HEAD,
		HTTP_PATCH,
		HTTP_OPTIONS,
		HTTP_UNKNOWN
	};

	enum class HTTP_VERSION : uint8_t
	{
		HTTP_1_0,
		HTTP_1_1,
		HTTP_UNKNOWN
	};

	enum class HTTP_STATUS_CODE : uint16_t
	{
		HTTP_200 = 200,
		HTTP_201 = 201,
		HTTP_204 = 204,

		HTTP_301 = 301,
		HTTP_304 = 304,

		HTTP_400 = 400,
		HTTP_403 = 403,
		HTTP_404 = 404,
		HTTP_405 = 405,
		HTTP_408 = 408,
		HTTP_413 = 413,
		HTTP_414 = 414,
		HTTP_431 = 431,

		HTTP_500 = 500,
		HTTP_501 = 501,
		HTTP_505 = 505
	};

	struct HTTP_Header
	{
		Str_View name;
		Str_View value;
	};

	class HTTP_Request
	{
	private:
		Str_View _target;
		Str_View _body;
		Array<HTTP_Header> _headers;
		HTTP_METHOD _method{HTTP_METHOD::HTTP_UNKNOWN};
		HTTP_VERSION _version{HTTP_VERSION::HTTP_UNKNOWN};

	public:
		HTTP_Request(Allocator* allocator):
			_headers{allocator} {}

	public:
		void set_target(const char* start, size_t length) { _target = Str_View{start, length}; }
		void set_body(const char* start, size_t length) { _body = Str_View{start, length}; }
		void set_header(const HTTP_Header& header) { _headers.push(header); }
		void set_method(HTTP_METHOD method) { _method = method; }
		void set_version(HTTP_VERSION version) { _version = version; }

	public:
		Str_View target() const { return _target; }
		Str_View body() const { return _body; }
		HTTP_METHOD method() const { return _method; }
		HTTP_VERSION version() const { return _version; }
		const Array<HTTP_Header>& headers() const { return _headers; }

	public:
		const HTTP_Header* find_header(Str_View name) const
		{
			for (const auto& header : _headers)
			{
				if (header.name.count() != name.count())
					continue;

				bool matched = true;
				for (size_t i = 0; i < name.count(); ++i)
				{
					// NOTE: tolower takes an int and is UB on a negative char, so the bytes
					// are widened through unsigned char first.
					auto lhs = tolower(static_cast<unsigned char>(header.name[i]));
					auto rhs = tolower(static_cast<unsigned char>(name[i]));

					if (lhs != rhs)
					{
						matched = false;
						break;
					}
				}

				if (matched)
				{
					return &header;
				}
			}

			return nullptr;
		}
	};

	class HTTP_Response
	{
	private:
		Str_View _body;
		Array<HTTP_Header> _headers;
		HTTP_STATUS_CODE _status_code{HTTP_STATUS_CODE::HTTP_200};

	public:
		HTTP_Response(Allocator* allocator):
			_headers{allocator} {}

	public:
		void set_status_code(HTTP_STATUS_CODE status_code) { _status_code = status_code; }
		void set_body(Str_View body) { _body = body; }
		void set_header(const HTTP_Header& header) { _headers.push(header); }

	public:
		Str_View body() const { return _body; }
		HTTP_STATUS_CODE status_code() const { return _status_code; }
		const Array<HTTP_Header>& headers() const { return _headers; }
	};

	using HTTP_Handler = HTTP_Response (*)(const HTTP_Request&);

	struct HTTP_Route
	{
		Str_View path;
		HTTP_Handler handler;
		HTTP_METHOD method;
	};

	class HTTP_Server
	{
	private:
		Socket _listener;
		Array<HTTP_Route> _routes;
		bool _owns_winsock{false};

	private:
		HTTP_Server(Socket&& listener, Allocator* allocator):
			_listener{std::move(listener)},
			_routes{allocator},
			_owns_winsock{true}
		{

		}

	public:
		HTTP_Server(const HTTP_Server&) = delete;
		HTTP_Server& operator=(const HTTP_Server&) = delete;

		HTTP_Server(HTTP_Server&& source):
			_listener{std::move(source._listener)},
			_routes{std::move(source._routes)},
			_owns_winsock{source._owns_winsock}
		{
			source._owns_winsock = false;
		}

		HTTP_Server& operator=(HTTP_Server&& source)
		{
			if (this == &source)
			{
				return *this;
			}

			if (_owns_winsock)
			{
				WSACleanup();
			}

			_listener = std::move(source._listener);
			_routes = std::move(source._routes);
			_owns_winsock = source._owns_winsock;

			source._owns_winsock = false;

			return *this;
		}

		~HTTP_Server()
		{
			if (_owns_winsock)
			{
				WSACleanup();
				_owns_winsock = false;
			}
		}

	public:
		static Result<HTTP_Server> create(uint16_t port, Str_View ip_address = "0.0.0.0" /*must be null-terminated*/, Allocator* allocator = Default_Allocator::get())
		{
			WSAData winsock_data{};

			const int startup_error = WSAStartup(MAKEWORD(2, 2), &winsock_data);
			if (startup_error != 0)
			{
				return Err("Failed to initialize Winsock: {}", startup_error);
			}

			auto listener_res = Socket::create();
			if (!listener_res)
			{
				WSACleanup();
				return listener_res.get_err();
			}

			Socket listener = std::move(listener_res.get_value());

			auto bind_res = listener.bind(port, ip_address);
			if (!bind_res)
			{
				WSACleanup();
				return bind_res.get_err();
			}

			auto listen_res = listener.listen();
			if (!listen_res)
			{
				WSACleanup();
				return listen_res.get_err();
			}

			return HTTP_Server{std::move(listener), allocator};
		}

	public:
		void add_route(HTTP_METHOD method, Str_View path, HTTP_Handler handler)
		{
			assert(handler != nullptr);
			assert(path.count() > 0);

			_routes.push(HTTP_Route{path, handler, method});
		}

		const HTTP_Route* find_route(HTTP_METHOD method, Str_View path) const
		{
			for (const auto& route : _routes)
			{
				if (route.method != method)
				{
					continue;
				}

				if (route.path == path)
				{
					return &route;
				}
			}

			return nullptr;
		}

		const Array<HTTP_Route>& routes() const { return _routes; }

	public:
		// TODO: accept connections and serve each one to completion.
		Result<void> run()
		{
			return {};
		}
	};
};
