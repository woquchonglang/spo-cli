module;
#include "readerwriterqueue.h"
export module spsc;

export namespace moodycamel {
template <typename T, size_t N = 512>
using Spsc = moodycamel::ReaderWriterQueue<T, N>;

template <typename T, size_t N = 512>
using Block_spsc = moodycamel::BlockingReaderWriterQueue<T, N>;
}
