#pragma once

#include <windows.h>

#include <stdint.h>

namespace Nutcrackz {

	namespace Utils {

		struct QPCFreq
		{
			QPCFreq()
			{
				LARGE_INTEGER freq;
				QueryPerformanceFrequency(&freq);
				InvertedFrequency = 1e9 / double(freq.QuadPart);
			}

			double InvertedFrequency;
		} static StaticQPCFrequency;

		int64_t GetCurrentTimeNanoSecs()
		{
			LARGE_INTEGER li;
			if (!QueryPerformanceCounter(&li))
				return 0;
			return int64_t(double(li.QuadPart) * StaticQPCFrequency.InvertedFrequency);
		}

	}

}
