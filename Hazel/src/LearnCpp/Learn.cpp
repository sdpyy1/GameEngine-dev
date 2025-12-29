#include "hzpch.h"
#include "Learn.h"
#define print(content) do { std::cout << content << std::endl; } while(0)

class classA {
public:
    classA()
    {
        std::cout << "classA 构造" << std::endl;
    }
	~classA()
	{
		std::cout << "classA 析构" << std::endl;
	}
};
////////////////////////////////////////////////////// 内存对齐 //////////////////////////////////////////////////////
struct structA {
	char a;
	uint32_t b;
	char c;
}; // sizeof = 12
struct structB {
	char a;
	char c;
	uint32_t b;
}; // sizeof = 8   a和c可以放在同一个4字节内
struct structC {
	double a;
	char c;
	uint32_t b;
}; // sizeof =16, 8字节对齐，c占4字节，b占4字节组成第二个0字节

// 手动指定内存对齐
struct alignas(8) structD {
	double a;
	char c;
	uint32_t b;
};

// 这样写可以指定每个字段占用的位数
struct Info {
	uint16_t a : 1;   // 1位
	uint16_t b : 2;   // 2位
	uint16_t c : 2;   // 2位
	uint16_t d : 3;   // 3位
	uint16_t e : 1;   // 1位
	uint16_t pad : 7; // 填充位
};

////////////////////////////////////////////////////// 指针与引用 //////////////////////////////////////////////////////
int* ptr() {
	int a = 10;
	int * ptr = &a;
	return ptr;
}
int& Ref() {
	int a = 10;
	int& ptr = a;
	return ptr;
}
int* newAndDelete()
{
	int* p = new int(10);
	int * mp = (int *)malloc(sizeof(int));
	return p;
}
void PtrAndRef()
{
	int a = 10;
	int& ref = a; // 引用相当于原始数据的别名
    std::cout << a << std::endl;
	int c = 20;
	ref = c;
	std::cout <<"给引用ref赋值，相当于给原始a赋值" << a << std::endl;


	int* p = ptr();
	std::cout <<"指向栈的指针，会出问题" << *p << std::endl;
	int& r = Ref();
	std::cout <<"指向栈的引用，也会出问题，因为本质也是指针" << r << std::endl;


	classA* classptr = (classA*)malloc(sizeof(classA));
	print(sizeof(classptr));
	classptr->classA::classA(); // malloc定义的类对象需要手动调用构造函数
	//free(classptr); // 这样写也不会调用析构函数
	delete classptr; // delete会调用析构函数

}
////////////////////////////////////////////////////// 指针常量与常量指针 //////////////////////////////////////////////////////
void PtrConstAndConstPtr()
{
	int a = 10;
	int b = 20;
	// const修饰指针，常量指针，可以修改指向，但不能修改内容
	const int* ptr = &a;
	ptr = &b; // 可以
	//*ptr = 20; // 不行

	// const修饰变量，指针常量，可以修改内容，但不能修改指针
	int* const ptr2 = &a;
	// ptr2 = &b;  不行
	*ptr2 = 20;
    std::cout << *ptr2 << std::endl;
}


void LearnClass::LearnEntryPoit()
{

	/*std::cout << sizeof(structD) << std::endl;
	std::cout << alignof(structD) << std::endl;*/
	// PtrAndRef();
    PtrConstAndConstPtr();
}
