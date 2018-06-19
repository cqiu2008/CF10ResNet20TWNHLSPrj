#ifndef __MYOSTREAM__H__
#define __MYOSTREAM__H__

#include <stdio.h>
class myostream{
public:
	const myostream& operator<<(int value)const;
	const myostream& operator<<(char* str)const;
	const myostream& operator<<(const myostream& (*fun)(const myostream&))const;
};

const myostream& endl(const myostream& mo);
extern myostream myout;
#endif
