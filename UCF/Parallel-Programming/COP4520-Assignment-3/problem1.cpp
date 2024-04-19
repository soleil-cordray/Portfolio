#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <condition_variable>
#include <random>
#include <atomic>

// Defined structure.
const int NUM_PRESENTS = 500000;
const int NUM_SERVANTS = 4;

// Thread-safe printing.
std::mutex output_mutex;
void safe_print(const std::string &message)
{
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << message << std::endl;
}

// Concurrent linked list: chain unordered presents together.
class ConcurrentLinkedList
{
private:
    std::list<int> list; // linked list
    mutable std::mutex mutex; // lock
    std::condition_variable cv, cv_remove; // synch
    std::atomic<int> presents{0}, notes{0}; // counts

public:
    // Improved strategy: add presents & notify all threads to ensure quick processing.
    bool add(int id)
    {
        // lock & ensure presents count within bounds
        std::unique_lock<std::mutex> lock(mutex);
        if (presents >= NUM_PRESENTS)
            return false;

        // efficient tag retrieval & addition
        int tag = presents++;
        list.push_back(tag);
        safe_print("Servant " + std::to_string(id) + " added present " + std::to_string(tag + 1));

        // ensure no delays in processing notes
        cv.notify_all();
        return true;
    }

    // Improved strategy: remove presents and notify all threads to balance adding and removing tasks.
    bool remove(int id)
    {
        // lock & wait remove only until list nonempty/notes quota met
        std::unique_lock<std::mutex> lock(mutex);
        cv_remove.wait(lock, [this] { return !list.empty() || notes >= presents; });

        if (!list.empty())
        {
            // efficient tag retrieval & deletion
            int tag = list.front();
            list.pop_front();

            notes++;
            safe_print("Servant " + std::to_string(id) + " wrote a 'Thank you' note for present " + std::to_string(tag + 1));

            // ensure all servants can react to changes immediately
            cv.notify_all();
            return true;
        }
        return false;
    }

    // Improved strategy: search efficiently without modifying the list.
    bool search(int id, int tag)
    {
        // lock, take snapshot for undisturbed search, then release lock
        std::list<int> snapshot;
        {
            std::unique_lock<std::mutex> lock(mutex);
            snapshot = list;
        } 

        bool found = std::find(snapshot.begin(), snapshot.end(), tag) != snapshot.end();
        safe_print("Servant " + std::to_string(id) + (found ? " confirmed present " : " did not find present ") + std::to_string(tag + 1));
        return found;
    }

    // store counts 
    int getPresentsCount() const { return presents; }
    int getNotesCount() const { return notes; }
};

// Servant tasks dynamically balanced between adding, removing, and searching.
void servantTask(ConcurrentLinkedList &chain, int id)
{
    // randomization
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, NUM_PRESENTS - 1);

    while (chain.getNotesCount() < NUM_PRESENTS || chain.getPresentsCount() < NUM_PRESENTS)
    {
        // distribute work evenly & fairly 
        int action = dis(gen) % 3;

        // Dynamic task switching.
        if (action == 0)
        {
            chain.add(id);
        }
        else if (action == 1)
        {
            chain.remove(id);
        }
        else
        {
            // simulate variable minotaur requests
            int search_tag = dis(gen) % NUM_PRESENTS;
            chain.search(id, search_tag);
        }
    }
}

int main()
{
    const int numPresents = 100;
    ConcurrentLinkedList chain;

    // initialize servant threads & assign tasks
    std::vector<std::thread> servants;
    for (int i = 0; i < NUM_SERVANTS; ++i)
    {
        servants.emplace_back(servantTask, std::ref(chain), i + 1);
    }

    // ensure execution completion
    for (auto &servant : servants)
    {
        servant.join();
    }

    // print results 
    safe_print("Sorting complete! Presents added: " + std::to_string(chain.getPresentsCount()) + ", 'Thank you' notes written: " + std::to_string(chain.getNotesCount()));

    return 0;
}
