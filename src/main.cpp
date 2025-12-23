// #include <iostream>
// #include <thread>
// #include <chrono>
// #include <cstdio>
// #include <string>

// #include <variant>
// // It's a meme but it works

// #include "disruptor/ring_buffer.h"
// #include "disruptor/event_handler.h"
// #include "disruptor/event_processor.h"
// #include "disruptor/sequencer.h"
// #include "disruptor/wait_strategies.h"
// #include "disruptor/exception_handler.h"

// #include "event_types.h"

// using namespace disruptor;

// inline int64_t now_ns()
// {
//     using namespace std::chrono;
//     static auto start = steady_clock::now();
//     return duration_cast<nanoseconds>(steady_clock::now() - start).count();
// }

// inline void log(const std::string &tag, int64_t seq, int64_t val)
// {
//     int64_t t_ns = now_ns();
//     printf("[%12lld ns] [%s] Sequence %lld Value %lld\n", t_ns, tag.c_str(), seq, val);
// }

// // ================================================
// // Common Events and Factory Functions
// // ================================================

// // struct MyEvent
// // {
// //     int64_t value;
// // };

// // auto myEventFactory = []() -> MyEvent
// // {
// //     return MyEvent{0};
// // };

// // s
// // ================================================
// // Simple Handler
// // ================================================

// // class SimpleHandler : public EventHandler<MyEvent>
// // {
// // public:
// //     void onEvent(MyEvent &event, int64_t sequence, bool endOfBatch) override
// //     {
// //         // log("Simple", sequelog("Simple", sequence, event.value);nce, event.value);
// //     }
// //     void onStart() override { std::cout << "[Simple] Started.\n"; }
// //     void onShutdown() override { std::cout << "[Simple] Shutdown.\n"; }
// // };

// class TradingHandler : public EventHandler<TradingEvent> {
// public:
//     void onEvent(TradingEvent &event, int64_t sequence, bool endOfBatch) override {
//         std::visit([&](auto&& arg) {
//             using T = std::decay_t<decltype(arg)>;

//             if constexpr (std::is_same_v<T, TickData>) {
//                 // Process TickData
//                 // log("TickData", sequence, static_cast<int64_t>(arg.price));
//             } 
//             else if constexpr (std::is_same_v<T, EZNewsData>) {
//                 // Process EZNewsData
//                 // log("EZNewsData", sequence, static_cast<int64_t>(arg.score));
//             }
//         }, event.data);
//     }
// };

// // ================================================
// // Diamond Handlers
// // ================================================

// // class HandlerA : public EventHandler<MyEvent>
// // {
// // public:
// //     void onEvent(MyEvent &event, int64_t sequence, bool) override
// //     {
// //         log("A", sequence, event.value);
// //     }
// // };

// // class HandlerB : public EventHandler<MyEvent>
// // {
// // public:
// //     void onEvent(MyEvent &event, int64_t sequence, bool) override
// //     {
// //         log("B", sequence, event.value);
// //     }
// // };

// // class HandlerC : public EventHandler<MyEvent>
// // {
// // public:
// //     void onEvent(MyEvent &event, int64_t sequence, bool) override
// //     {
// //         log("C", sequence, event.value);
// //     }
// // };

// // ================================================
// // Simple Example
// // ================================================

// // void simple()
// // {
// //     std::cout << "\n===== Running Simple Example =====\n";
// //     constexpr size_t bufferSize = 1024;
// //     BusySpinWaitStrategy waitStrategy;
// //     SingleProducerSequencer<bufferSize, BusySpinWaitStrategy> sequencer(waitStrategy);

// //     RingBuffer<MyEvent, bufferSize, decltype(sequencer), decltype(myEventFactory)>
// //         ringBuffer(sequencer, myEventFactory);

// //     auto barrier = sequencer.newBarrier({});
// //     SimpleHandler handler;

// //     DefaultExceptionHandler<MyEvent> exHandler;
// //     EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrier), SimpleHandler>
// //         processor(ringBuffer, barrier, handler, exHandler);

// //     Sequence &consumerSeq = processor.getSequence();
// //     ringBuffer.setGatingSequences({&consumerSeq});

// //     std::thread consumer([&]
// //                          { processor.run(); });

// //     // for (int i = 0; i < 10000; ++i)
// //     // {
// //     //     int64_t seq = ringBuffer.next();
// //     //     ringBuffer.get(seq).value = i;
// //     //     ringBuffer.publish(seq);
// //     //     // std::this_thread::sleep_for(std::chrono::milliseconds(50));
// //     // }

// //     auto t1 = std::chrono::steady_clock::now();

// //     int64_t lastSeq = -1;
// //     for (int i = 0; i < 1000000; ++i) { 
// //         lastSeq = ringBuffer.next();
// //         ringBuffer.get(lastSeq).value = i;
// //         ringBuffer.publish(lastSeq);
// //     }

// //     while (consumerSeq.get() < lastSeq) {
// //         std::this_thread::yield();
// //     }

// //     auto t2 = std::chrono::steady_clock::now();
    
// //     std::cout << "1M Events throughput: " 
// //               << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() 
// //               << " ms\n";

// //     processor.halt();
// //     consumer.join();
// // }

// // ================================================
// // Diamond Example
// // ================================================

// // void diamond()
// // {
// //     std::cout << "\n===== Running Diamond Example =====\n";

// //     constexpr size_t bufferSize = 1024;
// //     BusySpinWaitStrategy waitStrategy;
// //     SingleProducerSequencer<bufferSize, BusySpinWaitStrategy> sequencer(waitStrategy);

