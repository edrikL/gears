#include "gears/database2/database2.h"
#include "gears/database2/transaction.h"
#include "gears/database2/transaction_pipeline.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/base/common/base_class.h"

DECLARE_GEARS_WRAPPER(GearsDatabase2);

void Dispatcher<GearsDatabase2>::Init() {
	RegisterMethod("transaction", &GearsDatabase2::Transaction);
}

void GearsDatabase2::Transaction(JsCallContext *context) {
  // IN: SQLTransactionCallback callback, 
	//		 optional SQLTransactionErrorCallback errorCallback, 
	//     optional VoidCallback successCallback);
	JsRootedCallback *callback = NULL;
	JsRootedCallback *error_callback = NULL;
	JsRootedCallback *success_callback = NULL;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &callback },
		{ JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &error_callback },
		{ JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &success_callback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

	GearsSQLTransaction *tx = 
		GearsSQLTransaction::Create(this, callback, error_callback, 
			success_callback);

	pipeline_->Add(tx);

  // OUT: void
}
