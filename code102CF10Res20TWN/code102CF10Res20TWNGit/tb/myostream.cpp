#include "myostream.hpp"

const myostream& myostream::operator<<(int value)const{
    return *this;
}
const myostream& myostream::operator<<(char* str)const{
    return *this;
}

const myostream& myostream::operator<<(const myostream& (*fun)(const myostream&))const{
#if 1
	fun(*this);
	return *this;
#else
	return (*fun)(*this);
#endif
}

const myostream& endl(const myostream& mo){
    return mo;
}

myostream myout;
