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

#if defined(_MSC_VER)
	// Windows (MSVC): 64-bit support + No-Lock optimizations since HSTL is not thread-safe anyway
	#define HSTL_FREAD(buf, sz, cnt, f)      _fread_nolock(buf, sz, cnt, f)
	#define HSTL_FWRITE(buf, sz, cnt, f)     _fwrite_nolock(buf, sz, cnt, f)
	#define HSTL_FSEEK(f, off, origin)       _fseeki64_nolock(f, static_cast<long long>(off), origin)
	#define HSTL_FTELL(f)                    _ftelli64_nolock(f)
	#define HSTL_FERROR(f)                   ferror(f)

#elif defined(__GNUC__) || defined(__clang__)
	#define HSTL_FREAD(buf, sz, cnt, f)      fread_unlocked(buf, sz, cnt, f)
	#define HSTL_FWRITE(buf, sz, cnt, f)     fwrite_unlocked(buf, sz, cnt, f)
	#define HSTL_FSEEK(f, off, origin)       fseeko(f, static_cast<off_t>(off), origin)
	#define HSTL_FTELL(f)                    ftello(f)
	#define HSTL_FERROR(f)                   ferror(f)

#else
	// Standard C Fallback (Safe but slower, 2GB limit on Windows)
	#define HSTL_FREAD(buf, sz, cnt, f)      fread(buf, sz, cnt, f)
	#define HSTL_FWRITE(buf, sz, cnt, f)     fwrite(buf, sz, cnt, f)
	#define HSTL_FSEEK(f, off, origin)       fseek(f, static_cast<long>(off), origin)
	#define HSTL_FTELL(f)                    ftell(f)
	#define HSTL_FERROR(f)                   ferror(f)
#endif

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

			// NOTE: It seems that the defaul internal buffer that cstdio uses is a bit too small?
			// testing the API with writing 4KB chucks to a file had a terrible performance ~490ms compared
			// to ~430ms for fstream which was weird, after setting the internal buffer's size to 64KB the time dropped
			// to ~70ms which is mind blowing.
			setvbuf(file_handle, nullptr, _IOFBF, 65536);

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

			size_t read_amount = HSTL_FREAD(buffer, 1u, size, handle);

			if (HSTL_FERROR(handle) != 0)
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

			size_t written_amount = HSTL_FWRITE(buffer, 1u, size, handle);

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
			if (handle == nullptr)
			{
				return Err("The file is invalid (closed or moved)");
			}

			int64_t original_position = HSTL_FTELL(handle);
			if (original_position == -1)
			{
				return Err("Failed to get the cursor position");
			}

			if (HSTL_FSEEK(handle, offset, SEEK_SET) != 0)
			{
				return Err("Failed to seek to the desired offset");
			}

			auto read_result = read(buffer, size);
			if (HSTL_FSEEK(handle, original_position, SEEK_SET) != 0)
			{
				return Err("Failed to restore cursor position");
			}

			return read_result;
		}

		// Will write `size` bytes starting from the offset
		Result<size_t> write_at(const void* buffer, size_t size, size_t offset)
		{
			if (handle == nullptr)
			{
				return Err("The file is invalid (closed or moved)");
			}

			int64_t original_position = HSTL_FTELL(handle);
			if (original_position == -1)
			{
				return Err("Failed to get the cursor position");
			}

			if (HSTL_FSEEK(handle, offset, SEEK_SET) != 0)
			{
				return Err("Failed to seek to the desired offset");
			}

			auto write_result = write(buffer, size);
			if (HSTL_FSEEK(handle, original_position, SEEK_SET) != 0)
			{
				return Err("Failed to restore cursor position");
			}

			return write_result;
		}

		// Cursor Management //

		// Returns the current cursor position in bytes
		Result<size_t> tell() const
		{
			if (handle == nullptr)
			{
				return Err("The file is invalid (closed or moved)");
			}

			int64_t pos = HSTL_FTELL(handle);
			if (pos == -1)
			{
				return Err("Failed to get the cursor position");
			}

			return static_cast<size_t>(pos);
		}

		// Moves the cursor to the specified absolute offset
		// Returns the new offset on success
		Result<size_t> seek(size_t offset)
		{
			if (handle == nullptr)
			{
				return Err("The file is invalid (closed or moved)");
			}

			if (HSTL_FSEEK(handle, offset, SEEK_SET) != 0)
			{
				return Err("Failed to seek to the desired offset");
			}

			return offset;
		}

		// Utils //

		// Returns the size of the file in bytes
		Result<size_t> size() const
		{
			if (handle == nullptr)
			{
				return Err("The file is invalid (closed or moved)");
			}

			int64_t original_position = HSTL_FTELL(handle);
			if (original_position == -1)
			{
				return Err("Failed to get the cursor position");
			}

			if (HSTL_FSEEK(handle, 0, SEEK_END) != 0)
			{
				return Err("Failed to seek to the end of file");
			}

			size_t file_size = HSTL_FTELL(handle);
			if (file_size == -1)
			{
				return Err("Failed to get the file size");
			}

			if (HSTL_FSEEK(handle, original_position, SEEK_SET) != 0)
			{
				return Err("Failed to restore cursor position");
			}

			return file_size;
		}

		// Will read the entire content of the file
		Result<Array<uint8_t>> read_all(Allocator* allocator = Default_Allocator::get())
		{
			auto file_size_res = size();
			if (!file_size_res)
			{
				return Err(file_size_res.get_err());
			}

			Array<uint8_t> data{allocator};
			data.resize(file_size_res.get_value());

			auto read_result = read_at(data.buffer(), data.count(), 0u);
			if (!read_result)
			{
				return Err(read_result.get_err());
			}

			if (read_result.get_value() != file_size_res.get_value())
			{
				return Err("File read incomplete (Unexpected EOF)");
			}

			return data;
		}
	};
};
