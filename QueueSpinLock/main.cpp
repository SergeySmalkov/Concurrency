#pragma once

#include <atomic>
#include <vector>
#include <thread>
#include <iostream>

namespace spinlocks {
    class Node {
    public:
        Node ():owner(false), next(nullptr) {}
        std::atomic<Node*> next;
        std::atomic<bool> owner{false};
    };

    class QueueSpinLock {
    public:
        class Guard {
            friend class QueueSpinLock;

        public:
            explicit Guard(QueueSpinLock& spinlock) : spinlock_(spinlock) {

                spinlock_.Acquire(this);

            }

            ~Guard() {
                spinlock_.Release(this);
            }

        private:
            QueueSpinLock& spinlock_;
            Node node;
        };

    private:
        void Acquire(Guard* guard) {

            Node* old_node = tail_.exchange(&guard->node);

            if (!old_node) {
                return;
            }

            old_node->next = &guard->node;
            while(!guard->node.owner.load()) {

            }

        }

        void Release(Guard* owner) {
            Node* temp_owner = &owner->node;
            if (!tail_.compare_exchange_strong(temp_owner, nullptr)) {
                while (owner->node.next.load() == nullptr) {

                }
                owner->node.next.load()->owner.store(true);
            }
        }

    private:
        std::atomic<Node*> tail_{nullptr};
    };

}


int main () {
    spinlocks::QueueSpinLock qspinlock;
    size_t counter = 0;
    std::vector<std::thread> workers;
    for (size_t i = 0; i < 5; ++i) {
        workers.emplace_back([&]() {
            for (size_t j = 0; j < 10000000; ++j) {
                {
                    spinlocks::QueueSpinLock::Guard guard(qspinlock);  // <-- Acquire
                    counter++;
                }
            }

        });
    }
    for (size_t i = 0; i < 5; ++i) {
        workers[i].join();
    }
    std::cout << counter;
    return 0;
};
