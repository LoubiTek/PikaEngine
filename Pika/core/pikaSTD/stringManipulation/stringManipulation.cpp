#include "stringManipulation.h"
#include <cctype>

namespace pika
{

	void removeCharacters(char *dest, const char *source, const char *charsToRemove, size_t destSize)
	{
		int write = 0;
		for (int read = 0; source[read] != 0; read++)
		{
			if (findChar(charsToRemove, source[read]))
			{

			}
			else
			{
				dest[write] = source[read];
				write++;

				if (write >= destSize) { break; }
			}
		}
	}

	void toLower(char *dest, const char *source, size_t size)
	{
		for (int i = 0; i < size; i++)
		{
			if (dest[i] == 0) { break; }
			dest[i] = std::tolower(source[i]);
		}
	}

	void toUpper(char *dest, const char *source, size_t size)
	{
		for (int i = 0; i < size; i++)
		{
			if (dest[i] == 0) { break; }
			dest[i] = std::toupper(source[i]);
		}
	}

	bool findChar(const char *source, char c)
	{
		int i = 0;
		while (source[i] != 0)
		{
			if (source[i] == c)
			{
				return true;
			}
			i++;
		}
		return false;
	}

	size_t strlcpy(char *dst, const char *src, size_t size)
	{

		for (size_t i = 0; i < size-1; i++)
		{
			dst[i] = src[i];

			if (src[i] == '\0') { return i; }
		}

		dst[size - 1] = '\0';

		return size - 1;
	}

	size_t strlcpy(char *dst, std::string src, size_t size)
	{
		return strlcpy(dst, src.c_str(), size);
	}

	std::vector<std::string> split(const char *source, char c)
	{

		std::string s = "";
		std::vector<std::string> ret;

		for (int i = 0; source[i] != 0; i++)
		{
			if (source[i] == c)
			{
				if (s != "")
				{
					ret.push_back(s);
				}
				s = "";
			}
			else
			{
				s += source[i];
			}
		}

		if (s != "")
		{
			ret.push_back(s);
		}

		return ret;
	}

}