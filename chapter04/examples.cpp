#include "deque"
#include "mutex"
#include "future"
#include "thread"
#include "utility"
#include "condition_variable"
#include "chrono"

using namespace std;

mutex m;

//<editor-fold desc="Running code on a GUI thread using std::packaged_task">
deque<packaged_task<void()>> tasks;

bool guiShutdownMessageReceived() {
    return false;
}

void getAndProcessGuiMessage() {}

void guiThread() {
    while (!guiShutdownMessageReceived()) {
        getAndProcessGuiMessage();
        packaged_task<void()> task;
        {
            lock_guard lk(m);
            if (tasks.empty()) {
                continue;
            }
            task = move(tasks.front());
        }
        task();
    }
}

thread guiBackgroundThread(guiThread);

template<typename Func>
future<void> postTaskForGuiThread(Func f) {
    packaged_task<void()> task(f);
    future<void> res = task.get_future();
    lock_guard lk(m);
    tasks.push_back(move(task));
    return res;
}
//</editor-fold>


//<editor-fold desc="Handling multiple connections from a single thread using promises">
struct PayLoad {

};


struct DataPacket {
    int id = 0;
    PayLoad payLoad;
};

struct OutgoingPacket {
    PayLoad payLoad;
    promise<bool> promise_;
};

struct ConnectionIterator {
    int i = 0;
    promise<PayLoad> promise_;

    bool HasIncomingData() {
        return false;
    }

    DataPacket Incoming() {
        return DataPacket{};
    }

    promise<PayLoad> &GetPromise(int dataId) {
        return promise_;
    }

    bool HasOutgoingData() {
        return true;
    }

    OutgoingPacket TopOfOutgoingQueue() {
        return OutgoingPacket{};
    }

    void Send(PayLoad payLoad) {

    }

};

bool operator!=(const ConnectionIterator &lhs, const ConnectionIterator &rhs) {
    return lhs.i < rhs.i;
}

void operator++(ConnectionIterator &iter) {
    ++iter.i;
}

struct ConnectionSet {
    ConnectionIterator b, e;

    ConnectionIterator &begin() {
        return b;
    }

    ConnectionIterator &end() {
        return e;
    }

    const ConnectionIterator &begin() const {
        return b;
    }

    const ConnectionIterator &end() const {
        return e;
    }
};


bool Done(ConnectionSet &connectionSet) {
    return true;
}

void processConnections(ConnectionSet &connections) {
    while (!Done(connections)) {
        ConnectionIterator &connection = connections.begin();
        ConnectionIterator &end = connections.end();
        for (; connection != end; ++connection) {
            if (connection.HasIncomingData()) {
                DataPacket data = connection.Incoming();
                promise<PayLoad> &p = connection.GetPromise(data.id);
                p.set_value(data.payLoad);
            }
            if (connection.HasOutgoingData()) {
                OutgoingPacket data = connection.TopOfOutgoingQueue();
                connection.Send(data.payLoad);
                data.promise_.set_value(true);
            }
        }
    }
}
//</editor-fold>


//<editor-fold desc="Waiting for a condition variable with a timeout">
condition_variable cv;
bool done;

bool waitLoop() {
    auto const timeout = chrono::steady_clock::now() + chrono::milliseconds(500);
    unique_lock<std::mutex> lk(m);
    while (!done) {
        if (cv.wait_until(lk, timeout) == std::cv_status::timeout) {
            break;
        }
    }
    return done;
}
//</editor-fold>



















