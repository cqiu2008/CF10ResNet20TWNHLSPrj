#ifndef __MYOSTREAM__H__
#define __MYOSTREAM__H__

#include <cstdio>
#include <string>

class myostream{
public:
    friend myostream& operator<<(myostream& os,std::string str);
    friend myostream& operator<<(myostream& os,int val);
    friend myostream& operator<<(myostream& os,myostream& (*func)(void));
};



extern myostream myos;
myostream& endl();
int setw1(int a);


#endif
