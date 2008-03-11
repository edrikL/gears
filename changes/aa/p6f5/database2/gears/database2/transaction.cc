#include "gears/database2/transaction.h"
#include "gears/database2/statement.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

DECLARE_GEARS_WRAPPER(GearsSQLTransaction);

void Dispatcher<GearsSQLTransaction>::Init() {
	RegisterMethod("executeSql", &GearsSQLTransaction::ExecuteSql);
}

GearsSQLTransaction::GearsSQLTransaction() : 
						ModuleImplBaseClassVirtual("GearsSQLTransaction") {}


void GearsSQLTransaction::ExecuteSql(JsCallContext *context) {
	// IN: in DOMString sqlStatement, 
	//		 in ObjectArray arguments, 
	//		 in SQLStatementCallback callback, 
	//		 in SQLStatementErrorCallback errorCallback
	std::string16 sql_statement;
	JsArray sql_arguments;
	JsRootedCallback *callback = NULL;
	JsRootedCallback *error_callback = NULL;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &sql_statement },
    { JSPARAM_OPTIONAL, JSPARAM_ARRAY, &sql_arguments},
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &callback },
		{ JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &error_callback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

	scoped_ptr<Statement> scoped_statement(new Statement(sql_statement, 
		sql_arguments, callback, error_callback));

	// invoke callback immediately, for now
	scoped_statement->InvokeCallback(this);

  // OUT: void
}

void GearsSQLTransaction::InvokeCallback() {
	// prepare to return transaction
	JsParamToSend send_argv[] = {
		{ JSPARAM_MODULE, static_cast<ModuleImplBaseClass *>(this) }
	};

	GetJsRunner()->
		InvokeCallback(callback_.get(), ARRAYSIZE(send_argv), send_argv, NULL);
}

void GearsSQLTransaction::InvokeErrorCallback() {
}

void GearsSQLTransaction::InvokeSuccessCallback() {
}

GearsSQLTransaction* GearsSQLTransaction::Create(GearsDatabase2* database,
																					 JsRootedCallback* callback,
																					 JsRootedCallback* error_callback,
																					 JsRootedCallback* success_callback) {
	GearsSQLTransaction *tx = 
		CreateModule<GearsSQLTransaction>(database->GetJsRunner());

	tx->InitBaseFromSibling(database);
	
	// set callbacks
	tx->callback_.reset(callback);
	tx->error_callback_.reset(error_callback);
	tx->success_callback_.reset(success_callback);
	return tx;
}
