#include "event.h"

#include <iostream>

class ClassWithEvents
{
private:
    Utilities::Event<> a_called_event;
    Utilities::Event<float, int, bool> b_called_event;
public:
    Utilities::EventView<> a_called;
    Utilities::EventView<float, int, bool> b_called;

    ClassWithEvents()
        : a_called(a_called_event), b_called(b_called_event)
    {}

    void a()
    {
        a_called_event.call();
    }
    void b(float x, int y, bool z)
    {
        b_called_event.call(x, y, z);
    }
};

int main()
{
    {
        Utilities::Event<>::Listener a0; // OK
        //std::shared_ptr<Utilities::EventListener<>> a0; // OK
        {
            ClassWithEvents instance;
            {
                a0 = instance.a_called.listen([]() { std::cout << "A0\n"; });
                auto a1 = instance.a_called.listen([]() { std::cout << "A1\n"; });
                auto b1 = instance.b_called.listen([](float x, int y, bool z) {
                    std::cout << "B1: " << x << ", " << y << ", " << z << " \n";
                });

                std::cout << "[SCOPE 1]\n";

                instance.a();
                instance.b(1.1, 1, false);

                {
                    auto a2 = instance.a_called.listen([]() { std::cout << "A2\n"; });
                    auto b2 = instance.b_called.listen([](float x, int y, bool z) {
                        std::cout << "B2: " << x << ", " << y << ", " << z << " \n";
                    });

                    std::cout << "[SCOPE 2]\n";

                    instance.a();
                    instance.b(2.2, 2, true);
                }

                std::cout << "[SCOPE 1]\n";

                instance.a();
                instance.b(3.3, 3, false);
            }

            std::cout << "[SCOPE 0]\n";

            instance.b(4.4, 4, true);
            instance.a();
            std::cout << "End of scope for the event.\n";
        }
        std::cout << "End of scope for A0 listener.\n";
    }
    std::cout << "Done.\n";
    return 0;
}
