#pragma once

#include "Str.h"
#include "Result.h"
#include "Array.h"
#include "Memory.h"

#include <cstdio>

namespace hstl
{
	// I/O File handling
	// Files are open in __binary mode__ no support for __ASCII mode__

	class File
	{
	private:
		FILE* handle{nullptr};

	private:
		File() = default;

	public:
		File(const File&) = delete;
		File& operator=(const File&) = delete;

		File(File&& source):
			handle{source.handle}
		{
			source.handle = nullptr;
		}

		File& operator=(File&& source)
		{
			if (this == &source)
			{
				return *this;
			}

			if (handle)
			{
				fclose(handle);
			}

			handle = source.handle;

			source.handle = nullptr;

			return *this;
		}

		~File()
		{
			if (handle)
			{
				fclose(handle);
			}
		}

	public:
		enum class FILE_OPEN_MODE : uint8_t
		{
			// Read-only access
			READ,
			// Write-only. Truncates (overwrites) file. Creates if missing.
			WRITE_OVERWRITE,
			// Write-only. Appends to end. Creates if missing.
			WRITE_APPEND,
			// Read & Write. Preserves content. Fails if file doesn't exist.
			READ_WRITE_PRESERVE,
			// Read & Write. Truncates (overwrites) file. Creates if missing.
			READ_WRITE_OVERWRITE

		};

		// Opens the provided file with the provided mode
		static Result<File> open(Str_View filepath, FILE_OPEN_MODE mode)
		{

		}

		// Sequential I/O (Updates internal cursor) //

		// Will read `size` bytes starting from the current cursor position
		Result<size_t> read(void* buffer, size_t size)
		{

		}

		// Will write `size` bytes starting from the current cursor position
		Result<size_t> write(const void* buffer, size_t size)
		{

		}

		// Random Access I/O (Doesn't update internal cursor) //

		// Will read `size` bytes starting the offset
		Result<size_t> read_at(void* buffer, size_t size, size_t offset)
		{

		}

		// Will write `size` bytes starting from the offset
		Result<size_t> write_at(const void* buffer, size_t size, size_t offset)
		{

		}

		// Cursor Management //

		Result<size_t> tell() const
		{

		}

		Result<size_t> seek(size_t offset)
		{

		}

		// Utils //

		// Will read the entire content of the file
		Result<Array<uint8_t>> read_all(Allocator* allocator = Default_Allocator::get())
		{

		}

		// Returns the size of the file in bytes
		Result<size_t> size() const
		{

		}
	};
};
