# Event

A simple and elegant header-only event class for C++.

## Usage

The test files contain some example usages.

Include:

```
#include "path/to/event.h"
```

Create an event with no parameters:

```
Event<> my_event_with_no_params;
```

Create an event with an int and a float parameter:

```
Event<int, float> my_event;
```

A listen only interface to the event above.

```
EventView<int, float> my_event_view(my_event);
```

Handle `my_event`:

```
Event<int, float>::Listener my_listener; // Keep the object to listen.
// Alternatively:
//std::shared_ptr<EventListener<int, float>> my_listener;
...
my_listener = my_event.listen([](int a, float b) { /* do something */ });
...
my_listener = nullptr; // Stop listening manually by destructing the listener,
                      // or it could be destructed as a member of a class automatically.
```

Fire event:

```
my_event.call(1, 0.5);
```

alternatively:

```
my_event.emit(1, 0.5);
// or
my_event.invoke(1, 0.5);
// or
my_event(1, 0.5);
```

A practical case as class member:

```
class MyClass
{
public:
    EventView<> something_happened;
private:
    Event<> something_happened_event;
public:
    MyClass() : something_happened(something_happened_event)
    {
    }

    void something()
    {
        ...
        something_happened_event();
    }
};

...
MyClass my_object;
...
my_listener = my_object.something_happened.listen([]() {...});
...
```
