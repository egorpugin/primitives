#pragma once

#include <primitives/exceptions.h>

#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/status.h>

#include <system_error>

// client part

#define GRPC_SET_DEADLINE(s) \
    context->set_deadline(::std::chrono::system_clock::now() + ::std::chrono::seconds(s))

#define GRPC_CALL_PREPARE(resptype) \
    resptype response

#define GRPC_CALL_INTERNAL(svc, m, resptype, t) \
    ::primitives::grpc::check_result(svc->m(context.get(), request, &response), *context, #m, t)

#define GRPC_CALL(svc, m, resptype) \
    GRPC_CALL_PREPARE(resptype);    \
    GRPC_CALL_INTERNAL(svc, m, resptype, false)

#define GRPC_CALL_THROWS(svc, m, resptype) \
    GRPC_CALL_PREPARE(resptype);           \
    GRPC_CALL_INTERNAL(svc, m, resptype, true)

#define IF_GRPC_CALL(svc, m, resptype) \
    GRPC_CALL_PREPARE(resptype);       \
    if (GRPC_CALL_INTERNAL(svc, m, resptype, false))

// server part

#define DECLARE_SERVICE_METHOD(m, req, resp)            \
    ::grpc::Status m(::grpc::ServerContext *context,    \
                   const req *,                         \
                   resp *) override

#define DEFINE_SERVICE_METHOD_RAW(s, m, req, resp)              \
    ::grpc::Status s##Impl::m(::grpc::ServerContext *context,   \
                            const req *request,                 \
                            resp *response)                     \
    {

#define DEFINE_SERVICE_METHOD_WITH_PREFIX(s, m, req, resp, prefix) \
    DEFINE_SERVICE_METHOD_RAW(s, m, req, resp) \
    prefix; \
    try

#define DEFINE_SERVICE_METHOD(s, m, req, resp)       \
    DEFINE_SERVICE_METHOD_WITH_PREFIX(s, m, req, resp, )

#define GRPC_RETURN_MESSAGE_EC(m, ec)                   \
    do                                                  \
    {                                                   \
        if (!std::string(m).empty())                    \
        {                                               \
            context->AddTrailingMetadata("ec", #ec);    \
            context->AddTrailingMetadata("message", m); \
        }                                               \
        else                                            \
            context->AddTrailingMetadata("ec", "0");    \
        return grpc::Status::OK;                        \
    } while (0)

#define GRPC_RETURN_MESSAGE(m) GRPC_RETURN_MESSAGE_EC(m, 1)

#define GRPC_RETURN_OK()                         \
    context->AddTrailingMetadata("ec", "0");     \
    }                                            \
    catch (std::exception &e)                    \
    {                                            \
        LOG_ERROR(logger, e.what());             \
        GRPC_RETURN_MESSAGE_EC("Internal server exception was thrown.", 2); \
    }                                            \
    catch (...)                                  \
    {                                            \
        LOG_ERROR(logger, "unknown exception");  \
        GRPC_RETURN_MESSAGE_EC("Internal server exception was thrown.", 3); \
    }                                            \
    return grpc::Status::OK

//

namespace primitives::grpc
{

std::string get_metadata_variable(const std::multimap<::grpc::string_ref, ::grpc::string_ref> &metadata, const std::string &key)
{
    auto i = metadata.find(key);
    if (i == metadata.end())
        return {};
    return std::string(std::string_view(i->second.data(), i->second.size()));
}

struct CallResult
{
    std::error_code ec;
    std::string message;

    operator bool() const { return !ec; }
};

CallResult check_result(
    const ::grpc::Status &status,
    const ::grpc::ClientContext &context,
    const std::string &method,
    bool throws = false
)
{
    CallResult r;

    if (!status.ok())
    {
        auto err = "Method '" + method + "': RPC failed: " + std::to_string(status.error_code()) + ": " + status.error_message();
        if (throws)
            throw SW_RUNTIME_ERROR(err);
        r.ec = std::make_error_code((std::errc)status.error_code());
        r.message = err;
        return r;
    }

    auto result = get_metadata_variable(context.GetServerTrailingMetadata(), "ec");
    if (result.empty())
    {
        auto err = "Method '" + method + "': missing error code";
        if (throws)
            throw SW_RUNTIME_ERROR(err);
        r.ec = std::make_error_code((std::errc)1);
        r.message = err;
        return r;
    }
    auto ec = std::stoi(result.data());
    if (ec)
    {
        auto message = get_metadata_variable(context.GetServerTrailingMetadata(), "message");
        auto err = "Method '" + method + "' returned error: ec = " + result + ", message: " + message;
        if (throws)
            throw SW_RUNTIME_ERROR(err);
        r.ec = std::make_error_code((std::errc)ec);
        r.message = err;
    }

    return r;
}

}
