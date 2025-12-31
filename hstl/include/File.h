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
		File(FILE* handle):
			handle{handle}
		{

		}

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
				handle = nullptr;
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
		// file_path should be null-terminated
		static Result<File> open(Str_View file_path, FILE_OPEN_MODE mode)
		{
			const char* mode_str = nullptr;

			switch (mode)
			{
			case FILE_OPEN_MODE::READ:
				mode_str = "rb";
				break;
			case FILE_OPEN_MODE::WRITE_OVERWRITE:
				mode_str = "wb";
				break;
			case FILE_OPEN_MODE::WRITE_APPEND:
				mode_str = "ab";
				break;
			case FILE_OPEN_MODE::READ_WRITE_PRESERVE:
				mode_str = "rb+";
				break;
			case FILE_OPEN_MODE::READ_WRITE_OVERWRITE:
				mode_str = "wb+";
				break;
			}

			FILE* file_handle = fopen(file_path.data(), mode_str);

			if (file_handle == nullptr)
			{
				return Err("Failed to open the following file '{}'", file_path);
			}

			File file{file_handle};

			return file;
		}

		// Sequential I/O (Updates internal cursor) //

		// Will read `size` bytes starting from the current cursor position and advance it by
		// the read amount, if the read wasn't successful the cursor position is indeterminate
		Result<size_t> read(void* buffer, size_t size)
		{
			if (handle == nullptr)
			{
				return Err("The file is invalid (closed or moved)");
			}

			// HSTL is not thread-safe so we better use the no-lock version to avoid
			// the cost of locking and unlocking
		#if defined(_MSC_VER)
			size_t read_amount = _fread_nolock(buffer, 1, size, handle);
		#elif defined(__GNUC__) || defined(__clang__)
			size_t read_amount = fread_unlocked(buffer, 1, size, handle);
		#else
			size_t read_amount = fread(buffer, 1, size, handle);
		#endif

			if (ferror(handle) != 0)
			{
				// TODO: Provide a more insightful error message
				return Err("Failed to read from the file");
			}

			// The read amount can be less than the required amount, in that case
			// we've propably hit EOF
			return read_amount;
		}

		// Will write `size` bytes starting from the current cursor position and advance it by
		// the written amount, if the write wasn't successful the cursor position is indeterminate
		Result<size_t> write(const void* buffer, size_t size)
		{
			if (handle == nullptr)
			{
				return Err("The file is invalid (closed or moved)");
			}

			// HSTL is not thread-safe so we better use the no-lock version to avoid
			// the cost of locking and unlocking
		#if defined(_MSC_VER)
			size_t written_amount = _fwrite_nolock(buffer, 1, size, handle);
		#elif defined(__GNUC__) || defined(__clang__)
			size_t written_amount = fwrite_unlocked(buffer, 1, size, handle);
		#else
			size_t written_amount = fwrite(buffer, 1, size, handle);
		#endif

			if (ferror(handle) != 0)
			{
				// TODO: Provide a more insightful error message
				return Err("Failed to write to the file");
			}

			return written_amount;
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
