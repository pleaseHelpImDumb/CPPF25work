#include <iostream>
using namespace std;

class Deque // circular buffer
{
private:
    int *arr;
    int capacity_;

    int front;
    int back;
    int count;

    // expand

    void expand()
    {
    }

public:
    Deque(int capacity_ = 10) : capacity_{capacity_}
    {
        arr = new int[capacity_];
        count = 0;
        front = 0;
        back = 0;
    }
    // add (MOVING BACK)
    void add(int num)
    {
        if (isFull())
            expand();
        arr[back] = num;
        back = (back + 1) % capacity_;
        count = count + 1;
    }

    // pop (MOVING FRONT)
    int pop()
    {
        if (isEmpty())
            return -1;

        else
        {
            int o = arr[front];
            front = (front + 1) % capacity_;
            return o;
        }
    }
    // size
    int size() { return 1; }
    // isEmpty
    bool isEmpty()
    {
        return (count == 0);
    }
    // isFull
    bool isFull()
    {
        return (front == back && count != 0);
    }

    void print()
    {
        cout << "[";
        for (int i = 0; i < capacity_; i++)
        {
            cout << arr[i];

            if (i == front)
            {
                cout << "F";
            }
            if (i == back)
            {
                cout << "B";
            }

            cout << " ";
        }

        cout << "]\n";
    }

    // copy
    // copy operator
    // destructor
    ~Deque()
    {
    }
};

int main()
{
    cout << "-- DEQUE -- \n;";
    Deque que(5);
    que.add(2);
    que.print();
    que.add(3);
    que.add(4);
    que.print();
    que.add(3);
    que.add(4);
    que.print();
    cout << que.pop() << endl;
    cout << que.pop() << endl;
    cout << que.pop() << endl;
    cout << que.pop() << endl;
    que.print();
    cout << que.pop() << endl;
    que.print();
}
