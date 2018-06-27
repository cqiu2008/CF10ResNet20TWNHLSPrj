#include "myostream.hpp"

myostream& operator<<(myostream& os, std::string str){
//    printf("%s\n",str.c_str());
	return os;
}

myostream& operator<<(myostream& os, int val){
//    printf("%d\n",val);
	return os;
}
myostream& operator<<(myostream& os, myostream& (*func)(void)){
	return os;
}
myostream myos;
myostream& endl(){return myos;}
int setw1(int a){return a;}
