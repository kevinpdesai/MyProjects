#pragma once
#include "global.h"

class timer
{
#ifdef windows
	DWORD dw1, dw2;
#endif // windows

#ifdef linux
	struct timeval start, end;
	long mtime, seconds, useconds;
#endif

public:

	timer()
	{
		startTimer();
	}

	virtual ~timer(void)
	{
	}

	void startTimer()
	{
#ifdef windows
		dw1 = GetTickCount();
#endif // windows

#ifdef linux
		gettimeofday(&start, NULL);
#endif // linux
	}

	void stopTimer() 
	{
#ifdef windows
		dw2 = GetTickCount();	
#endif

#ifdef linux
		gettimeofday(&end, NULL);
#endif
	}

	long getTimeTaken()
	{
#ifdef windows
		return dw2-dw1;
#endif // windows

#ifdef linux
		seconds  = end.tv_sec  - start.tv_sec;
		useconds = end.tv_usec - start.tv_usec;
		mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
		return mtime;
#endif // linux
	}
};

