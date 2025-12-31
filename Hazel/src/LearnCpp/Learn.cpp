#include "hzpch.h"
#include "Learn.h"
#include <map>
#define print(content) do { std::cout << content << std::endl; } while(0)
////////////////////////////////////////////////////// MISC //////////////////////////////////////////////////////
void arrayTest(int arr[])
{ 
	// arr当作函数传入后，退化为指针，所以sizeof = 8
	printf("sizeof(arr)=%zu\n", sizeof(arr));
	arr[0] = 100;
    arr[1] = 200;
    arr[2] = 300;
	*arr = 1000;
	print(arr[0] << " " << arr[1] << " " << arr[2]);
}



////////////////////////////////////////////////////// Class //////////////////////////////////////////////////////
class classA {
public:
	
    classA():b(999)
    {
		print(a);
        std::cout << "classA 构造" << std::endl;
    }
	classA(classA& other){
		std::cout << "classA 拷贝构造" << std::endl;
	}
	~classA()
	{
		std::cout << "classA 析构" << std::endl;
	}

	 void func() {
        print(a);
	}
	 int constFunc() const {
		//func();  // 不允许在const函数中调用非const函数
		//b = 10; // 不允许修改变量
		c = 2000; // 除非这个变量被标记为mutable
        print(b); // 但是可以使用普通变量
		return b;
	}

	const int a = 20; 
	int b;  // 这种数据不会自动执行初始化
	mutable int c = 10000;
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
// struct也能继承，但默认是public
//struct parent {
//
//};
//struct child : parent {
//
//};
////////////////////////////////////////////////////// Static //////////////////////////////////////////////////////
static int a; // 会默认初始化为0
void staticFunc()
{
	static int staticNum = 10;  // 多次调用函数只会初始化一次，一直存在
	staticNum++;
	std::cout << staticNum << std::endl;
}
////////////////////////////////////////////////////// Const //////////////////////////////////////////////////////
const int b = 7770;
void constVarFunc(const int a) {  // 可以传递const 也可以不是const
	print(a);
}






////////////////////////////////////////////////////// STL //////////////////////////////////////////////////////

void MapLearn() {
	std::map<int, int> IntMap;
	print(IntMap[1]);
}

////////////////////////////////////////////////////// 构造函数 //////////////////////////////////////////////////////

class SomeClass {
public:
	SomeClass() {
		print("SomeClass构造");
	}
	SomeClass(const SomeClass& other) {
		print("SomeClass拷贝构造");
	}
};
class Base {
public:
	Base() {
		print("Base构造");
	}
	Base(const Base& other) {
		print("Base拷贝构造");
	}
};
class Person :public Base {
public:
	// Person() = default; // 因为定义了下面这个有参构造，如果不手动指定默认构造，默认构造类就会编译器报错，这里就是提示一下默认构造依旧存在
	Person() {
		print("Person构造");
	}

	// 委托构造，委托别的构造器执行
	Person(int a) :Person(12, "aaa") {
		print("Person委托构造");
	};

	// 移动构造
	Person(Person&& other)
		: Base(std::move(other)),
		someClass(std::move(other.someClass)),
		age(other.age),
		name(std::move(other.name))
	{
		print("Person移动构造");
	}

	// 拷贝构造，仍然需要在列表初始化指定成员的拷贝构造，否则还是会走默认构造先构造出对象
	Person(const Person& other):someClass(other.someClass) { // 如果并没有列表初始化SomeClass，someClass会执行默认构造行为
		// 如果没在列表初始化someclass，那someclass以及经过默认构造构造好了，这里都是赋值行为
		//this->someClass = other.someClass;  // 这种写法是错的，他是赋值操作，并且是先执行了someClass的默认构造，然后又进行赋值操作
		//this->someClass = SomeClass(other.someClass);// 这种写法虽然能够执行someClass的构造函数，但是是先执行了拷贝构造了一个临时对象，又执行了赋值
		print("Person拷贝构造");
	}
	Person(int age, std::string name) {
		this->age = age;
		this->name = name;
	}
	SomeClass someClass;
	int age;
	std::string name;

};
void someFuncNeedPerson(Person p) {
	print(p.age);
}
////////////////////////////////////////////////////// std::move //////////////////////////////////////////////////////
class Product {
public:
	Product() {
		print("默认构造");
	}
	Product(const Product & other) {
		print("拷贝构造");
	}
	Product(Product&& other) {
		print("移动构造");
	}
	explicit Product(int a) {
		// explicit会阻止隐式转换
		// funcNeedProduct(10);   比如这里需要product但是传入10，隐式转换就会走这个构造，添加了explicit就不行了

	}
};
template<typename... Args>
std::unique_ptr<Product> createProduct(Args&&... args) {
	return std::make_unique<Product>(std::forward<Args>(args)...);
}
void funcNeedProduct(Product a) {

}
////////////////////////////////////////////////////// 多线程 //////////////////////////////////////////////////////
bool flag = false;
void worker() {
	int i = 0;
	std::cout << "Worker thread started, waiting for flag...\n";
	// 如果 flag 被优化成寄存器缓存，可能永远看不到更新
	while (!flag) {
		// do nothing
		print(i++);
	}
	std::cout << "Worker thread detected flag = true!\n";
}
void volatileUseFunc() {
	std::thread t(worker);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "Main thread sets flag = true\n";
	flag = true;  // 修改 flag
	t.join();
	flag = false;
}

void LearnClass::LearnEntryPoit()
{

	/*std::cout << sizeof(structD) << std::endl;
	std::cout << alignof(structD) << std::endl;*/
	// PtrAndRef();
 //   PtrConstAndConstPtr();
	//print(a);
 //   staticFunc();
 //   staticFunc();
 //   staticFunc();
 //   staticFunc();
 //   staticFunc();
	//int ss = 10;
	//int ss = 10;
	//constVarFunc(ss);
	//classA a;
	//int b = a.constFunc();
	//int arr[] = { 1,2,3 };
	//printf("原数组sizeof(arr)=%zu\n", sizeof(arr));
	//arrayTest(arr);   // 传入arr退化为指针，所以函数内的修改会影响原数组
	//print(arr[0] << " " << arr[1] << " " << arr[2]);


	//classA a;	
	//classA b = a;
	//classA c;
	//// c = a; // 这种赋值方式，并不是调用拷贝构造函数，而是需要重载=后才能用
	//print(b.b);

	// MapLearn();
	//Person p(12);
	//print(p.age);
	//print(p.name);
	//Person p1 = p;
	
	//Person p;
	//Person p1 = std::move(p);
	//std::vector<Test>v;
	//v.emplace_back(Test{});
	//std::cout << &v[0] << '\n';
	//std::vector<Test> v2{ std::move(v) };
	//std::vector<Test> v3{ v2 };

	//std::cout << &v2[0] << '\n';

	//std::cout << &v3[0] << '\n';
	//Person p;
	//print("-------------------------------");
	//someFuncNeedPerson(p);

	//std::string str = "Hello, World!";
	//std::string anotherStr = std::move(str);

	//std::cout << "str: " << str << std::endl;  // 此时str的内容可能为空，因为资源已被移动
	//std::cout << "anotherStr: " << anotherStr << std::endl;

	//Product p;
	//auto p1 = createProduct(p);
	//auto p2 = createProduct(std::move(p));
	//auto p3 = createProduct(Product());


	//int* ptr = new int(10);
	//std::cout << ptr << std::endl;

	/*int a = 0x1234;
	char c = (char)(a);
	if (c == 0x12) {
		print("??");
	}
	else if(c == 0x34) {
		print("aaaaaaaaaaaaa");

	}*/
}
