#include "gears/database2/statement.h"

void Statement::Prepare() {
}

void Statement::Execute(GearsDatabase2* database) {
}

void Statement::InvokeCallback(GearsSQLTransaction* tx) {

	// for now, just return the statement
	JsParamToSend send_argv[] = {
		{ JSPARAM_STRING16, &sql_statement_ }
	};

	if (HasCallback()) {
		tx->GetJsRunner()->
			InvokeCallback(callback_.get(), ARRAYSIZE(send_argv), send_argv, NULL);
	}
}

void Statement::InvokeErrorCallback(GearsSQLTransaction* tx, 
																		GearsSQLError* error) {
}
