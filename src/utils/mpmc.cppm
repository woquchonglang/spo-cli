module;
#include "concurrentqueue.h"
export module mpmc;

export namespace moodycamel {
  template <typename T, typename Traits = ConcurrentQueueDefaultTraits>
    using Mpmc = moodycamel::ConcurrentQueue<T, Traits>;
}
