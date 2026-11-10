#include <boost/asio.hpp>
#include "asioLog.hpp"

import log;

void custom_tracking::location(const char *file_name, int line,
                               const char *function_name) {
    std::printf("At location %s:%d in %s\n", file_name, line, function_name);
}

void custom_tracking::creation(tracked_handler &h, const char *object_type,
                               void *object, std::uintmax_t native_handle,
                               const char *op_name) {
    std::printf("Starting operation %s.%s for native_handle = %" PRIuMAX
                " (no handler)\n",
                object_type, op_name, native_handle);
}

void custom_tracking::creation(boost::asio::execution_context & /*ctx*/,
                               tracked_handler &h, const char *object_type,
                               void * /*object*/, std::uintmax_t native_handle,
                               const char *op_name) {
    // Generate a unique id for the new handler.
    static std::atomic<std::uintmax_t> next_handler_id{ 1 };
    h.handler_id_ = next_handler_id++;

    // Copy the tree identifier forward from the current handler.
    if (*current_completion())
        h.tree_id_ = (*current_completion())->handler_.tree_id_;

    // Store various attributes of the operation to use in later output.
    h.object_type_ = object_type;
    h.native_handle_ = native_handle;

    std::printf("Starting operation %s.%s for native_handle = %" PRIuMAX
                ", handler = %" PRIuMAX ", tree = %" PRIuMAX "\n",
                object_type, op_name, h.native_handle_, h.handler_id_,
                h.tree_id_);
}

void custom_tracking::completion::invocation_begin() {
    std::printf("Entering handler %" PRIuMAX " in tree %" PRIuMAX "\n",
                handler_.handler_id_, handler_.tree_id_);
}

void custom_tracking::completion::invocation_end() {
    std::printf("Leaving handler %" PRIuMAX " in tree %" PRIuMAX "\n",
                handler_.handler_id_, handler_.tree_id_);
}

void custom_tracking::reactor_registration(
        boost::asio::execution_context &context, uintmax_t native_handle,
        uintmax_t registration) {
    std::printf("Adding to reactor native_handle = %" PRIuMAX
                ", registration = %" PRIuMAX "\n",
                native_handle, registration);
}

void custom_tracking::reactor_deregistration(
        boost::asio::execution_context &context, uintmax_t native_handle,
        uintmax_t registration) {
    std::printf("Removing from reactor native_handle = %" PRIuMAX
                ", registration = %" PRIuMAX "\n",
                native_handle, registration);
}

void custom_tracking::reactor_events(boost::asio::execution_context &context,
                                     uintmax_t registration, unsigned events) {
    std::printf(
            "Reactor readiness for registration = %" PRIuMAX
            ", events =%s%s%s\n",
            registration,
            (events & BOOST_ASIO_HANDLER_REACTOR_READ_EVENT) ? " read" : "",
            (events & BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT) ? " write" : "",
            (events & BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT) ? " error" : "");
}

void custom_tracking::reactor_operation(const tracked_handler &h,
                                        const char *op_name,
                                        const boost::system::error_code &ec) {
    std::printf("Performed operation %s.%s for native_handle = %" PRIuMAX
                ", ec = %s:%d\n",
                h.object_type_, op_name, h.native_handle_, ec.category().name(),
                ec.value());
}

void custom_tracking::reactor_operation(const tracked_handler &h,
                                        const char *op_name,
                                        const boost::system::error_code &ec,
                                        std::size_t bytes_transferred) {
    std::printf("Performed operation %s.%s for native_handle = %" PRIuMAX
                ", ec = %s:%d, n = %" PRIuMAX "\n",
                h.object_type_, op_name, h.native_handle_, ec.category().name(),
                ec.value(), static_cast<uintmax_t>(bytes_transferred));
}
