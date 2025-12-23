// #pragma once
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
// #include "global_operations.h"
// using namespace disruptor;

// class TradingHandler {
// public:

//     void onEvent(TradingEvent& event, int64_t sequence, bool endOfBatch) {
//         if (sequence % 100000 == 0) {
//             std::cout << "[Handler] Processed: " << sequence << std::endl;
//         }
//     }

//     void onStart() {}
//     void onShutdown() {}
//     void onBatchStart(int64_t batchSize, int64_t remaining) {}
//     void setSequenceCallback(disruptor::Sequence& sequence) {}
// };


// class TradingEngine {
// public:
//     static constexpr size_t kBufferSize = 1024;
   
//     using SequencerType = disruptor::SingleProducerSequencer<kBufferSize, disruptor::BusySpinWaitStrategy>;
//     using RingBufferType = disruptor::RingBuffer<TradingEvent, kBufferSize, SequencerType, decltype(&myEventFactory)>;
//     using BarrierType = disruptor::SequenceBarrier<SequencerType, disruptor::BusySpinWaitStrategy>;
//     using ProcessorType = disruptor::EventProcessor<TradingEvent, RingBufferType, BarrierType, TradingHandler>;
 
//     TradingEngine() 
//         : sequencer_(wait_strategy_),
//           ring_buffer_(sequencer_, &myEventFactory),
//           barrier_(sequencer_.newBarrier({})),
//           processor_(ring_buffer_, barrier_, handler_, ex_handler_) 
//     {
//         ring_buffer_.setGatingSequences({&processor_.getSequence()});
//     }


//     void start() {
//         if (!is_running_.exchange(true)) {
//             worker_thread_ = std::thread([this]() { 
//                 std::cout << "[Engine] Consumer thread started." << std::endl;
//                 processor_.run(); 
//             });
//         }
//     }


//     void stop() {
//         if (is_running_.exchange(false)) {
//             processor_.halt();
//             if (worker_thread_.joinable()) {
//                 worker_thread_.join();
//             }
//             std::cout << "[Engine] Consumer thread stopped." << std::endl;
//         }
//     }

//     void push_tick(double price, int64_t volume) {

//         if (KillSwitch::is_killed()) return;

//         int64_t seq = ring_buffer_.next();
//         TradingEvent& event = ring_buffer_.get(seq);
        
//         event.timestamp = 0; 
//         event.data = TickData{price, volume};
        

//         ring_buffer_.publish(seq);
//     }

//     int64_t get_consumer_cursor() {
//         return processor_.getSequence().get();
//     }

// private:
//     disruptor::BusySpinWaitStrategy wait_strategy_;
//     SequencerType sequencer_;
//     RingBufferType ring_buffer_;
    
//     TradingHandler handler_;
//     disruptor::DefaultExceptionHandler<TradingEvent> ex_handler_;
    
//     BarrierType barrier_;
//     ProcessorType processor_;
    
//     std::thread worker_thread_;
//     std::atomic<bool> is_running_{false};
// };

#pragma once
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include "event_types.h"
#include "global_operations.h"

#include "disruptor/ring_buffer.h"
#include "disruptor/event_processor.h"


class HandlerA { 
public:
    void onEvent(TradingEvent& e, int64_t s, bool b) { /* TODO: 日志逻辑 */ }
    void onStart() {} void onShutdown() {}
    void onBatchStart(int64_t bs, int64_t r) {}
    void setSequenceCallback(disruptor::Sequence& seq) {}
};

class HandlerB { 
public:
    void onEvent(TradingEvent& e, int64_t s, bool b) { /* TODO: 计算逻辑 */ }
    void onStart() {} void onShutdown() {}
    void onBatchStart(int64_t bs, int64_t r) {}
    void setSequenceCallback(disruptor::Sequence& seq) {}
};

class HandlerC { 
public:
    void onEvent(TradingEvent& e, int64_t s, bool b) {
        if (s % 100000 == 0) std::cout << "[Final Order] Seq: " << s << std::endl;
    }
    void onStart() {} void onShutdown() {}
    void onBatchStart(int64_t bs, int64_t r) {}
    void setSequenceCallback(disruptor::Sequence& seq) {}
};


class TradingEngine {
public:
    static constexpr size_t kBufferSize = 1024;

    using SequencerType = disruptor::SingleProducerSequencer<kBufferSize, disruptor::BusySpinWaitStrategy>;
    using RingBufferType = disruptor::RingBuffer<TradingEvent, kBufferSize, SequencerType, decltype(&myEventFactory)>;
    using BarrierType = disruptor::SequenceBarrier<SequencerType, disruptor::BusySpinWaitStrategy>;
    
    using ProcessorA = disruptor::EventProcessor<TradingEvent, RingBufferType, BarrierType, HandlerA>;
    using ProcessorB = disruptor::EventProcessor<TradingEvent, RingBufferType, BarrierType, HandlerB>;
    using ProcessorC = disruptor::EventProcessor<TradingEvent, RingBufferType, BarrierType, HandlerC>;

    TradingEngine() 
        : sequencer_(wait_strategy_),
          ring_buffer_(sequencer_, &myEventFactory),
          barrierAB_(sequencer_.newBarrier({})), 
          processorA_(ring_buffer_, barrierAB_, hA_, ex_),
          processorB_(ring_buffer_, barrierAB_, hB_, ex_),

          barrierC_(sequencer_.newBarrier({&processorA_.getSequence(), &processorB_.getSequence()})),
          processorC_(ring_buffer_, barrierC_, hC_, ex_)
    {

        ring_buffer_.setGatingSequences({&processorC_.getSequence()});
    }

    void start() {
        if (!is_running_.exchange(true)) {

            threads_.emplace_back([this]() { processorA_.run(); });
            threads_.emplace_back([this]() { processorB_.run(); });
            threads_.emplace_back([this]() { processorC_.run(); });
        }
    }

    void stop() {
        if (is_running_.exchange(false)) {
            KillSwitch::trigger_kill();

            barrierAB_.alert();
            barrierC_.alert();

            processorA_.halt();
            processorB_.halt();
            processorC_.halt();

            for (auto& t : threads_) { 
                if (t.joinable()) t.join(); 
            }
            threads_.clear();
            barrierAB_.clearAlert();
            barrierC_.clearAlert();
        }
    }

    void push_tick(double price, int64_t volume) {
        if (KillSwitch::is_killed()) return;
        int64_t seq = ring_buffer_.next();
        TradingEvent& event = ring_buffer_.get(seq);
        event.data = TickData{price, volume};
        ring_buffer_.publish(seq);
    }

    int64_t get_consumer_cursor() {
        return processorC_.getSequence().get();
    }

private:
    disruptor::BusySpinWaitStrategy wait_strategy_;
    SequencerType sequencer_;
    RingBufferType ring_buffer_;
    disruptor::DefaultExceptionHandler<TradingEvent> ex_;

    HandlerA hA_; ProcessorA processorA_;
    HandlerB hB_; ProcessorB processorB_;
    HandlerC hC_; ProcessorC processorC_;

    BarrierType barrierAB_;
    BarrierType barrierC_;

    std::vector<std::thread> threads_;
    std::atomic<bool> is_running_{false};
};