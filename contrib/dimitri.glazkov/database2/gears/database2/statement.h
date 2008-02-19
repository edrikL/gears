#ifndef GEARS_DATABASE2_STATEMENT_H__
#define GEARS_DATABASE2_STATEMENT_H__

#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/database2/error.h"
#include "gears/database2/database2.h"
#include "gears/database2/transaction.h"

// represents statement
class Statement {
 public:
	 Statement(const std::string16 &sql_statement, 
						 const JsArray &sql_arguments,
						 JsRootedCallback *callback,
						 JsRootedCallback *error_callback)
		 : sql_statement_(sql_statement), sql_arguments_(sql_arguments),
			 callback_(callback), error_callback_(error_callback), bogus_(true) {}

	 // per spec (http://www.whatwg.org/specs/web-apps/current-work/multipage/section-sql.html#executesql)
	 // preparing and sanitizing the statement is a separate step
	 void Prepare();

	 void Execute(GearsDatabase2* database);

	 bool IsBogus() { return bogus_; }

	 bool HasCallback() const { 
		 return callback_ != NULL
			 && !JsTokenIsNullOrUndefined(callback_->token()); 
	 }

	 bool HasErrorCallback() const { 
		 return error_callback_ != NULL
			 && !JsTokenIsNullOrUndefined(error_callback_->token());
	 }

	 void InvokeCallback(GearsSQLTransaction* tx);
	 void InvokeErrorCallback(GearsSQLTransaction* tx, GearsSQLError* error);

private:
	std::string16 sql_statement_;
	JsArray sql_arguments_;
	bool bogus_;
	scoped_ptr<JsRootedCallback> callback_;
	scoped_ptr<JsRootedCallback> error_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(Statement);
};

#endif // GEARS_DATABASE2_STATEMENT_H__
