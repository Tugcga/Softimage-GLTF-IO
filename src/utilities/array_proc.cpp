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