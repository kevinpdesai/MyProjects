#include "global.h"
#include "timer.h"

int main()
{
	timer *t = new timer();
	t->stopTimer();
	cout << endl << t->getTimeTaken() << " ms\n";
#ifdef windows
	_getch();
#endif // windows

	return 0;
}