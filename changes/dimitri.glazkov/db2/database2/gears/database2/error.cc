#include "gears/database2/error.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/base/common/base_class.h"

DECLARE_GEARS_WRAPPER(GearsSQLError);

void Dispatcher<GearsSQLError>::Init() {
	RegisterProperty("code", &GearsSQLError::GetCode, NULL);
	RegisterProperty("message", &GearsSQLError::GetErrorMessage, NULL);
}

void GearsSQLError::GetCode(JsCallContext *context) {
	// OUT: int
	context->SetReturnValue(JSPARAM_INT, code_);
}

void GearsSQLError::GetErrorMessage(JsCallContext *context) {
	// OUT: string
	context->SetReturnValue(JSPARAM_STRING16, &message_);
}