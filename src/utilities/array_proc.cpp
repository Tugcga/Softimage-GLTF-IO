#include <vector>

#include "utilities.h"

bool is_arrays_coincide(const std::vector<float>& array_a, const std::vector<float>& array_b)
{
	if (array_a.size() != array_b.size())
	{
		return false;
	}

	for (ULONG i = 0; i < array_a.size(); i++)
	{
		if (abs(array_a[i] - array_b[i]) > EPSILON)
		{
			return false;
		}
	}

	return true;
}

bool is_array_contains(const size_t value, const std::vector<size_t>& array)
{
	for (size_t i = 0; i < array.size(); i++)
	{
		if (array[i] == value)
		{
			return true;
		}
	}

	return false;
}