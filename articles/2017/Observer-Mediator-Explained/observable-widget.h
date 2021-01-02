/*
    Observable Widget Library
*/

#include "widget.h"

#ifndef OBSERVABLE_WIDGET
#define OBSERVABLE_WIDGET

template<typename Item>
class ObservableTextBox : public TextBox<Item> {
public:
    ObservableTextBox (const Item &item) : TextBox<Item> (item) {}

    class Observer {
    public:
        // Interface for Observer
        virtual void TextUpdated (const Item &) = 0;
    };
    void SetObserver (std::weak_ptr<Observer> &&p) { _observer = p; }

    // Interface for Invoker
    void OnInput () override {
        if (auto p = _observer.lock ())
            p->TextUpdated (this->GetText ());
    }

private:
    std::weak_ptr<Observer> _observer;
};

template<typename Item>
class ObservableListBox : public ListBox<Item> {
public:
    ObservableListBox (const std::vector<Item> &items,
                       unsigned index = 0)
        : ListBox<Item> (items, index) {}

    class Observer {
    public:
        // Interface for Observer
        virtual void SelectionChanged (const Item &) = 0;
    };
    void SetObserver (std::weak_ptr<Observer> &&p) { _observer = p; }

    // Interface for Invoker
    void OnChange () override {
        if (auto p = _observer.lock ())
            p->SelectionChanged (this->GetSelection ());
    }

private:
    std::weak_ptr<Observer> _observer;
};

#endif // !OBSERVABLE_WIDGET
