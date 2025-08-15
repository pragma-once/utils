#define PRAGMAONCE_EVENT_NAMESPACE_OVERRIDE my_namespace

#include "event.h"

#include <iostream>

int main()
{
    my_namespace::Event event;
    auto handler = event.listen([]() { std::cout << "Test.\n"; });
    event.call();
    return 0;
}
