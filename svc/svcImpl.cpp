/*
 * SvcImpl.cpp
 */
#include <iostream>
#include <sstream>

#include <google/protobuf/util/json_util.h>
#include <grpc++/server_context.h>
#include <grpc++/security/credentials.h>
#include <grpc++/security/server_credentials.h>

#include "svcImpl.h"
#include "passphrase.h"

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#ifdef ENABLE_SQLITE
#include <odb/sqlite/database.hxx>
#endif
#ifdef ENABLE_PG
#include <odb/pgsql/database.hxx>
#endif

using namespace odb::core;
using grpc::StatusCode;
using odb::query;

const int DEF_LIST_SIZE = 1000;
const std::string ERR_SVC_INVALID_ARGS = "Invalid arguments";

// default list size
#define DEF_LABEL_LIST_SIZE		100
#define MAX_LABEL_LIST_SIZE		10000
#define LOG(x) std::cerr

typedef uint64_t NOTIFICATION_TYPE;

static odb::database *odbconnect(
    struct ServiceConfig *config
)
{
#ifdef ENABLE_PG
    return new odb::pgsql::database(
            std::string(config->dbuser),
            std::string(config->dbpassword),
            std::string(config->dbname),
            std::string(config->dbhost),
            config->dbport);
#endif
#ifdef ENABLE_SQLITE
    return new odb::sqlite::database(config->sqliteDbName);
#endif
}

/**
 * gRPC published methods error catch log function
 * Parameters:
 * 	signature	method name
 * 	cause		ODB, stdlib or other exception ane
 * 	what		Error description
 * 	msg			Protobuf incoming message
 */
std::string logString (
    const std::string &signature,
    const std::string &cause,
    const std::string &what,
    const ::google::protobuf::Message *msg)
{
    std::stringstream ss;
    if (!signature.empty())
        ss << signature;

    if (!cause.empty())
        ss << " cause(" << cause << ")";
    if (!what.empty())
        ss << " what(" << what << ")";

    if (msg) {
        std::string s;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        options.always_print_primitive_fields = true;
        options.preserve_proto_field_names = true;
        google::protobuf::util::MessageToJsonString(*msg, &s, options);
        ss << " message: " << s;
    }
    return ss.str();
}

#define CHECK_PERMISSION(permission) \
	UserIds uids; \
	int flags = getAuthUser(context, &uids); \
	if ((flags == 0) && (permission != 0)) \
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);

#define BEGIN_GRPC_METHOD(signature, requestMessage, transact) \
		transaction transact; \
		try { \
			LOG(INFO) << logString(signature, "", "", requestMessage); \
	        t.reset(mDb->begin());

#define END_GRPC_METHOD(signature, requestMessage, responseMessage, transaction) \
		transaction.commit(); \
        LOG(INFO) << logString(signature, "", "", responseMessage); \
	} \
	catch (const odb::exception& oe) { \
		LOG(ERROR) << logString(signature, "odb::exception", oe.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (const std::exception& se) { \
		LOG(ERROR) << logString(signature, "std::exception", se.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (...) { \
		LOG(ERROR) << logString(signature, "unknown exception", "??", requestMessage); \
		transaction.rollback(); \
	}

#define INIT_CHECK_RANGE(list, r, sz, ofs)  \
	int sz = list.size(); \
	if (sz <= 0) \
		sz = DEF_LIST_SIZE; \
	int ofs = list.start(); \
	if (ofs < 0) \
		ofs = 0; \
	sz += ofs; \
	int r = 0;


#define CHECK_RANGE(r, sz, ofs) \
	if (r >= sz) \
		break; \
	if (r < ofs) \
	{ \
		r++; \
		continue; \
	}

template <class T>
std::unique_ptr<T> RcrImpl::load(uint64_t id)
{
	try	{
		std::unique_ptr<T> r(mDb->load<T>(id));
		return r;
	} catch (const std::exception &e) {
		LOG(ERROR) << "load object " << id << " error: " << e.what();
		// return nullptr
		std::unique_ptr<T> r;
		return r;
	}
}

// --------------------- RcrImpl ---------------------

const grpc::string ERR_NO_GRANTS("No grants to call");
const grpc::string ERR_NOT_IMPLEMENTED("Not implemented");

const grpc::Status& RcrImpl::STATUS_NO_GRANTS = grpc::Status(StatusCode::PERMISSION_DENIED, ERR_NO_GRANTS);
const grpc::Status& RcrImpl::STATUS_NOT_IMPLEMENTED = grpc::Status(StatusCode::UNIMPLEMENTED, ERR_NOT_IMPLEMENTED);

uint64_t VERSION_MAJOR = 0x010000;

RcrImpl::RcrImpl(struct ServiceConfig *config)
{
	mConfig = config;
	mDb = odbconnect(config);
}

RcrImpl::~RcrImpl()
{
	if (mDb)
		delete mDb;
}

struct ServiceConfig *RcrImpl::getConfig()
{
	return mConfig;
}

::grpc::Status RcrImpl::version(
    ::grpc::ServerContext* context,
    const ::rcr::VersionRequest* request,
    ::rcr::VersionResponse* response
)
{
    BEGIN_GRPC_METHOD("version", request, t)
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    response->set_value(VERSION_MAJOR);
    response->set_name(getRandomName());
    END_GRPC_METHOD("version", request, response, t)
    return grpc::Status::OK;
}

::grpc::Status RcrImpl::cardSearchEqual(
    ::grpc::ServerContext* context,
    const ::rcr::EqualSearchRequest* request,
    ::rcr::CardResponse* response
)
{
	if (request == nullptr)
		return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("cardSearchEqual", request, t)
	END_GRPC_METHOD("cardSearchEqual", request, response, t)
    return true ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}

::grpc::Status RcrImpl::chPropertyType(
    ::grpc::ServerContext* context,
    const ::rcr::ChPropertyTypeRequest* request,
    ::rcr::OperationResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
//    BEGIN_GRPC_METHOD("chPropertyType", request, t)
    response->set_id(1);
    response->set_code(2);
    response->set_description(request->value().description());
//    END_GRPC_METHOD("chPropertyType", request, response, t)
    return true ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}
