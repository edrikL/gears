#ifndef GEARS_DATABASE2_TRANSACTION_H__
#define GEARS_DATABASE2_TRANSACTION_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/database2/database2.h"

class GearsSQLTransaction : public ModuleImplBaseClassVirtual {
	public:
		GearsSQLTransaction();

		// IN: in DOMString sqlStatement, 
		//		 in ObjectArray arguments, 
		//		 in SQLStatementCallback callback, 
		//		 in SQLStatementErrorCallback errorCallback
		// OUT: void
		void ExecuteSql(JsCallContext *context);

		bool HasCallback() const { 
			return callback_ != NULL 
				&& !JsTokenIsNullOrUndefined(callback_->token());
		}

		bool HasErrorCallback() const { 
			return error_callback_ != NULL
				&& !JsTokenIsNullOrUndefined(error_callback_->token()); 
		}

		bool HasSuccessCallback() const {
			return success_callback_ != NULL
				&& !JsTokenIsNullOrUndefined(success_callback_->token());
		}

		void InvokeCallback();
		void InvokeErrorCallback();
		void InvokeSuccessCallback();

		static GearsSQLTransaction* Create(GearsDatabase2* database,
																		JsRootedCallback* callback,
																		JsRootedCallback* error_callback,
																		JsRootedCallback* success_callback);

	private:
		scoped_ptr<JsRootedCallback> callback_;
		scoped_ptr<JsRootedCallback> error_callback_;
		scoped_ptr<JsRootedCallback> success_callback_;

		DISALLOW_EVIL_CONSTRUCTORS(GearsSQLTransaction);
};

#endif // GEARS_DATABASE2_TRANSACTION_H__
