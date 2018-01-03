/*******************************************************************************
 * Copyright 2013-2017 Aerospike, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include <node.h>
#include "async.h"
#include "client.h"
#include "conversions.h"
#include "config.h"
#include "events.h"
#include "log.h"

extern "C" {
#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/as_config.h>
#include <aerospike/as_key.h>
#include <aerospike/as_record.h>
#include <aerospike/as_log.h>
}


/*******************************************************************************
 *  Constructor and Destructor
 ******************************************************************************/

AerospikeClient::AerospikeClient() {}

AerospikeClient::~AerospikeClient() {}

/*******************************************************************************
 *  Methods
 ******************************************************************************/

NAN_METHOD(AerospikeClient::SetLogLevel)
{
	AerospikeClient* client = ObjectWrap::Unwrap<AerospikeClient>(info.Holder());
	if (info[0]->IsObject()) {
		log_from_jsobject(client->log, info[0]->ToObject());
	}
	info.GetReturnValue().Set(info.Holder());
}


/**
 *  Instantiate a new 'AerospikeClient(config)'
 *  Constructor for AerospikeClient.
 */
NAN_METHOD(AerospikeClient::New)
{
	AerospikeClient * client = new AerospikeClient();
	client->as = (aerospike*) cf_malloc(sizeof(aerospike));
	client->log = (LogInfo*) cf_malloc(sizeof(LogInfo));

	// initialize the log to default values.
	client->log->fd = g_log_info.fd;
	client->log->level = g_log_info.level;

	// initialize the config to default values.
	as_config config;
	as_config_init(&config);

	if (info[0]->IsObject()) {
		Local<Value> v8_log_info = info[0]->ToObject()->Get(Nan::New("log").ToLocalChecked()) ;
		if (v8_log_info->IsObject()) {
			log_from_jsobject(client->log, v8_log_info->ToObject());
		}
	}

	if (info[0]->IsObject()) {
		int result = config_from_jsobject(&config, info[0]->ToObject(), client->log);
		if (result != AS_NODE_PARAM_OK) {
			Nan::ThrowError("Invalid client configuration");
		}
	}

	aerospike_init(client->as, &config);
	as_v8_debug(client->log, "Aerospike client initialized successfully");
	client->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

/**
 * Setup event callback for cluster events.
 */
NAN_METHOD(AerospikeClient::SetupEventCb)
{
	Nan::HandleScope scope;
	AerospikeClient* client = ObjectWrap::Unwrap<AerospikeClient>(info.This());

	Local<Function> callback;
	if (info.Length() > 0 && info[0]->IsFunction()) {
		callback = info[0].As<Function>();
		events_callback_init(&client->as->config, callback, client->log);
	} else {
		as_v8_error(client->log, "Callback function required");
		return Nan::ThrowError("Callback function required");
	}
}

/**
 *  Instantiate a new AerospikeClient.
 */
Local<Value> AerospikeClient::NewInstance(Local<Object> config)
{
	Nan::EscapableHandleScope scope;
	const int argc = 1;
	Local<Value> argv[argc] = { config };
	Local<Function> cons = Nan::New<Function>(constructor());
	Nan::TryCatch try_catch;
	Nan::MaybeLocal<Object> instance = Nan::NewInstance(cons, argc, argv);
	if (try_catch.HasCaught()) {
		Nan::FatalException(try_catch);
	}
	return scope.Escape(instance.ToLocalChecked());
}

/**
 *  Initialize a client object.
 *  This creates a constructor function, and sets up the prototype.
 */
void AerospikeClient::Init()
{
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(AerospikeClient::New);
	tpl->SetClassName(Nan::New("AerospikeClient").ToLocalChecked());

	// A client object created in Node.js, holds reference to the wrapped C++
	// object using an internal field.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "addSeedHost", AddSeedHost);
	Nan::SetPrototypeMethod(tpl, "applyAsync", ApplyAsync);
	Nan::SetPrototypeMethod(tpl, "batchExists", BatchExists);
	Nan::SetPrototypeMethod(tpl, "batchGet", BatchGet);
	Nan::SetPrototypeMethod(tpl, "batchRead", BatchReadAsync);
	Nan::SetPrototypeMethod(tpl, "batchSelect", BatchSelect);
	Nan::SetPrototypeMethod(tpl, "close", Close);
	Nan::SetPrototypeMethod(tpl, "connect", Connect);
	Nan::SetPrototypeMethod(tpl, "existsAsync", ExistsAsync);
	Nan::SetPrototypeMethod(tpl, "getAsync", GetAsync);
	Nan::SetPrototypeMethod(tpl, "hasPendingAsyncCommands", HasPendingAsyncCommands);
	Nan::SetPrototypeMethod(tpl, "indexCreate", IndexCreate);
	Nan::SetPrototypeMethod(tpl, "indexRemove", IndexRemove);
	Nan::SetPrototypeMethod(tpl, "info", Info);
	Nan::SetPrototypeMethod(tpl, "infoForeach", InfoForeach);
	Nan::SetPrototypeMethod(tpl, "isConnected", IsConnected);
	Nan::SetPrototypeMethod(tpl, "jobInfo", JobInfo);
	Nan::SetPrototypeMethod(tpl, "operateAsync", OperateAsync);
	Nan::SetPrototypeMethod(tpl, "putAsync", PutAsync);
	Nan::SetPrototypeMethod(tpl, "queryApply", QueryApply);
	Nan::SetPrototypeMethod(tpl, "queryAsync", QueryAsync);
	Nan::SetPrototypeMethod(tpl, "queryBackground", QueryBackground);
	Nan::SetPrototypeMethod(tpl, "queryForeach", QueryForeach);
	Nan::SetPrototypeMethod(tpl, "removeAsync", RemoveAsync);
	Nan::SetPrototypeMethod(tpl, "removeSeedHost", RemoveSeedHost);
	Nan::SetPrototypeMethod(tpl, "scanAsync", ScanAsync);
	Nan::SetPrototypeMethod(tpl, "scanBackground", ScanBackground);
	Nan::SetPrototypeMethod(tpl, "selectAsync", SelectAsync);
	Nan::SetPrototypeMethod(tpl, "setupEventCb", SetupEventCb);
	Nan::SetPrototypeMethod(tpl, "truncate", Truncate);
	Nan::SetPrototypeMethod(tpl, "udfRegister", Register);
	Nan::SetPrototypeMethod(tpl, "udfRemove", UDFRemove);
	Nan::SetPrototypeMethod(tpl, "updateLogging", SetLogLevel);

	constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
}
