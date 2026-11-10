#pragma once

#include <cinttypes>
#include <cstdint>
#include <cstdio>

#define BOOST_ASIO_INHERIT_TRACKED_HANDLER \
  : public ::custom_tracking::tracked_handler

#define BOOST_ASIO_ALSO_INHERIT_TRACKED_HANDLER \
    , public ::custom_tracking::tracked_handler

#define BOOST_ASIO_HANDLER_TRACKING_INIT ::custom_tracking::init()

#define BOOST_ASIO_HANDLER_LOCATION(args) ::custom_tracking::location args

#define BOOST_ASIO_HANDLER_CREATION(args) ::custom_tracking::creation args

#define BOOST_ASIO_HANDLER_COMPLETION(args) \
    ::custom_tracking::completion tracked_completion args

#define BOOST_ASIO_HANDLER_INVOCATION_BEGIN(args) \
    tracked_completion.invocation_begin args

#define BOOST_ASIO_HANDLER_INVOCATION_END tracked_completion.invocation_end()

#define BOOST_ASIO_HANDLER_OPERATION(args) ::custom_tracking::operation args

#define BOOST_ASIO_HANDLER_REACTOR_REGISTRATION(args) \
    ::custom_tracking::reactor_registration args

#define BOOST_ASIO_HANDLER_REACTOR_DEREGISTRATION(args) \
    ::custom_tracking::reactor_deregistration args

#define BOOST_ASIO_HANDLER_REACTOR_READ_EVENT  1
#define BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT 2
#define BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT 4

#define BOOST_ASIO_HANDLER_REACTOR_EVENTS(args) \
    ::custom_tracking::reactor_events args

#define BOOST_ASIO_HANDLER_REACTOR_OPERATION(args) \
    ::custom_tracking::reactor_operation args

#define BOOST_ASIO_HANDLER_SCHEDULER_RUN(args) (void)0

struct custom_tracking {
    // Base class for objects containing tracked handlers.
    struct tracked_handler {
        std::uintmax_t handler_id_ = 0; // To uniquely identify a handler.
        std::uintmax_t tree_id_ = 0;    // To identify related handlers.
        const char *object_type_; // The object type associated with the handler.
        std::uintmax_t native_handle_; // Native handle, if any.
    };

    // Initialise the tracking system.
    static void init() {}

    // Record a source location.
    static void location(const char *file_name, int line,
                         const char *function_name);

    // Record the creation of a tracked handler.
    static void creation(tracked_handler &h, const char *object_type,
                         void *object, std::uintmax_t native_handle,
                         const char *op_name);

    static void creation(boost::asio::execution_context & /*ctx*/,
                         tracked_handler &h, const char *object_type,
                         void * /*object*/, std::uintmax_t native_handle,
                         const char *op_name);

    struct completion {
        explicit completion(const tracked_handler &h)
                : handler_(h), next_(*current_completion()) {
            *current_completion() = this;
        }

        completion(const completion &) = delete;
        completion &operator=(const completion &) = delete;

        // Destructor records only when an exception is thrown from the handler, or
        // if the memory is being freed without the handler having been invoked.
        ~completion() { *current_completion() = next_; }

        // Records that handler is to be invoked with the specified arguments.
        template <class... Args> void invocation_begin(Args &&.../*args*/) {
            invocation_begin();
        }

        void invocation_begin();

        // Record that handler invocation has ended.
        void invocation_end();

        tracked_handler handler_;

        // Completions may nest. Here we stash a pointer to the outer completion.
        completion *next_;
    };

    static completion **current_completion() {
        static BOOST_ASIO_THREAD_KEYWORD completion *current = nullptr;
        return &current;
    }

    // Record an operation that is not directly associated with a handler.
    static void operation(boost::asio::execution_context & /*ctx*/,
                          const char * /*object_type*/, void * /*object*/,
                          std::uintmax_t /*native_handle*/,
                          const char * /*op_name*/) {}

    // Record that a descriptor has been registered with the reactor.
    static void reactor_registration(boost::asio::execution_context &context,
                                     uintmax_t native_handle,
                                     uintmax_t registration);

    // Record that a descriptor has been deregistered from the reactor.
    static void reactor_deregistration(boost::asio::execution_context &context,
                                       uintmax_t native_handle,
                                       uintmax_t registration);

    // Record reactor-based readiness events associated with a descriptor.
    static void reactor_events(boost::asio::execution_context &context,
                               uintmax_t registration, unsigned events);

    // Record a reactor-based operation that is associated with a handler.
    static void reactor_operation(const tracked_handler &h, const char *op_name,
                                  const boost::system::error_code &ec);

    // Record a reactor-based operation that is associated with a handler.
    static void reactor_operation(const tracked_handler &h, const char *op_name,
                                  const boost::system::error_code &ec,
                                  std::size_t bytes_transferred);
};