// //     RingBuffer<MyEvent, bufferSize, decltype(sequencer), decltype(myEventFactory)>
// //         ringBuffer(sequencer, myEventFactory);

// //     // Create handlers
// //     HandlerA handlerA;
// //     HandlerB handlerB;
// //     HandlerC handlerC;

// //     // Barrier for A and B with no dependents
// //     auto barrierA = sequencer.newBarrier({});
// //     auto barrierB = sequencer.newBarrier({});

// //     // Create processors for A and B
// //     DefaultExceptionHandler<MyEvent> exHandler;

// //     EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrierA), HandlerA>
// //         processorA(ringBuffer, barrierA, handlerA, exHandler);
// //     EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrierB), HandlerB>
// //         processorB(ringBuffer, barrierB, handlerB, exHandler);

// //     // Get references to their sequences
// //     Sequence &seqA = processorA.getSequence();
// //     Sequence &seqB = processorB.getSequence();

// //     // Barrier for C, depending on sequences of A and B
// //     auto barrierC = sequencer.newBarrier({&seqA, &seqB});

// //     // Create processor for C
// //     EventProcessor<MyEvent, decltype(ringBuffer), decltype(barrierC), HandlerC>
// //         processorC(ringBuffer, barrierC, handlerC, exHandler);

// //     // Get reference to C's sequence
// //     Sequence &seqC = processorC.getSequence();
// //     // Set gating sequences for the ring buffer
// //     ringBuffer.setGatingSequences({&seqC});

// //     // Start threads
// //     std::thread threadA([&]
// //                         { processorA.run(); });
// //     std::thread threadB([&]
// //                         { processorB.run(); });
// //     std::thread threadC([&]
// //                         { processorC.run(); });

// //     // Publish events
// //     for (int i = 0; i < 5; ++i)
// //     {
// //         int64_t seq = ringBuffer.next();
// //         ringBuffer.get(seq).value = i;
// //         ringBuffer.publish(seq);
// //         std::this_thread::sleep_for(std::chrono::milliseconds(50));
// //     }

// //     // Allow some time for processing
// //     std::this_thread::sleep_for(std::chrono::seconds(2));

// //     // Halt processors
// //     processorA.halt();
// //     processorB.halt();
// //     processorC.halt();

// //     // Join threads
// //     threadA.join();
// //     threadB.join();
// //     threadC.join();
// // }

// void playground()
// {
//     std::cout << "\n===== Running Simple Example =====\n";
//     constexpr size_t bufferSize = 1024;
//     BusySpinWaitStrategy waitStrategy;
//     SingleProducerSequencer<bufferSize, BusySpinWaitStrategy> sequencer(waitStrategy);

//     RingBuffer<TradingEvent, bufferSize, decltype(sequencer), decltype(myEventFactory)>
//         ringBuffer(sequencer, myEventFactory);

//     auto barrier = sequencer.newBarrier({});
//     TradingHandler handler;

//     DefaultExceptionHandler<TradingEvent> exHandler;
//     EventProcessor<TradingEvent, decltype(ringBuffer), decltype(barrier), TradingHandler>
//         processor(ringBuffer, barrier, handler, exHandler);

//     Sequence &consumerSeq = processor.getSequence();
//     ringBuffer.setGatingSequences({&consumerSeq});

//     std::thread consumer([&]
//                          { processor.run(); });

//     // for (int i = 0; i < 10000; ++i)
//     // {
//     //     int64_t seq = ringBuffer.next();
//     //     ringBuffer.get(seq).value = i;
//     //     ringBuffer.publish(seq);
//     //     // std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     // }

//     auto t1 = std::chrono::steady_clock::now();

//     int64_t lastSeq = -1;

//     for (int i = 0; i < 1000000; ++i) {
//         lastSeq = ringBuffer.next();
//         TradingEvent& event = ringBuffer.get(lastSeq);
        
//         event.timestamp = i;
        
//         if (i % 100 == 0) {
//             event.data = EZNewsData{0.85}; 
//         } else {
//             event.data = TickData{100.0 + i, 1}; 
//         }
        
//         ringBuffer.publish(lastSeq);
//     }

//     while (consumerSeq.get() < lastSeq) {
//         std::this_thread::yield();
//     }

//     auto t2 = std::chrono::steady_clock::now();
    
//     std::cout << "1M Events throughput: " 
//               << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() 
//               << " ms\n";

//     processor.halt();
//     consumer.join();
// }


// // ================================================
// // Main
// // ================================================
// int main()
// {
//     // simple();
//     // diamond();
//     playground();
//     return 0;
// }


#include "disruptor.h"  
#include "global_operations.h"  
#include <iostream>
#include <chrono>


int main() {
    try {

        TradingEngine engine;

        std::cout << "--- Engine initialized, starting consumer thread ---" << std::endl;

        engine.start();

        auto t1 = std::chrono::steady_clock::now();
        const int64_t total_events = 1000000;

        for (int64_t i = 0; i < total_events; ++i) {

            engine.push_tick(100.5 + (i % 10), 10 + (i % 100));

            // if (i == 500000) ControlCenter::trigger_kill();
        }


        while (engine.get_consumer_cursor() < (total_events - 1)) {
            if (KillSwitch::is_killed()) break;
            std::this_thread::yield(); 
        }

        auto t2 = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        std::cout << "--- Test Complete ---" << std::endl;
        std::cout << "Processed " << total_events << " events in " << duration << " ms" << std::endl;
        std::cout << "Throughput: " << (total_events / (duration / 1000.0)) / 1000000.0 << " M events/s" << std::endl;


        engine.stop();

    } catch (const std::exception& e) {
        std::cerr << "Engine failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}