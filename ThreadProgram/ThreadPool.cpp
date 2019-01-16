#include "ThreadPool.h"
#include <iostream>
using namespace std;

void fun1(int slp)
{
    cout << "hello fun1 !" << " thread id= " << std::this_thread::get_id();

    if (slp > 0)
    {
        cout << "===fun1 sleep ========", slp, std::this_thread::get_id();
        std::this_thread::sleep_for(std::chrono::milliseconds(slp));
    }
}

void fun2()
{
    cout << "hello fun2 !" << " thread id= " << std::this_thread::get_id();
}

struct gfun
{
    int operator()(int n)
    {
        cout << "hello, gfun!" << n << std::this_thread::get_id();
        return 42;
    }
};


class A
{
public:
    A() {};
    ~A() {};

    static int Afun(int n = 0)
    {
        std::cout << n << "  hello ,Afun!" << std::this_thread::get_id() << endl;
        return n;
    }

    static std::string Bfun(int n, std::string str, char c)
    {
        std::cout << n << " hello Bfun!" << str.c_str() << "  " << (int)c
            << std::this_thread::get_id() << endl;
        return str;
    }

};

int main()
{
    cout << std::thread::hardware_concurrency();
    try
    {
        std::threadpool executor{ 50 };
        A a;
        std::future<void> ff = executor.commit(fun1, 0);
        std::future<int> fg = executor.commit(gfun{}, 0);
        std::future<int> gg = executor.commit(a.Afun, 9999);
        std::future<std::string> gh = executor.commit(A::Bfun, 9998, "mult args", 123);
        std::future<std::string> fh = executor.commit([]()->std::string {std::cout << "hello, fh" 
            << std::this_thread::get_id() << std::endl; return "hello, fh ret!"; });

        std::cout << "======sleep=======" << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::microseconds(900));

        for (int i = 0; i < 50; i++)
        {
            executor.commit(fun1, i * 100);
        }
        std::cout << "====commit all======" << std::this_thread::get_id() << "idle size" << executor.idleCount() << std::endl;


        ff.get();
        std::cout << fg.get() << " " << fh.get().c_str() << " " << std::this_thread::get_id() << std::endl;
        std::cout << " =======  sleep ========= " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << " =======  fun1,55 ========= " << std::this_thread::get_id() << std::endl;
        executor.commit(fun1, 55).get();    //����.get()��ȡ����ֵ��ȴ��߳�ִ����
        std::cout << "end... " << std::this_thread::get_id() << std::endl;

        std::threadpool pool(4);

        std::vector<std::future<int>> results;

        for (int i = 0; i < 8; i++)
        {
            results.emplace_back(
                pool.commit([i]
            {
                std::cout << "hello" << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "word" << i << std::endl;
                return i * i;
            }
                )
            );
        }
        std::cout << " =======  commit all2 ========= " << std::this_thread::get_id() << std::endl;

        for (auto && result:results)
        {
            std::cout << result.get() << ' ';
        }
        std::cout << std::endl;

        return 0;

    }
    catch (const std::exception& e)
    {
        std::cout << "some unhappy happened..." << std::this_thread::get_id() << e.what() << std::endl;
    }
}